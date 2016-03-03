/**
 *****************************************************************************
   @addtogroup rst 
   @{
   @file     RstLib.c
   @brief    Set of reset functions.
   @version  V0.1
   @author   PAD CSE group
   @date     January 2012 
        

All files for ADuCRF101 provided by ADI, including this file, are
provided  as is without warranty of any kind, either expressed or implied.
The user assumes any and all risk from the use of this code.
It is the responsibility of the person integrating this code into an application
to ensure that the resulting application performs as required and is safe.

**/

#include "RstLib.h"
#include "ADuCRF101.h"


/**
	@brief int RstClr(int iSource);
			========== Clears reset status bit(s).
	@param iSource :{RSTCLR_PORLV| RSTCLR_PORHV| RSTCLR_EXTRST| RSTCLR_WDRST| RSTCLR_SWRST}
		- RSTCLR_PORHV clears Power-on reset HV status bit
		- RSTCLR_PORLV clears Power-on reset LV status bit 
		- RSTCLR_EXTRST clears external reset status bit
                - RSTCLR_WDRST clears watchdog reset status bit
                - RSTCLR_SWRST clears software reset status bit
	@return 1
**/
int RstClr(int iSource)
{
  pADI_RESET->RSTCLR = iSource;
  return 1;
}


/**
	@brief int RstSta(void);
			========== Reads and return source of reset.
	@return 1
**/
int RstSta(void)
{
  int iSta;
  iSta = pADI_RESET->RSTSTA;
  return iSta;
}

/**
	@brief int ShutSta(void);
			========== Reads and return source of wake up from shutdown mode.
	@return 1
**/
int ShutSta(void)
{
  int iSta;
  iSta = pADI_PWRCTL->SHUTDOWN;
  return iSta;
}

/**@}*/
