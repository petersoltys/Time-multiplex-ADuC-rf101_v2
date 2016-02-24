/**
 *****************************************************************************
   @file     settings.h
   @brief    configurating file defining base settings


   @version  V2.1B
   @author   Peter Soltys
   @date     february 2016  


  @note : in radioeng.c was changed intial value from \n
      static RIE_BOOL             bPacketTx                     = RIE_FALSE; \n
      static RIE_BOOL             bPacketRx                     = RIE_FALSE; \n
  to \n
      static RIE_BOOL             bPacketTx                     = RIE_TRUE; \n
      static RIE_BOOL             bPacketRx                     = RIE_TRUE; \n
      
  @section Disclaimer
  THIS SOFTWARE IS PROVIDED BY BC PETER SOLTYS. ``AS IS'' AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT, ARE
  DISCLAIMED. IN NO EVENT SHALL BC PETER SOLTYS. BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

  YOU ASSUME ANY AND ALL RISK FROM THE USE OF THIS CODE OR SUPPORT FILE.

  IT IS THE RESPONSIBILITY OF THE PERSON INTEGRATING THIS CODE INTO AN APPLICATION
  TO ENSURE THAT THE RESULTING APPLICATION PERFORMS AS REQUIRED AND IS SAFE.
  <hr>

**/

/*******************************************************************************
* Radio configuration macros
*/

/**
   @param BaseConfig :{DR_1_0kbps_Dev10_0kHz , DR_38_4kbps_Dev20kHz ,DR_300_0kbps_Dev75_0kHz }
      - DR_1_0kbps_Dev10_0kHz  Base configuration of 1 kbps datarate, 10.0 kHz frequency deviation.
      - DR_38_4kbps_Dev20kHz  Base configuration of 38.4 kbps datarate, 20 kHz frequency deviation.
      - DR_300_0kbps_Dev75_0kHz  Base configuration of 300 kbps datarate, 75 kHz frequency deviation.
**/
#define RADIO_CFG         DR_300_0kbps_Dev75_0kHz 

/**
   @brief Radio Transmitter Modulation Type.
   @param ModulationType :{FSK_Modulation , GFSK_Modulation }
      - FSK_Modulation  Frequency shift keying modulatino
      - GFSK_Modulation  Gausian frequency shift keying modulatino
**/
#define RADIO_MODULATION GFSK_Modulation

/**
   @brief Frequency for radio communications
   @param Frequency :{431000000-928000000} 
      - This must be within the available bands of the radio: 
         - 431000000Hz to 464000000Hz and 
         - 862000000Hz to 928000000Hz.
   @note 433.92 Mhz (EU) free frequency
**/
#define RADIO_FREQENCY     433920000//**433.92 Mhz |EU}(Bakalarka Oto Petura)

/**
   @brief  PA Type for Radio Transmission. 
   @param  PAType :{DifferentialPA, SingleEndedPA} Select Single Ended or Differential PA Type
**/
#define PA_TYPE DifferentialPA

/**
   @brief  Power Level for Radio Transmission. 
   @param  Power :{PowerLevel0 ,PowerLevel1 ,PowerLevel2 ,PowerLevel3,
           PowerLevel4 ,PowerLevel5 ,PowerLevel6 ,PowerLevel7,
           PowerLevel8 ,PowerLevel9 ,PowerLevel10,PowerLevel11,
           PowerLevel12,PowerLevel13,PowerLevel14,PowerLevel15} 
**/
#define RADIO_POWER        PowerLevel15

/**
    @brief   Enable or Disable Data Whitening of payload data.  

             Data whitening can be employed to avoid long runs of 1s or 0s
             in the transmitted data stream. 

             This ensures sufficient bit transitions in the packet, which 
             aids in receiver clock and data recovery because the encoding 
             breaks up long runs of 1s or 0s in the transmit packet. 

             The data, excluding the preamble and sync word, is automatically
             whitened before transmission by XORing the data with an 8-bit 
             pseudorandom sequence. 

             At the receiver, the data is XORed with the same pseudorandom 
             sequence, thereby reversing the whitening. 

             The linear feedback shift register polynomial used is x7 + x1 + 1.

    @param  bEnable :{RIE_FALSE, RIE_TRUE}
         - RIE_TRUE if Manchester Encoding is to be enabled. 
         - RIE_FALSE if disabled.
**/
#define DATA_WHITENING     RIE_FALSE
 /**
    @brief Enable or Disable Manchester Encoding of payload data.

           Manchester encoding can be used to ensure a dc-free (zero mean) 
           transmission. 

           A Binary 0 is mapped to 10, and a Binary 1 is mapped to 01. 

           Manchester encoding and decoding are applied to the payload data 
           and the CRC. 
     
    @param  bEnable :{RIE_FALSE,RIE_TRUE}
         - RIE_TRUE if Manchester Encoding is to be enabled. 
         - RIE_FALSE if disabled.
 **/
#define RADIO_MANCHASTER     RIE_FALSE

/*******************************************************************************
* Common settings
*/
 /**
    @brief Enable or Disable binary mode

           Binary mode is transmitting data packets in maximal lenght 240
           All received data in slave are stored together
           
           In (normal) string mode packets are terminated with STRING_TERMINATOR
    @bug    not entirely verified (still bugs)
    @param  bool :{1 , 0}
         - 1 if Binary mode
         - 0 if String mode.
 **/
