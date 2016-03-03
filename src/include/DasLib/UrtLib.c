/**
 *****************************************************************************
   @addtogroup urt 
   @{
   @file     UrtLib.c
   @brief    Set of UART peripheral functions.
   @version  V0.2
   @author   PAD CSE group
   @date     December 2012 

   @par Revision History:
   - V0.1, January 2012: initial version. 
   - V0.2, December 2012: addition of iChan parameter in all functions,
                         modification of UrtCfg() to UrtLinCfg() function.           

All files for ADuCRF101 provided by ADI, including this file, are
provided  as is without warranty of any kind, either expressed or implied.
The user assumes any and all risk from the use of this code.
It is the responsibility of the person integrating this code into an application
to ensure that the resulting application performs as required and is safe.

**/

#include "UrtLib.h"
#include "ADuCRF101.h"


/**
	@brief int UrtLinCfg(int iChan, int iBaud, int iBits, int iFormat)
			==========Configure the UART.
	@param iChan :{0}	\n
		Set to 0. This value is ignored since there is only one channel.
	@param iBaud :{50-460800}	\n
		Set iBaud to the baudrate required:
	@param iBits :{COMLCR_WLS_5BITS,COMLCR_WLS_6BITS,COMLCR_WLS_7BITS,COMLCR_WLS_8BITS}	\n
			- COMLCR_WLS_5BITS for data length 5bits.
			- COMLCR_WLS_6BITS for data length 6bits.
			- COMLCR_WLS_7BITS for data length 7bits.
			- COMLCR_WLS_8BITS for data length 8bits.
	@param iFormat :{COMLCR_STOP_EN|COMLCR_PEN_EN|COMLCR_EPS_EN|COMLCR_SP_EN|COMLCR_BRK_EN}	\n
		The bitwise or of the data format required:
			- COMLCR_STOP_EN for more than stop bits. COMLCR_STOP_DIS by default.
			- COMLCR_PEN_EN Parity used (enabled). COMLCR_PEN_DIS by default.
			- COMLCR_EPS_EN configure Even parity. COMLCR_EPS_DIS by default.
			- COMLCR_SP_EN configure Sticky parity. COMLCR_SP_DIS by default.
			- COMLCR_BRK_SET configure Break (Set SOUT pin to 0). COMLCR_BRK_CLR by default.
	@return Value of COMLSR: See UrtLinSta() function for bit details.
	@note
		- Powers up UART if not powered up.
		- Standard baudrates are accurate to better than 0.1% plus clock error.\n
		- Non standard baudrates are accurate to better than 1% plus clock error.
**/

int UrtLinCfg(int iChan, int iBaud, int iBits, int iFormat)
	{
	int i1;
	i1 = (16000000/32)/iBaud;
	pADI_UART->COMDIV = i1;
	pADI_UART->COMFBR = 0x8800|(((((2048/32)*16000000)/i1)/iBaud)-2048);
	pADI_UART->COMIEN = 0;
	pADI_UART->COMLCR = (iFormat&0x7c)|(iBits&3);
	return	pADI_UART->COMLSR;
	}

/**
	@brief int UrtLinSta(int iChan)
			==========Read the status byte of the UART.
	@param iChan :{0}	\n
		Set to 0. This value is ignored since there is only one channel.
	@return value of COMLSR:
		- COMLSR_DR = Data ready.
		- COMLSR_OE = Overrun error.
		- COMLSR_PE = Parity error.
		- COMLSR_FE = Framing error.
		- COMLSR_BI = Break indicator.
		- COMLSR_THRE = Transmit queue empty.
		- COMLSR_TEMT = Transmit holding register empty.
	@warning	UART must be configured before checking status.
**/

int UrtLinSta(int iChan)
{
	return	pADI_UART->COMLSR;
}

/**
	@brief int UrtTx(int iChan, int iTx)
			==========Write 8 bits of iTx to the UART.
	@param iChan :{0}	\n
		Set to 0. This value is ignored since there is only one channel.
	@param iTx :{0-255}	\n
		Byte to transmit.
	@return 1 
	@warning
		UrtLinSta should be called and Tx buffer state checked before calling UrtTx.\n
		Character is lost if TX buffer already full.
**/

int UrtTx(int iChan, int iTx)
{
    pADI_UART->COMTX = iTx;
    return 1;
}

/**
	@brief int UrtRx(int iChan)
			==========Read the UART data.
	@param iChan :{0}	\n
		Set to 0. This value is ignored since there is only one channel.
	@return The byte in the Rx buffer (COMRX).

**/

int UrtRx(int iChan)
{
  return pADI_UART->COMRX&0xff;
}

