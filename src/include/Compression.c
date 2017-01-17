
#include "..\settings.h"
#include "Compression.h"
#include "stdio.h"
#include <string.h>


/**
   @fn     hexaToBinaryCompression
   @brief  converting ASCII hexadecimal words system to binary compressed system
   @param  uint8_t * hexaInput : pointer at hexadecimal chars inb memory
   @param  uint8_t * binaryOutput : pointer at place in memorz to store compressed data
   @param  uint16_t hexaLen : lenght of hexadecimal string
   @return returning binarz lenght of compressed binary output
   @note   input lenght in source memory place is double lenght as destination lenght
**/
uint16_t hexaToBinaryCompression( uint8_t * hexaInput, 
                                 uint8_t * binaryOutput, 
                                 uint16_t hexaLen)
{
  int16_t binaryLen = 0, processed = 0, wordLen;
  uint8_t *procesPtr = hexaInput;
#if ADAPTIVE_COMPRESSION
  uint8_t hexa = TRUE;
#endif
  while(1){
    
    // get lenght of packet
    wordLen = 0;
    while(*procesPtr != STRING_TERMINATOR && processed < hexaLen){
#if ADAPTIVE_COMPRESSION
      //check if hexadecimal number (char)
      if ((*procesPtr < '0' || *procesPtr > '9'))
        if (*procesPtr < 'A' || *procesPtr > 'F' )
          hexa = FALSE;
#endif
      procesPtr++;
      wordLen ++;
      processed ++;
    }
    processed ++;
    procesPtr++;  // move over STRING_TERMINATOR
    //printf("wordLen %d \n",wordLen);
#if ADAPTIVE_COMPRESSION   
    if (hexa){  //if hexadecimal word
#endif
      // translate data if lenght number is even
      if (wordLen % 2){
        // word is not even number
        // drop word
      }
      else{
        
        //translate  from HEXADECIMAL chars to binary data
        hexaToBin(hexaInput, &binaryOutput[1], wordLen/2);
        
        binaryLen += wordLen/2 + 1;
        
        *binaryOutput = wordLen/2; // store word lenght
        binaryOutput += wordLen/2 +1 ;
      }
#if ADAPTIVE_COMPRESSION
    }else{
      memcpy(&binaryOutput[1], hexaInput, wordLen+1);
      
      binaryLen += wordLen + 1;

      *binaryOutput = 0; // 0 is flag for non translated packet
      binaryOutput += wordLen +1 ;
    }
#endif
    hexaInput += wordLen + 1;
    
    //break if last word processed
    if (processed >= hexaLen){
      return binaryLen;
    }
  }
}

/**
   @fn     binaryToHexaDecompression
   @brief  converting binary compressed data to ASCII chars in hexadecimal system
   @param  uint8_t* binaryInput : pointer at compressed binary data in memory
   @param  uint8_t* hexaOutput : pointer at destination place in memory for hexadecimal ASCII chars
   @param  uint16_t binaryLen : source (binary) lenght of data
   @note   output lenght in destination memory is double lenght as source lenght
**/
uint16_t binaryToHexaDecompression( uint8_t * binaryInput, 
                                    uint8_t * hexaOutput, 
                                    int16_t binaryLen)
{
  uint16_t hexaLen = 0;
#if ADAPTIVE_COMPRESSION
  uint16_t len = 0;
#endif

    while(binaryLen > 0){
      binToHexa(binaryInput +1 , hexaOutput, *binaryInput);
#if ADAPTIVE_COMPRESSION
      if (*binaryInput == 0){
        binaryInput++;
        while(binaryInput[len] != STRING_TERMINATOR){
          len++;
        }
        len++;
        memcpy(hexaOutput, binaryInput, len);
        hexaLen += len;
        binaryInput += len;
        hexaOutput += len; 
        binaryLen -= len;
      }
      else{
#endif
        hexaOutput += (*binaryInput*2);
        
        *hexaOutput = STRING_TERMINATOR;
        hexaOutput++;
        
        hexaLen += (*binaryInput*2)+1;
        binaryLen -= (*binaryInput +1);
        
        binaryInput += (*binaryInput + 1);
#if ADAPTIVE_COMPRESSION     
      }
#endif
    }
    return hexaLen;
}

