/**
 *****************************************************************************
   @addtogroup adc 
   @{
   @file     AdcLib.c
   @brief    Set of ADC peripheral functions.
   @version  V0.3
   @author   PAD CSE group
   @date     April 2013 
        
   @par Revision History:
   - V0.1, January 2012: initial version. 
   - V0.2, August 2012: gain_correction() function corrected.
                        AdcGn() and AdcOf() functions removed.
                        12-bit results. 
   - V0.3, April 2013: changed comments in Adcinit()					

All files for ADuCRF101 provided by ADI, including this file, are
provided  as is without warranty of any kind, either expressed or implied.
The user assumes any and all risk from the use of this code.
It is the responsibility of the person integrating this code into an application
to ensure that the resulting application performs as required and is safe.

**/

#include "AdcLib.h"
#include "ADuCRF101.h"


int gain_correction(int);
int offset_correction(void);

/**
	@brief int AdcPd(int iPd)
			========== Enables/disable the ADC.
	@param iPd :{ADCCON_ENABLE_EN, ADCCON_ENABLE_DIS}
		- ADCCON_ENABLE_EN to enable the ADC
		- ADCCON_ENABLE_DIS to disable the ADC

        @return 1.
	@warning This function also turns on/off the reference.
**/
int AdcPd(int iPd)
{
  if (iPd == ADCCON_ENABLE_DIS)
  {
    pADI_ADC0->ADCCFG = 0; 
    pADI_ADC0->ADCCON = 0x90; // also turn off the reference
  }
  else
    pADI_ADC0->ADCCON &= 0xEF; // turns on ADC and reference
  return(1);
}


/**
	@brief int AdcInit(int iRefBuf, int iInt, int iDMA)
			========== Configures the ADC modes of operation.
	@param iRefBuf :{ADCCON_REFBUF_EN,ADCCON_REFBUF_DIS}
		- ADCCON_REFBUF_EN for Turn on the reference buffer.
		- ADCCON_REFBUF_DIS for Turn off the reference buffer.
	@param iInt :{ADCCON_IEN_DIS, ADCCON_IEN_EN}
		- ADCCON_IEN_DIS to disable the ADC interrupt.
		- ADCCON_IEN_EN to Enable the ADC interrupt.
        @param iDMA :{ADCCON_DMA_DIS, ADCCON_DMA_EN}
		- ADCCON_DMA_DIS to disable the ADC DMA.
		- ADCCON_DMA_EN to Enable the ADC DMA.
	@return 1.
**/
int AdcInit(int iRefBuf, int iInt, int iDMA)
{
  int	i1;
  i1 = iRefBuf;
  i1 |= iInt;
  i1 |= iDMA;
  pADI_ADC0->ADCCON = i1;
  return 1;
}


/**
	@brief int AdcCnv(int iMode, int iStart)
			========== start or stop the ADC in its conversion mode.
	@param iMode :{ADCCON_MOD_SOFT,ADCCON_MOD_CONT,ADCCON_MOD_T0OVF,ADCCON_MOD_T1OVF,ADCCON_MOD_GPIO}
		- ADCCON_MOD_SOFT for single conversion. Start using AdcGo().
		- ADCCON_MOD_CONT for continuous convert mode.
		- ADCCON_MOD_T0OVF for Timer0 overflow.
		- ADCCON_MOD_T1OVF for Timer overflow.
		- ADCCON_MOD_GPIO for GPIO, P1.3.
        @param iStart :{ADCCON_START_DIS, ADCCON_START_EN}
		- ADCCON_START_DIS to clear start bit .
		- ADCCON_START_EN for start ADC conversion.
	@return 1.
**/
int AdcCnv(int iMode, int iStart)
{
  int	i1; 
  i1 = pADI_ADC0->ADCCON & 0xF0; // keep high nibble
  if (iMode == ADCCON_MOD_SOFT) // software convert start
  {
    pADI_ADC0->ADCCON = i1|ADCCON_MOD_SOFT|iStart; // configures mode 0 and start of conversion
  }
  else // other conversion mode where ADCCON_START bit has no effect
  {
    if (iStart == ADCCON_START_EN) // user wants to start converting
      pADI_ADC0->ADCCON = i1|iMode;
    else // want to stop converting -> need to change to iMode = 0
      pADI_ADC0->ADCCON = i1;
  }
  return(1);
}

/**
	@brief int AdcRd(int iCal)
			========== Reads ADC result.
	@param iCal :{AD_Cal}
		- 0 to return the raw ADC result
		- 1 to apply offset and gain correction before returning the ADC result
	@returns raw ADC result or ADC result after error correction  
**/

int AdcRd(int iCal)
{ 
  int i1;
  if (iCal == 0)
  {
  return( pADI_ADC0->ADCDAT>>2);
  } 
  else
    i1 = gain_correction(offset_correction());
  return (i1>>2);
}


/**
	@brief int AdcSta(void)
			========== Reads ADC status.
	@return ADCSTA_READY bit
		0 when a conversion is not complete
                1 when a conversion is complete      
		
**/
int AdcSta(void)
{
  return( pADI_ADC0->ADCSTA&ADCSTA_READY);
}


