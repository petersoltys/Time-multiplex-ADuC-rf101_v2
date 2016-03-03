/**
 *****************************************************************************
   @addtogroup pwm 
   @{
   @file     PwmLib.c
   @brief    Set of PWM peripheral functions.
   @version  V0.1
   @author   PAD CSE group
   @date     September 2012 

All files for ADuCRF101 provided by ADI, including this file, are
provided  as is without warranty of any kind, either expressed or implied.
The user assumes any and all risk from the use of this code.
It is the responsibility of the person integrating this code into an application
to ensure that the resulting application performs as required and is safe.

**/

#include "ADuCRF101.h"


/**
      @brief int PwmInit(unsigned int iPWMCP, unsigned int iPWMIEN, unsigned int iSYNC, unsigned int iTRIP)
			========== Configure the PWM peripheral clock, interrupt, synchronisation and trip.
      
      @param iPWMCP :{PWMCON0_PWMCP_UCLKDIV2,PWMCON0_PWMCP_UCLKDIV4,PWMCON0_PWMCP_UCLKDIV8,PWMCON0_PWMCP_UCLKDIV16,
                      PWMCON0_PWMCP_UCLKDIV32,PWMCON0_PWMCP_UCLKDIV64 ,PWMCON0_PWMCP_UCLKDIV128,PWMCON0_PWMCP_UCLKDIV256 }
		- PWMCON0_PWMCP_UCLKDIV2 = UCLK/2
		- PWMCON0_PWMCP_UCLKDIV4 = UCLK/4
		- PWMCON0_PWMCP_UCLKDIV8 = UCLK/8
		- PWMCON0_PWMCP_UCLKDIV16 = UCLK/16
		- PWMCON0_PWMCP_UCLKDIV32  = UCLK/32
		- PWMCON0_PWMCP_UCLKDIV64  = UCLK/64
		- PWMCON0_PWMCP_UCLKDIV128 = UCLK/128
		- PWMCON0_PWMCP_UCLKDIV256  = UCLK/256 

      @param iPWMIEN :{PWMCON0_PWMIEN_DIS,PWMCON0_PWMIEN_EN}
		- PWMCON0_PWMIEN_DIS for PWM Interupt disable
		- PWMCON0_PWMIEN_EN for PWM Interupt enable

      @param iSYNC :{PWMCON0_SYNC_DIS,PWMCON0_SYNC_EN}
		- PWMCON0_SYNC_DIS to disable PWM Synchronization
		- PWMCON0_SYNC_EN to enable all PWM counters to reset on the next clock 
           edge after the detection of a falling edge on the SYNC pin.

      @param iTRIP :{PWMCON1_TRIPEN_DIS,PWMCON1_TRIPEN_EN}
		- PWMCON1_TRIPEN_DIS to disable PWM trip functionality
		- PWMCON1_TRIPEN_EN to enable PWM trip function

      @return 1 
      @note 
        This function disables HBridge mode and other configurations. 
        It should be called before any other PWM functions.
**/                                             
int PwmInit(unsigned int iPWMCP, unsigned int iPWMIEN, unsigned int iSYNC, unsigned int iTRIP)
{
  unsigned int i1;
  i1= iPWMCP;
  i1|= iPWMIEN;
  i1|= iSYNC;
  pADI_PWM->PWMCON0 = i1;
  pADI_PWM->PWMCON1 = iTRIP;

  return 1;
}




