/**
 *****************************************************************************
   @file     DmaLib.h
   @brief Set of DMA peripheral functions.
		
   @version    V0.1
   @author     PAD CSE group
   @date       December 2012
   

All files for ADuCRF101 provided by ADI, including this file, are
provided  as is without warranty of any kind, either expressed or implied.
The user assumes any and all risk from the use of this code.
It is the responsibility of the person integrating this code into an application
to ensure that the resulting application performs as required and is safe.

**/
#include <ADuCRF101.h>

// Define the structure of a DMA descriptor.
typedef struct dmaDesc
{
   unsigned int srcEndPtr;
   unsigned int destEndPtr;
   unsigned int ctrlCfgVal;
   unsigned int reserved4Bytes;
} DmaDesc;


/* CCD allocation must be an integral power of two, i.e., 14 channels is allocated as 16 */
#define CCD_SIZE 16

// Define dmaChanDesc as an array of descriptors aligned to the required 
// boundary for all supported compilers. 
// Support primary and alternate.
#define DMACHAN_DSC_ALIGN 0x200

extern int DmaInit(void); 
extern int DmaTransferSetup(int iChan, int iNumVals, unsigned char *pucDMA);
extern int DmaChanSetup (unsigned int iChan, unsigned char iEnable, int iPriority); 


//DMA channel numbers.
#define	SPI1TX_C	0
#define	SPI1RX_C	1
#define	UARTTX_C	2
#define	UARTRX_C	3
#define	I2CSTX_C	4
#define	I2CSRX_C	5
#define	I2CMTX_C	6
#define	I2CMRX_C	7
#define	ADC0_C	        11
#define	SPI0TX_C	12
#define	SPI0RX_C	13

#define DISABLE 0
#define ENABLE 1




