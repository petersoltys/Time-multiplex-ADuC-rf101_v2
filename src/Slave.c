/**
 *****************************************************************************
   @example  Slave.c
   @brief    prigram used for transmiting data in time multiplex
             data are received via UART and sended trouhgt radio interface
             during associated time slot 
             working with Master.c
             tested on ADucRF101MKxZ

             

   @version  V2.1
   @author   Peter Soltys
   @date     febtuary 2016 

   @par Revision History:
   - V1.0, July 2015  : initial version. 
   - V1.1, august 2015  : fully  functional.  (only shyntetic data transmitted)
   - V1.2, august 2015  : fully  functional.  (transmiting received data via UART (only fixed lenght packet))
   - V1.3, september 2015  : faster version with higher throughput and transmiting variable lenght packets 
                             working with "UWB - Coordinate Reader Deployment" from Peter Mikula
   - V1.4, january 2015  : added synchronization
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

RIE_Responses RIE_Response = RIE_Success;
unsigned char  Buffer[255];
RIE_U8         PktLen;
RIE_S8         RSSI;

// [level][num of packet][packet data]
char pktMemory[2][NUM_OF_PACKETS_IN_MEMORY][PACKET_MEMORY_DEPTH];
/*
  pktMemory is 2 levels deep 
  puspose is changing in circle 
  0 actual receiving buffer
  1 actual sending buffer
  for pointing are used flags : actualRxBuffer, actualTxBuffer,
*/
char numOfPackets[2] = {0,0};//similar like pktMemory

char actualRxBuffer=0;
char actualTxBuffer=1;

char rxUARTbuffer[255];
char* rxPktPtr ;
int rxUARTcount = 0;
/////////flags/////////////////////
char TX_flag=0, RX_flag=0, terminate_flag=0, buffer_change_flag=0 ;
char my_slot = 0;

void UART_Int_Handler (void);

int i=0,j=0;
int debugTimer=0;

#if THROUGHPUT_MEASURE
int txThroughput;
int rxThroughput;
#endif

/*
* initializing uart port
* speed UART_BAUD_RATE_SLAVE baud
* 8 bits
* one stop bit
* output port P1.0\P1.1
*/
void uart_init(void){
  rxPktPtr =&pktMemory[actualRxBuffer][numOfPackets[actualRxBuffer]][HEAD_LENGHT+1];//pinting beyound packet head
  
	UrtLinCfg(0,UART_BAUD_RATE_SLAVE,COMLCR_WLS_8BITS,COMLCR_STOP_DIS);//configure uart
  DioCfg(pADI_GP1,0x9); // UART functionality on P1.0\P1.1
  
#if LEN_OF_RX_PKT == 0
  UrtIntCfg(0,COMIEN_ERBFI);// enable Rx interrupts
  NVIC_EnableIRQ(UART_IRQn);// setup to receive data using interrupts
#endif
  
  DmaInit();// Create DMA descriptor
 // Set transfer parameters
 //                 transfer  target
 //                 channel   count pointer
  DmaTransferSetup(UARTTX_C,  20,   Buffer);
  DmaTransferSetup(UARTRX_C,  20,   rxPktPtr);
  NVIC_EnableIRQ ( DMA_UART_TX_IRQn );// Enable DMA UART TX interrupt
  NVIC_EnableIRQ ( DMA_UART_RX_IRQn );// Enable DMA UART RX interrupt
  
#if LEN_OF_RX_PKT
//enable RX DMA chanel for receiving packets forom UART
  DmaChanSetup(UARTRX_C,ENABLE,ENABLE);// Enable DMA channel  
  DmaTransferSetup(UARTRX_C,LEN_OF_RX_PKT,rxPktPtr);
  UrtDma(0,COMIEN_EDMAR);//start rx 
#endif
}

/*
* initializing port with led
* inicializovanie portu na ktorom je pripojena user specified led
*/
void led_init(void){
	  // P4.2  as output
  DioCfg(pADI_GP4,0x10);
  DioOen(pADI_GP4, BIT2); 
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
    DmaChanSetup(UARTTX_C,ENABLE,ENABLE);// Enable DMA channel  
    DmaTransferSetup(UARTTX_C,len,buff);
    UrtDma(0,COMIEN_EDMAT);
}

