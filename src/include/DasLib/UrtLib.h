/**
 *****************************************************************************
   @file     UrtLib.h
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

*****************************************************************************/

extern int UrtLinCfg(int iChan, int iBaud, int iBits, int iFormat);
extern int UrtLinSta(int iChan);

extern int UrtTx(int iChan, int iTx);
extern int UrtRx(int iChan);

extern int UrtIntCfg(int iChan, int iIrq);
extern int UrtIntSta(int iChan);

extern int UrtModCfg(int iChan, int iMcr, int iWr);
extern int UrtModSta(int iChan);

extern int UrtDma(int iChan, int iDmaSel);
extern int UrtBrk(int iChan, int iBrk);

