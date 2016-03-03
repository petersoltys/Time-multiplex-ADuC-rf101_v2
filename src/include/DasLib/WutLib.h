/**
 *****************************************************************************
   @file     WutLib.h
   @brief    Set of wake up Timer peripheral functions.
   @version  V0.1
   @author   PAD CSE group
   @date     January 2012 

All files for ADuCRF101 provided by ADI, including this file, are
provided  as is without warranty of any kind, either expressed or implied.
The user assumes any and all risk from the use of this code.
It is the responsibility of the person integrating this code into an application
to ensure that the resulting application performs as required and is safe.

*****************************************************************************/

extern int WutCfg(int iMode, int iWake, int iPre, int iClkSrc);
extern int WutCfgInt(int iSource, int iEnable);
extern int WutClrInt(int iSource);
extern int WutGo(int iEnable);
extern int WutInc(int iInc);
extern unsigned long WutLdRd(int iField);
extern int WutLdWr(int iField, unsigned long lTld);
extern int WutSta(void);
extern long WutVal(void);

