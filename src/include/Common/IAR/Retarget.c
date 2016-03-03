/*
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

    Module       : Retarget.c
    Description  : uart interface
    Date         : December 2012
    Version      : v2.00
    Changelog    : v1.00 Initial
                v2.00 use of UrtLib functions
*/
#include <include.h>
#include "UrtLib.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define CR     0x0D



/*************************************************************************/
/* size_t __read(int handle,unsigned char *buf,size_t bufSize)           */
/*              Write data to a stream                                   */
/*        Needed for retargetting the IAR DLIB library for the ADUCRF101  */
/*************************************************************************/
size_t __read(int handle,unsigned char *buf,size_t bufSize)
{
   size_t i;
   for (i=0x0; i<bufSize;i++)
      {
      // Wait for character available
      while(!(COMLSR_DR==(pADI_UART->COMLSR & COMLSR_DR)));
      buf[i] = pADI_UART->COMRX;
      }
   return i;
}

/*************************************************************************/
/* __write(int handle,const unsigned char *buf,size_t bufSize)           */
/*              Read data from a stream                                  */
/*        Needed for retargetting the IAR DLIB library for the ADUCRF101  */
/*************************************************************************/
size_t __write(int handle,const unsigned char *buf,size_t bufSize)
{
   size_t i;
   for (i=0x0; i<bufSize;i++)
      {
      if (buf[i] == '\n')
         {
         while(!(COMLSR_THRE==(UrtLinSta(0) & COMLSR_THRE)));
         UrtTx(0, 0x0D);
         }
      while(!(COMLSR_THRE==(UrtLinSta(0) & COMLSR_THRE)));
      UrtTx(0, buf[i]);
      }
   return i;
}











