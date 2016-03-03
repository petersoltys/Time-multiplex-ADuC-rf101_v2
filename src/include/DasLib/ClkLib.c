/**
 *****************************************************************************
   @addtogroup clk 
   @{
   @file     ClkLib.c
   @brief    Set of functions to control and configure the system and peripheral clocks.
   @version  V0.1
   @author   PAD CSE group
   @date     January 2012 
              

All files for ADuCRF101 provided by ADI, including this file, are
provided  as is without warranty of any kind, either expressed or implied.
The user assumes any and all risk from the use of this code.
It is the responsibility of the person integrating this code into an application
to ensure that the resulting application performs as required and is safe.

**/

#include "ClkLib.h"
#include "ADuCRF101.h"


/**
	@brief int SysClkCfg(int iCd, int iClkScr, int iClkOut, int iXosc)
			========== Configures system clock.
	@param iCd :{CLKCON_CD_DIV1,CLKCON_CD_DIV2,CLKCON_CD_DIV4,CLKCON_CD_DIV8,CLKCON_CD_DIV16,CLKCON_CD_DIV32,
                     CLKCON_CD_DIV64,CLKCON_CD_DIV128}
		- CLKCON_CD_DIV1
		- CLKCON_CD_DIV2
		- CLKCON_CD_DIV4
		- CLKCON_CD_DIV8
		- CLKCON_CD_DIV16
		- CLKCON_CD_DIV32
		- CLKCON_CD_DIV64
		- CLKCON_CD_DIV128
	@param iClkScr :{CLKCON_CLKMUX_HFOSC,CLKCON_CLKMUX_LFXTAL,CLKCON_CLKMUX_LFOSC,CLKCON_CLKMUX_EXTCLK}
		- CLKCON_CLKMUX_HFOSC for 16MHz internal oscillator.
		- CLKCON_CLKMUX_LFXTAL for 32.768kHz external crystal.
		- CLKCON_CLKMUX_LFOSC for 32.768kHz internal oscillator.
		- CLKCON_CLKMUX_EXTCLK for external clock on P0.5.
	@param iClkOut :{CLKCON_CLKOUT_UCLKCG,CLKCON_CLKOUT_UCLK,CLKCON_CLKOUT_PCLK,CLKCON_CLKOUT_HFOSC,CLKCON_CLKOUT_LFOSC,CLKCON_CLKOUT_LFXTAL}
		- CLKCON_CLKOUT_UCLKCG
		- CLKCON_CLKOUT_UCLK
                - CLKCON_CLKOUT_PCLK
                - CLKCON_CLKOUT_HFOSC
                - CLKCON_CLKOUT_LFOSC
                - CLKCON_CLKOUT_LFXTAL
	@param iXosc :{XOSCCON_ENABLE_DIS,XOSCCON_ENABLE_EN}
		- XOSCCON_ENABLE_DIS disable external 32kHz crystal circuit.
		- XOSCCON_ENABLE_EN enable external 32kHx crystal circuit.
	@return 1.
	
**/

int SysClkCfg(int iCd, int iClkScr, int iClkOut, int iXosc)
{
  int	i1;
  XOSCCON_ENABLE_BBA = iXosc;
  i1 = iCd;
  i1 |= iClkScr;
  i1 |= iClkOut;
  pADI_CLKCTL->CLKCON = i1;
  return 1;	
}


/**
	@brief int PerClkIni(int iMode, int iClk)
			========== Configures all peripheral clocks for specified mode.
	@param iMode :{0,1}
		- 0 for flexi mode
		- 1 for active mode
	@param iClk :{CLK_DMA| CLK_FEE| CLK_SRAM| CLK_ADC| CLK_T2| CLK_SPI0| CLK_SPI1| CLK_COM| CLK_I2C| CLK_PWM| CLK_T0| CLK_T1}
		- CLK_DMA DMA clock Enable bit.
		- CLK_FEE Flash clocks Enable bit.
		- CLK_SRAM SRAM clocks Enable bit.
		- CLK_ADC ADC clocks Enable bit.
		- CLK_T2 T2 clocks enable bit.
		- CLK_SPI0 SPI0 clocks enable bit.
		- CLK_SPI1 SPI1 clocks enable bit.
		- CLK_COM UART clocks enable bit.
		- CLK_I2C I2C clocks enable bit.
		- CLK_PWM PWM clocks enable bit.
		- CLK_T0 T0 clocks enable bit.
		- CLK_T1 T1 clocks enable bit.
	@return 1.
	
**/
int PerClkIni(int iMode, int iClk)
{
  if (iMode)
    pADI_CLKCTL->CLKACT = ~iClk;
  else
    pADI_CLKCTL->CLKPD = iClk;      
  return 1;	
}


