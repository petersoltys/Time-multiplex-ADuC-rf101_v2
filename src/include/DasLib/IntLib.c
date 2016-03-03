/**
 *****************************************************************************
   @addtogroup int 
   @{
   @file     IntLib.c
   @brief    Set of External interrupt functions.
   @version  V0.1
   @author   PAD CSE group
   @date     January 2012 

All files for ADuCRF101 provided by ADI, including this file, are
provided  as is without warranty of any kind, either expressed or implied.
The user assumes any and all risk from the use of this code.
It is the responsibility of the person integrating this code into an application
to ensure that the resulting application performs as required and is safe.

**/

#include "IntLib.h"
#include "ADuCRF101.h"


/**
	@brief int EiClr(int iEiNr)
			========== Clear external interrupt flag

	@param iEiNr :{EICLR_IRQ0_CLR, EICLR_IRQ1_CLR, EICLR_IRQ2_CLR, EICLR_IRQ3_CLR, 
                       EICLR_IRQ4_CLR, EICLR_IRQ5_CLR, EICLR_IRQ6_CLR, EICLR_IRQ7_CLR, EICLR_IRQ8_CLR}
		- EICLR_IRQ0_CLR, clears external interrupt 0 
		- EICLR_IRQ1_CLR, clears external interrupt 1 
		- EICLR_IRQ2_CLR, clears external interrupt 2 
		- EICLR_IRQ3_CLR, clears external interrupt 3 
		- EICLR_IRQ4_CLR, clears external interrupt 4 
		- EICLR_IRQ5_CLR, clears external interrupt 5 
		- EICLR_IRQ6_CLR, clears external interrupt 6 
		- EICLR_IRQ7_CLR, clears external interrupt 7 
		- EICLR_IRQ8_CLR, clears external interrupt 8 
	@return 1
	@warning 
                For level triggered interrupts, the condition that generated the interrupt must be removed.
		
**/
int EiClr(int iEiNr)
{
  pADI_INTERRUPT->EICLR = 0x1UL << iEiNr;
  return 1;
}

/**
	@brief int EiCfg(int iEiNr, int iEnable, int iMode)
			========== Configures external interrupt
	@param iEiNr :{EXTINT0, EXTINT1, EXTINT2, EXTINT3, EXTINT4, EXTINT5, EXTINT6, EXTINT7, EXTINT8}
		- EXTINT0, External Interrupt 0 
		- EXTINT1, External Interrupt 1 
		- EXTINT2, External Interrupt 2 
		- EXTINT3, External Interrupt 3 
		- EXTINT4, External Interrupt 4 
		- EXTINT5, External Interrupt 5 
		- EXTINT6, External Interrupt 6 
		- EXTINT7, External Interrupt 7 
		- EXTINT8, External Interrupt 8 
        @param iEnable :{INT_DIS,INT_EN}
                - INT_DIS for disabled.
                - INT_EN for enabled.
        @param iMode :{INT_RISE, INT_FALL, INT_EDGES, INT_HIGH, INT_LOW}
                - INT_RISE Rising edge
                - INT_FALL Falling edge
                - INT_EDGES Rising or falling edge
                - INT_HIGH High level
                - INT_LOW Low level
	@return 1
	@warning
		the NVIC also needs to be configured
                external interrupts are available regardless of the GPIO configuration
                only ext int 0, 1 and 8 are available in SHUTDOWN mode
**/

int EiCfg(int iEiNr, int iEnable, int iMode)
{
  volatile unsigned long *pEIxCFG;
  unsigned long EIxCFG_A, EI0CFG_A, ulOffset, ulContent, ulMask; 
  EI0CFG_A = (unsigned long)& pADI_INTERRUPT->EI0CFG;

  EIxCFG_A = EI0CFG_A + ((iEiNr/4)*4);    // determine correct EIxCFG register
  pEIxCFG = (volatile unsigned long *)EIxCFG_A;
  ulOffset = (iEiNr % 4)*4;    // determine correct offset in register  
    
  if (iEnable == INT_DIS) 
  {     
    ulMask = 0xFUL << ulOffset;
    *pEIxCFG = *pEIxCFG & ~ulMask ; // clear the appropriate bit in the correct EIxCFG register 
  }
  else 
  {
    pADI_INTERRUPT->EICLR = 0x1UL << iEiNr; // clears flag first
    ulContent = (0x8UL | iMode) << ulOffset;  // calculate the value to write
    ulMask = 0xFUL << ulOffset;
    *pEIxCFG = (*pEIxCFG & ~ulMask) | ulContent; // set the appropriate bits in the correct EIxCFG register  
  }
  return 1;
}


/**
	@brief int NmiClr()
			========== Clears NMI interrupt flag
	@return 1
	@warning
		
**/
int NmiClr()
{
  pADI_INTERRUPT->NMICLR = 0x1;
  return 1;
}

/**@}*/
