/**
 *****************************************************************************
   @file     Master.c
   @brief    prigram used for Receiving data in time multiplex
             data are received via RF Interface and sended to UART
             working with Slave.c
             tested on ADucRF101MKxZ

             

   @version  V2.1B
   @author   Peter Soltys
   @date     febtuary 2016  

   @par Revision History:
   - V1.1, July 2015  : initial version. 
   - V1.2, august 2015  : fully  functional.    
   - V1.3, september 2015  : faster version with higher throughput
   - V1.4, january 2016  : added synchronization
   - V2.0, febtuary 2016  : new time multiplex conception
   - V2.1, febtuary 2016  : fixed synchronization
   - V2.1B, febtuary 2016  : Binary data packets (instead of strings with STRING_TERMINATOR)

  @note : in radioeng.c was changed intial value from \n
      static RIE_BOOL             bPacketTx                     = RIE_FALSE; \n
      static RIE_BOOL             bPacketRx                     = RIE_FALSE; \n
  to \n
      static RIE_BOOL             bPacketTx                     = RIE_TRUE; \n
      static RIE_BOOL             bPacketRx                     = RIE_TRUE; \n
      

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

/** 
   @brief  memory to store all data to send/to receive
   @param  - level
           - packet num
           - packet data
   @note    pktMemory is 2 levels deep 
            puspose is changing in circle 
            0 actual receiving buffer
            1 actual sending buffer
            for pointing are used flags : actualRxBuffer, actualTxBuffer
            
   @note  size of mermory is restricted because ADuc rf101 have only 16KBytes SRAM
   @note  UART nust be much faster(tested on 128000 baud rate) than RF-link (to release memory)
   @see   actualRxBuffer 
   @see   actualTxBuffer
**/
//            [level ][ packet num.][packet data]
char pktMemory[2][NUM_OF_PACKETS_IN_MEMORY][PACKET_MEMORY_DEPTH];

          // [level]
char numOfPkt[2] = {0,0};//similar like pktMemory
                      // [level][lenght]
unsigned char lenghtOfPkt[2][NUM_OF_PACKETS_IN_MEMORY] = {0,0};//similar like pktMemory

unsigned char actualPacket;

signed char actualRxBuffer=0,actualTxBuffer=1;

char lastRadioTransmitBuffer[PACKET_MEMORY_DEPTH];//buffer with last radio dommand
char dmaTxBuffer[255];//buffer for DMA TX UART channel
#define UART_BUFFER_DEEP 50
char rxBuffer[UART_BUFFER_DEEP];//buffer for RX UART channel - only for directives

unsigned char slave_ID = 1;//Slave ID
signed char send=0;
//signed char nextRxPkt=0;

signed char TX_flag=0,RX_flag=0, flush_flag=0;
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


/** 
   @fn     void uartInit(void)
   @brief  initialize uart port
   @code    
      uartInit();
   @endcode
   @note    speed UART_BAUD_RATE_MASTER baud
            8 bits
            one stop bit
            output port P1.0/P1.1
**/
void uartInit(void){
  UrtLinCfg(0,UART_BAUD_RATE_MASTER,COMLCR_WLS_8BITS,COMLCR_STOP_DIS);
  DioCfg(pADI_GP1,0x9); // UART functionality on P1.0/P1.1

  UrtIntCfg(0,COMIEN_ERBFI);// enable Rx interrupts
  NVIC_EnableIRQ(UART_IRQn);// setup to receive data using interrupts
  
  DmaInit();// Create DMA descriptor
  DmaTransferSetup(UARTTX_C,  20,   Buffer);
  NVIC_EnableIRQ ( DMA_UART_TX_IRQn );// Enable DMA UART TX interrupt
}

/**
* @brief initialize port with led diode
*/
void ledInit(void){
    // P4.2  as output
  DioCfg(pADI_GP4,0x10);
  DioOen(pADI_GP4, BIT2); 
}

