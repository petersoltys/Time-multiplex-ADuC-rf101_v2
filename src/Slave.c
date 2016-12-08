/**
 *****************************************************************************
   @file     Slave.c
   @brief    prigram used for transmiting data in time multiplex
             data are received via UART and sended trouhgt radio interface
             during associated time slot 
             working with Master.c
             tested on ADucRF101MKxZ

             

   @author      Bc. Peter Soltys
   @supervisor  doc. Ing. Milos Drutarovsky Phd.
   @version     
   @date        08.12.2016(DD.MM.YYYY) 

   @par Revision History:
   - V1.0, July 2015  : initial version. 
   - V1.1, august 2015  : fully  functional.  (only shyntetic data transmitted)
   - V1.2, august 2015  : fully  functional.  (transmiting received data via UART (only fixed lenght packet))
   - V1.3, september 2015  : faster version with higher throughput and transmiting variable lenght packets 
                             working with "UWB - Coordinate Reader Deployment" from Peter Mikula
   - V1.4, january 2015  : added synchronization
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

//global variables 
RIE_Responses RIE_Response = RIE_Success;
uint8_t        Buffer[255];
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
   @note  UART nust be much faster(tested on 115200 baud rate) than RF-link (to release memory)
   @see   actualRxBuffer 
   @see   actualTxBuffer
**/
#pragma pack(1)
struct pkt_memory {
  uint8_t packet[NUM_OF_PACKETS_IN_MEMORY][PACKET_MEMORY_DEPTH];
  uint8_t lenghtOfPkt[NUM_OF_PACKETS_IN_MEMORY];
  uint8_t numOfPkt;
}pktMemory[2];

uint8_t PRNG_data = FALSE;   //flag to set random data generation
struct PRNGslave slavePRNG;

char actualRxBuffer=0;
char actualTxBuffer=1;

uint8_t rxPingPong = 0;                            //ping pong pointer in rxUARTbuffer
uint8_t rxUARTbuffer[2][PACKET_MEMORY_DEPTH];    //buffer for TX UART channel
uint16_t rxUARTbufferCount[2] = {0,0};
uint16_t rxUARTcount = 0;


uint8_t* rxPktPtr ;
/////////flags/////////////////////
uint8_t TX_flag = FALSE, RX_flag=FALSE, terminate_flag=FALSE, buffer_change_flag=0, memory_full_flag = 0 ;
uint8_t my_slot = FALSE, pkt_received_flag = FALSE, close_packet_flag = FALSE;

void UART_Int_Handler (void);
uint16_t dma_printf(const char * format /*format*/, ...);

int i=0,j=0;
int debugTimer=0;

/** 
   @fn      void storePkt(void)
   @brief   store buffered packet via UART to packetMemory
   @note    place this code where is nothing else to do.
   @code    
      //place this code at place where is MCU waiting for something
      if (pkt_received_flag == TRUE)
        storePkt();
   @endcode

**/
void storePkt(void){
  uint8_t pktNum, *destPtr, *sourcePtr, pingPong;
  uint16_t packetLen;
  
  pingPong = rxPingPong;
  pingPong++;
  if(pingPong >= 2)
    pingPong = 0;
  
  packetLen = rxUARTbufferCount[pingPong];
  if (packetLen > 240)//if packet is longer as supported drop packet
    return;
  sourcePtr = &rxUARTbuffer[pingPong][0];
  
  pktNum = pktMemory[actualRxBuffer].numOfPkt;
  
  if (pktNum < NUM_OF_PACKETS_IN_MEMORY){     //copy to memory
    destPtr = &pktMemory[actualRxBuffer].packet[pktNum][HEAD_LENGHT];   //pinting beyound packet head
    
    memcpy(destPtr, sourcePtr, packetLen); //copy packet to memory
   
    pktMemory[actualRxBuffer].lenghtOfPkt[pktNum] = packetLen;
    pktMemory[actualRxBuffer].numOfPkt++;
  }
  else{
    dma_printf("\npacket memory is full #");
  }
  pkt_received_flag = FALSE;
}


