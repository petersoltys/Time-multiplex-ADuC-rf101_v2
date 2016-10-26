#ifndef _PRNG_h
#define _PRNG_h

#define SEED 500
#define MAX_DIFF 100
#define RANDOM_LENGHT 10
#include <stdint.h>
#include "../../Integrity/crc.h"

//to ensure arrangement variables in byte by byte grid structure (without "bubles")
//is used directive #pragma pack(1)
#pragma pack(1)
struct PRNGrandomPacket {
  uint8_t  slave_id;
  uint32_t numberOfPacket;
  int16_t  random[RANDOM_LENGHT];
  crc crc;
} ;
struct PRNGslave {
   uint32_t next;
   uint32_t numberOfMissingPackets;
   uint32_t numberOfReceivedPackets;
   uint16_t receivedBytes;
   struct PRNGrandomPacket packet;
};

extern void hexaToBin(uint8_t* from, uint8_t* to, uint16_t binaryLen );
extern void binToHexa(uint8_t* from, uint8_t* to, uint16_t binaryLen );
extern int8_t PRNGcheck(struct PRNGslave* localPktArray, struct PRNGrandomPacket * receivedPkt,uint8_t * message, uint8_t numOfSlaves);
extern void PRNGnew(struct PRNGslave * slave);
extern void PRNGinit(struct PRNGslave * slaves, uint8_t numberOfSlaves );

#endif
