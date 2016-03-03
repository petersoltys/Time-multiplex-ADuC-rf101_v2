/**
 *****************************************************************************
   @file     IntLib.h
   @brief    Set of external interrupt functions.
   @version  V0.1
   @author   PAD CSE group
   @date     January 2012 

All files for ADuCRF101 provided by ADI, including this file, are
provided  as is without warranty of any kind, either expressed or implied.
The user assumes any and all risk from the use of this code.
It is the responsibility of the person integrating this code into an application
to ensure that the resulting application performs as required and is safe.

*****************************************************************************/

extern int EiCfg(int iEiNr, int iEnable, int iMode);
extern int EiClr(int iEiNr);
extern int NmiClr(void);


