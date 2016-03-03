/**
 *****************************************************************************
   @file     AdcLib.h
   @brief    Set of ADC peripheral functions.
   @version  V0.2
   @author   PAD CSE group
   @date     August 2012 
        
   @par Revision History:
   - V0.1, January 2012: initial version. 
   - V0.2, August 2012: gain_correction() function corrected.
                        AdcGn() and AdcOf() functions removed.

All files for ADuCRF101 provided by ADI, including this file, are
provided  as is without warranty of any kind, either expressed or implied.
The user assumes any and all risk from the use of this code.
It is the responsibility of the person integrating this code into an application
to ensure that the resulting application performs as required and is safe.

*****************************************************************************/

extern int AdcPd(int iPd);
extern int AdcInit(int iRefBuf, int iInt, int iDMA);
extern int AdcCnv(int iMode, int iStart);
extern int AdcRd(int iCal);
extern int AdcSta(void);
extern int AdcCfg(int iChan, int iRef, int iClk, int iAcq);