/**
	@brief int UrtIntCfg (int iChan, int iIrq)
			========== Enables/Disables UART Interrupt sources.
	@param iChan :{0}	\n
		Set to 0. This value is ignored since there is only one channel.
	@param iIrq :{COMIEN_ERBFI_EN| COMIEN_ETBEI_EN| COMIEN_ELSI_EN| COMIEN_EDSSI_EN| COMIEN_EDMAT_EN| COMIEN_EDMAR_EN}
		- Set to the bitwise or combination of
		- COMIEN_ERBFI_EN to enable UART RX IRQ. COMIEN_ERBFI_DIS by default.
		- COMIEN_ETBEI_EN to enable UART TX IRQ. COMIEN_ETBEI_DIS by default.
		- COMIEN_ELSI_EN to enable UART Status IRQ. COMIEN_ELSI_DIS by default.
		- COMIEN_EDSSI_EN to enable UART Modem status IRQ. COMIEN_EDSSI_DIS by default.
		- COMIEN_EDMAT_EN to enable UART DMA Tx IRQ. COMIEN_EDMAT_DIS by default.
		- COMIEN_EDMAR_EN to enable UART DMA Rx IRQ. COMIEN_EDMAR_DIS by default.
	@return 1.
*/

int UrtIntCfg(int iChan, int iIrq)
{
   pADI_UART->COMIEN = iIrq;
   return 1;
}

/**
	@brief int UrtIntSta(int iChan)
			========== Return UART interrupt status.
	@param iChan :{0}	\n
		Set to 0. This value is ignored since there is only one channel.
	@return COMIIR.
*/

int UrtIntSta(int iChan)
{
   return pADI_UART->COMIIR;
}

/**
	@brief int UrtModCfg(int iChan, int iMcr, int iWr)
			==========Write iMcr to UART Modem Control Register
	@param iChan :{0}	\n
		Set to 0. This value is ignored since there is only one channel.
	@param iMcr :{COMMCR_DTR_EN|COMMCR_RTS_EN|COMMCR_LOOPBACK_EN}	
		Set to the modem control combination required (COMMCR):
		- COMMCR_DTR_EN for Data terminal ready. COMMCR_DTR_DIS by default.
		- COMMCR_RTS_EN for Request to send. COMMCR_RTS_DIS by default.
		- COMMCR_LOOPBACK_EN for Loop back. COMMCR_LOOPBACK_DIS by default.
	@param iWr :{0,1}
		- 0 to read mode only (ignores iMcr).
		- 1 to write and read mode.
	@return value of COMMSR	
**/
	
int UrtModCfg(int iChan, int iMcr, int iWr)
{
  if(iWr)	
    pADI_UART->COMMCR = iMcr;  
  return pADI_UART->COMMSR&0x0ff;
}

/**
	@brief int UrtModSta(int iChan)
			==========Read the Modem status register byte of the UART.
	@param iChan :{0}	\n
		Set to 0. This value is ignored since there is only one channel.
	@return value of COMMSR:
		- COMMSR_DCD = Data carrier detect level
		- COMMSR_RI = Ring indicator level
		- COMMSR_DSR = Data set ready status
		- COMMSR_CTS = Clear to Send input level
		- COMMSR_DDCD = Delta DCD status
		- COMMSR_TERI = trailing edge Ring indicator status
		- COMMSR_DDSR = Delta DSR status
		- COMMSR_DCTS = Delta CTS status
	@warning	UART must be configured before checking status
**/

int UrtModSta(int iChan)	
{
  return pADI_UART->COMMSR;
}

/**
	@brief int UrtDma(int iChan, int iDmaSel)
			==========Enables/Disables DMA channel.
	@param iChan :{0}	\n
		Set to 0. This value is ignored since there is only one channel.
	@param iDmaSel :{COMIEN_EDMAT_EN| COMIEN_EDMAR_EN}
		- Set to the bitwise or combination of
		- COMIEN_EDMAT_EN to enable UART DMA Tx IRQ. COMIEN_EDMAT_DIS by default.
		- COMIEN_EDMAR_EN to enable UART DMA Rx IRQ. COMIEN_EDMAR_DIS by default.
	@return 1.
*/

int UrtDma(int iChan, int iDmaSel)
{
  int	i1;
  i1 = pADI_UART->COMIEN & ~COMIEN_EDMAT_MSK & ~COMIEN_EDMAR_MSK; 
  i1 |= iDmaSel;
  pADI_UART->COMIEN = i1;
  return 1;
}

/**
	@brief int UrtBrk(int iChan, int iBrk)
			==========Force SOUT pin to 0
	@param iChan :{0}	\n
		Set to 0. This value is ignored since there is only one channel.
	@param iBrk :{COMLCR_BRK_CLR, COMLCR_BRK_SET}	
		- COMLCR_BRK_CLR to disable SOUT break condition (SOUT behaves as normal)
		- COMLCR_BRK_SET to force SOUT break condition - SOUT remains high until this bit is cleared
	@return Value of COMLSR: See UrtLinSta() function for bit details.
**/

int UrtBrk(int iChan, int iBrk)
{
  if (iBrk == 0)
    pADI_UART->COMLCR &= 0x3F;				  // Disable break condition on SOUT pin
  else
    pADI_UART->COMLCR |= 0x40;				  // force break condition on SOUT pin
  return pADI_UART->COMLSR;
}

/**@}*/
