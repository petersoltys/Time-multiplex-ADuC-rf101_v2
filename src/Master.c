/**
 *****************************************************************************
   @file     Master.c
   @brief    prigram used for Receiving data in time multiplex
             data are received via RF Interface and sended to UART
             working with Slave.c
             tested on ADucRF101MKxZ

             

   @version     'V2.2'-15-gaba903a
   @supervisor  doc. Ing. Milos Drutarovsky Phd.
   @author      Bc. Peter Soltys
   @date        26.05.2016(DD.MM.YYYY)

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
#include <stdarg.h>
#include "library.h"
#include "settings.h"

#define CHAR_OFFSET '0' 

#define LED_OFF DioSet(pADI_GP4,BIT2)   //led off
#define LED_ON  DioClr(pADI_GP4,BIT2)   //led on


RIE_Responses  RIE_Response = RIE_Success;
uint8_t        Buffer[PACKETRAM_LEN];
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

#pragma pack(1)
struct pkt_memory {
  uint8_t packet[NUM_OF_PACKETS_IN_MEMORY][PACKET_MEMORY_DEPTH];
  uint8_t lenghtOfPkt[NUM_OF_PACKETS_IN_MEMORY];
  uint8_t numOfPkt;
}pktMemory[2];
/*
//            [level ][ packet num.][packet data]
uint8_t pktMemory[2][NUM_OF_PACKETS_IN_MEMORY][PACKET_MEMORY_DEPTH];

          // [level]
char numOfPkt[2] = {0,0};   //similar like pktMemory
                      // [level][lenght]
uint8_t lenghtOfPkt[2][NUM_OF_PACKETS_IN_MEMORY] ;   //similar like pktMemory
*/
uint8_t actualPacket;

int8_t actualRxBuffer=0,actualTxBuffer=1;

//char lastRadioTransmitBuffer[PACKET_MEMORY_DEPTH];    //buffer with last radio dommand
uint8_t dmaTxBuffer[2][(PACKET_MEMORY_DEPTH*2)];        //buffer for DMA TX UART channel
uint8_t dmaTxPingPong = 0;                              //ping pong pointer in dmaTxBuffer
#define UART_BUFFER_DEEP 50
uint8_t rxBuffer[UART_BUFFER_DEEP];    //buffer for RX UART channel - only for directives
uint8_t dmaMessageBuffer[UART_BUFFER_DEEP]; 

uint8_t slave_ID = 1;   //Slave ID
int8_t send=FALSE;
//int8_t nextRxPkt=0;

int8_t TX_flag=FALSE,RX_flag=FALSE, flush_flag=FALSE;
int8_t sync_flag = FALSE;    //flag starting sending synchronization packet
int8_t sync_wait = FALSE;
int8_t firstRxPkt = FALSE;
uint8_t rxUARTcount=0;
uint8_t rxPAcketTOut=0;

//variables for DMA_UART_TX_Int_Handler
uint8_t  dmaTxPkt = 0;          /*!< @brief global variable, pointer pointing at actually transmitted packet trought UART */
int8_t   dmaTx_flag = FALSE;    /*!< @brief flag about transmitting operation */
int8_t   dmaTxReady = FALSE;    /*!< @brief flag mean that all variables are set for transmit */
uint8_t  dmaTxLen;              /*!< @brief global variable, with lenght of packet to send */
uint8_t* dmaTxPtr;              /*!< @brief global pointer UART transmitting */

