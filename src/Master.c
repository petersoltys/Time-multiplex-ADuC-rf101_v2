/**
 *****************************************************************************
   @file     Master.c
   @brief    prigram used for Receiving data in time multiplex
             data are received via RF Interface and sended to UART
             working with Slave.c
             tested on ADucRF101MKxZ

             

   @author      Bc. Peter Soltys
   @supervisor  doc. Ing. Milos Drutarovsky Phd.
   @version     
   @date        16.01.2017(DD.MM.YYYY)

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
#include "PRNG.h"
#include "library.h"
#include "settings.h"

#define CHAR_OFFSET '0' 

#define LED_OFF DioSet(pADI_GP4,BIT2)   //led off
#define LED_ON  DioClr(pADI_GP4,BIT2)   //led on

#define YIELD setTransfer();

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

struct slaveStatistic{
  uint8_t active;
  uint16_t continualInactivity;
}slave[NUMBER_OF_SLAVES];

struct radioConfiguration{
  RIE_U32 BaseFrequency ;
  RIE_BaseConfigs BaseConfig ;
  RIE_ModulationTypes ModulationType ;
  RIE_PATypes PAType ;
  RIE_PAPowerLevel Power ;
  RIE_BOOL Whitening ;
  RIE_BOOL Manchester ;
}radioConf;
RIE_U32 BestFrequency ;

uint8_t actualPacket;

int8_t actualRxBuffer = 0, actualTxBuffer = 1;
  
#define UART_BUFFER_DEEP 50
uint8_t rxBuffer[2][UART_BUFFER_DEEP];    //buffer for RX UART channel - only for directives
uint8_t *rxPtr;
uint8_t rxPingPong = 0;
uint8_t rxUARTcount[2]={0,0};
uint8_t rxPAcketTOut=0;

//char lastRadioTransmitBuffer[PACKET_MEMORY_DEPTH];    //buffer with last radio dommand
uint8_t dmaTxBuffer[2][(PACKET_MEMORY_DEPTH*2)];        //buffer for DMA TX UART channel
uint8_t dmaTxPingPong = 0;                              //ping pong pointer in dmaTxBuffer
uint8_t dmaMessageBuffer[UART_BUFFER_DEEP]; 

uint8_t slave_ID = 1;   //Slave ID
int8_t send=FALSE;
//int8_t nextRxPkt=0;

int8_t TX_flag=FALSE,RX_flag=FALSE, flush_flag = FALSE;
int8_t sync_flag = FALSE;    //flag starting sending synchronization packet
int8_t sync_wait = FALSE;
int8_t firstRxPkt = FALSE;
int8_t messageFlag = FALSE;


//variables for DMA_UART_TX_Int_Handler
uint8_t  dmaTxPkt = 0;          /*!< @brief global variable, pointer pointing at actually transmitted packet trought UART */
int8_t   dmaTx_flag = FALSE;    /*!< @brief flag about transmitting operation */
int8_t   dmaTxReady[2] = {FALSE, FALSE};    /*!< @brief flag mean that all variables are set for transmit */
uint16_t dmaTxLen;              /*!< @brief global variable, with lenght of packet to send */
uint8_t* dmaTxPtr;              /*!< @brief global pointer UART transmitting */
uint8_t  dmaTxPktTotal;
uint16_t dmaTxTimeoutCounter=0;

struct PRNGslave slaves[NUMBER_OF_SLAVES];

void DMA_UART_TX_Int_Handler (void);
uint8_t rf_printf(const char * format /*format*/, ...);


void WriteToFlash(uint8_t *pArray, unsigned long ulStartAddress, unsigned int uiSize)
{
   unsigned int uiPollFEESTA = 0;
   volatile unsigned long *flashAddress;
   unsigned int i = 0;
   
   flashAddress = ( unsigned long      *)ulStartAddress;
   FeeWrEn(1);
   uiPollFEESTA = FeeSta();						// Read Status to ensure it is clear
   for (i = 0; i < uiSize; i++)
   { 
      uiPollFEESTA = 0;
      *flashAddress++  = *pArray++;
      do
         {uiPollFEESTA = FeeSta();}
      while((uiPollFEESTA & FEESTA_CMDBUSY) == FEESTA_CMDBUSY);
   }  
   FeeWrEn(0);       // disable a write to Flash memory
   do
     {uiPollFEESTA = FeeSta();}
   while((uiPollFEESTA & FEESTA_CMDBUSY) == FEESTA_CMDBUSY);
}



