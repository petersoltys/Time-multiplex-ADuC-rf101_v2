/**
 *****************************************************************************
   @example  Master.c
   @brief    prigram used for Receiving data in time multiplex
             data are received via RF Interface and sended to UART
             working with Slave.c
             tested on ADucRF101MKxZ

             

   @version  V1.3
   @author   Peter Soltys
   @date     august 2015  

   @par Revision History:
   - V1.1, July 2015  : initial version. 
   - V1.2, august 2015  : fully  functional.    
   - V1.3, september 2015  : faster version with higher throughput
   - V1.4, january 2016  : added synchronization
   - V2.0, febtuary 2016  : new time multiplex conception
   - V2.1, febtuary 2016  : fixed synchronization

note : in radioeng.c was changed intial value from
    static RIE_BOOL             bPacketTx                     = RIE_FALSE; 
    static RIE_BOOL             bPacketRx                     = RIE_FALSE; 
to 
    static RIE_BOOL             bPacketTx                     = RIE_TRUE; 
    static RIE_BOOL             bPacketRx                     = RIE_TRUE; 

**/
#include "include.h"
#include "settings.h"
#include <stdarg.h>

#define LED_OFF DioSet(pADI_GP4,BIT2)//led off
#define LED_ON  DioClr(pADI_GP4,BIT2)//led on



RIE_Responses  RIE_Response = RIE_Success;
unsigned char  Buffer[240];
RIE_U8         PktLen;
RIE_S8         RSSI;


//            [level ][ packet num.][packet data]
char pktMemory[2][NUM_OF_PACKETS_IN_MEMORY][PACKET_MEMORY_DEPTH];
//pktMemory have 9600 Bytes if is 20 packet depth and ADuc rf101 have only 16KBytes SRAM
//because of this restriction is nesesary for right function to be 
//UART much faster(tested on 128000 baud rate) than RF-link
/*
  pktMemory is 2 levels deep 
  puspose is changing in circle 
  0 actual receiving buffer
  1 actual sending buffer
  for pointing are used flags : actualRxBuffer, actualTxBuffer
*/
unsigned char numOfPkt[2] = {0,0};//similar as pktMemory
unsigned char actualPacket;

signed char actualRxBuffer=0,actualTxBuffer=1;

char lastRadioTransmitBuffer[PACKET_MEMORY_DEPTH];//buffer with last radio dommand
char dmaTxBuffer[255];//buffer for DMA TX UART channel
#define UART_BUFFER_DEEP 50
char rxBuffer[UART_BUFFER_DEEP];//buffer for RX UART channel - only for directives

unsigned char slave_ID = 1;//Slave ID
signed char send=0;
//signed char nextRxPkt=0;

signed char TX_flag=0, flush_flag=0;
signed char sync_flag = 0;//flag starting sending synchronization packet
signed char sync_wait = 0;
signed char firstRxPkt = 0;
unsigned char rxUARTcount=0;

//variables for DMA_UART_TX_Int_Handler
signed char dmaTxSlv=0,dmaTxPkt=0,dmaTx_flag=0;
char* dmaTxPtr;

#if THROUGHPUT_MEASURE
int rxThroughput;
int txThroughput;
#endif

void DMA_UART_TX_Int_Handler (void);


/*
* initializing uart port
* speed UART_BAUD_RATE_MASTER baud
* 8 bits
* one stop bit
* output port P1.0\P1.1
*/
void uartInit(void){
  UrtLinCfg(0,UART_BAUD_RATE_MASTER,COMLCR_WLS_8BITS,COMLCR_STOP_DIS);
  DioCfg(pADI_GP1,0x9); // UART functionality on P1.0\P1.1

  UrtIntCfg(0,COMIEN_ERBFI);// enable Rx interrupts
  NVIC_EnableIRQ(UART_IRQn);// setup to receive data using interrupts
  
  DmaInit();// Create DMA descriptor
  DmaTransferSetup(UARTTX_C,  20,   Buffer);
  NVIC_EnableIRQ ( DMA_UART_TX_IRQn );// Enable DMA UART TX interrupt
}

/*
* inicializovanie portu na ktorom je pripojena user specified led
*/
void ledInit(void){
    // P4.2  as output
  DioCfg(pADI_GP4,0x10);
  DioOen(pADI_GP4, BIT2); 
}