/** 
   @fn      void uartInit(void)
   @brief   initialize uart port
   @code    
      uartInit();
   @endcode
   @note    speed UART_BAUD_RATE_MASTER baud
            8 bits
            one stop bit
            output port P1.0/P1.1
**/
void uart_init(void){
  rxPingPong++;   // ==1
  rxPktPtr = &rxUARTbuffer[rxPingPong][0];
  
  UrtLinCfg(0,UART_BAUD_RATE_SLAVE,COMLCR_WLS_8BITS,COMLCR_STOP_DIS);//configure uart
  DioCfg(pADI_GP1,0x9);         // UART functionality on P1.0/P1.1
  
  UrtIntCfg(0,COMIEN_ERBFI);    // enable Rx interrupts at any char
  NVIC_EnableIRQ(UART_IRQn);    // setup to receive data using interrupts
  
  DmaInit();                    // initialize dma channel

}

/**
* @brief initialize port with led diode
*/
void led_init(void){
    // P4.2  as output
  DioCfg(pADI_GP4,0x10);
  DioOen(pADI_GP4, BIT2); 
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
     RIE_Response = RadioInit(RADIO_CFG);  
  // Set the Frequency to operate at 433 MHz
  if (RIE_Response == RIE_Success)
     RIE_Response = RadioSetFrequency(BASE_RADIO_FREQUENCY);
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

void changeRadioConf(void){
  char *ptr;
  long int freq = strtol((char*)&Buffer[4],&ptr,10);
  if (RIE_Response == RIE_Success)
    RIE_Response = RadioSetFrequency(freq);
}

/** 
   @fn     void  dmaSend(void* buff, int len)
   @brief  send one packet on UART with DMA controller
   @param  buff :{} pointer to data to send
   @param  len :{int range} number of bytes to send
   @pre    uartInit() must be called before this can be called.
   @code  
        uartInit();
        len =vsprintf(dmaTxBuffer, format,args);    //vlozenie formatovaneho retazca do buff
        dmaSend(dmaTxBuffer,len);
   @endcode
   @note    after end of transmision is called DMA_UART_TX_Int_Handler (void)
   @see DMA_UART_TX_Int_Handler
**/
void  dmaSend(void* buff, int len){
    //DMA UART stream
    DmaChanSetup(UARTTX_C,ENABLE,ENABLE);   // Enable DMA channel  
    DmaTransferSetup(UARTTX_C,len,buff);
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
   @see     DMA_UART_TX_Int_Handler  
   @return  uint16_t number of sending chars (if == 0 error)
**/
uint16_t dma_printf(const char * format /*format*/, ...)
{
  char buff[256];
  uint16_t len;
  va_list args;
  va_start( args, format );
   
  len =vsprintf(buff, format,args);   //vlozenie formatovaneho retazca do buff
  dmaSend(buff,len);

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
      if(len<PACKETRAM_LEN)    //check max lenght 
        radioSend(buff,len+1);    //send formated packet
   @endcode
   @note    output stream is trought radio interface
            function is waiting until whole packet is trnsmited
   @return -1 if packet is longer than 240 bytes
            0 if everithing done OK
**/
int8_t radioSend(void* buff, uint8_t len){
  #if T_PROCESSING
  uint16_t safe_timer=0;
  #endif
  if (RIE_Response == RIE_Success){   //send packet
    if (len > 240)
      return -1; //if packet is longer than 240 bytes
    RIE_Response = RadioTxPacketVariableLen(len, (uint8_t*)buff); 
    RX_flag = FALSE;
  }
  
  if (RIE_Response == RIE_Success){   //wait untill packet sended
    while(!RadioTxPacketComplete()){
      //here is free time to store packet to memory
      if (pkt_received_flag == TRUE)
        storePkt();
    }
  }
  #if T_PROCESSING
  while (safe_timer < T_PROCESSING)
    safe_timer++;
  #endif
  if (RIE_Response == RIE_Success){   //start again receiving mod
    RIE_Response = RadioRxPacketVariableLen(); 
    RX_flag = TRUE;
  }
  
    //DMA UART stream
#if TX_STREAM
  dmaSend(buff,len-1);
#endif
  if (RIE_Response == RIE_Success)
    return 0;
  else
    return 1;
}

/** 
   @fn     uint8_t rf_printf(const char * format , ...)
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
   @return  uint8_t - number of sending chars (if == 0 error)
**/
uint8_t rf_printf(const char * format /*format*/, ...){
  char buff[256];
  uint8_t len;
  va_list args;
  va_start( args, format );

  len=vsprintf(buff, format,args);    //vlozenie formatovaneho retazca do buff
  if(len>PACKETRAM_LEN){              //kontrola maximalnej dlzky retazca
    va_end( args );
    return 0;
  }
  
  radioSend(buff,len+1);              //send formated packet

  va_end( args );
  return len;
}

/** 
   @fn     uint8_t radioRecieve(void)
   @brief  function receive one packet with radio interface
   @pre    radioInit() must be called before this function is called.
   @code    
      radioInit();
      if (radioReceive())
        printf("\npacket was received and read from radio interface");
        printf(Buffer);   //Buffer is global bufer for radio interface
      else
        printf("\npacket was not received correctly before timeout");
   @endcode
   @note   function is also reding packet form radio interface to uint8_t Buffer[PACKETRAM_LEN]
   @return uint8_t - 1 == packet received, 0 == packet was not received correctly before timout
**/
uint8_t radioRecieve(void){    //pocka na prijatie jedneho paketu
  uint16_t timeout_timer = 0;
  //if in radiocontrolller ocuured some problem
  if (RIE_Response != RIE_Success)
    radioInit();
  
  if (RIE_Response == RIE_Success && RX_flag == FALSE){
    RIE_Response = RadioRxPacketVariableLen();
    RX_flag = TRUE;
  }

  if (RIE_Response == RIE_Success && RX_flag == TRUE){
    while (!RadioRxPacketAvailable()){
      timeout_timer++;
      //turn on led if nothing is received after timeout
      if (timeout_timer > T_TIMEOUT){
        //LED_ON;
        return 0;
      }

      //here is free time to store packet to memory
      if (pkt_received_flag == TRUE)
        storePkt();
    }
    RX_flag = FALSE;
  }
  
  //LED_ON;
  //citanie paketu z rf kontrolera
  if (RIE_Response == RIE_Success)
    RIE_Response = RadioRxPacketRead(sizeof(Buffer),&PktLen,Buffer,&RSSI);
  //LED_OFF;
  
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
   @fn     void setTimeToSync(uint16_t time)
   @brief  set general purpose timer0 for synchronization timeout
   @param  time {0- uint range} should be (N * SYNC_INTERVAL) where N=1-3
   @see    void GP_Tmr0_Int_Handler ()
   @see    SYNC_INTERVAL
   @note   timer predivider factor = 256, processor clock, periodic mode
**/
void setTimeToSync(uint16_t time){
  GptLd (pADI_TM0, time);
  GptCfg(pADI_TM0, TCON_CLK_UCLK, TCON_PRE_DIV256, TCON_ENABLE_EN|TCON_RLD_EN|TCON_MOD_PERIODIC);
  while (GptSta(pADI_TM0)& TSTA_CON);   // wait for sync of TCON write. required because of use of asynchronous clock
  GptClrInt(pADI_TM0,TCLRI_TMOUT);
  while (GptSta(pADI_TM0)& TSTA_CLRI);  // wait for sync of TCLRI write. required because of use of asynchronous clock
  NVIC_EnableIRQ(TIMER0_IRQn);
}


/** 
   @fn     uint8_t transmit(void)
   @brief  function is transmiting all prepared packets throught radio link
   @note   rotate packet memory
           all data packets are received via UART
   @see    pktMemory
   @return uint8_t - number of transmitted packets
**/
uint8_t transmit(void){
  uint8_t* pktMemoryPtr;
  uint8_t txPkt = 0;

  my_slot=TRUE;

  NVIC_DisableIRQ(UART_IRQn);
    //change buffer pointers
    actualRxBuffer++;
    actualTxBuffer++;
    if(actualRxBuffer >= 2)
      actualRxBuffer = 0;
    else if(actualTxBuffer >= 2)
      actualTxBuffer = 0;
    
    pktMemory[actualRxBuffer].numOfPkt = 0;
  NVIC_EnableIRQ(UART_IRQn);  
  
  while (my_slot == TRUE && (txPkt < pktMemory[actualTxBuffer].numOfPkt) ){     //while interupt ocurs send avaliable packets
    
    pktMemoryPtr = &pktMemory[actualTxBuffer].packet[txPkt][0];
    
    //build head of packet
    pktMemoryPtr[2] = pktMemory[actualTxBuffer].numOfPkt + CHAR_OFFSET;
    pktMemoryPtr[1] = txPkt + CHAR_OFFSET + 1;
    pktMemoryPtr[0] = SLAVE_ID + CHAR_OFFSET; 

    //dma_printf(pktMemoryPtr);
    if (radioSend(pktMemoryPtr,(pktMemory[actualTxBuffer].lenghtOfPkt[txPkt]+HEAD_LENGHT)))//send packet
      dma_printf("\n too long packet %d#",(pktMemory[actualTxBuffer].lenghtOfPkt[txPkt]+HEAD_LENGHT));
    
    txPkt++;
  }
  memory_full_flag = FALSE;
  return txPkt;   //return number of transmited packets
}

/** 
   @fn     uint8_t retransmit(void)
   @brief  function retransmit missing packets if requested
   @return uint8_t - number of retransmited packets
**/
uint8_t retransmit(void){
  uint8_t pkt = 3;
  char reTxPkt[NUM_OF_PACKETS_IN_MEMORY + 5];

  my_slot = TRUE;
  strcpy(reTxPkt,(char*)Buffer);
  //retransmit only until interupt occur
  while ((reTxPkt[pkt]!='\0') && my_slot == TRUE && terminate_flag == FALSE)
  {
    radioSend(&pktMemory[actualTxBuffer].packet[pkt][0],(pktMemory[actualTxBuffer].lenghtOfPkt[pkt]+3));
    pkt++;
  }
  return (pkt-3);
}
/** 
   @fn     void SetInterruptPriority (void)
   @brief  Initialize interrupt priority
**/
void SetInterruptPriority (void){
  NVIC_SetPriority(UART_IRQn,5);          //receiving of packets if not constant lenght of packet
  NVIC_SetPriority(DMA_UART_RX_IRQn,6);   //terminate DMA RX priority
  NVIC_SetPriority(DMA_UART_TX_IRQn,7);   //terminate DMA TX priority
  NVIC_SetPriority(TIMER1_IRQn,9);        //blinking low priority
  NVIC_SetPriority(TIMER0_IRQn,8);        //terminate transmiting higher prioritz
  NVIC_SetPriority(EINT8_IRQn,4);         //highest priority for radio interupt
}
/** 
   @fn     uint8_t button_pushed(void)
   @brief  checking if boot button is pushed
   @note   IO port must be initialized
   @return uint8_t - 1 = button pushed, 0 = button is not pushed
**/
uint8_t button_pushed(void){
  if(DioRd(pADI_GP0)&0x40)    //boot button P0.6 
    return 0;
  else
    return 1;
}
/** 
   @fn     void fill_memory(void)
   @brief  function is filling memory with PRNG data to verify stability of design
   @note   function is turn off DMA interrupt of UART
**/
void fill_memory(void){
  uint8_t* pktptr;
  int len = 0,i = 1;
  NVIC_DisableIRQ ( DMA_UART_RX_IRQn );   // Disable DMA UART RX interrupt
  
  for (i = 0; i < NUM_OF_PACKETS_IN_MEMORY; i++){
    pktptr = &pktMemory[actualRxBuffer].packet[i][0];
    len = HEAD_LENGHT;
    while(len < (PACKET_MEMORY_DEPTH-HEAD_LENGHT)-(sizeof(struct PRNGslave)*2) ){
      PRNGnew(&slavePRNG);
      binToHexa((uint8_t*)&slavePRNG.packet, &pktptr[len], sizeof(struct PRNGrandomPacket));
      pktptr[len + (sizeof(struct PRNGrandomPacket)*2)] = '$';
      //memcpy(&pktptr[len+HEAD_LENGHT],&random_packet,PRNG_PKT_LEN);
      len += (sizeof(struct PRNGrandomPacket)*2)+1;
    }
    pktMemory[actualRxBuffer].lenghtOfPkt[pktMemory[actualRxBuffer].numOfPkt] = len-HEAD_LENGHT;
    pktMemory[actualRxBuffer].numOfPkt++;
  }  
  memory_full_flag = TRUE;
  NVIC_EnableIRQ ( DMA_UART_RX_IRQn );    // Enable DMA UART RX interrupt
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
    dma_printf("\nintegrity check ok#");
    LED_ON;
  }
  else{
    dma_printf("\nproblem in integrity of firmware #");  
    LED_OFF;
    //while(1);
  }
}

/** 
   @fn     int main(void)
   @brief  main function of slave program
   @note   Program is using time multiplex to send data to master device
   @return int 
**/
int main(void)
{
  uint16_t frequency_timout = 0;
  memset(pktMemory, 0, sizeof(pktMemory));
  WdtGo(T3CON_ENABLE_DIS);    //stop watch-dog
  
  //initialize all interfaces
  SetInterruptPriority();
  uart_init();
  checkIntegrityOfFirmware();
  led_init();
  
  radioInit();    //inicialize radio conection
  
  while (1){
    if (radioRecieve()){
      LED_ON;
      frequency_timout = 0;
      //if this slot identifier belongs to this slave
      if ( 0 == strcmp((char*)Buffer,TIME_SLOT_ID_SLAVE)){
        close_packet_flag = TRUE;
        if(pktMemory[actualRxBuffer].numOfPkt)    //if is something to send
          transmit();
        else
          radioSend(ZERO_PACKET, 4);    //send zero packet meanin nothing to send
      }
      
      //check if retransmit request
      if (0 == memcmp(Buffer,RETRANSMISION_ID,3/*chars to compare*/))   //check if re-tx slot
        retransmit();
      
      //check if sync packet
      if (0 == memcmp(Buffer,"SYNC",4/*chars to compare*/))
        setTimeToSync((Buffer[4]-'0') * SYNC_INTERVAL);
      
      //check if frequency configuration change
      if (0 == memcmp(Buffer,"FREQ",4))
        changeRadioConf();
      
      //check if message
      if (Buffer[0] == '#')
        dma_printf((char *)Buffer);
    }
    else {
      LED_OFF;
      
      //  check if is something still comunicating at actual used frequency
      // and reset radio modul
      frequency_timout++;
      if (frequency_timout > FREQ_TIMEOUT){
        RIE_Response = RadioHWreset();
        radioInit();
        RadioSetFrequency(BASE_RADIO_FREQUENCY);
        if (RIE_Response == RIE_Success){   //start again receiving mod
          RIE_Response = RadioRxPacketVariableLen(); 
          RX_flag = TRUE;
        }
        frequency_timout = 0;
      }
    }
    /*PRNG settings*/
    if (button_pushed()){   //(re)initialize PRNG
      if ((PRNG_data == TRUE) && memory_full_flag == FALSE ){
        fill_memory();
      }
      else if (PRNG_data == FALSE){
        PRNG_data = TRUE;
        PRNGinit(&slavePRNG,SLAVE_ID);
      }
    }
    #if PRNG_CONTINUAL_GENERATING 
    //fill up the memory
    if ((PRNG_data == TRUE) && memory_full_flag == FALSE )
      fill_memory();
    #endif

  }
  
}
///////////////////////////////////////////////////////////////////////////
// GP Timer0 Interrupt handler 
// used to set synchronization pin
///////////////////////////////////////////////////////////////////////////
/** 
    @fn      void GP_Tmr0_Int_Handler(void)
    @brief   Interrupt handler for synchronization timing
    @note    synchronization at falling edge 
             handler manage also rising edge (hughe jitter)
    @see     setTimeToSync()
**/
void GP_Tmr0_Int_Handler(void){
  if (GptSta(pADI_TM0)== TSTA_TMOUT) // if timout interrupt
  {
    if (SYNC_PIN_READ)
      SYNC_PIN_LOW;   //negativ synchronization
    else
    {                 //if low then set high and stop timer
      SYNC_PIN_HIGH;//reset synchronization
      GptCfg(pADI_TM0, TCON_CLK_UCLK, TCON_PRE_DIV256, TCON_ENABLE_DIS);  //stop timer
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
/** 
    @fn      void GP_Tmr1_Int_Handler (void)
    @brief   Interrupt handler for timer1 already unused
**/
void GP_Tmr1_Int_Handler(void){
  if (GptSta(pADI_TM1)== TSTA_TMOUT) // if timout interrupt
  { 
  }
}

///////////////////////////////////////////////////////////////////////////
// UART DMA Interrupt handler for transmit
// used for ending of DMA operation of dma_send()
///////////////////////////////////////////////////////////////////////////
/** 
    @fn      void DMA_UART_TX_Int_Handler (void)
    @brief   terminating DMA transaction of function dma_send()
    @see     dma_send()
**/
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
/*
void hexaToBin(uint8_t* from, uint8_t to, len){

}*/
/** 
    @fn      void DMA_UART_RX_Int_Handler()
    @brief   Interrupt handler terminating DMA receiving transaction if constant
             lenght of packet is set
    @see     MAX_LEN_OF_RX_PKT
**/
void DMA_UART_RX_Int_Handler   ()
{
  UrtDma(0,0); // turn off pending dma transfer
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
// used for spliting data to packets
// function is taken from example UARTLoopback.c and modified
///////////////////////////////////////////////////////////////////////////
/** 
    @fn      void UART_Int_Handler ()
    @brief   Interrupt handler managing store received data trought UART
              - string mode = appedning chars untill STRING_TERMINATOR is received 
              - binary mode = appending untill buffer is full == 240 == PACKETRAM_LEN chars
    @see     STRING_TERMINATOR
**/
void UART_Int_Handler (void)
{   
  uint8_t ch, ucCOMIID0; 

  ch  = UrtRx(0);           //call UrtRd() clears COMIIR_STA_RXBUFFULL
  
  // check if framing error == uart is correctly synchronized
  if ((UrtLinSta(0) & COMLSR_FE) == COMLSR_FE){// Framing Error
    DioCfgPin(pADI_GP1,PIN0,0); // turn off RxD function at pin for a moment
    while(ch--);                //random waiting time
    DioCfgPin(pADI_GP1,PIN0,1); // turn on RxD function at pin
    return;
  } 
  
  *rxPktPtr = ch;
  rxUARTcount++;
  if(rxUARTcount < PACKET_MEMORY_DEPTH)
    rxPktPtr++;
  
  if ( ch == '$' ){
    // check if is in buffer enought place to store new word to packet
    if ((rxUARTcount >= (PACKET_MEMORY_DEPTH - (HEAD_LENGHT + MAX_LEN_OF_RX_PKT))) || close_packet_flag == TRUE){
      if (rxUARTcount < PACKET_MEMORY_DEPTH){  //if packet is too long drop last word
        rxUARTbufferCount[rxPingPong] = rxUARTcount;
      }
      
      rxPingPong++;
      if (rxPingPong == 2)
        rxPingPong=0;
      //set pointer to next receiving
      rxPktPtr = rxUARTbuffer[rxPingPong];    //buffer for TX UART channel
      
      rxUARTcount = 0;
      close_packet_flag = FALSE;
      pkt_received_flag = TRUE;
    }
    rxUARTbufferCount[rxPingPong] = rxUARTcount;
  }else{
    if (rxUARTcount >= PACKET_MEMORY_DEPTH){
      rxPktPtr = rxUARTbuffer[rxPingPong];
      rxUARTcount = 0;
    }
  }
} 