/**
	@brief int AdcCfg(int iChan, int iRef, int iClk, int iAcq )
			========== Configures ADC channel, conversion speed and reference.

        @param iChan :{ADCCFG_CHSEL_ADC0,ADCCFG_CHSEL_ADC1,ADCCFG_CHSEL_ADC2,ADCCFG_CHSEL_ADC3,ADCCFG_CHSEL_ADC4,ADCCFG_CHSEL_ADC5,ADCCFG_CHSEL_DIFF0,ADCCFG_CHSEL_DIFF1,ADCCFG_CHSEL_DIFF2,ADCCFG_CHSEL_TEMP,ADCCFG_CHSEL_VBATDIV4,ADCCFG_CHSEL_LVDDDIV2,ADCCFG_CHSEL_VREF,ADCCFG_CHSEL_AGND}
                - ADCCFG_CHSEL_ADC0 selects single ended ADC0 input.
                - ADCCFG_CHSEL_ADC1 selects single ended ADC1 input. 
                - ADCCFG_CHSEL_ADC2 selects single ended ADC2 input.
                - ADCCFG_CHSEL_ADC3 selects single ended ADC3 input.
                - ADCCFG_CHSEL_ADC4 selects single ended ADC4 input.
                - ADCCFG_CHSEL_ADC5 selects single ended ADC5 input.
                - ADCCFG_CHSEL_DIFF0 selects differential ADC0 - ADC1 inputs.
                - ADCCFG_CHSEL_DIFF1 selects differential ADC2 - ADC3 inputs.
                - ADCCFG_CHSEL_DIFF2 selects differential ADC4 - ADC5 inputs.
                - ADCCFG_CHSEL_TEMP selects internal temperature sensor.
                - ADCCFG_CHSEL_VBATDIV4 selects internal supply divided by 4.
                - ADCCFG_CHSEL_LVDDDIV2 selects internal 1.8V regulator output (LVDD1) divided by 2.
                - ADCCFG_CHSEL_VREF selects internal ADC reference input for gain calibration.
                - ADCCFG_CHSEL_AGND selects internal ADC ground input for offset calibration.

	@param iRef :{ADCCFG_REF_INTERNAL125V,ADCCFG_REF_LVDD}
		- ADCCFG_REF_INTERNAL125V for select the internal 1.25 V reference as the ADC reference (default).
		- ADCCFG_REF_LVDD for LVDD as the ADC reference.

	@param iClk :{ADCCFG_CLK_FCOREDIV4,ADCCFG_CLK_FCOREDIV8,ADCCFG_CLK_FCOREDIV16,ADCCFG_CLK_FCOREDIV232)}
		- ADCCFG_CLK_FCOREDIV4 for fcore / 4 (default).
		- ADCCFG_CLK_FCOREDIV8 for fcore / 8.
		- ADCCFG_CLK_FCOREDIV16 for fcore / 16.
		- ADCCFG_CLK_FCOREDIV32 for fcore / 32.
		
	@param iAcq :{ADCCFG_ACQ_2,ADCCFG_ACQ_4,ADCCFG_ACQ_8,ADCCFG_ACQ_16}
		- 0 or ADCCFG_ACQ_2 for 2 Acquisition clocks.
		- 1 or ADCCFG_ACQ_4 for 4 Acquisition clocks.
		- 2 or ADCCFG_ACQ_8 for 8 Acquisition clocks (default).
		- 3 or ADCCFG_ACQ_16 for 16 Acquisition clocks.
	
	@return 1.
**/

int AdcCfg(int iChan, int iRef, int iClk, int iAcq)
{
  int	i1;
  i1 = iChan;
  i1 |= iRef;
  i1 |= iClk;
  i1 |= iAcq;
  pADI_ADC0->ADCCFG = i1;
  return (1);
}

/**
	@brief int gain_correction(int)
			========== correct gain.

	@return ADC_reading_postGN. 
**/ 
int gain_correction(int adc_result)
{
  unsigned long adc_gain, adc_result_temp, gain_acc, ADC_reading_postGN;
  unsigned  char gain_count;
  unsigned int bit_pos;   
  adc_gain = pADI_ADC0->ADCGN;         //read adc gain trim 
  adc_result_temp = adc_result;    // temp adc result, this is the value to be shifted

  gain_acc=0;        // correction coefficient that is accumulated then either added or subtracted from the adc output
            
  gain_count = 14;               // setup counter
  bit_pos=4096;   // setup value to mask bits in gain trim value

  while (gain_count > 0)
  {
    adc_result_temp = adc_result_temp >> 1;          //shift result to divide by 2
    if((adc_gain & bit_pos)== bit_pos)                          // if unmasked bit is set in trim word then accumulate else do nothing
    {
      gain_acc = gain_acc + adc_result_temp;    
    }
    else
    {
      gain_acc=gain_acc;
    }
    bit_pos = bit_pos >> 1;
    gain_count = gain_count - 1;
  }
    
  if((adc_gain & 8192) == 8192)                     // if sign bit (bit 13) is set then subtract correction coeff, else add
  {
    ADC_reading_postGN = adc_result - gain_acc;
  }
  else
  {
    ADC_reading_postGN = adc_result + gain_acc;
  }
  return (ADC_reading_postGN);      
}

/**
	@brief int offset_correction(void)
			========== Correct offset.

	@return  -  ADC_reading_postOF. 
**/ 
int offset_correction(void)
{
  unsigned long adc_offset, adc_result, ADC_reading_postOF;
  
  adc_offset = pADI_ADC0->ADCOF; //read adc offset trim
  adc_result = pADI_ADC0->ADCDAT;

  if((adc_offset & 8192) == 8192)  // if sign bit is set then subtract correction coeff, else add
  {    
    ADC_reading_postOF = adc_result - (adc_offset - 8192); // remove sign bit before subtracting
  }
  else
  {
    ADC_reading_postOF = adc_result + adc_offset;
  }
  return (ADC_reading_postOF);      
}  

/**@}*/
