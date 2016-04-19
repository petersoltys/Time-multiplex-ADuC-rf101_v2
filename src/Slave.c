/**
 *****************************************************************************
   @file     Slave.c
   @brief    prigram used for transmiting data in time multiplex
             data are received via UART and sended trouhgt radio interface
             during associated time slot 
             working with Master.c
             tested on ADucRF101MKxZ

             

   @version     'V2.2'-0-gd953f50
   @supervisor  doc. Ing. Milos Drutarovsky Phd.
   @author      Bc. Peter Soltys
   @date        19.04.2016(DD.MM.YYYY) 

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
uint8_t  Buffer[255];
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
// [level][num of packet][packet data]
uint8_t pktMemory[2][NUM_OF_PACKETS_IN_MEMORY][PACKET_MEMORY_DEPTH];
/*
  pktMemory is 2 levels deep 
  puspose is changing in circle 
  0 actual receiving buffer
  1 actual sending buffer
  for pointing are used flags : actualRxBuffer, actualTxBuffer,
*/
          // [level]
char numOfPkt[2] = {0,0};   //similar like pktMemory
                      // [level][lenght]
uint8_t lenghtOfPkt[2][NUM_OF_PACKETS_IN_MEMORY];   //similar like pktMemory

char PRNG_data = FALSE;   //flag to set random data generation

//to ensure arrangement variables in byte by byte grid structure (without "bubles")
//is used directive #pragma pack(1)
#pragma pack(1)
struct rand_pkt {
  char  Slave;
  char  slave_id;
  unsigned long  randomPktNum;
  #if WEEAK_RANDOM_FUNCTION == 1
  static unsigned long next;    //variable for PRNG
  #else
  long next;                    //variable for PRNG
  #endif
  int   rnadom;
} random_packet;  


char actualRxBuffer=0;
char actualTxBuffer=1;

uint8_t dmaRxBuffer[PACKET_MEMORY_DEPTH*2];    //buffer for DMA TX UART channel
uint8_t rxUARTbuffer[255];
uint8_t* rxPktPtr ;
int rxUARTcount = 0;
/////////flags/////////////////////
char TX_flag=FALSE, RX_flag=FALSE, terminate_flag=FALSE, buffer_change_flag=0, memory_full_flag = 0 ;
char my_slot = FALSE;

void UART_Int_Handler (void);

int i=0,j=0;
int debugTimer=0;

#if THROUGHPUT_MEASURE
int txThroughput;
int rxThroughput;
#endif

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
  
  UrtLinCfg(0,UART_BAUD_RATE_SLAVE,COMLCR_WLS_8BITS,COMLCR_STOP_DIS);//configure uart
  DioCfg(pADI_GP1,0x9);         // UART functionality on P1.0/P1.1
  
#if LEN_OF_RX_PKT == 0
  UrtIntCfg(0,COMIEN_ERBFI);    // enable Rx interrupts
  NVIC_EnableIRQ(UART_IRQn);    // setup to receive data using interrupts
#endif
  
  DmaInit();// Create DMA descriptor
 // Set transfer parameters
 //                 transfer  target
 //                 channel   count pointer
  DmaTransferSetup(UARTTX_C,  20,   Buffer);
  DmaTransferSetup(UARTRX_C,  20,   rxPktPtr);
  NVIC_EnableIRQ ( DMA_UART_TX_IRQn );    // Enable DMA UART TX interrupt
  NVIC_EnableIRQ ( DMA_UART_RX_IRQn );    // Enable DMA UART RX interrupt
  
#if LEN_OF_RX_PKT
//enable RX DMA chanel for receiving packets forom UART
  DmaChanSetup(UARTRX_C,ENABLE,ENABLE);   // Enable DMA channel  
  DmaTransferSetup(UARTRX_C,LEN_OF_RX_PKT,(uint8_t*)rxUARTbuffer);
  UrtDma(0,COMIEN_EDMAR);                 //start rx 