/* 
* this function set general purpose timer0 for synchronization intervals
* see void GP_Tmr1_Int_Handler ()
*/
void setSynnicTimer(void){
  GptLd (pADI_TM0, SYNC_INTERVAL); // Interval of 2ms
  GptCfg(pADI_TM0, TCON_CLK_UCLK, TCON_PRE_DIV256, TCON_ENABLE|TCON_MOD_PERIODIC);
  while (GptSta(pADI_TM0)& TSTA_CON); // wait for sync of TCON write. required because of use of asynchronous clock
  GptClrInt(pADI_TM0,TCLRI_TMOUT);
  while (GptSta(pADI_TM0)& TSTA_CLRI); // wait for sync of TCLRI write. required because of use of asynchronous clock
  NVIC_EnableIRQ(TIMER0_IRQn);
}

/*
* function for initialise the Radio
* funkcia na inicializovanie radioveho prenosu
*/
void radioInit(void){
  // Initialise the Radio
  if (RIE_Response == RIE_Success)
     RIE_Response = RadioInit(RADIO_CFG);  
  // Set the Frequency to operate at 433 MHz
  if (RIE_Response == RIE_Success)
     RIE_Response = RadioSetFrequency(RADIO_FREQENCY);
  // Set modulation type
  if (RIE_Response == RIE_Success)
     RadioSetModulationType(RADIO_MODULATION);
  // Set the PA and Power Level
  if (RIE_Response == RIE_Success)
     RIE_Response = RadioTxSetPA(PA_TYPE,RADIO_POWER);
  // Set data whitening
  if (RIE_Response == RIE_Success)
     RIE_Response = RadioPayldDataWhitening(DATA_WHITENING);
  // Set data Manchaster encoding
  if (RIE_Response == RIE_Success)
    RadioPayldManchesterEncode(RADIO_MANCHASTER);
}


/*
* send one packet on UART with DMA controller
* before call this function is nessesary to initialize UART
*/
void  dmaSend(unsigned char* buff, int len){
  //DMA UART stream
  DmaInit();
  DmaTransferSetup(UARTTX_C,len,buff);
  DmaChanSetup(UARTTX_C,ENABLE,ENABLE);// Enable DMA channel  
  UrtDma(0,COMIEN_EDMAT);
}

/*
* this function is equivalent to function printf from library stdio.h
* output stream is managed with DMA controller 
* before call this function is nessesary to initialize UART
*/
int dma_printf(const char * format /*format*/, ...)
{
  unsigned char len;
  va_list args;
  va_start( args, format );
   
  len =vsprintf(dmaTxBuffer, format,args);//vlozenie formatovaneho retazca do buff
  dmaSend(dmaTxBuffer,len);

  va_end( args );
  return len;
}




/*
* send one packet trought radio
*
*/
void radioSend(char* buff, unsigned char len){
  if (RIE_Response == RIE_Success){
    strcpy(lastRadioTransmitBuffer, buff);//copy last command to buffer
    while(!RadioTxPacketComplete());
  }
  
  if (RIE_Response == RIE_Success){
    RIE_Response = RadioTxPacketVariableLen(len, buff); 
  }
  
  if (RIE_Response == RIE_Success)//start again receiving mod
    RIE_Response = RadioRxPacketVariableLen(); 
  
#if THROUGHPUT_MEASURE
  txThroughput=txThroughput+len;
#endif
    //DMA UART stream
#if TX_STREAM
  DmaChanSetup(UARTTX_C,ENABLE,ENABLE);// Enable DMA channel  
  DmaTransferSetup(UARTTX_C,len-1,buff);
  UrtDma(0,COMIEN_EDMAT);
#endif
}

/*
* this function is equivalent to function printf from librarz stdio.h
* but output stream is throught radio transmitter
* before call this function is nessesary to initialize radio
* any one formated string (call) is sended in one packet
*/
unsigned char rf_printf(const char * format /*format*/, ...){
  char buff[256];
  unsigned char len;
  va_list args;
  va_start( args, format );

  len=vsprintf(buff, format,args);//vlozenie formatovaneho retazca do buff
  if(len>255){//kontrola maximalnej dlzky retazca
    va_end( args );
    return 0;
  }
      
  radioSend(buff,len+1);//send formated packet

  va_end( args );
  return len;
}