#if CHECK_RANDOM
//to ensure arrangement variables in byte by byte grid structure (without "bubles")
//is used directive #pragma pack(1)
#pragma pack(1)
struct rand_pkt {
  char  Slave;
  char  slave_id;
  uint32_t  randomPktNum;
  #if WEEAK_RANDOM_FUNCTION == 1
  static unsigned long next;    //variable for PRNG
  #else
  long next;                    //variable for PRNG
  #endif
  int   rnadom;
} random_pkt[NUM_OF_SLAVE];  

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
  DioCfg(pADI_GP1,0x9);         // UART functionality on P1.0/P1.1

  UrtIntCfg(0,COMIEN_ERBFI);    // enable Rx interrupts
  NVIC_EnableIRQ(UART_IRQn);    // setup to receive data using interrupts
  
  DmaInit();                    // Create DMA descriptor
  //DmaTransferSetup(UARTTX_C,  20,   Buffer);
  NVIC_EnableIRQ ( DMA_UART_TX_IRQn );    // Enable DMA UART TX interrupt
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
  GptLd (pADI_TM0, SYNC_INTERVAL);        // Interval of 2ms
  GptCfg(pADI_TM0, TCON_CLK_UCLK, TCON_PRE_DIV256, TCON_ENABLE|TCON_MOD_PERIODIC);
  while (GptSta(pADI_TM0)& TSTA_CON);     // wait for sync of TCON write. required because of use of asynchronous clock
  GptClrInt(pADI_TM0,TCLRI_TMOUT);
  while (GptSta(pADI_TM0)& TSTA_CLRI);    // wait for sync of TCLRI write. required because of use of asynchronous clock
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
   @fn     void  dmaSend(void* buff, int len)
   @brief  send one packet on UART with DMA controller
   @param  buff :{} pointer to data to send
   @param  len :{int range} number of bytes to send
   @pre    uartInit() must be called before this can be called.
   @code  
        uartInit();
        len =vsprintf(dmaMessageBuffer, format,args);//vlozenie formatovaneho retazca do buff
        dmaSend(dmaMessageBuffer,len);
   @endcode
   @note    after end of transmision is called DMA_UART_TX_Int_Handler (void)
   @see DMA_UART_TX_Int_Handler
**/
void  dmaSend(void* buff, int len){
  //DMA UART stream
  DmaInit();
  DmaTransferSetup(UARTTX_C, len, buff);
  DmaChanSetup(UARTTX_C,ENABLE,ENABLE);   // Enable DMA channel  
  UrtDma(0,COMIEN_EDMAT);
}
/** 
   @fn     uint16_t dma_printf(const char * format , ...)
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
   @return  uint16_t - number of sending chars (if == 0 error)
**/
uint16_t dma_printf(const char * format /*format*/, ...)
{
  uint16_t len;
  va_list args;
  va_start( args, format );
   
  len = vsprintf((char*)dmaMessageBuffer, format,args);    //vlozenie formatovaneho retazca do buff
  dmaSend(dmaMessageBuffer,len);

  va_end( args );
  return len;
}


/** 
   @fn     void radioSend(void* buff, uint8_t len)
   @brief  send one packet trought radio interface
   @param  buff : pointer at memory to be sended
   @param  len : {0-240} number of bytes to be sended
   @pre    radioInit() must be called before this function is called.
   @code    
      len=vsprintf(buff, format,args);    //@see rf_printf();
      if(len<PACKETRAM_LEN){                        //check max lenght 
        radioSend(buff,len+1);            //send formated packet
   @endcode
   @note    output stream is trought radio interface
            function is waiting until whole packet is trnsmited
**/
void radioSend(void* buff, uint8_t len){
  
  if (RIE_Response == RIE_Success){   //send packet
    RIE_Response = RadioTxPacketVariableLen(len, buff); 
    RX_flag = FALSE;
  }
  
  if (RIE_Response == RIE_Success){   //wait untill packet sended
    while(!RadioTxPacketComplete());
  }
  
  if (RIE_Response == RIE_Success){   //set again receiving state
    RIE_Response = RadioRxPacketVariableLen(); 
    RX_flag = TRUE;
  }
  
    //DMA UART stream
#if TX_STREAM
  dmaSend(buff,len-1);
#endif
}
/** 
   @fn     uint8_t rf_printf(const char * format , ...)
   @brief  equivalent to function printf from library stdio.h with stream trought radio interface
   @param  format : {} pointer at string to formating output string
   @param  ... :{} additive parameters for formating string
   @pre    radioInoit() must be called before this function.
   @code    
      radioInit();
      int num = 10;
      rf_printf("number %d",num);
   @endcode
   @note    output stream is trought radio interface
            function is waiting until whole packet is transmited
            any one formated string (call) is sended in one packet
   @return  uint8_t - number of sending chars (if == 0 error)
**/
uint8_t rf_printf(const char * format /*format*/, ...){
  char buff[256];
  uint8_t len;
  va_list args;
  va_start( args, format );

  len=vsprintf(buff, format,args);    //vlozenie formatovaneho retazca do buff
  if(len>PACKETRAM_LEN){                        //kontrola maximalnej dlzky retazca
    va_end( args );
    return 0;
  }
      
  radioSend(buff,len+1);              //send formated packet

  va_end( args );
  return len;
}

