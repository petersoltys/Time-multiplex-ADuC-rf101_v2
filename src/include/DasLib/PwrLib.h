/**
 *****************************************************************************
   @file     PwrLib.h
   @brief    Set of power optimisation functions.
   @version  V0.1
   @author   PAD CSE group
   @date     January 2012 

All files for ADuCRF101 provided by ADI, including this file, are
provided  as is without warranty of any kind, either expressed or implied.
The user assumes any and all risk from the use of this code.
It is the responsibility of the person integrating this code into an application
to ensure that the resulting application performs as required and is safe.

*****************************************************************************/

extern int SramCfg(int iEnable);
extern int PwrSta(void);
extern int PsmCfg(int iEnable);
extern int PwrCfg(int iMode, int iExit);