/**

      @brief int PwmTime(int iPair, unsigned int uiFreq, unsigned int uiPWMH_High, unsigned int uiPWML_High)
			========== Configure period and duty cycle of a pair.

      @param iPair :{0,1,2,3}
		- 0 for PWM Pair0
		- 1 for PWM Pair1
		- 2 for PWM Pair2
		- 3 for PWM Pair3

      @param uiFreq :{0-0xFFFF}                    \n
	            The PWM Period is calculated as follows:  \n
	            tuclk x (PWMxLEN+1) * Nprescale    
                    where tuclk is 16MHz/CLKCON[2:0], Nprescale is PWMCON0[8:6]

      @param uiPWMH_High :{0-0xFFFF}
	           - Pass values between 0-0xFFFF for duty cycle of high and low side	
                   - High Side High period must be greater or equal to Low side High period (iPWM0High>=iPWM1High)
		   - High Side high period must be less than uiFreq
		   - uiPWMH_High/uiFreq gives the duty cycle for ratio for PWMH high      \n
		     For example, uiFreq = 0x50 and uiPWMH_High = 0x30; then duty cycle is 60:40 for high:low on PWMH pin

      @param uiPWML_High :{0-0xFFFF}
		   - Pass values between 0-0xFFFF for duty cycle of high and low side	
                   - Low output High period must be less or equal to High side High period (iPWM0High>=iPWM1High)
		   - Low Side high period must be less than uiFreq
		   - uiPWML_High/uiFreq gives the duty cycle for ratio for PWML high    \n
		     For example, uiFreq = 0x50 and uiPWML_High = 0x10; then duty cycle is 20:80 for high:low on PWML pin

      @return 0	  Failure: uiPWMH_High < uiPWML_High
      @return 1	  Success
      @return 2	  Failure: uiPWML_High > pADI_PWM->PWM0COM1 - this will result in lower than expected duty cycle on PWML output
**/
int PwmTime(int iPair, unsigned int uiFreq, unsigned int uiPWMH_High,unsigned int uiPWML_High)
{
  if  (uiPWMH_High < uiPWML_High)
    return 0x0; // Error: PWM0 High period must be >= PWM1 High period; PWM2 High period must be >= PWM3 High period; PWM4 High period must be >= PWM5 High period
  
  switch (iPair)
  {
  case 0:
    pADI_PWM->PWM0LEN = uiFreq;
	pADI_PWM->PWM0COM0 = pADI_PWM->PWM0LEN;
	pADI_PWM->PWM0COM1 = uiPWMH_High;
	if ( uiPWML_High < pADI_PWM->PWM0COM1)
		pADI_PWM->PWM0COM2 = uiPWML_High;
	else			   // PWML output is dictated by PWMH. PWML high period will be PWMLEN-PWMH_HIGH
		return 2;    
    break;
  case 1:
    pADI_PWM->PWM1LEN = uiFreq;
	pADI_PWM->PWM1COM0 = pADI_PWM->PWM1LEN;
	pADI_PWM->PWM1COM1 = uiPWMH_High;
	if ( uiPWML_High < pADI_PWM->PWM1COM1)
		pADI_PWM->PWM1COM2 = uiPWML_High;
	else			   // PWML output is dictated by PWMH. PWML high period will be PWMLEN-PWMH_HIGH
		return 2;    
    break;
  case 2:
   pADI_PWM->PWM2LEN = uiFreq;
	pADI_PWM->PWM2COM0 = pADI_PWM->PWM2LEN;
	pADI_PWM->PWM2COM1 = uiPWMH_High;
	if ( uiPWML_High < pADI_PWM->PWM2COM1)
		pADI_PWM->PWM2COM2 = uiPWML_High;
	else			   // PWML output is dictated by PWMH. PWML high period will be PWMLEN-PWMH_HIGH
		return 2;    
    break;
  case 3:
   pADI_PWM->PWM3LEN = uiFreq;
	pADI_PWM->PWM3COM0 = pADI_PWM->PWM3LEN;
	pADI_PWM->PWM3COM1 = uiPWMH_High;
	if ( uiPWML_High < pADI_PWM->PWM3COM1)
		pADI_PWM->PWM3COM2 = uiPWML_High;
	else			   // PWML output is dictated by PWMH. PWML high period will be PWMLEN-PWMH_HIGH
		return 2;    
    break;
  default:
    break;
  }
  return 1;
}

/**
      @brief int PwmClrInt(unsigned int iSource)
			========== Clear PWMs interrupt flags.

      @param iSource :{PWMCLRI_TRIP|PWMCLRI_IRQPWM3|PWMCLRI_IRQPWM2|PWMCLRI_IRQPWM1|PWMCLRI_IRQPWM0}
		- PWMCLRI_x for : clear the latched PWM interrupt. 

      @return 1  
**/
int PwmClrInt(unsigned int iSource)
{
  pADI_PWM->PWMCLRI |= iSource;
  return 1;
}
/**
      @brief int PwmLoad(int iLoad)
			========== Controls how PWM compare registers are loaded.

      @param iLoad :{PWMCON0_LCOMP_DIS,PWMCON0_LCOMP_EN}
		- PWMCON0_LCOMP_DIS to use previous compare register values
		- PWMCON0_LCOMP_EN to load compare registers on the next PWM timer transition (low to high edge)

      @return 1  
**/
int PwmLoad(int iLoad)
{
   unsigned int i1;
  
  i1= (pADI_PWM->PWMCON0 & 0xFFF7); // mask off bit 3
  i1 |= iLoad; 
  pADI_PWM->PWMCON0 = i1; 
  return 1;
}


