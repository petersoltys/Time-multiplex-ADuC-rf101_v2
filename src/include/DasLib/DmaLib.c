/**
 *****************************************************************************
   @addtogroup dma 
   @{
   @file       DmaLib.c
   @brief      Set of DMA peripheral functions.
   
   @version    V0.1
   @author     PAD CSE group
   @date       December 2012


All files for ADuCRF101 provided by ADI, including this file, are
provided  as is without warranty of any kind, either expressed or implied.
The user assumes any and all risk from the use of this code.
It is the responsibility of the person integrating this code into an application
to ensure that the resulting application performs as required and is safe.

**/

#include <stdio.h>
#include <string.h>
#include "DmaLib.h"


#if defined(__ARMCC_VERSION) || defined(__GNUC__)
DmaDesc dmaChanDesc     [CCD_SIZE * 2] __attribute__ ((aligned (DMACHAN_DSC_ALIGN)));      
#endif

#ifdef __ICCARM__
#pragma data_alignment=(DMACHAN_DSC_ALIGN)
DmaDesc dmaChanDesc     [CCD_SIZE * 2];
#endif

/**
	@brief int DmaInit(void)
			========== Sets the Address of DMA Data base pointer.

	@return 1.
        this function must be called before using the DMA controller.
**/
int DmaInit(void)
{  	 
   memset(dmaChanDesc,0x0,sizeof(dmaChanDesc));      // Clear all the DMA descriptors (individual blocks will update their  descriptors
   pADI_DMA->DMAPDBPTR = (unsigned int)&dmaChanDesc; // Setup the DMA base pointer.
   pADI_DMA->DMACFG = 1;                             // Enable DMA controller	
   return 1;	
}
    

/**
   @brief int DmaChanSetup (unsigned int iChan, unsigned char iEnable, int iPriority)	
         ========== Enables or disables DMA channel and set priority.
            
   @param iChan :{SPI1TX_C,SPI1RX_C,UARTTX_C,UARTRX_C,I2CSTX_C,I2CSRX_C,I2CMTX_C,I2CMRX_C,ADC0_C,SPI0TX_C,SPI0RX_C}
		- Set to peripheral required.
                - 0 or SPI1TX_C for SPI1 Transmit
                - 1 or SPI1RX_C for SPI1 Receive
                - 2 or UARTTX_C for UART Transmit
                - 3 or UARTRX_C for UART Receive
                - 4 or I2CSTX_C for I2C Slave Transmit
                - 5 or I2CSRX_C for I2C Slave Receive
                - 6 or I2CMTX_C for I2C Master Transmit
                - 7 or I2CMRX_C for I2C Master Receive
                - 11 or ADC0_C for ADC0 read
                - 12 or SPI0TX_C for SPI0 Transmit
                - 13 or SPI0RX_C for SPI0 Receive
	@param iEnable :{DISABLE, ENABLE}
		- DISABLE or 0
                - ENABLE or 1 
	@param iPriority :{DISABLE, ENABLE}
		- DISABLE or 0
                - ENABLE or 1
*/

int DmaChanSetup (unsigned int iChan, unsigned char iEnable, int iPriority)
{
  if (iEnable)
  {
    pADI_DMA->DMARMSKCLR = (0x1 << iChan ); // disable mask
    pADI_DMA->DMAENSET = (0x1 << iChan );  // enable channel
    pADI_DMA->DMAPRISET = (0x1 << iChan ); // configure priority   
  }
  else
  {
    pADI_DMA->DMARMSKSET = (0x1 << iChan ); // enable mask
    pADI_DMA->DMAENCLR = (0x1 << iChan );
    pADI_DMA->DMAPRICLR = (0x1 << iChan ); // configure priority   
  }
 return (1);
}