/** 
   @fn     void setSynnicTimer(void)
   @brief  set general purpose timer0 for synchronization intervals
   @see    void GP_Tmr1_Int_Handler ()
   @note   timer predivider factor = 256, processor clock, periodic mode
**/
void setSynnicTimer(void){
  GptLd (pADI_TM0, SYNC_INTERVAL); // Interval of 2ms
  GptCfg(pADI_TM0, TCON_CLK_UCLK, TCON_PRE_DIV256, TCON_ENABLE|TCON_MOD_PERIODIC);
  while (GptSta(pADI_TM0)& TSTA_CON); // wait for sync of TCON write. required because of use of asynchronous clock
  GptClrInt(pADI_TM0,TCLRI_TMOUT);
  while (GptSta(pADI_TM0)& TSTA_CLRI); // wait for sync of TCLRI write. required because of use of asynchronous clock
  NVIC_EnableIRQ(TIMER0_IRQn);
}

/** 
   @fn     void radioInit(void)
   @brief  initialize Radio interface
   @code    
      radioInit();
   @endcode
   @see settings.h
   @note   see settings.h for radio configuration
**/
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

/** 
   @fn     void  dmaSend(unsigned char* buff, int len)
   @brief  send one packet on UART with DMA controller
   @param  buff :{} pointer to data to send
   @param  len :{int range} number of bytes to send
   @pre    uartInit() must be called before this can be called.
   @code  
        uartInit();
        len =vsprintf(dmaTxBuffer, format,args);//vlozenie formatovaneho retazca do buff
        dmaSend(dmaTxBuffer,len);
   @endcode
   @note    after end of transmision is called DMA_UART_TX_Int_Handler (void)
   @see DMA_UART_TX_Int_Handler
**/
void  dmaSend(unsigned char* buff, int len){
  //DMA UART stream
  DmaInit();
  DmaTransferSetup(UARTTX_C,len,buff);
  DmaChanSetup(UARTTX_C,ENABLE,ENABLE);// Enable DMA channel  
  UrtDma(0,COMIEN_EDMAT);
}
/** 
   @fn     int dma_printf(const char * format , ...)
   @brief  this function is equivalent to function printf from library stdio.h
   @param  format : {} pointer at string to formating output string
   @param  ... :{} additive parameters for formating string
   @pre    uartInit() must be called before this can be called.
   @code    
      uartInit();
      int num = 10;
      dma_printf();
   @endcode
   @note    output stream is managed with DMA controller 
            after end of transmision is called DMA_UART_TX_Int_Handler
   @see DMA_UART_TX_Int_Handler  
   @return  int number of sending chars (if == 0 error)
**/
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


