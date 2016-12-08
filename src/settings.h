/**
 *****************************************************************************
   @file     settings.h
   @brief    configurating file defining base settings


   @author      Bc. Peter Soltys
   @supervisor  doc. Ing. Milos Drutarovsky Phd.
   @version     
   @date        08.12.2016(DD.MM.YYYY) 


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
#ifndef __SETTINGS_H
#define __SETTINGS_H

/*******************************************************************************
* Hardware defined macros
*/

/** @brief size of RAM memory for packet in radio interface **/
#define PACKETRAM_LEN          240    


/*******************************************************************************
* Radio configuration macros
*/

/**
   @param BaseConfig :{DR_1_0kbps_Dev10_0kHz , DR_38_4kbps_Dev20kHz ,DR_300_0kbps_Dev75_0kHz }
      - DR_1_0kbps_Dev10_0kHz    Base configuration of 1 kbps datarate, 10.0 kHz frequency deviation.
      - DR_38_4kbps_Dev20kHz     Base configuration of 38.4 kbps datarate, 20 kHz frequency deviation.
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
         890 Mhz frequency of filter at used radio modules
**/
#define BASE_RADIO_FREQUENCY    890000000
#define MIN_RADIO_FREQUENCY     880000000
#define MAX_RADIO_FREQUENCY     900000000
#define FREQ_STEP               10000

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
    @brief  set function to calculate CRC sum
    @note   the reason why use crcSlow is that crcFast is using much more memory       
    @param  bool :{1 , 0}
         - 1 if crcFast
         - 0 if crcSlow
 **/
#define CRC_FAST 0


 /**
    @brief  char witch terminate all received packets     
    @param  char :{'$'}
 **/
#define STRING_TERMINATOR '$'

 /**
    @brief  set maximal memory (packet) depth (size of one packet)
    @note   macro set size of packet memory
    @note   radio interface memory is 240 bytes
    @see    NUM_OF_PACKETS_IN_MEMORY
    @param  memory deepth :{0 , 240}
 **/
#define PACKET_MEMORY_DEPTH 240 

 /**
    @brief  maximal number of packets (stored in memory) to send/receive 
    @note   macro set size of packet memory
    @note   maximum is restricted by avaliable memory (16kB SRAM)
            2*PACKET_MEMORY_DEPTH*NUM_OF_PACKETS_IN_MEMORY < 16kB
    @see    PACKET_MEMORY_DEPTH
    @param  number of packets :{1 , 20}
 **/
#define NUM_OF_PACKETS_IN_MEMORY 20

 /**
    @brief  number of expected slave devices
    @note   number is restricted by size of memory
            macro set size of packet memory
    @param  number of slave devices :{1 , 10} 4
 **/
#define NUMBER_OF_SLAVES 4  //number of slave devices

 /**
    @brief  lenght of received packets from UART
    @note   macro set size of packet memory
    @param  bool :{0 , 240}
          - 0 if variable lenght of packets
          - 1-240 constant lenght of packets
 **/
#define MAX_LEN_OF_RX_PKT 65

 /**
    @brief  time interval to interrupt for synchronization
    @note   time to interrupt = (1/40 000 000) * 256 * SYNC_INTERVAL [s]
            200 = 1.28 ms       //up to 255 (uint8_t)
    @param  count number :{200}
 **/
#define SYNC_INTERVAL 200 

 /**
    @brief  max time(number of increments) to response of requested device
    @note   interval witch is counted until packet is received (empiric 5500 at max lenght packet)
    @see    radioRecieve()
    @param  time :{7000 ~ 0.8ms}
 **/
#define T_TIMEOUT 7000     //max time(number of increments) to response of slave 

/**
    @brief  max time(number of increments) to flush all buffered packets
    @note   interval witch is counted until all packets are transmitted
    @see    flushBufferedPackets()
    @param  time :{7000 ~ 0.8ms}
 **/
#define DMA_TIMEOUT 100000     //max time(number of increments) to response of slave 

/**
    @brief  max numbers of tries to receiv packet at new frequency
    @note   after timeout slave set BASE_RADIO_FREQUENCY and is waiting for master frequency change
    @see    BASE_RADIO_FREQUENCY
 **/
#define FREQ_TIMEOUT 200    // max numbers of tries to receive packet at new frequency

 /**
    @brief  max number of receiving timeouts to restart receiving operation
    @see    radioRecieve()
    @see    T_TIMEOUT
    @param  (uint8_t) 0 - 255 
 **/
#define RX_PKT_TOUT_CNT 20

/*******************************************************************************
* Master interface settings
*/

 /**
    @brief  UART baudrate with is using master
 **/