/**
	@brief int PerClkAct(int iEnable, int iClk)
			========== Enables/disables relevant clocks for active mode.
	@param iEnable :{0,1}
		- 0 disable
		- 1 enable
	@param iClk :{CLK_DMA| CLK_FEE| CLK_SRAM| CLK_ADC| CLK_T2| CLK_SPI0| CLK_SPI1| CLK_COM| CLK_I2C| CLK_PWM| CLK_T0| CLK_T1}
		- CLK_DMA DMA clock Enable bit.
		- CLK_FEE Flash clocks Enable bit.
		- CLK_SRAM SRAM clocks Enable bit.
		- CLK_ADC ADC clocks Enable bit.
		- CLK_T2 T2 clocks enable bit.
		- CLK_SPI0 SPI0 clocks enable bit.
		- CLK_SPI1 SPI1 clocks enable bit.
		- CLK_COM UART clocks enable bit.
		- CLK_I2C I2C clocks enable bit.
		- CLK_PWM PWM clocks enable bit.
		- CLK_T0 T0 clocks enable bit.
		- CLK_T1 T1 clocks enable bit.
	@return 1
**/
int PerClkAct(int iEnable, int iClk)
{
  if (iEnable)
    pADI_CLKCTL->CLKACT |= iClk; // enable clock(s)
  else
    pADI_CLKCTL->CLKACT &= (0xFFF - iClk); // disable clock(s)
  return 1;	

}


/**
	@brief int PerClkPd(int iEnable, int iClk)
			========== Enables/disables relevant clocks for sleep modes.
	@param iEnable :{0,1}
		- 0 disable
		- 1 enable
	@param iClk :{CLK_DMA| CLK_FEE| CLK_SRAM| CLK_ADC| CLK_T2| CLK_SPI0| CLK_SPI1| CLK_COM| CLK_I2C| CLK_PWM| CLK_T0| CLK_T1}
		- CLK_DMA DMA clock Enable bit.
		- CLK_FEE Flash clocks Enable bit.
		- CLK_SRAM SRAM clocks Enable bit.
		- CLK_ADC ADC clocks Enable bit.
		- CLK_T2 T2 clocks enable bit.
		- CLK_SPI0 SPI0 clocks enable bit.
		- CLK_SPI1 SPI1 clocks enable bit.
		- CLK_COM UART clocks enable bit.
		- CLK_I2C I2C clocks enable bit.
		- CLK_PWM PWM clocks enable bit.
		- CLK_T0 T0 clocks enable bit.
		- CLK_T1 T1 clocks enable bit.
	@return 1
**/
int PerClkPd(int iEnable, int iClk)
{      
  if (iEnable)
    pADI_CLKCTL->CLKPD |= iClk; // enable clock(s)
  else
    pADI_CLKCTL->CLKPD &= (0xFFF - iClk); // disable clock(s)
  return 1;	
}


/**
	@brief int ExtClkCfg(int iXosc)
			========== Enables/disables external 32kHz crystal circuit.
	@param iXosc :{XOSCCON_ENABLE_DIS,XOSCCON_ENABLE_EN}
		- XOSCCON_ENABLE_DIS disable external 32kHz crystal circuit
		- XOSCCON_ENABLE_EN enable external 32kHx crystal circuit
	@return 1.
	
**/

int ExtClkCfg(int iXosc)
{
  XOSCCON_ENABLE_BBA = iXosc;
  return 1;	
}

/**@}*/