/*
* this function is equivalent to function printf from library stdio.h
* output stream is managed with DMA controller 
* before call this function is nessesary to initialize UART
* Tato funkcia je ekvivalentna k funkcii printf z kniznice stdio.h
* Vystup je smerovany cez DMA na UART
* Pred volanim tejto funkcie je nutne inicializovat uart
*/
int dma_printf(const char * format /*format*/, ...)
{
  char buff[256];
  unsigned char len;
  va_list args;
  va_start( args, format );
   
  len =vsprintf(buff, format,args);//vlozenie formatovaneho retazca do buff
  dmaSend(buff,len);

  va_end( args );
  return len;
}

///*
//* send one packet using radio channel
//* vysle jeden paket cez radivoy kanal
//*/
//void radioSend(char* buff, char len){
//  unsigned int safe_timer=0;
//  //LED_ON;
//  if (RIE_Response == RIE_Success){
//    RIE_Response = RadioTxPacketVariableLen(len, buff); 
//    
//  if (RIE_Response == RIE_Success)  
//  while(!RadioTxPacketComplete());
//  }
//  //LED_OFF;
//  while (safe_timer < T_PROCESSING)
//    safe_timer++;
//  
//#if THROUGHPUT_MEASURE
//  txThroughput=txThroughput+len;
//#endif
//    //DMA UART stream
//#if TX_STREAM
////  printf(buff);
//  dmaSend(buff,len-1);
//#endif
//}
/*
* send one packet trought radio
*
*/
void radioSend(char* buff, unsigned char len){
  #if T_PROCESSING
  unsigned int safe_timer=0;
  #endif
  if (RIE_Response == RIE_Success){//send packet
    RIE_Response = RadioTxPacketVariableLen(len, buff); 
    RX_flag = 0;
  }
  
  if (RIE_Response == RIE_Success){//wait untill packet sended
    while(!RadioTxPacketComplete());
  }
  #if T_PROCESSING
  while (safe_timer < T_PROCESSING)
    safe_timer++;
  #endif
  if (RIE_Response == RIE_Success){//start again receiving mod
    RIE_Response = RadioRxPacketVariableLen(); 
    RX_flag = 1;
  }
  
#if THROUGHPUT_MEASURE
  txThroughput=txThroughput+len;
#endif
    //DMA UART stream
#if TX_STREAM
  dmaSend(buff,len-1);
#endif
}

/*
* this function is equivalent to function printf from librarz stdio.h
* but output stream is throught radio transmitter
* before call this function is nessesary to initialize radio
* any one formated string (call) is sended in one packet
* Tato funkcia je pdobna k funkcii printf z kniznice stdio.h
* Vystup je smerovany cez radiovy prenos
* Pred volanim tejto funkcie je nutne inicializovat radio kontroler
*/
unsigned char rf_printf(const char * format /*format*/, ...){
  char buff[256];
  unsigned char len;
  va_list args;
  va_start( args, format );

  len=vsprintf(buff, format,args);//vlozenie formatovaneho retazca do buff
  if(len>240){//kontrola maximalnej dlzky retazca
    va_end( args );
    return 0;
  }
  
  radioSend(buff,len+1);//send formated packet

  va_end( args );
  return len;
}