#define UART_BAUD_RATE_MASTER 115200//256000//9600//

 /**
    @brief  string defining format of "ID slot" packet
 **/
#define TIME_SLOT_ID_MASTER "0slot"

 /**
    @brief  number of retransmission trying until slave is marked as not responding
    @param  retransmission attempts :{3}
 **/
#define RETRANSMISION 3     //number of retransmiting if no response

/*******************************************************************************
* Slave interface settings
*/

 /**
    @brief  UART baudrate with is using master
    @note   For compatibility with "UWB - Coordinate Reader Deployment" from 
            Peter Mikula baudrate should be 9600
 **/
#define UART_BAUD_RATE_SLAVE 9600


//macros to build string
#define MAKE_STRING(...)  #__VA_ARGS__
#define STRING(x)       MAKE_STRING(x)
#define CONCATENATE(arg1, arg2)   CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2)  CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2)  arg1##arg2

 /** @brief  number of actual slave
     @param slave number{1 - NUMBER_OF_SLAVES}
 **/
 //Slave == 1..4 number
#define SLAVE_ID 4

//slave identificating macros
 /** @brief  format of slot identificator  
     @note   "1slot"
 **/
#define TIME_SLOT_ID_SLAVE  STRING(CONCATENATE(SLAVE_ID,slot))    //number in string is Slave == 1..4 number
 /** @brief  format of zero packet
     @note   "100"
 **/
#define ZERO_PACKET STRING(CONCATENATE(SLAVE_ID,00))             //first number in string is Slave == 1..4 number
 /** @brief  format of retransmision packet
     @note   "1RE"
 **/
#define RETRANSMISION_ID STRING(CONCATENATE(SLAVE_ID,RE))        //number in string is Slave == 1..4 number


//head definition
 /** @brief  lenght of head in bytes **/
#define HEAD_LENGHT 3

//hardware based macros
 /** @brief  appended time(number of increments) after transmition to procesing on master **/
#define T_PROCESSING 0 

//synchronization pin settings
#define SYNC_PIN_HIGH DioSet(pADI_GP4,BIT2)
#define SYNC_PIN_LOW  DioClr(pADI_GP4,BIT2)
#define SYNC_PIN_READ DioRd(pADI_GP4)&0x04    //state of sync pin

/*******************************************************************************
* debug macros
*/
#define SIMULATE_RETX 0   /*!< @brief sending retransmitin message to test */
#define DEBUG_MESAGES 0   /*!< @brief stream of mesages to UART */
#define RX_STREAM 0       /*!< @brief stream of redeived data to UART**/
#define TX_STREAM 0       /*!< @brief stream of transmited data to UART**/
#define SEND_HEAD 0       /*!< @brief send also heads of packets on UART**/
#define HEXA_TRANSFER 0   /*!< @brief sending packets via UART in hexadecimal ASCII chars */

/*! @brief start checking PRNG packets local (on master} 
    @note  packets are not streamed on UART only messages
*/
#define PRNG_CONTINUAL_GENERATING 0 /*!< @brief system of generating PRNG packets 1=only during pushed button 0=automaticaly after first push */
#define CHECK_PRNG_LOCAL 0 /*!< @brief build checking of packet localy at master device */
#define PRNG_PKT_LEN 12   /*!< @brief lenght in bytes of PRNG packet */
#define RAND_SEED 500     /*!< @brief initial number for PRNG */
          



#endif    //#ifndef __SETTINGS_H

/*! \mainpage ADuc-RF-101 Time multiplex
 
  \section intro_sec Introduction
 
	Program is using time multiplex for transmiting data from optionaly number of Slaves to Maseter device 
	via RF-channel with checking data integrity using hardware implement CRC and retransmiting losted data packets.

	Debuged for ADucRF101MKxZ development kit

	- Author:   Peter Soltys
	- Version:  
	- Hardware: ADucRF101MKxZ
	- Date:       08.12.2016	    19.04.2016	    19.04.2016	    19.04.2016	   19.04.2016(DD.MM.YYYY)
	- Project:  Time-multiplex-ADuc-RF101
  - DEV:      Keil 5.1 Evaluation
	- Note:     v2.1B fixed synchronization and added binary mode




	Compatible with program  UWB - Coordinate Reader Deployment from Peter Mikula
	[uwb_coordinate_reader](https://github.com/Gresthorn/UWB_COORDINATE_READER "uwb_coordinate_reader")
 
  Program is able to find online on [Github](https://github.com/petersoltys/Time-multiplex-ADuC-rf101_v2 "Github")
  \section build_sec Build
 
  \subsection step1 Step 1: Opening project in Keil 
   
  etc...
 */
                              