/*
* set timer in periodic cycle at interval aproximetly 10ms = 1time slot
* 
*/
void sendLastRadioPacket(void){
  unsigned char len = 0;
  len = strlen(lastRadioTransmitBuffer);
  radioSend(lastRadioTransmitBuffer, len);
}

/*
* function receive one packet from radio
* function will wait until packet is received
*/
char Radio_recieve(void){//pocka na prijatie jedneho paketu
  unsigned char retransmision = 0;
  unsigned int timeout_timer = 0;
  
	if (RIE_Response == RIE_Success){
    RIE_Response = RadioRxPacketVariableLen();
  }

	if (RIE_Response == RIE_Success){
    while (!RadioRxPacketAvailable()){
      timeout_timer++;
      
      //turn on led if nothing is received
      if (timeout_timer > T_TIMEOUT){
        //LED_ON;
        //after RETRANSMISION times end transaction
        if (retransmision < RETRANSMISION-1)
          sendLastRadioPacket();
        else
          return 0;
        
        retransmision++;
        timeout_timer=0;
      }
    }
    timeout_timer=0;
  }
  
  LED_ON;
  //citanie paketu z rf kontrolera
	if (RIE_Response == RIE_Success)
    RIE_Response = RadioRxPacketRead(sizeof(Buffer),&PktLen,Buffer,&RSSI);
  LED_OFF;
  
  #if THROUGHPUT_MEASURE
    rxThroughput=rxThroughput+PktLen;
  #endif
    //DMA UART stream
  #if RX_STREAM
    DmaChanSetup(UARTTX_C,ENABLE,DISABLE);// Enable DMA channel  
    DmaTransferSetup(UARTTX_C,PktLen-1,Buffer);
    UrtDma(0,COMIEN_EDMAT);
  #endif
  
  //back to receiving mode
  if (RIE_Response == RIE_Success){
    RIE_Response = RadioRxPacketVariableLen();   
  }
  return 1;
}

void copyBufferToMemory(void){
  //extracting number of actual packets
  actualPacket = Buffer[1]-CHAR_OFFSET;
  
  #if SIMULATE_RETX
      dmaSend(" s ",3);
      //dma_printf("saving actualPacket %d ",actualPacket);
  #endif
  strcpy(&pktMemory[actualRxBuffer][actualPacket-1][0],Buffer);//copy packet to memory
}

/*
* send request for retranmit missing packets
*
*/
char getMissPkt(void)
{
  signed char ch, i, numOfReTxPackets = 0;
  char str[25]=RETRANSMISION_ID;
  char str2[2]={0,0};
  
////////check for missing packets////////////////////////////////////
  for(i=0; i < numOfPkt[actualRxBuffer] ; i++)//iterate throught packets
  {
    ch = pktMemory[actualRxBuffer][i][1];//extract number of packet or waiting flag
    #if SIMULATE_RETX
      ch='w';
    #endif
    if( ch == 'w')//check wait flag
    {
      str2[0]= i+CHAR_OFFSET+1;

      strcat(str,str2);//append number of missed packet
      numOfReTxPackets++;
    }
  }

//////////////send request for retransmition if needed//////////////
  if (numOfReTxPackets != 0)
  {
    rf_printf(str);

    #if DEBUG_MESAGES || SIMULATE_RETX
      dma_printf("  %s of %d \n", str, numOfPkt[actualRxBuffer]);
    #endif
    
//////////////receive missing packets//////////////
    for (i=0; i<numOfReTxPackets; i++){
      //if packet was really received copy buffered packet
      if (Radio_recieve()){
        copyBufferToMemory();
      }
    }
  }
}

/*
* flush all received packets
*/
void flushBufferedPackets(void){
  
  //switch buffer 
  actualRxBuffer++;
  actualTxBuffer++;
  if(actualTxBuffer >= 2)
    actualTxBuffer=0;
  else
    actualRxBuffer=0;
  
  dmaTxPkt =0;
  flush_flag =1;
  //call DMA_UART_TX_Int_Handler all managment is inside this function
  DMA_UART_TX_Int_Handler ();
}