/*
* function receive one packet from radio
* function will wait until packet is received
*/
char radioRecieve(void){//pocka na prijatie jedneho paketu
  unsigned int timeout_timer = 0;
  
	if (RIE_Response == RIE_Success && RX_flag == 0){
    RIE_Response = RadioRxPacketVariableLen();
    RX_flag = 1;
  }

	if (RIE_Response == RIE_Success && RX_flag == 1){
    while (!RadioRxPacketAvailable()){
      timeout_timer++;
      //turn on led if nothing is received after timeout
      if (timeout_timer > T_TIMEOUT+1000){
        LED_ON;
        return 0;
      }
    }
    RX_flag=0;
  }
  
  //LED_ON;
  //citanie paketu z rf kontrolera
	if (RIE_Response == RIE_Success)
    RIE_Response = RadioRxPacketRead(sizeof(Buffer),&PktLen,Buffer,&RSSI);
  //LED_OFF;
  
  #if THROUGHPUT_MEASURE
    rxThroughput=rxThroughput+PktLen;
  #endif
    //DMA UART stream
  #if RX_STREAM
    dmaSend(Buffer,PktLen-1);
  #endif
  
  //back to receiving mode
  if (RIE_Response == RIE_Success && RX_flag == 0){
    RIE_Response = RadioRxPacketVariableLen();   
    RX_flag = 1;
  }
  return 1;
}
///*
//* function receive one packet from radio
//* function will wait until packet is received
//*/
//void Radio_recieve(void){//pocka na prijatie jedneho paketu
//  
//	if (RIE_Response == RIE_Success){
//    RIE_Response = RadioRxPacketVariableLen();   
//    terminate_flag = 0 ;
//  }
//	if (RIE_Response == RIE_Success){
//    while (!RadioRxPacketAvailable() && terminate_flag==0){
//      debugTimer++;
//      if (debugTimer > T_TIMEOUT)//turn on led if nothing is received (master stop working)
//        LED_ON;
//      }
//    debugTimer=0;
//  }
//  //citanie paketu z rf kontrolera
//	if (RIE_Response == RIE_Success)
//    RIE_Response = RadioRxPacketRead(sizeof(Buffer),&PktLen,Buffer,&RSSI);
//  
//  #if THROUGHPUT_MEASURE
//    rxThroughput=rxThroughput+PktLen;
//  #endif
//    //DMA UART stream
//  #if RX_STREAM
//    dmaSend(Buffer,PktLen-1);
//  #endif
//  
//  //back to receiving mode
//  if (RIE_Response == RIE_Success){
//    RIE_Response = RadioRxPacketVariableLen();   
//  }
//}


/*
* function set timer for interval 5 10 15ms aproximetly depends on number 
* of SYNC packet 
* after timeout is generated interrupt, to set synchronization pin
*/
void setTimeToSync(unsigned int time){
  GptLd (pADI_TM0, time);
  GptCfg(pADI_TM0, TCON_CLK_UCLK, TCON_PRE_DIV256, TCON_ENABLE_EN|TCON_RLD_EN|TCON_MOD_PERIODIC);
  while (GptSta(pADI_TM0)& TSTA_CON); // wait for sync of TCON write. required because of use of asynchronous clock
  GptClrInt(pADI_TM0,TCLRI_TMOUT);
  while (GptSta(pADI_TM0)& TSTA_CLRI); // wait for sync of TCLRI write. required because of use of asynchronous clock
  NVIC_EnableIRQ(TIMER0_IRQn);
}

#if THROUGHPUT_MEASURE
/*
* function set timer for 1s timeout
* only for measuring purpose
*/
void troughputMeasureInit(void){
  GptLd (pADI_TM1, 0x0); // Interval of 1s
  GptCfg(pADI_TM1, TCON_CLK_LFOSC, TCON_PRE_DIV32768, TCON_ENABLE|TCON_MOD_PERIODIC);
  while (GptSta(pADI_TM1)& TSTA_CON); // wait for sync of TCON write. required because of use of asynchronous clock
  GptClrInt(pADI_TM1,TCLRI_TMOUT);
  while (GptSta(pADI_TM1)& TSTA_CLRI); // wait for sync of TCLRI write. required because of use of asynchronous clock
  NVIC_EnableIRQ(TIMER1_IRQn);
}
#endif

#if THROUGHPUT_MEASURE == 2
/*
* function sending false data packets, only for measuring purpose
*/
char transmit(void){
  my_slot=1;
  char txPkt=0;
  
  while(my_slot==1 && txPkt < 10){
    //all times like packet 2
    rf_printf("%c02 my shyntetic data ... number of tx data %d Bytes",SLAVE_ID,txThroughput);
    txPkt++;
  }
  return txPkt;
}
#else