void readRadioConfiguration(){
  //initialize eeprom
  
  //read radioconfiguration from FLASH
  //memcpy(&radioConf, (const void*)0x1f000, sizeof(radioConf));
  //permanently set basic conf.
  memset(&radioConf,0xff,sizeof(radioConf));
  
  //set default radio configuration if eeprom is 
  if (radioConf.BaseFrequency == 0xffffffff){
    radioConf.BaseFrequency = BASE_RADIO_FREQUENCY;
    radioConf.BaseConfig = RADIO_CFG;
    radioConf.ModulationType = RADIO_MODULATION;
    radioConf.PAType = PA_TYPE;
    radioConf.Power = RADIO_POWER;
    radioConf.Whitening = DATA_WHITENING;
    radioConf.Manchester = RADIO_MANCHASTER;
    WriteToFlash((uint8_t *)&radioConf, 0x1f000, sizeof(radioConf));
  }
}

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
  // set pointer for receiving buffer
  rxPtr = rxBuffer[rxPingPong];
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

void setBestFrequency(){
  RIE_Response = RadioSetFrequency(radioConf.BaseFrequency);
  if(RIE_Response == RIE_Success){
    rf_printf("FREQ%d",BestFrequency);
    if (RIE_Response == RIE_Success){   //wait untill packet sended
      while(!RadioTxPacketComplete());
    }
    RIE_Response = RadioSetFrequency(BestFrequency);
  }
}
/** 
   @fn     void findFrequency(void)
   @brief  find radio frequency with lowest RSSI
   @see    settings.h
   @note   enabled if RADIO_FREQUENCY == 0
**/
void findFrequency(void){
  uint32_t f = MIN_RADIO_FREQUENCY ;
  RIE_S8  lowestRSSI;
  BestFrequency = MIN_RADIO_FREQUENCY;
  
  while (f < MAX_RADIO_FREQUENCY){
    
    RIE_Response = RadioSetFrequency(f);
    if(RIE_Response == RIE_Success)
      RadioRadioGetRSSI(&RSSI); //zisti uroven signalu na nastavenej frekvencii
    else
      printf("\nproblem during frequency searching#");
    #if DEBUG_MESAGES
    if(RIE_Response == RIE_Success){
      printf("\non frequency %d is RSSI = %d dB#",f,RSSI);
    }
    #endif
    if (RSSI<lowestRSSI){
      lowestRSSI = RSSI;
      BestFrequency = f;
    }
    if(RIE_Response == RIE_Success)
      f+=FREQ_STEP;                 //zmena frekvenice o 10Khz
  }
  setBestFrequency();
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
  //read configuration from eeprom
  readRadioConfiguration();
  // Initialise the Radio
  if (RIE_Response == RIE_Success)
     RIE_Response = RadioInit(radioConf.BaseConfig);  
  // Set modulation type
  if (RIE_Response == RIE_Success)
     RadioSetModulationType(radioConf.ModulationType);
  // Set the PA and Power Level
  if (RIE_Response == RIE_Success)
     RIE_Response = RadioTxSetPA(radioConf.PAType,radioConf.Power);
  // Set data whitening
  if (RIE_Response == RIE_Success)
     RIE_Response = RadioPayldDataWhitening(radioConf.Whitening);
  // Set data Manchaster encoding
  if (RIE_Response == RIE_Success)
    RadioPayldManchesterEncode(radioConf.Manchester);
    // Set the Frequency to operate at
  if (RIE_Response == RIE_Success)
    findFrequency();
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
#if SLOW_FLUSH == 0

/** 
   @fn     void setTransfer(void)
   @brief  function preparing data to send trought UART
   @see    DMA_UART_TX_Int_Handler
   @note   function is executing during waiting time in function radioRecieve
   @see    radioRecieve

**/
void setTransfer(void){
  uint8_t *pointer, localPingPong, *sourcePtr ;
  int16_t len;
  
  // set local ping pong
  localPingPong = dmaTxPingPong;  
  if (localPingPong >= 1)
    localPingPong = 0;
  else
    localPingPong = 1;
    
  if(flush_flag == TRUE && dmaTxReady[localPingPong] == FALSE){
    
    //packet iterate 0..as needed
    if(dmaTxPkt < dmaTxPktTotal){
      
      pointer = &pktMemory[actualTxBuffer].packet[dmaTxPkt][HEAD_LENGHT];   //pointer at actuall packet
      
      //try if packet is received waiting flag 'w'
      if(pointer[1]!='w'){
        
        len = pktMemory[actualTxBuffer].lenghtOfPkt[dmaTxPkt];
        
#if COMPRESSION
        dmaTxLen = binaryToHexaDecompression( pointer, 
                                              &dmaTxBuffer[localPingPong][0], 
                                              len - HEAD_LENGHT);
#else
        binToHexa(pointer,&dmaTxBuffer[localPingPong][0],len);
#endif
        dmaTxReady[localPingPong] = TRUE;
        if (dmaTx_flag == FALSE){
          dmaTx_flag = TRUE;
          DMA_UART_TX_Int_Handler();
        }
      }
      else{
        //dma_printf("\nmissing packet %d #",dmaTxPkt+1);        //message about missing packet
      }
      dmaTxPkt++;
    }
    else{                                                   //all data sended
      flush_flag=FALSE;
    }
  }
}
#endif

/** 
   @fn     uint8_t radioRecieve(void)
   @brief  function receive one packet with radio interface
   @pre    radioInit() must be called before this function is called.
   @code    
      radioInit();
      if (radioReceive())
        printf("\npacket was received and read from radio interface");
        printf(Buffer);//Buffer is global bufer for radio interface
      else
        printf("\npacket was not received correctly before timeout");
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

      RIE_Response = RadioHWreset();
    setBestFrequency();
  }

  if (RIE_Response == RIE_Success && RX_flag == TRUE){
    while (!RadioRxPacketAvailable()){
      timeout_timer++;
#if COMPRESSION && SLOW_FLUSH == 0
      YIELD;
#endif
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
          printf("\npacket was sucsesfully received with correct packet head#");
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
    dma_printf("\nslave id dismatch or not recognizet packet %d instead %d#",slv,slave_ID);
  
  if (actualPacket==0)    //if zero packet
    return 0;
  
  //if number of packet is in range
  if (actualPacket > 0 && actualPacket <= NUM_OF_PACKETS_IN_MEMORY)
    //if number of slave is in range
    if (slv <= NUMBER_OF_SLAVES && slv > 0)
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
          printf("\npacket was sucsesfully received with correct packet head#");
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
      //dma_printf("\nsaving actualPacket %d #",actualPacket);
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
  #if DEBUG_MESAGES || SIMULATE_RETX
    int8_t validReceivedPackets = 0;
  #endif
  
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
    radioSend(str, numOfReTxPackets+4);
    //rf_printf(str);

    #if DEBUG_MESAGES || SIMULATE_RETX
      dma_printf("\n  %s of %d #", str, pktMemory[actualRxBuffer].numOfPkt);
    #endif
    
//////////////receive missing packets//////////////
    for (i=0; i<numOfReTxPackets; i++){
      //if packet was received copy buffered packet
      if (radioRecieve()){
        if (validPacket()){
          copyBufferToMemory();
          #if DEBUG_MESAGES || SIMULATE_RETX
          validReceivedPackets ++;
          #endif
        }
      }
      else{     //send again request to get miss packets
        if (retransmision < RETRANSMISION -1){
          radioSend(str, numOfReTxPackets+4);
          //rf_printf(str);
          i=0;//-1
        }
        else
          break;
      }
    }
    #if DEBUG_MESAGES || SIMULATE_RETX
    if (numOfReTxPackets != validReceivedPackets){
      dma_printf("\nmissing %d packets after retransmission#", (numOfReTxPackets-validReceivedPackets));
    }
    #endif
  }
}

void Send(char* buff, int len){
  while(len--){
    putchar(*buff);
    buff++;
  }
}
void slowFlush(){
    uint8_t *pointer;
    uint16_t len;
//  if(flush_flag == TRUE && dmaTxReady[localPingPong] == FALSE){
    
    //packet iterate 0..as needed
    while(dmaTxPkt < dmaTxPktTotal){
      
      pointer = &pktMemory[actualTxBuffer].packet[dmaTxPkt][0];   //pointer at actuall packet
      
      //try if packet is received waiting flag 'w'
      if(pointer[1]!='w'){
        
        len = pktMemory[actualTxBuffer].lenghtOfPkt[dmaTxPkt];
#if COMPRESSION
        dmaTxLen = binaryToHexaDecompression( &pointer[HEAD_LENGHT], 
                                              &dmaTxBuffer[0][0], 
                                              len - HEAD_LENGHT);
        while(dmaTx_flag);    //wait for dma transfer done
        dmaTx_flag = TRUE;
        dmaSend((char *)dmaTxBuffer,dmaTxLen);
#else
        dmaTxLen = len;
        while(dmaTx_flag);    // wait for dma transfer done
        dmaTx_flag = TRUE;
        dmaSend((char *)&pointer[HEAD_LENGHT],dmaTxLen-HEAD_LENGHT);
#endif
        
//        dmaTx_flag = TRUE;
//        while(dmaTx_flag == TRUE);
      }
      else{
        dma_printf("\nmissing packet %d #",dmaTxPkt+1);        //message about missing packet
      }
      dmaTxPkt++;
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
#if SLOW_FLUSH == 0  
  //wait untill all packets are flushed
  while(flush_flag==TRUE){

#if COMPRESSION
    YIELD;
#else
    dmaTxTimeoutCounter++;
    // @bug for some unknown reason interrupt does not occur
    //probably sometime interrupt does not occur
    if (dmaTxTimeoutCounter >= DMA_TIMEOUT)
      DMA_UART_TX_Int_Handler();
#endif
  }
#endif
  dmaTxTimeoutCounter=0;
  //switch buffer 
  actualRxBuffer++;
  actualTxBuffer++;
  if(actualTxBuffer >= 2)
    actualTxBuffer=0;
  if(actualRxBuffer >= 2)
    actualRxBuffer=0;
  
  dmaTxPkt =0;
  dmaTxPktTotal = pktMemory[actualTxBuffer].numOfPkt;
#if SLOW_FLUSH
  slowFlush();
#else
  flush_flag = TRUE;
  
  #if COMPRESSION
    setTransfer();
  #else
  //call DMA_UART_TX_Int_Handler all managment is inside this function
  DMA_UART_TX_Int_Handler ();
  #endif
#endif
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
   @fn     void sendID(void)
   @brief  send slaveID packet for active slave
   @note   after 1000 time slots is again trying to connect new slaves
**/
void sendID(void){
  char i = NUMBER_OF_SLAVES;

  while (i--){ // iterate trought all slaves while find active one
    if (slave[slave_ID - 1].active == 1){
      break;
    }
    else{
      
      slave[slave_ID - 1].continualInactivity ++;
      if (slave[slave_ID - 1].continualInactivity > 1000){  //after long time try again activity of slave
        slave[slave_ID - 1].continualInactivity = 0;
        slave[slave_ID - 1].active = 1;
        setBestFrequency();
        break;
      }
      
      slave_ID++;     //increment slave number 
      if (slave_ID > NUMBER_OF_SLAVES)
        slave_ID = 1;
    }
  }
  //send slot identificator
  rf_printf("%dslot",slave_ID);   //start packet for new multiplex
//  IDmessage[0] = slave_ID +'0'; //for some unknown reason rf_printf is more reliable
//  radioSend(IDmessage,6);
}

/** 
   @fn     int8_t receivePackets(void)
   @brief  receive all packets of sended time slot;
   @pre    radioInit() must be called before this function is called.
   @code    
      radioInit();
      rf_printf("\n1slot");       //slot identificator
      if (receivePackets())
        flushPacktes();         //send all received packtes on UART
   @endcode
   @note   function si also retransmitin slot ID packtes if no response
**/
int8_t receivePackets(void){
  uint8_t retransmision=0;     //number of retransmision attempt
  uint8_t received = 0;        //number of received valid packets
  uint8_t unsuccessfulCounter = NUM_OF_PACKETS_IN_MEMORY;
  
  sendID();
  
  while (1){                //loop for receiving all expecting packets
    
    //if one packet received before timeout
    if (radioRecieve()){
      if (zeroPacket()){
        slave[slave_ID-1].active = 1;
        slave[slave_ID-1].continualInactivity = 0;
        return 0;
      }
      if (validPacket()){
        copyBufferToMemory();
        slave[slave_ID-1].active = 1;
        slave[slave_ID-1].continualInactivity = 0;
      }
      else
        return 0;           //if not recognizet packet
      received ++;
      
      if ((actualPacket >= pktMemory[actualRxBuffer].numOfPkt))
        return received;
    }
    else {                  //try retransmit again if no one packet received  
      if (firstRxPkt == FALSE){
        if (retransmision < RETRANSMISION ){
          //send slot identificator
          sendID();
          retransmision++;
        }
        else{                                //if nothing after RETRANSMISION times
          //if slave is not responding 50 times turn off transmission
          slave[slave_ID-1].continualInactivity ++;
          if (slave[slave_ID-1].continualInactivity > 10)
            slave[slave_ID-1].active = 0;
          return 0;
        }
      }else{
        //if no receiving more packets        
        if ((int16_t)(pktMemory[actualRxBuffer].numOfPkt - (received + unsuccessfulCounter)) <= 0 ){   
          return -unsuccessfulCounter;       //missing some packets
        }
        else
          unsuccessfulCounter++;
      }
    }
  }
}
/** 
   @fn     void initializeNewSlot(void)
   @brief  Initialize variables for new ID slot
**/
void initializeNewSlot(void){
  slave_ID++;     //increment slave number 
  if (slave_ID > NUMBER_OF_SLAVES)
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

#if CHECK_PRNG_LOCAL

/** 
   @fn     void initializeRandomCheck(void)
   @brief  initializing values for reference PRNG
**/
void initializeRandomCheck(void){
  char i;
  //initialize random packets
  for(i=0; i<NUMBER_OF_SLAVES;i++){
    PRNGinit(&slaves[i],i+1);
  }
}

/** 
   @fn     checkBufferedRandomPackets(void)
   @brief  comparing local and received PRNG packet 
   @note   defining this function set CHECK_PRNG_LOCAL macro to 1
   @see    srandc()
**/
void checkBufferedRandomPackets(void){
  uint8_t packet, word, * rnd_pkt_in_memory;
  char message[100];
  
  for(packet = 0; packet < pktMemory[actualRxBuffer].numOfPkt; packet++){
    
    //pointer in packet memory
    rnd_pkt_in_memory = &pktMemory[actualRxBuffer].packet[packet][HEAD_LENGHT]; 
    
    if (pktMemory[actualRxBuffer].packet[packet][1] == 'w')
      printf("\nmissing radio packet %d#",packet);
    else{
      #if COMPRESSION
      while (rnd_pkt_in_memory <= (pktMemory[actualRxBuffer].lenghtOfPkt[packet]/((sizeof(struct PRNGrandomPacket))+1)); word > 0 ; word --){
        #if ADAPTIVE_COMPRESSION
        while (*rnd_pkt_in_memory == 0){ // not compressed message from slave
          rnd_pkt_in_memory++;
          while (*rnd_pkt_in_memory != PACKET_TERMINATOR){
            putchar(*rnd_pkt_in_memory);
            rnd_pkt_in_memory++;
          }
          putchar(*rnd_pkt_in_memory);
          rnd_pkt_in_memory++;
        }
        #endif
        rnd_pkt_in_memory++;
        //check one random word
        if (PRNGcheck(slaves,(struct PRNGrandomPacket*)rnd_pkt_in_memory,(uint8_t*)message,NUMBER_OF_SLAVES))
            puts(message);
        else{
            //puts("packet is valid");
        }
        rnd_pkt_in_memory += (sizeof(struct PRNGrandomPacket)*2)+1;
      }
      #else
      for (word = (pktMemory[actualRxBuffer].lenghtOfPkt[packet]/((sizeof(struct PRNGrandomPacket)*2)+1)); word > 0 ; word --){
        hexaToBin((uint8_t*)rnd_pkt_in_memory,(uint8_t*)rnd_pkt_in_memory,sizeof(struct PRNGrandomPacket));
        //check one random word
        if (PRNGcheck(slaves,(struct PRNGrandomPacket*)rnd_pkt_in_memory,(uint8_t*)message,NUMBER_OF_SLAVES))
            puts(message);
        else{
            //puts("packet is valid");
        }
        rnd_pkt_in_memory += (sizeof(struct PRNGrandomPacket)*2)+1;
      }
      #endif
    }
  }
  actualRxBuffer++;
  actualTxBuffer++;
  if(actualTxBuffer >= 2)
    actualTxBuffer=0;
  if(actualRxBuffer >= 2)
    actualRxBuffer=0;
}
#endif

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
    printf("\nintegrity check ok#");
    LED_ON;
  }
  else{
    printf("\nproblem in integrity of firmware #");  
    LED_OFF;
    //while(1);
  }
}