/** 
   @fn     void binToHexa(uint8_t* from, uint8_t* to, uint16_t len )
   @brief  function to converting binarz data to ASCII chars for hexadecimal system
   @param  uint8_t* from : pointer at binary data in memory
   @param  uint8_t* to : pointer at destination place in memory for hexadecimal ASCII chars
   @param  uint16_t len : source (binary) lenght of data, hexadecimal data are *2 lenght
   @note   output lenght in destination memory place is double lenght as source lenght
**/

void binToHexa(uint8_t* from, uint8_t* to, uint16_t len ){

  uint16_t i;
  for (i = 0 ; i < len ; i++){    //conversion of binary data to ascii chars 0 ... F 
    *to = ((*from & 0xf0)>>4)+'0';
    if (*to > '9')         //because ASCII table is 0123456789:;<=>?@ABCDEF
      *to += 7;
    
    to++;                  //increment pointer
    
    *to = (*from & 0x0f) + '0';
    if (*to > '9')         //because ASCII table is 0123456789:;<=>?@ABCDEF
      *to += 7;
        
    to++;                 //increment pointer
    from++;
  }
}
/*
void binToHexa(uint8_t* from, uint8_t* to, uint16_t len ){
  uint8_t ch;
  uint16_t i;
  for (i=0;i<len;i++){    //conversion of binary data to ascii chars 0 ... F 
    ch = (from[i]&0x0f) + '0';
    if (ch > '9')         //because ASCII table is 0123456789:;<=>?@ABCDEF
      ch += 7;
    to[(i*2)+1] = ch;
    ch = ((from[i]&0xf0)>>4)+'0';
    if (ch > '9')         //because ASCII table is 0123456789:;<=>?@ABCDEF
      ch += 7;
    to[(i*2)] = ch;
  }
}*/