/** 
   @fn     void radioSend(char* buff, unsigned char len)
   @brief  send one packet trought radio interface
   @param  buff :{1-240} pointer at memory to be sended
   @param  len :{0-240} number of bytes to be sended
   @pre    radioInit() must be called before this function is called.
   @code    
      len=vsprintf(buff, format,args);//@see rf_printf();
      if(len<240){//check max lenght 
        radioSend(buff,len+1);//send formated packet
   @endcode
   @note    output stream is trought radio interface
            function is waiting until whole packet is trnsmited
**/
void radioSend(char* buff, unsigned char len){
  
  if (RIE_Response == RIE_Success){//send packet
    RIE_Response = RadioTxPacketVariableLen(len, buff); 
    RX_flag = 0;
  }
  
  if (RIE_Response == RIE_Success){//wait untill packet sended
    while(!RadioTxPacketComplete());
  }
  
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
/** 
   @fn     int rf_printf(const char * format , ...)
   @brief  this function is equivalent to function printf from library stdio.h
   @param  format : {} pointer at string to formating output string
   @param  ... :{} additive parameters for formating string
   @pre    radioInoit() must be called before this can be called.
   @code    
      radioInit();
      int num = 10;
      rf_printf();
   @endcode
   @note    output stream is trought radio interface
            function is waiting until whole packet is transmited
            any one formated string (call) is sended in one packet
   @return  int number of sending chars (if == 0 error)
**/
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
/** 
   @fn     char radioRecieve(void)
   @brief  function receive one packet with radio interface
   @pre    radioInit() must be called before this function is called.
   @code    
      radioInit();
      if (radioReceive())
        printf("packet was received and read from radio interface");
        printf(Buffer);//Buffer is global bufer for radio interface
      else
        printf("packet was not received correctly before timeout");
   @endcode
   @note   function is also reding packet form radio interface to unsigned char Buffer[240]
   @return  char  1 == packet received, 0 == packet was not received correctly before timout
**/
char radioRecieve(void){
  unsigned int timeout_timer = 0;
  
	if (RIE_Response == RIE_Success && RX_flag == 0){
    RIE_Response = RadioRxPacketVariableLen();
    RX_flag = 1;
  }

	if (RIE_Response == RIE_Success && RX_flag == 1){
    while (!RadioRxPacketAvailable()){
      timeout_timer++;
      //turn on led if nothing is received after timeout
      if (timeout_timer > T_TIMEOUT){
        LED_ON;
        return 0;
      }
    }
    RX_flag=0;
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
    dmaSend(Buffer,PktLen-1);
  #endif
  
  //back to receiving mode
  if (RIE_Response == RIE_Success && RX_flag == 0){
    RIE_Response = RadioRxPacketVariableLen();   
    RX_flag = 1;
  }
  return 1;
}
/** 
   @fn     char validPacket(void)
   @brief  validate received packt head in global variable (Buffer)
   @pre    radioInit() must be called before this function is called.
   @pre    radioReceive() with returned 1
   @code    
      radioInit();
      if (radioReceive())
        if (validPacket())
          printf("packet was sucsesfully received with correct packet head");
   @endcode
   @note   function send messages about incorrect packet on UART
   @return  char  1 == valid packet head, 0 == invalid packet head
**/
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
/** 
   @fn     void copyBufferToMemory(void)
   @brief  copy received packet to packet memory
   @pre    radioInit() must be called before this function is called.
   @pre    radioReceive() with returned 1
   @pre    validPacket() with returned 1
   @code    
      radioInit();
      if (radioReceive())
        if (validPacket()){
          printf("packet was sucsesfully received with correct packet head");
          copyBufferToMemory();
          }
   @endcode
   @note   function save packet at place defined in packet head
**/
void copyBufferToMemory(void){
  //extracting number of actual packets
  actualPacket = Buffer[1]-CHAR_OFFSET;
  
  #if SIMULATE_RETX
      dmaSend(" s ",3);
      //dma_printf("saving actualPacket %d ",actualPacket);
  #endif
  #if BINARY_MODE
  memcpy((&pktMemory[actualRxBuffer][actualPacket-1][0]),Buffer,PktLen);//copy packet to memory
  lenghtOfPkt[actualRxBuffer][actualPacket-1] = PktLen;
  #else
  strcpy(&pktMemory[actualRxBuffer][actualPacket-1][0],Buffer);//copy packet to memory
  #endif
}
/** 
   @fn     void ifMissPktGet(void)
   @brief  function check packet meory for missing packets, send rquest for 
           retransmit losted packets and save returned packets
   @pre    radioInit() must be called before this function is called.
**/
void ifMissPktGet(void)
{
  signed char ch, i, numOfReTxPackets = 0, retransmision=0;
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
      //if packet was received copy buffered packet
      if (radioRecieve()){
        if (validPacket())
          copyBufferToMemory();
      }
      else{//send again request to get miss packets
        if (retransmision < RETRANSMISION -1){
          rf_printf(str);
          i=0;//-1
        }
        else
          break;
      }
    }
  }
}
/** 
   @fn     void flushBufferedPackets(void)
   @brief  rotate memory and start sending received packets on UART with DMA
   @see    pktMemory
   @pre    uartInit() must be called before this function is called.
   @note   all managment about sending packets is in @see DMA_UART_TX_Int_Handler()
**/
void flushBufferedPackets(void){
  
  //wait untill all packets are flushed
  while(flush_flag);
  //switch buffer 
  actualRxBuffer++;
  actualTxBuffer++;
  if(actualTxBuffer >= 2)
    actualTxBuffer=0;
  if(actualRxBuffer >= 2)
    actualRxBuffer=0;
  
  dmaTxPkt =0;
  flush_flag =1;
  //call DMA_UART_TX_Int_Handler all managment is inside this function
  DMA_UART_TX_Int_Handler ();
}