/**
   @brief int DmaTransferSetup(int iChan, int iNumVals, unsigned char *pucDMA)	
         ==========Sets up DMA config structure for the required channel
            
   @param iChan :{SPI1TX_C,SPI1RX_C,UARTTX_C,UARTRX_C,I2CSTX_C,I2CSRX_C,I2CMTX_C,I2CMRX_C,ADC0_C,SPI0TX_C,SPI0RX_C}
		- Set to peripheral required.
                - 0 or SPI1TX_C for SPI1 Transmit
                - 1 or SPI1RX_C for SPI1 Receive
                - 2 or UARTTX_C for UART Transmit
                - 3 or UARTRX_C for UART Receive
                - 4 or I2CSTX_C for I2C Slave Transmit
                - 5 or I2CSRX_C for I2C Slave Receive
                - 6 or I2CMTX_C for I2C Master Transmit
                - 7 or I2CMRX_C for I2C Master Receive
                - 11 or ADC0_C for ADC0 read
                - 12 or SPI0TX_C for SPI0 Transmit
                - 13 or SPI0RX_C for SPI0 Receive
	@param iNumVals :{1-1024}
		- Number of values to be transferred. 
        @param *pucDMA :{0-0xFFFFFFFF}
                - Pass a pointer to memory for DMA transfers
        @return 1:

**/
int DmaTransferSetup(int iChan, int iNumVals, unsigned char *pucDMA)
{
  DmaDesc Desc;

  switch (iChan)
  {   
  case SPI1TX_C:  // channel 0     
    Desc.srcEndPtr    = (unsigned int)(pucDMA + iNumVals - 0x1);  
    Desc.destEndPtr   = (unsigned int)(&pADI_SPI1->SPITX);
    Desc.ctrlCfgVal = 0xC0000001 + ((iNumVals - 0x1) << 4);
    break;		 
  case SPI1RX_C:   // channel 1    
    Desc.srcEndPtr    = (unsigned int)(&pADI_SPI1->SPIRX); 
    Desc.destEndPtr   = (unsigned int)(pucDMA + iNumVals - 0x1); 
    Desc.ctrlCfgVal = 0x0C000001 + ((iNumVals - 0x1) << 4);
    break;  
  case UARTTX_C:  // channel 2
    Desc.srcEndPtr    = (unsigned int)(pucDMA + iNumVals - 0x1);	    
    Desc.destEndPtr   = (unsigned int)(&pADI_UART->COMTX);		 	 		
    Desc.ctrlCfgVal = 0xC0000001 + ((iNumVals - 0x1) << 4);
    break;  
  case UARTRX_C: // channel 3
    Desc.srcEndPtr    = (unsigned int)(&pADI_UART->COMRX);	    
    Desc.destEndPtr   = (unsigned int)(pucDMA + iNumVals - 0x1);		 	 		
    Desc.ctrlCfgVal = 0x0C000001 + ((iNumVals - 0x1) << 4);
    break;  
  case I2CSTX_C:  // channel 4
    Desc.srcEndPtr    = (unsigned int)(pucDMA + iNumVals - 0x1);	      
    Desc.destEndPtr   = (unsigned int)(&pADI_I2C->I2CSTX);
    Desc.ctrlCfgVal = 0xC0000001 + ((iNumVals - 0x1) << 4);
    break;			 	
  case I2CSRX_C:  // channel 5
    Desc.srcEndPtr    = (unsigned int)(&pADI_I2C->I2CSRX);	      
    Desc.destEndPtr   = (unsigned int)(pucDMA + iNumVals - 0x1);
    Desc.ctrlCfgVal = 0x0C000001 + ((iNumVals - 0x1) << 4);
    break;			 	
  case I2CMTX_C:  // channel 6
    Desc.srcEndPtr    = (unsigned int)(pucDMA + iNumVals - 0x1);
    Desc.destEndPtr   = (unsigned int)(&pADI_I2C->I2CMTX);
    Desc.ctrlCfgVal = 0xC0000001 + ((iNumVals - 0x1) << 4);
    break;
  case I2CMRX_C:  // channel 7
    Desc.srcEndPtr    = (unsigned int)(&pADI_I2C->I2CMRX);
    Desc.destEndPtr   = (unsigned int)(pucDMA + iNumVals - 0x1);
    Desc.ctrlCfgVal = 0x0C000001 + ((iNumVals - 0x1) << 4);
    break;
  case ADC0_C: // channel 11 
    Desc.srcEndPtr    = (unsigned int)(&pADI_ADC0->ADCDAT);  
    Desc.destEndPtr   = (unsigned int)(pucDMA + 2 * (iNumVals - 0x1));
    Desc.ctrlCfgVal = 0x5D000001 + ((iNumVals - 0x1) << 4);
    break;
  case SPI0TX_C: // channel 12 
    Desc.srcEndPtr    = (unsigned int)(pucDMA + iNumVals - 0x1);  
    Desc.destEndPtr   = (unsigned int)(&pADI_SPI0->SPITX);
    Desc.ctrlCfgVal = 0xC0000001 + ((iNumVals - 0x1) << 4);
    break;		 		 
  case SPI0RX_C:  // channel 13     
    Desc.srcEndPtr    = (unsigned int)(&pADI_SPI0->SPIRX); 
    Desc.destEndPtr   = (unsigned int)(pucDMA + iNumVals - 0x1); 
    Desc.ctrlCfgVal = 0x0C000001 + ((iNumVals - 0x1) << 4);
    break;   
  default:	 
    break;	
  }  
  dmaChanDesc[iChan] = Desc;
  return 1;	
}

/**@}*/	