/**
      @brief int PwmGo(unsigned int iPWMEN, unsigned int iHMODE)
			========== Starts/Stops the PWM in standard or H-Bridge mode.

      @param iPWMEN :{PWMCON0_PWMEN_DIS,PWMCON0_PWMEN_EN}
		- PWMCON0_PWMEN_DIS to disable the PWM peripheral.
		- PWMCON0_PWMEN_EN to enable/start the PWM peripheral.
      @param iHMODE :{PWMCON0_HMODE_DIS,PWMCON0_HMODE_EN}
		- PWMCON0_HMODE_DIS to operate in standard mode.
		- PWMCON0_HMODE_EN to operate in H-Bridge mode. 

      @return 1  

**/
int PwmGo(unsigned int iPWMEN, unsigned int iHMODE)
{
  if (iHMODE == PWMCON0_HMODE_DIS)
    PWMCON0_HMODE_BBA = 0;			 // Standard mode
   else
     PWMCON0_HMODE_BBA = 1;			 // H-Bridge mode

  if (iPWMEN == PWMCON0_PWMEN_DIS)
    PWMCON0_PWMEN_BBA = 0;			 // Disable PWM 
  else
    PWMCON0_PWMEN_BBA = 1;			 // Enable PWM 
  return 1;
}

/**
      @brief int PwmInvert(int iInv1, int iInv3, int iInv5, int iInv7)
			========== Selects PWM outputs for inversion.

      @param iInv1 :{PWMCON0_PWM1INV_DIS, PWMCON0_PWM1INV_EN}
		- PWMCON0_PWM1INV_DIS to disable PWM1 output inversion.
		- PWMCON0_PWM1INV_EN to enable PWM1 output inversion.
      @param iInv3 :{PWMCON0_PWM3INV_DIS,PWMCON0_PWM3INV_EN}
		- PWMCON0_PWM3INV_DIS to disable PWM3 output inversion.
		- PWMCON0_PWM3INV_EN to enable PWM3 output inversion.
      @param iInv5 :{PWMCON0_PWM5INV_DIS,PWMCON0_PWM5INV_EN}
		- PWMCON0_PWM5INV_DIS to disable PWM5 output inversion.
		- PWMCON0_PWM5INV_EN to enable PWM5 output inversion.	
      @param iInv7 :{PWMCON0_PWM7INV_DIS,PWMCON0_PWM7INV_EN}
		- PWMCON0_PWM7INV_DIS to disable PWM7 output inversion.
		- PWMCON0_PWM7INV_EN to enable PWM7 output inversion.      
      @return 1  

**/
int PwmInvert(int iInv1,int iInv3,int iInv5, int iInv7)
{
  unsigned int i1;
  
  i1= (pADI_PWM->PWMCON0 & 0x87FF); // mask off bits  11, 12, 13 and 14
  i1 |= (iInv1 +iInv3+iInv5+iInv7); 
  pADI_PWM->PWMCON0 = i1; 
  return 1;
}
/**
      @brief int PwmHBCfg(unsigned int iENA, unsigned int iPOINV, unsigned int iHOFF, unsigned int iDIR)
			========== HBridge configure PWMs.

      @param iENA :{PWMCON0_ENA_DIS,PWMCON0_ENA_EN}
		- PWMCON0_ENA_DIS for Use the values previously stored in the internal compare registers.
		- PWMCON0_ENA_EN. 

      @param iPOINV :{PWMCON0_POINV_DIS,PWMCON0_POINV_EN}
		- PWMCON0_POINV_DIS for PWM outputs normal
		- PWMCON0_POINV_EN for invert all PWM outputs

      @param iHOFF :{PWMCON0_HOFF_DIS,PWMCON0_HOFF_EN}
		- PWMCON0_HOFF_DIS for PWM outputs as normal.
		- PWMCON0_HOFF_EN for Forces PWM0 and PWM2 outputs high and PWM1 and PWM3 low (default). 

      @param iDIR :{PWMCON0_DIR_DIS,PWMCON0_DIR_EN}
		- PWMCON0_DIR_DIS for Enable PWM2 and PWM3 as the output signals while PWM0 and PWM1 are held low.
		- PWMCON0_DIR_EN for Enables PWM0 and PWM1 as the output signals while PWM2 and PWM3 are held low.

      @return 1    
**/          
int PwmHBCfg(unsigned int iENA, unsigned int iPOINV, unsigned int iHOFF, unsigned int iDIR)
{
  unsigned int i1;

  i1= (pADI_PWM->PWMCON0 & 0xFDCB);	   // Mask bits 9,5,4,2
  i1|=iENA;
  i1|=iPOINV;
  i1|=iHOFF;
  i1|=iDIR;
  pADI_PWM->PWMCON0 |= i1; 
  return 1;
}

/**@}*/
