/**
 *****************************************************************************
   @addtogroup pwr 
   @{
   @file     PwrLib.c
   @brief    Set of power optimisation functions.
   @version  V0.3
   @author   PAD CSE group
   @date     March 2013 
        
   @par Revision History:
   - V0.1, January 2012: initial version. 
   - V0.2, January 2013: modification of PsmCfg() function.  
   - V0.3, March 2013: modification of PsmCfg() function. 

All files for ADuCRF101 provided by ADI, including this file, are
provided  as is without warranty of any kind, either expressed or implied.
The user assumes any and all risk from the use of this code.
It is the responsibility of the person integrating this code into an application
to ensure that the resulting application performs as required and is safe.

**/

#include "PwrLib.h"
#include "ADuCRF101.h"


/**
	@brief int SramCfg(int iEnable);
			========== Enable 1/2 SRAM power gating in hibernate mode.
	@param iEnable :{SRAMRET_RETAIN_DIS,SRAMRET_RETAIN_EN}
		- SRAMRET_RETAIN_DIS
		- SRAMRET_RETAIN_EN
	@return 1.
**/
int SramCfg(int iEnable)
{
  pADI_PWRCTL->PWRKEY = PWRKEY_VALUE_KEY1;
  pADI_PWRCTL->PWRKEY = PWRKEY_VALUE_KEY2;
  pADI_PWRCTL->SRAMRET = iEnable;
  return 1;
}


/**
	@brief int PwrSta(void)
			========== Reads PWRMOD to check the status of the WIC acknowledge bit.
	@return PWRMOD
	
**/

int PwrSta(void)
{
  return pADI_PWRCTL->PWRMOD;	
}


/**
	@brief int PsmCfg(int iEnable);
			========== Enable or disable supply monitor.
	@param iEnable :{PSMCON_PD_DIS| PSMCON_PD_EN}
		- PSMCON_PD_DIS to enable power supply monitor
                - PSMCON_PD_EN to disable power supply monitor
        @warning: During low power mode, the PSM must be disabled.
	@return 1
**/

int PsmCfg(int iEnable)
{
  if (iEnable == PSMCON_PD_DIS) 
    pADI_PWRCTL->PSMCON = 0x1;  // bit 0 should be written 1 always.   
  else 
    pADI_PWRCTL->PSMCON |= PSMCON_PD_EN;     
  return 1;
}


/**
	@brief int PwrCfg(int iMode, int iExit);
			========== Configure sleep mode.
	@param iMode :{PWRMOD_MOD_FLEXI,PWRMOD_MOD_HIBERNATE,PWRMOD_MOD_SHUTDOWN}
		- PWRMOD_MOD_FLEXI
                - PWRMOD_MOD_HIBERNATE
                - PWRMOD_MOD_SHUTDOWN
	@param iExit :{SCR_SLEEPONEXIT_DIS,SCR_SLEEPONEXIT_EN}
		- SCR_SLEEPONEXIT_DIS
                - SCR_SLEEPONEXIT_EN
	@return 1 
**/

int PwrCfg(int iMode, int iExit)
{      
  if (iExit==SCR_SLEEPONEXIT_EN)
    SCB->SCR |= SCR_SLEEPONEXIT_EN;
  if ((iMode == PWRMOD_MOD_HIBERNATE)||(iMode == PWRMOD_MOD_SHUTDOWN) )
    SCB->SCR |= SCR_SLEEPDEEP;
   
  if ((iMode == PWRMOD_MOD_FLEXI) ||(iMode == PWRMOD_MOD_HIBERNATE) ||(iMode == PWRMOD_MOD_SHUTDOWN) )
   {
     pADI_PWRCTL->PWRKEY = PWRKEY_VALUE_KEY1;
     pADI_PWRCTL->PWRKEY = PWRKEY_VALUE_KEY2;
     pADI_PWRCTL->PWRMOD = iMode;
   }
  else
    return 0;
  return 1;
}

/**@}*/
