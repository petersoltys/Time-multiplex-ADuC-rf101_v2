/** 
@file     system_ADUCRF101.c
@brief    CMSIS Cortex-M3 Device Peripheral Access Layer Implementation File
          for the ADuCRF101
@version  v1.0
@author   PAD CSE group
@date     January 14th 2013 
 
@section disclaimer Disclaimer
THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES INC. ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT, ARE
DISCLAIMED. IN NO EVENT SHALL ANALOG DEVICES INC. BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

YOU ASSUME ANY AND ALL RISK FROM THE USE OF THIS CODE OR SUPPORT FILE.

IT IS THE RESPONSIBILITY OF THE PERSON INTEGRATING THIS CODE INTO AN APPLICATION
TO ENSURE THAT THE RESULTING APPLICATION PERFORMS AS REQUIRED AND IS SAFE.

**/

#include <stdint.h>
#include "ADUCRF101.h"


/*----------------------------------------------------------------------------
  DEFINES
 *---------------------------------------------------------------------------*/

/* Extract the Clock Divider */
#define __CCLK_DIV (1 << (pADI_CLKCTL->CLKCON & CLKCON_CD_MSK) )

/* define the clock multiplexer input frequencies */
#define __HFOSC    16000000
#define __LFXTAL      32768
#define __LFOSC       32768

/*----------------------------------------------------------------------------
  Internal Clock Variables
 *---------------------------------------------------------------------------*/
static uint32_t uClk = 0;    /* Undivided System Clock Frequency (UCLK)   */
static uint32_t uClkDiv = 0; /* Divided System Clock Frequency (UCLK_DIV) */

/* Frequency of the external clock source connected to P0.5 */
static uint32_t SystemExtClock = 0;

/*----------------------------------------------------------------------------
  Clock functions
 *---------------------------------------------------------------------------*/
void SystemCoreClockUpdate (void)            /* Get Core Clock Frequency     */
{
/* pre-processor verification that clock mux mask and allowed values agree   */
#if ((CLKCON_CLKMUX_HFOSC   \
    | CLKCON_CLKMUX_LFXTAL  \
    | CLKCON_CLKMUX_LFOSC   \
    | CLKCON_CLKMUX_EXTP05) \
    == CLKCON_CLKMUX_MSK)

    /* update the system core clock according the the current clock mux setting */
    switch (pADI_CLKCTL->CLKCON & CLKCON_CLKMUX_MSK ) {

        case CLKCON_CLKMUX_HFOSC:
            uClk = __HFOSC;
            break;
        case CLKCON_CLKMUX_LFXTAL:
            uClk = __LFXTAL;
            break;
        case CLKCON_CLKMUX_LFOSC:
            uClk = __LFOSC;
            break;
        case CLKCON_CLKMUX_ECLKIN:
            uClk = SystemExtClock;
            break;
        /* no need to catch default case due to pre-processor test */
    }

    /* update the divided system clock */
    uClkDiv = uClk / __CCLK_DIV;

#else
#error "Clock mux mask and allowed value mismatch!"
#endif
}

/**
 * Initialize the system
 *
 * @param  none
 * @return none
 *
 * @brief  Setup the microcontroller system.
 *         Initialize the System and update the SystemFrequency variable.
 */
void SystemInit (void)
{
   /* reset CLKCON register */
   pADI_CLKCTL->CLKCON = CLKCON_RVAL;

   /* reset XOSCCON register */
   pADI_CLKCTL->XOSCCON = XOSCCON_RVAL;

   /* compute internal clocks */
   SystemCoreClockUpdate();
}

/**
 * @brief  Sets the system external clock frequency
 *
 * @param  ExtClkFreq   External clock frequency in Hz
 * @return none
 *
 * Sets the clock frequency of the source connected to P0.5 clock input source
 */
void SetSystemExtClkFreq (uint32_t ExtClkFreq)
{
    SystemExtClock = ExtClkFreq;
}

/**
 * @brief  Gets the system external clock frequency
 *
 * @return External Clock frequency
 *
 * Gets the clock frequency of the source connected to P0.5 clock input source
 */
uint32_t GetSystemExtClkFreq (void)
{
    return  SystemExtClock;
}


/* set the system clock dividers */
void SystemSetClockDivider(uint16_t div)
{
    /* critical region */
    __disable_irq();

    /* read-modify-write without any interrupts */

    pADI_CLKCTL->CLKCON &= ~(CLKCON_CD_MSK);   /* keep everything else */
    pADI_CLKCTL->CLKCON |= div;  /* set new value */

    /* end critical region */
    __enable_irq();

    /* refresh internal clock variables */
    SystemCoreClockUpdate();
}


uint32_t SystemGetClockFrequency(void)
{
    return uClkDiv;
}