/** 
   @fn     void synchronize(void)
   @brief  send synchronization packets in constant time delays
   @pre    radioInit() must be called before this function is called.
   @note   function flags controlled by @see GP_Tmr0_Int_Handler  and @see UART_Int_Handler
**/
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
/** 
   @fn     char zeroPacket(void)
   @brief  check Buffer if is zero packet
   @note   zero packet mean that slave have no buffered packets
   @return  1 == zero packet, 0 == no zero packet
**/
char zeroPacket(void){
  char zero[3]={CHAR_OFFSET,CHAR_OFFSET,CHAR_OFFSET};
  zero[0]+=slave_ID;
  if (memcmp(Buffer,zero,3)==0)
    return 1;
  return 0;
}
/** 
   @fn     char receivePackets(void)
   @brief  receive all packets of sended time slot;
   @pre    radioInit() must be called before this function is called.
   @pre    rf_printf("1slot") or equivalent should be called first.
   @code    
      radioInit();
      rf_printf("1slot");//slot identificator
      if (receivePackets())
        flushPacktes();//send all received packtes on UART
   @endcode
   @note   function si also retransmitin slot ID packtes if no response
**/
char receivePackets(void){
  char retransmision=0; //number of retransmision attempt
  char received = 0;//number of received valid packets
  char count = 0;
  
  while (1){//loop for receiving all expecting packets
    
    //if one packet received before timeout
    if (radioRecieve()){
      if (zeroPacket())
        return 0;
      if (validPacket())
        copyBufferToMemory();
      else
        return 0;//if not recognizet packet
      received ++;
      
      if ((actualPacket >= numOfPkt[actualRxBuffer]))
        return received;
    }
    
    else {//try retransmit again if no one received packet 
      if (firstRxPkt == 0)
        if (retransmision < RETRANSMISION ){
          rf_printf(TIME_SLOT_ID_MASTER);//send again slot ID
          retransmision++;
        }
        else//if nothing after RETRANSMISION times
          return 0;
    }
    if (firstRxPkt == 1)
      if (count >= numOfPkt[actualRxBuffer])
        return -1;//missing some packets
  }
}
/** 
   @fn     void initializeNewSlot(void)
   @brief  Initialize variables for new ID slot
**/
void initializeNewSlot(void){
  slave_ID++;//increment slave number 
  if (slave_ID > NUM_OF_SLAVE)
    slave_ID = 1;
  
  firstRxPkt=0;
  numOfPkt[actualRxBuffer]= 0;
}

