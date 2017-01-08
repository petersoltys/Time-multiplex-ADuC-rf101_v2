
#ifndef __COMPRESSION_H
#define __COMPRESSION_H

#include <stdint.h>
#include "../../tests/PktTester/PRNG.h"

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
                                 uint16_t hexaLen);
                                 
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
                                    int16_t binaryLen);

#endif