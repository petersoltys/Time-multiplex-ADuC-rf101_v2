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

    Module       : retarget.c
   Description  :
    Date         : December 2012
    Version      : v2.00
    Changelog    : v1.00 Initial
                v2.00 use of UrtLib functions
*/

#include <stdio.h>
#include <rt_misc.h>
#include <library.h>

#pragma import(__use_no_semihosting_swi)

 #define CR     0x0D
struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;


// Re-targetting the Realview library functions
/*
 * writes the character specified by c (converted to an unsigned char) to
 * the output stream pointed to by stream, at the position indicated by the
 * asociated file position indicator (if defined), and advances the
 * indicator appropriately. If the file position indicator is not defined,
 * the character is appended to the output stream.
 * Returns: the character written. If a write error occurs, the error
 *          indicator is set and fputc returns EOF.
 */
int fputc(int ch, FILE * stream )
{
   if(ch == '\n')
      {
      while(!(COMLSR_THRE==(UrtLinSta(0) & COMLSR_THRE)));
      UrtTx(0, CR); /* output CR */
      }
   while(!(COMLSR_THRE==(UrtLinSta(0) & COMLSR_THRE)));
   UrtTx(0, ch);
   return(ch);
}

int __backspace(FILE *stream)
{
   return 0x0;

}
/*
 * obtains the next character (if present) as an unsigned char converted to
 * an int, from the input stream pointed to by stream, and advances the
 * associated file position indicator (if defined).
 * Returns: the next character from the input stream pointed to by stream.
 *          If the stream is at end-of-file, the end-of-file indicator is
 *          set and fgetc returns EOF. If a read error occurs, the error
 *          indicator is set and fgetc returns EOF.
 */
int fgetc(FILE * stream)
{
   return (UrtRx(0));
}


int ferror(FILE *f) {
  /* Your implementation of ferror */
  return EOF;
}


void _ttywrch(int ch)       { UrtTx(0, ch); }


void _sys_exit(int return_code) {
label:  goto label;  /* endless loop */
}