/*
* transmit 3 synchronization packets
*/
void synchronize(void){
  setSynnicTimer();
  sync_wait = 1;
  while(sync_wait == 1);//wait for timer interrupt
  sync_wait = 1;
  radioSend("SYNC3",6);//first syn. packet
  while(sync_wait == 1);
  sync_wait = 1;
  radioSend("SYNC2",6);
  while(sync_wait == 1);
  radioSend("SYNC1",6);//last syn. packet

  sync_wait = 1;
  while(sync_wait == 1);//free time before transmitting
  sync_flag = 0;
}  

char validPacket(void){
  unsigned char i,slv,pktNum;
  
    //extracting slave identifier
  slv = Buffer[0]-CHAR_OFFSET;
  //extracting number of actual packets
  actualPacket = Buffer[1]-CHAR_OFFSET;
  //extracting number of actual packets
  pktNum = Buffer[2]-CHAR_OFFSET;
  
  //if first received packet extract num of packets and check slave ID
  if (firstRxPkt==0){
    firstRxPkt=1;
    
    //extracting number of buffered TX packets
    numOfPkt[actualRxBuffer] = pktNum;
    if (numOfPkt[actualRxBuffer] > NUM_OF_PACKETS_IN_MEMORY)
      {return 0;}
    
    //write "waiting flag"- ("w") for expected packets
    for (i=0; i < numOfPkt[actualRxBuffer] ;i++)
      pktMemory[actualRxBuffer][i][1]='w';
  }
  
  if (slv != slave_ID)//check of slave id (number) expected/transmiting
    dma_printf("slave id dismatch or not recognizet packet");
  
  if (actualPacket==0)//if zero packet
    return 0;
  
  //if number of packet is in range
  if (actualPacket > 0 && actualPacket <= NUM_OF_PACKETS_IN_MEMORY)
    //if number of slave is in range
    if (slv <= NUM_OF_SLAVE && slv > 0)
        //if expected total number of packet is same as at begining
        if (pktNum == numOfPkt[actualRxBuffer])
          return 1;
  return 0;
}
/*
*/
void SetInterruptPriority (void){
  NVIC_SetPriority(UART_IRQn,5);//receiving directives
  NVIC_SetPriority(DMA_UART_TX_IRQn,7);//terminate DMA TX priority
  NVIC_SetPriority(TIMER1_IRQn,8);//blinking low priority
  NVIC_SetPriority(TIMER0_IRQn,9);//terminate transmiting higher prioritz
  NVIC_SetPriority(EINT8_IRQn,10);//highest priority for radio interupt
}
/*
*
*/

int main(void)
{ 
  signed char i;
  
  WdtGo(T3CON_ENABLE_DIS);
  
  uartInit();
  ledInit();
  radioInit();
  SetInterruptPriority ();

  while(1)
  {
///////////////////////slot identificator transmiting////////////////////
    rf_printf(TIME_SLOT_ID_MASTER);//start packet for new multiplex
    
///////////////////////receiving data from slave////////////////////
    while (1){//loop for receiving all expecting packets
 
      //check if packet is really received
      if (Radio_recieve()){
        //extracting number of actual packets
        actualPacket = Buffer[1]-CHAR_OFFSET;
        
        if (validPacket()){
          copyBufferToMemory();
        
          if (actualPacket >= NUM_OF_PACKETS_IN_MEMORY || actualPacket == numOfPkt[actualRxBuffer]){
            break;
          }
        }
        else
          break;
      }
      else 
        break;
    }
    //if some data in packets
    if (numOfPkt[actualRxBuffer]){
      #if DEBUG_MESAGES
        dma_printf("expected %d packets\n", numOfPkt[actualRxBuffer]);
      #endif
      getMissPkt();//get back losted packets
      flushBufferedPackets();//send on UART received packets
    }
    //if synchronize message received
    if (sync_flag == 1)
      synchronize();
    
    slave_ID++;//increment slave number 
    if (slave_ID >= NUM_OF_SLAVE)
      slave_ID = 1;
    
    firstRxPkt=0;
    numOfPkt[actualRxBuffer]= 0;
  }
}