void sendMessage(void){
  uint8_t locPingPong;
  locPingPong = rxPingPong;
  locPingPong++;
  if (locPingPong > 1)
    locPingPong=0;
  
  radioSend(rxBuffer[locPingPong],rxUARTcount[locPingPong]);
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
  #if CHECK_PRNG_LOCAL
  initializeRandomCheck();
  #endif

  while(1)
  {
    
    if (receivePackets()){            //if some data packets are received
      #if DEBUG_MESAGES
        dma_printf("\nredeived %d pkts #", pktMemory[actualRxBuffer].numOfPkt);
      #endif
      ifMissPktGet();                 //get back if losted some packets
      
      
      #if CHECK_PRNG_LOCAL
      checkBufferedRandomPackets();   //check received data localy
      #else
      flushBufferedPackets();         //send on UART received packets
      #endif
    }
    
    //if synchronize message received
    if (sync_flag == TRUE)
      synchronize();
    
    //check if some message to broadcast
    if (messageFlag == TRUE){
      sendMessage();
      messageFlag = FALSE;
    }
    
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
  uint8_t localPingPong;
  UrtDma(0,0);                       // prevents further UART DMA requests
  DmaChanSetup ( UARTTX_C , DISABLE , DISABLE );    // Disable DMA channel
#if SLOW_FLUSH
  dmaTx_flag = FALSE;
#else
  #if COMPRESSION  
  dmaTxReady[dmaTxPingPong] = FALSE;//set flag about transfer
  
  if (dmaTxPingPong > 0)//change ping pong buffer
    localPingPong = 0;
  else
    localPingPong = 1;
    
  if (dmaTxReady[localPingPong] == TRUE){
    
    dmaTxPingPong = localPingPong;
    
    dmaSend(&dmaTxBuffer[dmaTxPingPong][0], dmaTxLen);   //send only data without head
        
    //dmaTxReady[dmaTxPingPong] = FALSE;//set flag about transfer
  }
  else{
    dmaTx_flag = FALSE;
  }
  #else    
        
  if(flush_flag == TRUE){
    if(dmaTxPkt < dmaTxPktTotal){      //packet iterate 0..as needed
            
      dmaTxPtr = &pktMemory[actualTxBuffer].packet[dmaTxPkt][0];   //pointer at actuall packet
      
      if(dmaTxPtr[1]!='w'){                         //try if packet is received waiting flag 'w'
        
        dmaTxLen = pktMemory[actualTxBuffer].lenghtOfPkt[dmaTxPkt];
    #if SEND_HEAD
        dmaSend(dmaTxPtr,dmaTxLen);          //send data with head
    #else
        dmaSend(dmaTxPtr + HEAD_LENGHT, dmaTxLen - HEAD_LENGHT);   //send only data without head
    #endif
      }
      else{
        dma_printf("\nmissing packet %d#",dmaTxPkt+1);        //message about missing packet
      }
      dmaTxPkt++;
      dmaTxTimeoutCounter = 0;
    }
    else{                                                   //all data sended
      flush_flag = FALSE;
    }
  }
  #endif
#endif
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
// Hard Fault Interrupt handler 
// if pointer going out of array
///////////////////////////////////////////////////////////////////////////
static volatile unsigned int _Continue;
void HardFault_Handler(void) {
  _Continue = 0u;

  while (_Continue == 0u);   
  NVIC_SystemReset();
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
    ch  = UrtRx(0);           //call UrtRd() clears COMIIR_STA_RXBUFFULL
  
    // check if framing error == uart is correctly synchronized
    if ((UrtLinSta(0) & COMLSR_FE) == COMLSR_FE){// Framing Error
      DioCfgPin(pADI_GP1,PIN0,0); // turn off RxD function at pin for a moment
      while(ch--);                //random waiting time
      DioCfgPin(pADI_GP1,PIN0,1); // turn on RxD function at pin
      return;
    }
  
    *rxPtr = ch;
    rxPtr++;
    rxUARTcount[rxPingPong]++;
    
    if (ch == '$'){
      *rxPtr = '\0';
      if (memcmp((char*)(rxPtr - 5),"SYNC$",5) == 0){   //end of packet pointer
        sync_flag = TRUE;
        rxUARTcount[rxPingPong] = 0;
      }else{
        messageFlag = TRUE;
      }
      // switch ping pong register
      rxPingPong++;
      if (rxPingPong > 1)
        rxPingPong = 0;
      rxPtr = rxBuffer[rxPingPong];
    }
    if (rxUARTcount[rxPingPong] >= UART_BUFFER_DEEP)                      //check not overflow buffer
      rxUARTcount[rxPingPong] = 0;
  }
} 