/*
* function is transmiting all prepared packets throught radio link
* all data packets are received via UART
*/
char transmit(void){
  char* pktMemoryPtr;
  unsigned char txPkt = 0;

  my_slot=1;

  NVIC_DisableIRQ(UART_IRQn);
    //change buffer pointers
    actualRxBuffer++;
    actualTxBuffer++;
    if(actualRxBuffer>=2)
      actualRxBuffer=0;
    else if(actualTxBuffer>=2)
      actualTxBuffer=0;
    
    numOfPackets[actualRxBuffer]=0;
  NVIC_EnableIRQ(UART_IRQn);  
  
  while (my_slot == 1 && (txPkt < numOfPackets[actualTxBuffer]) ){     //while interupt ocurs send avaliable packets
    
    pktMemoryPtr = &pktMemory[actualTxBuffer][txPkt][0];
    
    //build head of packet
    pktMemoryPtr[2] = numOfPackets[actualTxBuffer] + CHAR_OFFSET;
    pktMemoryPtr[1] = txPkt + CHAR_OFFSET + 1;
    pktMemoryPtr[0] = SLAVE_ID + CHAR_OFFSET; 

    //dma_printf(pktMemoryPtr);
    rf_printf(pktMemoryPtr);//send packet

    txPkt++;
  }
  return txPkt;//return number of transmited packets
}
#endif

/*
* function retransmit missing packets
* @return : number of retransmited packets
*/
char retransmit(void){
  char pkt = 3;
  char reTxPkt[NUM_OF_PACKETS_IN_MEMORY + 5];

  my_slot=1;
  strcpy(reTxPkt,Buffer);
  while ((reTxPkt[pkt]!='\0') && my_slot==1 && terminate_flag == 0)//retransmit only until interupt occur
  {
    rf_printf(&pktMemory[actualTxBuffer][(reTxPkt[pkt]-CHAR_OFFSET)][0]);
    pkt++;
  }
  return (pkt-3);
}
/*
*/
void SetInterruptPriority (void){
  NVIC_SetPriority(UART_IRQn,5);//receiving of packets if not constant lenght of packet
  NVIC_SetPriority(DMA_UART_RX_IRQn,6);//terminate DMA RX priority
  NVIC_SetPriority(DMA_UART_TX_IRQn,7);//terminate DMA TX priority
	NVIC_SetPriority(TIMER1_IRQn,9);//blinking low priority
  NVIC_SetPriority(TIMER0_IRQn,8);//terminate transmiting higher prioritz
  NVIC_SetPriority(EINT8_IRQn,4);//highest priority for radio interupt
}