///////////////////////////////////////////////////////////////////////////
// GP Timer1 Interrupt handler used for blinking led
///////////////////////////////////////////////////////////////////////////
void GP_Tmr1_Int_Handler (void)
{

  if (GptSta(pADI_TM1)== TSTA_TMOUT) // if timout interrupt
  {

#if THROUGHPUT_MEASURE
    dma_printf("troughputs TX %d bytes/s RX %d bytes/s */*/*/ \n",txThroughput,rxThroughput); 
    txThroughput=0;
    rxThroughput=0;
#endif
  }
}

///////////////////////////////////////////////////////////////////////////
// GP Timer0 Interrupt handler 
// used for synchronization
///////////////////////////////////////////////////////////////////////////
void GP_Tmr0_Int_Handler(void){
  if (GptSta(pADI_TM0)== TSTA_TMOUT){ // if timout interrupt
    sync_wait=0;
    if (sync_flag==0)//if synchronize is complete
      GptCfg(pADI_TM0, TCON_CLK_UCLK, TCON_PRE_DIV256, TCON_ENABLE_DIS);//stop timer
  }
}

///////////////////////////////////////////////////////////////////////////
// DMA UART TX Interrupt handler 
// used for sending data on UART
// and for terminating of UART stream
///////////////////////////////////////////////////////////////////////////
void DMA_UART_TX_Int_Handler (void)
{
  int len;
  UrtDma(0,0);  // prevents further UART DMA requests
  DmaChanSetup ( UARTTX_C , DISABLE , DISABLE );// Disable DMA channel
  
  if(flush_flag == 1){
    if(dmaTxPkt < numOfPkt[actualTxBuffer]){//packet itete 0..as needed
      
      dmaTxPtr = &pktMemory[actualTxBuffer][dmaTxPkt][0];//pointer at actuall packet
      
      if(dmaTxPtr[1]!='w'){//try if packet is received waiting flag
        
        while(dmaTxPtr[len]!='\0'){//get lenght of packet
          len++;
        }
        
        #if SEND_HEAD
          dmaSend(dmaTxPtr,len);//send data with head
        #else
          dmaSend(dmaTxPtr+HEAD_LENGHT, len-HEAD_LENGHT);//send only data without head
        #endif
        
        dmaTx_flag=1;
      }
      else{
        dma_printf("missing packet %d ",dmaTxPkt+1);//message about missing packet
        dmaTx_flag = 1 ;
      }
      dmaTxPkt++;
    }
    else{ //all data sended
      flush_flag=0;
      dmaTx_flag=1;//end of recurent calls
    }
      
    if(dmaTx_flag == 1)
      //if some data are sended DMA_UART_TX_Int_Handler is called after end of transmision
      dmaTx_flag=0;
    else
      DMA_UART_TX_Int_Handler();//recurent call while are data sended 
  }
}

///////////////////////////////////////////////////////////////////////////
// DMA UART RX Interrupt handler 
///////////////////////////////////////////////////////////////////////////
void DMA_UART_RX_Int_Handler   ()
{
  UrtDma(0,0); // prevents additional byte to restart DMA transfer
}
///////////////////////////////////////////////////////////////////////////
// UART Interrupt handler 
// used for receiving directives
// function is taken from example UARTLoopback.c and modified
///////////////////////////////////////////////////////////////////////////
void UART_Int_Handler ()
{ 	
  unsigned char ucLineSta, ucCOMIID0; 
  char ch;
 
  ucCOMIID0 = UrtIntSta(0);		// Read UART Interrupt ID register

  if ((ucCOMIID0 & COMIIR_STA_RXBUFFULL) == COMIIR_STA_RXBUFFULL)	  // Receive buffer full interrupt
  {
    ch	= UrtRx(0);   //call UrtRd() clears COMIIR_STA_RXBUFFULL
  
    rxBuffer[rxUARTcount]= ch;
    rxUARTcount++;
    
    if (ch == '$')
      if (memcmp(&rxBuffer[rxUARTcount-5],"SYNC$",5) == 0){//end of packet pointer
        sync_flag = 1;
        memset(rxBuffer,'\0', rxUARTcount);//clear buffer
        rxUARTcount = 0;
      }
    if (rxUARTcount >= UART_BUFFER_DEEP)//check not overflow buffer
      rxUARTcount = 0;
  }
} 