#endif
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
**/
void radioSend(void* buff, uint8_t len){
  #if T_PROCESSING
  uint16_t safe_timer=0;
  #endif
  if (RIE_Response == RIE_Success){   //send packet
    RIE_Response = RadioTxPacketVariableLen(len, (uint8_t*)buff); 
    RX_flag = FALSE;
  }
  
  if (RIE_Response == RIE_Success){   //wait untill packet sended
    while(!RadioTxPacketComplete());
  }
  #if T_PROCESSING
  while (safe_timer < T_PROCESSING)
    safe_timer++;
  #endif
  if (RIE_Response == RIE_Success){   //start again receiving mod
    RIE_Response = RadioRxPacketVariableLen(); 
    RX_flag = TRUE;
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
        printf("packet was received and read from radio interface");
        printf(Buffer);   //Buffer is global bufer for radio interface
      else
        printf("packet was not received correctly before timeout");
   @endcode
   @note   function is also reding packet form radio interface to uint8_t Buffer[PACKETRAM_LEN]
   @return uint8_t - 1 == packet received, 0 == packet was not received correctly before timout
**/
uint8_t radioRecieve(void){    //pocka na prijatie jedneho paketu
  uint16_t timeout_timer = 0;
  
  if (RIE_Response == RIE_Success && RX_flag == FALSE){
    RIE_Response = RadioRxPacketVariableLen();
    RX_flag = TRUE;
  }

  if (RIE_Response == RIE_Success && RX_flag == TRUE){
    while (!RadioRxPacketAvailable()){
      timeout_timer++;
      //turn on led if nothing is received after timeout
      if (timeout_timer > T_TIMEOUT){
        LED_ON;
        return 0;
      }
    }
    RX_flag = FALSE;
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

#if THROUGHPUT_MEASURE
/** 
   @fn     void troughputMeasureInit(void)
   @brief  set general purpose timer0 to 1s interval to measure troughput
   @see    void GP_Tmr1_Int_Handler ()
   @note   timer predivider factor = 32768, low freq. oscillator clock, periodic mode
**/
void troughputMeasureInit(void){
  GptLd (pADI_TM1, 0x0);  // Interval of 1s
  GptCfg(pADI_TM1, TCON_CLK_LFOSC, TCON_PRE_DIV32768, TCON_ENABLE|TCON_MOD_PERIODIC);
  while (GptSta(pADI_TM1)& TSTA_CON);   // wait for sync of TCON write. required because of use of asynchronous clock
  GptClrInt(pADI_TM1,TCLRI_TMOUT);
  while (GptSta(pADI_TM1)& TSTA_CLRI);  // wait for sync of TCLRI write. required because of use of asynchronous clock
  NVIC_EnableIRQ(TIMER1_IRQn);
}
#endif

#if THROUGHPUT_MEASURE == 2
/** 
   @fn     uint8_t transmit(void)
   @brief  send synthetic data to master just to measure troughput
   @return uint8_t - number of transmitted packets
**/
uint8_t transmit(void){
  my_slot=TRUE;
  uint8_t txPkt=0;
  
  while(my_slot==TRUE && txPkt < 10){
    //all times like packet 2
    rf_printf("%c02 my shyntetic data ... number of tx data %d Bytes",SLAVE_ID,txThroughput);
    txPkt++;
  }
  return txPkt;
}
#else

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
    if(actualRxBuffer>=2)
      actualRxBuffer=0;
    else if(actualTxBuffer>=2)
      actualTxBuffer=0;
    
    numOfPkt[actualRxBuffer]=0;
  NVIC_EnableIRQ(UART_IRQn);  
  
  while (my_slot == TRUE && (txPkt < numOfPkt[actualTxBuffer]) ){     //while interupt ocurs send avaliable packets
    
    pktMemoryPtr = &pktMemory[actualTxBuffer][txPkt][0];
    
    //build head of packet
    pktMemoryPtr[2] = numOfPkt[actualTxBuffer] + CHAR_OFFSET;
    pktMemoryPtr[1] = txPkt + CHAR_OFFSET + 1;
    pktMemoryPtr[0] = SLAVE_ID + CHAR_OFFSET; 

    //dma_printf(pktMemoryPtr);
    #if BINARY_MODE
    radioSend(pktMemoryPtr,(lenghtOfPkt[actualTxBuffer][txPkt]+HEAD_LENGHT));//send packet
    #else
    rf_printf(pktMemoryPtr);    //send packet
    #endif
    
    txPkt++;
  }
  memory_full_flag = FALSE;
  return txPkt;   //return number of transmited packets
}
#endif

/** 
   @fn     uint8_t retransmit(void)
   @brief  function retransmit missing packets if requested
   @return uint8_t - number of retransmited packets
**/
uint8_t retransmit(void){
  uint8_t pkt = 3;
  char reTxPkt[NUM_OF_PACKETS_IN_MEMORY + 5];

  my_slot=TRUE;
  strcpy(reTxPkt,(char*)Buffer);
  //retransmit only until interupt occur
  while ((reTxPkt[pkt]!='\0') && my_slot==TRUE && terminate_flag == FALSE)
  {
    #if BINARY_MODE
    radioSend(&pktMemory[actualTxBuffer][pkt][0],(lenghtOfPkt[actualTxBuffer][pkt]+3));
    #else 
    rf_printf(&pktMemory[actualTxBuffer][(reTxPkt[pkt]-CHAR_OFFSET)][0]);
    #endif
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
   @fn     void srandc(unsigned int seed)
   @brief  function is setting initial value of PRNG
   @see    randc(void)
**/
void srandc(unsigned int seed)
{
  #if WEEAK_RANDOM_FUNCTION == 1
  random_packet.next = seed;
  #else
  random_packet.next = (long)seed;
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
int randc(void) // RAND_MAX assumed to be 32767
{
  #if WEEAK_RANDOM_FUNCTION == 1
  //official ANSI implementation
    random_packet.next = random_packet.next * 1103515245 + 12345;
    return (unsigned int)(random_packet.next/65536) % 32768;
  #else
  //official random implementation for C from CodeGuru forum
    return(((random_packet.next = random_packet.next * 214013L + 2531011L) >> 16) & 0x7fff);
  #endif
}



/** 
   @fn     void main(void)
   @brief  function is filling memory with PRNG data to verify stability of design
   @note   function is turn off DMA interrupt of UART
**/
void fill_memory(void){
  #define PRNG_PKT_LEN 12 
  uint8_t* pktptr;
  int len = 0,i = 1;
  NVIC_DisableIRQ ( DMA_UART_RX_IRQn );   // Disable DMA UART RX interrupt
  
  for (i = 0; i < NUM_OF_PACKETS_IN_MEMORY; i++){
    pktptr = &pktMemory[actualRxBuffer][i][0];
    len = 0;
    while(len < PACKET_MEMORY_DEPTH-HEAD_LENGHT-PRNG_PKT_LEN ){
      random_packet.randomPktNum ++;
      random_packet.rnadom = randc();
      memcpy(&pktptr[len+HEAD_LENGHT],&random_packet,PRNG_PKT_LEN);
      len = len + PRNG_PKT_LEN;
    }
    lenghtOfPkt[actualRxBuffer][numOfPkt[actualRxBuffer]] = len;
    numOfPkt[actualRxBuffer]++;
  }  
  memory_full_flag = TRUE;
  NVIC_EnableIRQ ( DMA_UART_RX_IRQn );    // Enable DMA UART RX interrupt
}
/** 
   @fn     void random_init(void)
   @brief  initialize all nesessary values for PRNG
**/
void random_init(void){
  random_packet.Slave='S';
  random_packet.slave_id = SLAVE_ID;
  random_packet.randomPktNum = 0;
  random_packet.next = 0;
  random_packet.rnadom = 0;
  srandc(RAND_SEED);
}

/** 
   @fn     int main(void)
   @brief  main function of slave program
   @note   Program is using time multiplex to send data to master device
   @return int 
**/
int main(void)
{   
  //int rnd;
  //double x;
  memset(lenghtOfPkt, 0, (NUM_OF_PACKETS_IN_MEMORY*2));
  memset(pktMemory, 0, (NUM_OF_PACKETS_IN_MEMORY*PACKET_MEMORY_DEPTH*2));
  WdtGo(T3CON_ENABLE_DIS);    //stop watch-dog

  //initialize all interfaces
  SetInterruptPriority();
  uart_init();
  led_init();
  random_init();
  
  LED_OFF;
#if THROUGHPUT_MEASURE
  troughputMeasureInit();
#endif
  radioInit();    //inicialize radio conection
    
  while (1){
    if (radioRecieve()){
      
      //if this slot identifier belongs to this slave
      if ( 0 == strcmp((char*)Buffer,TIME_SLOT_ID_SLAVE)){
        if(numOfPkt[actualRxBuffer])    //if is something to send
          transmit();
        else
          rf_printf(ZERO_PACKET);
      }
      
      //check if retransmit request
      if (0 == memcmp(Buffer,RETRANSMISION_ID,3/*chars to compare*/))   //check if re-tx slot
        retransmit();
      
      //check if sync packet
      if (0 == memcmp(Buffer,"SYNC",4/*chars to compare*/))
        setTimeToSync((Buffer[4]-'0') * SYNC_INTERVAL);
      
    }
    
    if (button_pushed()){   //initialize PRNG
      PRNG_data = TRUE;
      srandc(RAND_SEED);
    }
    //fill up the memory
    if ((PRNG_data == TRUE) && memory_full_flag == FALSE )
      fill_memory();

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
    @brief   Interrupt handler for measuring troughput
    @note    only if THROUGHPUT_MEASURE is set in settings.h
    @see     settings.h
**/
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
    @see     LEN_OF_RX_PKT
**/
void DMA_UART_RX_Int_Handler   ()
{
  //only if constant lenght of packet
#if LEN_OF_RX_PKT
uint8_t ch1, ch2;
  
  UrtDma(0,0); // prevents additional byte to restart DMA transfer
  #if BINARY_MODE
  rxUARTcount += LEN_OF_RX_PKT;   //count received data
  
  //check place in memory
  if (rxUARTcount <= PACKET_MEMORY_DEPTH-HEAD_LENGHT-LEN_OF_RX_PKT){
    rxPktPtr = &rxUARTbuffer[rxUARTcount];
  }
  else{
    rxPktPtr = &pktMemory[actualRxBuffer][numOfPkt[actualRxBuffer]][HEAD_LENGHT];
    
    for (i=0;i<LEN_OF_RX_PKT/2;i++){    //conversion of binary data to ascii chars 0 ... F 
      ch1 = rxUARTbuffer[i*2]-'0';
      if (ch1 > 9)         //because ASCII table is 0123456789:;<=>?@ABCDEF
        ch1 -= 7;
      ch2 = rxUARTbuffer[(i*2)+1]-'0';
      if (ch2 > 9)         //because ASCII table is 0123456789:;<=>?@ABCDEF
        ch2 -= 7;
      rxPktPtr[i] = (ch1<<4) | ch2;
    }
    
    //memcpy(rxPktPtr,rxUARTbuffer,rxUARTcount);    //copy buffer to mmory
    //save number of received bytes
    lenghtOfPkt[actualRxBuffer][numOfPkt[actualRxBuffer]] = rxUARTcount;
    rxUARTcount = 0;
    
    rxPktPtr = rxUARTbuffer;
    //incremet number of received packets
    if(numOfPkt[actualRxBuffer] < NUM_OF_PACKETS_IN_MEMORY)
      numOfPkt[actualRxBuffer]++;
    else
      memory_full_flag = TRUE;
  }
  #else       //normal mode (does not append data to packets)
  //pinter to point at new place in memory pointing beyound packet head
  rxPktPtr = &pktMemory[actualRxBuffer][numOfPkt[actualRxBuffer]][HEAD_LENGHT];
  memcpy(rxPktPtr,rxUARTbuffer,LEN_OF_RX_PKT);    //copy buffer to mmory
  rxPktPtr = rxUARTbuffer;
  
  //if is buffered more than 10 packets dont increment and overwrite last packet
  if(numOfPkt[actualRxBuffer] < NUM_OF_PACKETS_IN_MEMORY)
    numOfPkt[actualRxBuffer]++;
  else
    memory_full_flag = TRUE;
  #endif
  
  //enable RX DMA chanel for receiving next packets forom UART
  DmaChanSetup(UARTRX_C,ENABLE,ENABLE);   // Enable DMA channel  
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
/** 
    @fn      void UART_Int_Handler ()
    @brief   Interrupt handler managing store received data trought UART
              - string mode = appedning chars untill STRING_TERMINATOR is received 
              - binary mode = appending untill buffer is full == 240 == PACKETRAM_LEN chars
    @see     STRING_TERMINATOR
**/
void UART_Int_Handler (void)
{   
#if LEN_OF_RX_PKT==0
  uint8_t ucCOMIID0; 
  char ch;
 
  ucCOMIID0 = UrtIntSta(0);    // Read UART Interrupt ID register

  if ((ucCOMIID0 & COMIIR_STA_RXBUFFULL) == COMIIR_STA_RXBUFFULL)    // Receive buffer full interrupt
  {
    
    ch  = UrtRx(0);           //call UrtRd() clears COMIIR_STA_RXBUFFULL
  
    rxUARTbuffer[rxUARTcount]= ch;
    rxUARTcount++;
    
    #if BINARY_MODE
    if (rxUARTcount >= (PACKET_MEMORY_DEPTH-HEAD_LENGHT)){          //packt full
      if (numOfPkt[actualRxBuffer] < NUM_OF_PACKETS_IN_MEMORY){     //copy to memory
        rxPktPtr = &pktMemory[actualRxBuffer][numOfPkt[actualRxBuffer]][HEAD_LENGHT];   //pinting beyound packet head
        memcpy(rxPktPtr,rxUARTbuffer,rxUARTcount);
        lenghtOfPkt[actualRxBuffer][numOfPkt[actualRxBuffer]] = rxUARTcount;
        numOfPkt[actualRxBuffer]++;
    #else
    if (ch == STRING_TERMINATOR){         //end of packet pointer
      rxUARTbuffer[rxUARTcount]= '\0';    //write end of string
      if (numOfPkt[actualRxBuffer] < NUM_OF_PACKETS_IN_MEMORY){
        rxPktPtr = &pktMemory[actualRxBuffer][numOfPkt[actualRxBuffer]][HEAD_LENGHT];   //pinting beyound packet head
        strcpy(rxPktPtr,rxUARTbuffer);
        numOfPkt[actualRxBuffer]++;     
    #endif
      }
      else
        dma_printf("packet memory is full ");
      
      rxUARTcount=0;       
    }
  }
#endif
} 