/*
*
*/
int main(void)
{   
  WdtGo(T3CON_ENABLE_DIS);//stop watch-dog

  //initialize all interfaces
  SetInterruptPriority();
	uart_init();
	led_init();
  LED_OFF;
#if THROUGHPUT_MEASURE
  troughputMeasureInit();
#endif
	radioInit();//inicialize ratio conection
  
  while (1){
    if (radioRecieve()){
      
      //if this slot identifier belongs to this slave
      if ( 0 == strcmp(Buffer,TIME_SLOT_ID_SLAVE)){
        if(numOfPackets[actualRxBuffer])//if is something to send
          transmit();
        else
          rf_printf(ZERO_PACKET);
      }
      
      //check if retransmit request
      if (0 == memcmp(Buffer,RETRANSMISION_ID,3))//check if re-tx slot
        retransmit();
      
      //check if sync packet
      if (0 == memcmp(Buffer,"SYNC",4))
        setTimeToSync((Buffer[4]-'0') * SYNC_INTERVAL);
    }
  }
}
///////////////////////////////////////////////////////////////////////////
// GP Timer0 Interrupt handler 
// used to set synchronization pin
///////////////////////////////////////////////////////////////////////////
void GP_Tmr0_Int_Handler(void){
  if (GptSta(pADI_TM0)== TSTA_TMOUT) // if timout interrupt
  {
    if (SYNC_PIN_READ)
      SYNC_PIN_LOW;//negativ synchronization
    else
    {//if high then set low and stop timer
      SYNC_PIN_HIGH;//reset synchronization
      GptCfg(pADI_TM0, TCON_CLK_UCLK, TCON_PRE_DIV256, TCON_ENABLE_DIS);//stop timer
      // wait for sync of TCON write. required because of use of asynchronous clock
      while (GptSta(pADI_TM0)& TSTA_CON); 
      NVIC_DisableIRQ(TIMER0_IRQn);
    }
  }
}
///////////////////////////////////////////////////////////////////////////
// GP Timer1 Interrupt handler 
// used for measure trouthput any 1s
///////////////////////////////////////////////////////////////////////////
void GP_Tmr1_Int_Handler(void){
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
// UART DMA Interrupt handler for transmit
// used for ending of DMA operation of dma_send()
///////////////////////////////////////////////////////////////////////////
void DMA_UART_TX_Int_Handler ()
{
  UrtDma(0,COMIEN_EDMAR);  // prevents further UART DMA requests
// Disable DMA channel
  DmaChanSetup ( UARTTX_C , DISABLE , DISABLE );
}
///////////////////////////////////////////////////////////////////////////
// DMA UART Interrupt handler 
// used for build packet
///////////////////////////////////////////////////////////////////////////
void DMA_UART_RX_Int_Handler   ()
{
  //only if constant lenght of packet
#if LEN_OF_RX_PKT

  UrtDma(0,COMIEN_EDMAT); // prevents additional byte to restart DMA transfer
  
  //if is buffered more than 10 packets dont increment and overwrite last packet
  if(numOfPackets[actualRxBuffer] < NUM_OF_PACKETS_IN_MEMORY)
    numOfPackets[actualRxBuffer]++;
  
  //ste pinter to point at new place in memory pointing beyound packet head
  rxPktPtr = &pktMemory[actualRxBuffer][numOfPackets[actualRxBuffer]][HEAD_LENGHT];
  
  //enable RX DMA chanel for receiving next packets forom UART
  DmaChanSetup(UARTRX_C,ENABLE,ENABLE);// Enable DMA channel  
  DmaTransferSetup(UARTRX_C,LEN_OF_RX_PKT,rxPktPtr);
  UrtDma(0,COMIEN_EDMAR);

#endif
}
///////////////////////////////////////////////////////////////////////////
// Hard Fault Interrupt handler 
// if pointer going out of array
///////////////////////////////////////////////////////////////////////////
void HardFault_Handler(void){
  LED_ON;
}
///////////////////////////////////////////////////////////////////////////
// UART Interrupt handler 
// used for spliting data to packets
// function is taken from example UARTLoopback.c and modified
///////////////////////////////////////////////////////////////////////////
void UART_Int_Handler (void)
{ 	
#if LEN_OF_RX_PKT==0
  unsigned char ucCOMIID0; 
  char ch;
 
  ucCOMIID0 = UrtIntSta(0);		// Read UART Interrupt ID register

  if ((ucCOMIID0 & COMIIR_STA_RXBUFFULL) == COMIIR_STA_RXBUFFULL)	  // Receive buffer full interrupt
  {
    
    ch	= UrtRx(0);   //call UrtRd() clears COMIIR_STA_RXBUFFULL
  
    rxUARTbuffer[rxUARTcount]= ch;
    rxUARTcount++;
    
    if (ch == '$'){//end of packet pointer
      rxUARTbuffer[rxUARTcount]= '\0';//write end of string
      if (numOfPackets[actualRxBuffer] < NUM_OF_PACKETS_IN_MEMORY){
        rxPktPtr = &pktMemory[actualRxBuffer][numOfPackets[actualRxBuffer]][HEAD_LENGHT];//pinting beyound packet head
        strcpy(rxPktPtr,rxUARTbuffer);
        numOfPackets[actualRxBuffer]++;
      }
      else
        dma_printf("packet memory is full ");
      
      rxUARTcount=0;       
    }
  }
#endif
} 