#define BINARY_MODE 0

 /**
    @brief  char witch terminate all received packets     
    @param  char :{'$'}
 **/
#define STRING_TERMINATOR '$'

 /**
    @brief  maximal memory (packet) depth
    @note   macro set greatness of packet memory
    @param  memory deepth :{0 , 240}
 **/
#define PACKET_MEMORY_DEPTH 240 

 /**
    @brief  maximal number of packets to send/receive
    @note   macro set greatness of packet memory
    @param  number of packets :{1 , 20}
 **/
#define NUM_OF_PACKETS_IN_MEMORY 20

 /**
    @brief  number of expected slave devices
    @note   number is restricted by size of memory
            macro set greatness of packet memory
    @param  number of slave devices :{1 , 10} 4
 **/
#define NUM_OF_SLAVE 4 //number of slave devices

 /**
    @brief  lenght of received packets from UART
    @note   macro set greatness of packet memory
    @bug    constant lenght not entirely verified (still bugs)
    @param  bool :{0 , 240}
          - 0 if variable lenght of packets
          - 1-240 constant lenght of packets
 **/
#define LEN_OF_RX_PKT 0 

 /**
    @brief  offset in numbers of head
    @note   offset in numbers of head, because 0x00 is defined like 
            end of string => working with packets as with strings
    @param  char :{'0'}
 **/
#define CHAR_OFFSET '0' 

 /**
    @brief  time interval to interrupt for synchronization
    @note   time to interrupt = (1/40 000 000) * 256 * SYNC_INTERVAL [s]
            200 = 1.28 ms //up to 255 (unsigned char)
    @param  count number :{200}
 **/
#define SYNC_INTERVAL 200 

 /**
    @brief  max time(number of increments) to response of requested devide
    @note   interval witch is counted until packet is received
    @see    radioRecieve()
    @param  time :{50000}
 **/
#define T_TIMEOUT 50000 //max time(number of increments) to response of slave

/*******************************************************************************
* Master interface settings
*/

 /**
    @brief  UART baudrate with is using master
 **/
#define UART_BAUD_RATE_MASTER 128000

 /**
    @brief  string defining format of "ID slot" packet
 **/
#define TIME_SLOT_ID_MASTER "%dslot",slave_ID

 /**
    @brief  number of retransmission trying until slave is marked as not responding
    @param  retransmission attempts :{3}
 **/
#define RETRANSMISION 3 //number of retransmiting comand if no response

/*******************************************************************************
* Slave interface settings
*/

 /**
    @brief  UART baudrate with is using master
    @note   Baudrate is ste to 9600 because of compatibility with 
            "UWB - Coordinate Reader Deployment" from Peter Mikula
 **/
#define UART_BAUD_RATE_SLAVE 128000

//slave identificating macros
 /** @brief  format of slot identificator  
     @param slave number{1 - NUM_OF_SLAVE}
 **/
#define TIME_SLOT_ID_SLAVE "2slot"//number in string is Slave == 1..4 number
 /** @brief  format of zero packet
     @param slave number{1 - NUM_OF_SLAVE} first number
 **/
#define ZERO_PACKET "200"         //first number in string is Slave == 1..4 number
 /** @brief  format of retransmision packet
     @param slave number{1 - NUM_OF_SLAVE}
 **/
#define RETRANSMISION_ID "2RE"    //number in string is Slave == 1..4 number
 /** @brief  number of actual slave
     @param slave number{1 - NUM_OF_SLAVE}
 **/
#define SLAVE_ID 2                //Slave == 1..4 number


//head definition
 /** @brief  lenght of head in bytes **/
#define HEAD_LENGHT 3
//format inside of sprintf
#define HEAD_FORMAT "%d%c%c",SLAVE_ID,txPkt,numOfPackets[actualTxBuffer]-1 

//hardware based macros
 /** @brief  appended time(number of increments) after transmition to procesing on master **/
#define T_PROCESSING 0 

//synchronization pin settings
#define SYNC_PIN_HIGH DioSet(pADI_GP4,BIT2)
#define SYNC_PIN_LOW  DioClr(pADI_GP4,BIT2)
#define SYNC_PIN_READ DioRd(pADI_GP4)&0x04//state of sync pin

/*******************************************************************************
* debug macros
*/
#define SIMULATE_RETX 0 /*!< @brief sending retransmitin message to test */
#define DEBUG_MESAGES 0 /*!< @brief stream of mesages to UART */
#define RX_STREAM 0 /*!< @brief stream of redeived data to UART**/
#define TX_STREAM 0 /*!< @brief stream of transmited data to UART**/
#define SEND_HEAD 0 /*!< @brief send also heads of packets on UART**/

/**
  @brief measured data troughput
  @param {0-3}
          - if ==0 no measuring
          - if ==1 measure throughput of received data from UART
          - if ==2 measure maximum throughput with shyntetic data
  @note measured are all data included packet heads (aproximetlz 5000 Bytes/s by slave)
**/
#define THROUGHPUT_MEASURE 0  
                              
                              
                              