/** 
   @fn     void SetInterruptPriority (void)
   @brief  Initialize interrupt priority
**/
void SetInterruptPriority (void){
  NVIC_SetPriority(UART_IRQn,5);//receiving directives
  NVIC_SetPriority(DMA_UART_TX_IRQn,7);//terminate DMA TX priority
  NVIC_SetPriority(TIMER1_IRQn,8);//blinking low priority
  NVIC_SetPriority(TIMER0_IRQn,9);//terminate transmiting higher prioritz
  NVIC_SetPriority(EINT8_IRQn,10);//highest priority for radio interupt
}
/** 
   @fn     int main(void)
   @brief  main function of master program
   @note   Program is using time multiplex to receive data from 4 slave devices
   @return  int 
**/
int main(void)
{ 
  signed char i,retransmision;
  
  WdtGo(T3CON_ENABLE_DIS);
  
  uartInit();
  ledInit();
  radioInit();
  SetInterruptPriority ();

  while(1)
  {
    //send slot identificator
    rf_printf(TIME_SLOT_ID_MASTER);//start packet for new multiplex

    if (receivePackets()){//if some data packets are received
      #if DEBUG_MESAGES
        dma_printf("redeived %d pkts ", numOfPkt[actualRxBuffer]);
      #endif
      ifMissPktGet();//get back if losted some packets
      flushBufferedPackets();//send on UART received packets
    }
    
    //if synchronize message received
    if (sync_flag == 1)
      synchronize();
    
    initializeNewSlot();
  }
}

///////////////////////////////////////////////////////////////////////////
// GP Timer1 Interrupt handler used for blinking led
///////////////////////////////////////////////////////////////////////////
/** 
    @fn      void GP_Tmr1_Int_Handler (void)
    @brief   Interrupt handler for measuring troughput
    @note    only if THROUGHPUT_MEASURE is set in settings.h
    @see     settings.h
**/
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
/** 
    @fn      void GP_Tmr0_Int_Handler(void)
    @brief   Interrupt handler for synchronization timing
    @see     synchronize()
**/
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
/** 
    @fn      void DMA_UART_TX_Int_Handler (void)
    @brief   Interrupt handler managing sending content of pktMemory on UART with DMA
    @note    also is terminating DMA transaction of function dma_send()
    @see     dma_send()
    @see     flushPackets()
**/
void DMA_UART_TX_Int_Handler (void)
{
  int len=0;
  UrtDma(0,0);  // prevents further UART DMA requests
  DmaChanSetup ( UARTTX_C , DISABLE , DISABLE );// Disable DMA channel
  
  if(flush_flag == 1){
    if(dmaTxPkt < (numOfPkt[actualTxBuffer])){//packet itete 0..as needed
      
      dmaTxPtr = &pktMemory[actualTxBuffer][dmaTxPkt][0];//pointer at actuall packet
      
      if(dmaTxPtr[1]!='w'){//try if packet is received waiting flag
        
        #if BINARY_MODE
          len = lenghtOfPkt[actualTxBuffer][dmaTxPkt];
        #else
          while(dmaTxPtr[len]!='\0'){//get lenght of packet
            len++;
          }
        #endif
          
        #if SEND_HEAD
          dmaSend(dmaTxPtr,len);//send data with head
        #else
          dmaSend(dmaTxPtr+HEAD_LENGHT, len-HEAD_LENGHT);//send only data without head
        #endif
        
        
//        dmaTx_flag=1;
      }
      else{
        dma_printf("missing packet %d ",dmaTxPkt+1);//message about missing packet
//        dmaTx_flag = 1 ;
      }
      dmaTxPkt++;
    }
    else{ //all data sended
      flush_flag=0;
    }
  }
}

///////////////////////////////////////////////////////////////////////////
// DMA UART RX Interrupt handler 
///////////////////////////////////////////////////////////////////////////
/** 
    @fn      void DMA_UART_RX_Int_Handler()
    @brief   Interrupt handler terminating DMA receiving transaction 
    @note    not used
**/
void DMA_UART_RX_Int_Handler   ()
{
  UrtDma(0,0); // prevents additional byte to restart DMA transfer
}
///////////////////////////////////////////////////////////////////////////
// UART Interrupt handler 
// used for receiving directives
// function is taken from example UARTLoopback.c and modified
///////////////////////////////////////////////////////////////////////////
/** 
    @fn      void UART_Int_Handler ()
    @brief   Interrupt handler waiting for SYNC$ word to set synchronization flag
    @note    function is taken from example UARTLoopback.c and modified
    @see     synchronize()
**/
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