void setTransfer(void){
  if(flush_flag == TRUE && dmaTxReady == FALSE && dmaTx_flag == FALSE){
    if(dmaTxPkt < (pktMemory[actualTxBuffer].numOfPkt)){      //packet iterate 0..as needed
      
      dmaTxPtr = &pktMemory[actualTxBuffer].packet[dmaTxPkt][0];   //pointer at actuall packet
      
      if(dmaTxPtr[1]!='w'){                         //try if packet is received waiting flag
        
        dmaTxLen = pktMemory[actualTxBuffer].lenghtOfPkt[dmaTxPkt];

        #if HEXA_TRANSFER
          binToHexa(dmaTxPtr,&dmaTxBuffer[dmaTxPingPong][0],dmaTxLen);
        #endif
          
        dmaTxReady = TRUE;
        if (dmaTx_flag == FALSE){
          dmaTx_flag = TRUE;
          DMA_UART_TX_Int_Handler();
        }
      }
      else{
        //dma_printf("missing packet %d ",dmaTxPkt+1);        //message about missing packet
      }
      dmaTxPkt++;
    }
    else{                                                   //all data sended
      flush_flag=FALSE;
    }
  }
}
/** 
   @fn     uint8_t radioRecieve(void)
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
   @note    function is also reding packet form radio interface to uint8_t Buffer[PACKETRAM_LEN]
   @return  uint8_t - 1 == packet received, 0 == packet was not received correctly before timout
**/
uint8_t radioRecieve(void){
  uint16_t timeout_timer = 0;
  
  if (RIE_Response == RIE_Success && RX_flag == FALSE ){
    RIE_Response = RadioRxPacketVariableLen();
    RX_flag = TRUE;
    rxPAcketTOut=0;
  }
  //watchdog at radio interface
  if (rxPAcketTOut > RX_PKT_TOUT_CNT){
    
    if (RIE_Response == RIE_Success)
      RIE_Response = RadioHWreset();
    
    radioInit();

    if (RIE_Response == RIE_Success)
      RIE_Response = RadioRxPacketVariableLen();
    RX_flag = TRUE;
    rxPAcketTOut=0;
  }

  if (RIE_Response == RIE_Success && RX_flag == TRUE){
    while (!RadioRxPacketAvailable()){
      timeout_timer++;
      setTransfer();
      //turn on led if nothing is received after timeout
      if (timeout_timer > T_TIMEOUT){
        LED_ON;
        rxPAcketTOut++;
        return 0;
      }
    }
    rxPAcketTOut = 0;
    RX_flag=FALSE;
  }
  
  LED_ON;
  //citanie paketu z rf kontrolera
  if (RIE_Response == RIE_Success)
    RIE_Response = RadioRxPacketRead(sizeof(Buffer),&PktLen,Buffer,&RSSI);
  LED_OFF;
  
    //DMA UART stream
  #if RX_STREAM
    dmaSend(Buffer,PktLen-1);
  #endif
  
  //back to receiving mode
  if (RIE_Response == RIE_Success && RX_flag == FALSE){
    RIE_Response = RadioRxPacketVariableLen();   
    RX_flag = TRUE;
  }
  return 1;
}
/** 
   @fn     uint8_t validPacket(void)
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
   @return uint8_t - 1 == valid packet head, 0 == invalid packet head
**/
uint8_t validPacket(void){
  uint8_t i,slv,pktNum;
  
    //extracting slave identifier
  slv = Buffer[0]-CHAR_OFFSET;
  //extracting number of actual packets
  actualPacket = Buffer[1]-CHAR_OFFSET;
  //extracting number of actual packets
  pktNum = Buffer[2]-CHAR_OFFSET;
  
  //if first received packet extract num of packets and check slave ID
  if (firstRxPkt==FALSE){
    firstRxPkt=TRUE;
    
    //extracting number of buffered TX packets
    pktMemory[actualRxBuffer].numOfPkt = pktNum;
    if (pktMemory[actualRxBuffer].numOfPkt > NUM_OF_PACKETS_IN_MEMORY)
      {return 0;}
    
    //write "waiting flag"- ("w") for expected packets
    for (i=0; i < pktMemory[actualRxBuffer].numOfPkt ;i++)
      pktMemory[actualRxBuffer].packet[i][1]='w';
  }
  
  if (slv != slave_ID)    //check of slave id (number) expected/transmiting
    dma_printf("slave id dismatch or not recognizet packet");
  
  if (actualPacket==0)    //if zero packet
    return 0;
  
  //if number of packet is in range
  if (actualPacket > 0 && actualPacket <= NUM_OF_PACKETS_IN_MEMORY)
    //if number of slave is in range
    if (slv <= NUM_OF_SLAVE && slv > 0)
        //if expected total number of packet is same as at begining
        if (pktNum == pktMemory[actualRxBuffer].numOfPkt)
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
  uint8_t* buf = &pktMemory[actualRxBuffer].packet[actualPacket-1][0];
  //extracting number of actual packets
  actualPacket = Buffer[1]-CHAR_OFFSET;
  
  #if SIMULATE_RETX
      dmaSend(" s ",3);
      //dma_printf("saving actualPacket %d ",actualPacket);
  #endif
  
  memcpy(buf,Buffer,PktLen);//copy packet to memory
  pktMemory[actualRxBuffer].lenghtOfPkt[actualPacket-1] = PktLen;
}
/** 
   @fn     void ifMissPktGet(void)
   @brief  function check packet meory for missing packets, send rquest for 
           retransmit losted packets and save returned packets
   @pre    radioInit() must be called before this function is called.
**/
void ifMissPktGet(void)
{
  int8_t ch, i, numOfReTxPackets = 0, retransmision=0;
  char str[25]=RETRANSMISION_ID;
  char str2[2]={0,0};
  
////////check for missing packets////////////////////////////////////
  for(i=0; i < pktMemory[actualRxBuffer].numOfPkt ; i++)  //iterate throught packets
  {
    ch = pktMemory[actualRxBuffer].packet[i][1];       //extract number of packet or waiting flag
    #if SIMULATE_RETX
      ch='w';
    #endif
    if( ch == 'w')                      //check wait flag
    {
      str2[0]= i+CHAR_OFFSET+1;

      strcat(str,str2);                 //append number of missed packet
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
      else{     //send again request to get miss packets
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
  uint16_t counter=0;
  //wait untill all packets are flushed
  while(flush_flag==TRUE){
    setTransfer();
    counter++;
  }
  if (counter)
    counter=0;
  //switch buffer 
  actualRxBuffer++;
  actualTxBuffer++;
  if(actualTxBuffer >= 2)
    actualTxBuffer=0;
  if(actualRxBuffer >= 2)
    actualRxBuffer=0;
  
  dmaTxPkt =0;
  flush_flag =TRUE;
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
  sync_wait = TRUE;
  while(sync_wait == TRUE);      //wait for timer interrupt
  sync_wait = TRUE;
  radioSend("SYNC3",6);       //first syn. packet
  while(sync_wait == TRUE);
  sync_wait = TRUE;
  radioSend("SYNC2",6);
  while(sync_wait == TRUE);
  radioSend("SYNC1",6);       //last syn. packet

  sync_wait = TRUE;
  while(sync_wait == TRUE);      //free time before transmitting
  sync_flag = FALSE;
} 
/** 
   @fn     uint8_t zeroPacket(void)
   @brief  check Buffer if is zero packet
   @note   zero packet mean that slave have no buffered packets
   @return uint8_t - 1 == zero packet, 0 == no zero packet
**/
uint8_t zeroPacket(void){
  char zero[3]={CHAR_OFFSET,CHAR_OFFSET,CHAR_OFFSET};
  zero[0]+=slave_ID;
  if (memcmp(Buffer,zero,3/*chars to compare*/)==0)
    return 1;
  return 0;
}
/** 
   @fn     int8_t receivePackets(void)
   @brief  receive all packets of sended time slot;
   @pre    radioInit() must be called before this function is called.
   @pre    rf_printf("1slot") or equivalent should be called first.
   @code    
      radioInit();
      rf_printf("1slot");       //slot identificator
      if (receivePackets())
        flushPacktes();         //send all received packtes on UART
   @endcode
   @note   function si also retransmitin slot ID packtes if no response
**/
int8_t receivePackets(void){
  uint8_t retransmision=0;     //number of retransmision attempt
  uint8_t received = 0;        //number of received valid packets
  uint8_t unsuccessfulCounter = NUM_OF_PACKETS_IN_MEMORY;
  
  //send slot identificator
  rf_printf(TIME_SLOT_ID_MASTER);   //start packet for new multiplex
  
  while (1){                //loop for receiving all expecting packets
    
    //if one packet received before timeout
    if (radioRecieve()){
      if (zeroPacket())
        return 0;
      if (validPacket())
        copyBufferToMemory();
      else
        return 0;           //if not recognizet packet
      received ++;
      
      if ((actualPacket >= pktMemory[actualRxBuffer].numOfPkt))
        return received;
    }
    else {                  //try retransmit again if no one received packet 
      if (firstRxPkt == FALSE){
        if (retransmision < RETRANSMISION ){
          rf_printf(TIME_SLOT_ID_MASTER);   //send again slot ID
          retransmision++;
        }
        else{                                //if nothing after RETRANSMISION times
          return 0;
        }
      }else{
        //if no receiving more packets
        if ((int16_t)(pktMemory[actualRxBuffer].numOfPkt - (received + unsuccessfulCounter)) <= 0 )
          return -unsuccessfulCounter;     //missing some packets
        else
          unsuccessfulCounter++;
      }
    }
//    if (firstRxPkt == TRUE)
//      if (count >= pktMemory[actualRxBuffer].numOfPkt)
//        return -1;                          //missing some packets
  }
}
/** 
   @fn     void initializeNewSlot(void)
   @brief  Initialize variables for new ID slot
**/
void initializeNewSlot(void){
  slave_ID++;     //increment slave number 
  if (slave_ID > NUM_OF_SLAVE)
    slave_ID = 1;
  
  firstRxPkt=FALSE;
  pktMemory[actualRxBuffer].numOfPkt= 0;
}

/** 
   @fn     void SetInterruptPriority (void)
   @brief  Initialize interrupt priority
**/
void SetInterruptPriority (void){
  
  NVIC_SetPriority(TIMER1_IRQn,5);        //troughput timer
  NVIC_SetPriority(TIMER0_IRQn,4);        //synchronization timer
  NVIC_SetPriority(DMA_UART_TX_IRQn,3);   //terminate DMA TX priority
  NVIC_SetPriority(UART_IRQn,2);          //receiving directives (short messages)
  NVIC_SetPriority(EINT8_IRQn,1);         //highest priority for radio interupt
}


/** 
   @fn     void srandc(unsigned int seed)
   @brief  function is setting initial value of PRNG
   @see    randc(void)
**/
void srandc(unsigned int seed , struct rand_pkt* random_packet)
{
  #if WEEAK_RANDOM_FUNCTION == 1
  random_packet.next = seed;
  #else
  random_packet->next = (long)seed;
  #endif
}
/** 
   @fn     int randc(void)
   @brief  PRNG function of linear kongurent generator
   @note   before first fun should be initialized seed value
   @note   PRNG values can change by macro WEEAK_RANDOM_FUNCTION
   @see    WEEAK_RANDOM_FUNCTION
   @see    srandc()
   @return int - rnadom number
**/
int randc(struct rand_pkt* random_packet) // RAND_MAX assumed to be 32767
{
  #if WEEAK_RANDOM_FUNCTION == 1
  //official ANSI implementation
    random_packet->next = random_packet->next * 1103515245 + 12345;
    return (unsigned int)(random_packet->next/65536) % 32768;
  #else
  //official random implementation for C from CodeGuru forum
    return(((random_packet->next = random_packet->next * 214013L + 2531011L) >> 16) & 0x7fff);
  #endif
}

void initializeRandomCheck(void){
  char slave;
  //initialize random packets
  for (slave = 0;slave < NUM_OF_SLAVE; slave++){
    random_pkt[slave].Slave = 'S';
    random_pkt[slave].slave_id = slave+1;
    random_pkt[slave].randomPktNum = 0;
    random_pkt[slave].next = RAND_SEED;
  }
}

void checkRandomBufferedPackets(void){
  uint8_t packet, word;
  uint32_t temp_pkt_num_ref, temp_pkt_num_mem;
  struct rand_pkt *rnd_pkt_in_memory, *rnd_pkt_ref;
  
  for(packet = 0; packet < pktMemory[actualRxBuffer].numOfPkt; packet++){
    
    //pointer in packet memory
    rnd_pkt_in_memory = (struct rand_pkt*) &pktMemory[actualRxBuffer].packet[packet][HEAD_LENGHT]; 
    for (word = (pktMemory[actualRxBuffer].lenghtOfPkt[packet]/PRNG_PKT_LEN); word > 0 ; word --){
      //check one random word
      if (rnd_pkt_in_memory->Slave == 'S'){ //if random Packet start char
        if (rnd_pkt_in_memory->slave_id <= NUM_OF_SLAVE){ //if valid slave id
          //initialze new random word
          rnd_pkt_ref = &random_pkt[rnd_pkt_in_memory->slave_id-1];
          rnd_pkt_ref->randomPktNum++;
          rnd_pkt_ref->rnadom = randc(rnd_pkt_ref);
          if (rnd_pkt_ref->randomPktNum == rnd_pkt_in_memory->randomPktNum){
            if (rnd_pkt_ref->next == rnd_pkt_in_memory->next){
              if (rnd_pkt_ref->rnadom == rnd_pkt_in_memory->rnadom){
                //packet valid
              }else{
                printf("\nwrong random number");
              }
            }else{
              printf("\nwrong seed number at packet number %d",rnd_pkt_ref->randomPktNum);
              rnd_pkt_ref->next = rnd_pkt_in_memory->next;
            }
          }else{
            temp_pkt_num_ref = rnd_pkt_ref->randomPktNum;
            temp_pkt_num_mem = rnd_pkt_in_memory->randomPktNum;
            printf("\nmissing %d packets",(temp_pkt_num_mem - temp_pkt_num_ref));
            //set values to synchronization
            rnd_pkt_ref->randomPktNum = rnd_pkt_in_memory->randomPktNum;
            rnd_pkt_ref->next = rnd_pkt_in_memory->next;
          }
        }else{
          printf("\nslave number is out of range");
        }
      }else{
        printf("\nwrong synchronizing of packet");
      }
      rnd_pkt_in_memory ++;
    }
  }
  actualRxBuffer++;
  actualTxBuffer++;
  if(actualTxBuffer >= 2)
    actualTxBuffer=0;
  if(actualRxBuffer >= 2)
    actualRxBuffer=0;
}
/**
   @fn     void checkIntegrityOfFirmware(void)
   @brief  fgunction to check firmware
   @note   for right function is nessesary to download firmware with external 
           programmer (CM3WSD) tool from .hex file located in obj folder
   @note   programmer CM3WSD is located in "Integrity" folder
   @code   ::code in conv.bat
   @pre    for right generation of .hex file must be call script Integrity.bat located in Integrity folder
*/
void checkIntegrityOfFirmware(void){
  #define BEGIN_OF_CODE_MEMORY    (uint8_t *)0x0  // pointer at begining of code memory
  #define LENGHT_OF_CODE_MEMORY   0x20000         // end of code memory
/*
512(bytes is one page)*256(pages) = 131072 = 0x20000
*/

  crc retval;
  
  #if CRC_FAST
    crcInit();
  //the reason why use crcSlow is that crcFast is using much more memory
    retval = crcFast(BEGIN_OF_CODE_MEMORY,LENGHT_OF_CODE_MEMORY);
  #else
    retval = crcSlow(BEGIN_OF_CODE_MEMORY,LENGHT_OF_CODE_MEMORY);
  #endif
  
  if (retval == 0){
    printf("\nintegrity check ok\n");
    LED_ON;
  }
  else{
    printf("\nproblem in integrity of firmware \n");  
    LED_OFF;
    //while(1);
  }
}

/** 
   @fn     int main(void)
   @brief  main function of master program
   @note   Program is using time multiplex to receive data from 4 slave devices
   @return int 
**/
int main(void)
{ 
  

  WdtGo(T3CON_ENABLE_DIS);
  memset(pktMemory, 0, sizeof(pktMemory));  
  
  uartInit();
  checkIntegrityOfFirmware();
  ledInit();
  radioInit();
  SetInterruptPriority ();
  #if CHECK_RANDOM
  initializeRandomCheck();
  #endif

  while(1)
  {
    
    if (receivePackets()){            //if some data packets are received
      #if DEBUG_MESAGES
        dma_printf("redeived %d pkts ", numOfPkt[actualRxBuffer]);
      #endif
      ifMissPktGet();                 //get back if losted some packets
      
      
      #if CHECK_RANDOM == 1
      checkRandomBufferedPackets();
      #else
      flushBufferedPackets();         //send on UART received packets
      #endif
    }
    
    //if synchronize message received
    if (sync_flag == TRUE)
      synchronize();
    
    initializeNewSlot();
  }
}

///////////////////////////////////////////////////////////////////////////
// GP Timer1 Interrupt handler used for blinking led
///////////////////////////////////////////////////////////////////////////
/** 
    @fn      void GP_Tmr1_Int_Handler (void)
    @brief   Interrupt handler for timer 1 already unused
**/
void GP_Tmr1_Int_Handler (void)
{

  if (GptSta(pADI_TM1)== TSTA_TMOUT)    // if timout interrupt
  {

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
  if (GptSta(pADI_TM0)== TSTA_TMOUT){     // if timout interrupt
    sync_wait=sync_wait;
    if (sync_flag==FALSE)                     //if synchronize is complete
      GptCfg(pADI_TM0, TCON_CLK_UCLK, TCON_PRE_DIV256, TCON_ENABLE_DIS);    //stop timer
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
  UrtDma(0,0);                       // prevents further UART DMA requests
  DmaChanSetup ( UARTTX_C , DISABLE , DISABLE );    // Disable DMA channel
  
  if (dmaTxReady == TRUE){
    #if HEXA_TRANSFER
      #if SEND_HEAD
        dmaSend(&dmaTxBuffer[dmaTxPingPong][0],dmaTxLen*2);        //send data with head
      #else
        dmaSend(&dmaTxBuffer[dmaTxPingPong][0]+HEAD_LENGHT, (dmaTxLen*2)-(HEAD_LENGHT*2) );   //send only data without head
        if (dmaTxPingPong > 0)//change ping pong buffer
          dmaTxPingPong = 0;
        else
          dmaTxPingPong = 1;
      #endif
    #else
      #if SEND_HEAD
        dmaSend(dmaTxPtr,dmaTxLen);          //send data with head
      #else
        dmaSend(dmaTxPtr+HEAD_LENGHT, (dmaTxLen)-(HEAD_LENGHT) );   //send only data without head
      #endif
    #endif
    dmaTxReady = FALSE;//set flag about transfer
  }
  else{
    dmaTx_flag = FALSE;
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
  UrtDma(0,0);    // prevents additional byte to restart DMA transfer
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
  uint8_t ucCOMIID0; 
  char ch;
 
  ucCOMIID0 = UrtIntSta(0);      // Read UART Interrupt ID register

  if ((ucCOMIID0 & COMIIR_STA_RXBUFFULL) == COMIIR_STA_RXBUFFULL)    // Receive buffer full interrupt
  {
    ch  = UrtRx(0);             //call UrtRd() clears COMIIR_STA_RXBUFFULL
  
    rxBuffer[rxUARTcount]= ch;
    rxUARTcount++;
    
    if (ch == '$')
      if (memcmp(&rxBuffer[rxUARTcount-5],"SYNC$",5/*chars to compare*/) == 0){   //end of packet pointer
        sync_flag = TRUE;
        memset(rxBuffer,'\0', rxUARTcount);                   //clear buffer
        rxUARTcount = 0;
      }
    if (rxUARTcount >= UART_BUFFER_DEEP)                      //check not overflow buffer
      rxUARTcount = 0;
  }
} 
