#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "PRNG.h"
#include "../../Integrity/crc.h"

/**
 * @fn    static void PRNGsrand(unsigned int seed,struct PRNGslave * slave)
 * @brief PRNGsrand function is setting initial (seed) value of PRNG
 * @param struct PRNGslave * slave : pointer to array of slave structures
 * @param seed initial value
 */
static void PRNGsrand(unsigned int seed,struct PRNGslave * slave)
{
  slave->next = (long)seed;
}

/**
 * @fn     static int16_t PRNGrand(struct PRNGslave * slave)
 * @brief  PRNGrand function generating pseudo-random value
 * @param  struct PRNGslave * slave : pointer to array of slave structures
 * @return int pseudo-random value
 */
static int16_t PRNGrand(struct PRNGslave * slave) // RAND_MAX assumed to be 32767
{
  //official random implementation for C from CodeGuru forum
    return(((slave->next = slave->next * 214013L + 2531011L) >> 16) & 0x7fff);
}

/**
   @fn     void PRNGinit(struct PRNGslave * slave, uint8_t numberOfSlave )
   @brief  PRNGinit is initializing values for PRNG slaves
   @param  struct PRNGslave * slave : pointer to array of slave structures
   @param  uint8_t numberOfSlaves : number of slaves in array
   @code
        numberOfSlaves = 4;
        struct PRNGslave slaves[numberOfSlaves];
        PRNGinit(slaves,numberOfSlave);
   @endcode
**/
void PRNGinit(struct PRNGslave * slave, uint8_t numberOfSlav )
{
    crcInit();
    PRNGsrand(SEED,slave);
    slave->packet.slave_id = numberOfSlav;
    slave->packet.numberOfPacket = 0;
    slave->numberOfMissingPackets = 0;
    slave->numberOfReceivedPackets = 0;
}

/**
   @fn     void PRNGnew(struct PRNGslave * slave)
   @brief  PRNGnew is building new PRNG packet
   @param  struct PRNGslave* slave : pointer slave structure to be new generated
   @code
        #include "PRNG.h"

        numberOfSlaves = 4;
        struct PRNGslave slaves[numberOfSlaves];
        for(i=0; i<numberOfSlaves;i++)
            PRNGinit(slaves[i],i+1);
        uint8_t hexaBuffer[sizeof(struct PRNGrandomPacket)*2+2];  //buffer for received hexadecimal packet

        PRNGnew(&slaves[0]);
        PRNGnew(&slaves[1]);
        PRNGnew(&slaves[2]);

        binToHexa(&slaves[0].packet,hexaBuffer,sizeof(struct PRNGrandomPacket));
        hexaBuffer[(sizeof(struct PRNGrandomPacket)*2)] = '$';
        hexaBuffer[(sizeof(struct PRNGrandomPacket)*2)+1] = '\0';
        ...
   @endcode
**/
void PRNGnew(struct PRNGslave * slave)
{
    uint8_t i;
    slave->packet.numberOfPacket++;
    for(i=0; i<(sizeof(slave->packet.random)/2) ;i++){
        slave->packet.random[i] = PRNGrand(slave);
    }
    /*crc is compuded from packet without crc part and packet terminator '$' */
    slave->packet.crc = crcFast((unsigned char*)&slave->packet, sizeof(slave->packet)-2);
}

/**
   @fn     static int8_t PRNGcheckRandom(struct PRNGslave* localSlave, struct PRNGrandomPacket * receivedPkt)
   @brief  function is checking PRNG part of packet
   @param  struct PRNGslave* localSlave : pointer slave structure with local generated PRNG numbers
   @param  struct PRNGrandomPacket * receivedPkt : pointer at received packet to comparision
   @return 1 if dismatch else 0
**/
static int8_t PRNGcheckRandom(struct PRNGslave* localSlave, struct PRNGrandomPacket * receivedPkt){
    uint8_t i;
    for(i = 0; i < sizeof(receivedPkt->random)/2 ; i++){
        if (localSlave->packet.random[i] != receivedPkt->random[i]){
            return 1;
        }
    }
    return 0;
}

/**
 * @fn    int8_t PRNGcheck(struct PRNGslave* localPktArray, struct PRNGrandomPacket * receivedPkt,uint8_t * message, uint8_t numOfSlaves)
 * @brief PRNGcheck is checking any part of PRNG packet
 * @param struct PRNGslave * slave : pointer to array of slave structures
 * @param struct PRNGrandomPacket * receivedPkt : pointer at received packet
 * @param uint8_t * message : out: pointer at array of chars with returned message
 * @param uint8_t numberOfSlaves : number of slaves in array
 * @return 1 if packet is not correct
 * @code
        #include "PRNG.h"

        numberOfSlaves = 4;
        struct PRNGslave slaves[numberOfSlaves];
        for(i=0; i<numberOfSlaves;i++)
            PRNGinit(slaves[i],i+1);
        uint8_t message[50];  //variable for output string
        uint8_t hexaBuffer[sizeof(struct PRNGrandomPacket)*2+2];  //buffer for received hexadecimal packet

        hexaToBin(hexaBuffer,hexaBuffer,sizeof(struct PRNGrandomPacket));
        hexaBuffer[(sizeof(struct PRNGrandomPacket))] = '$';
        hexaBuffer[(sizeof(struct PRNGrandomPacket))+1] = '\0';

        if (PRNGcheck(slaves,(struct PRNGrandomPacket*)hexaBuffer,message,numberOfSlaves))
            printf("message : %s",message);
        else{
            printf("packet is valid");
        }
   @endcode
 */
int8_t PRNGcheck(struct PRNGslave* localPktArray, struct PRNGrandomPacket * receivedPkt,uint8_t * message, uint8_t numOfSlaves)
{
    uint16_t missing;
    struct PRNGslave * localSlave;
    crc checksum;

    /*check CRC of received packet*/
    checksum = crcFast((unsigned char*)receivedPkt, sizeof(struct PRNGrandomPacket)-2);
    if (checksum == receivedPkt->crc){
        /*check slave id if is in range*/
        if (numOfSlaves >= receivedPkt->slave_id ){
            localSlave = &localPktArray[receivedPkt->slave_id -1];
            if (localSlave->packetNumberDiff >= 0)
              PRNGnew(localSlave);
            else
              localSlave->packetNumberDiff++;
            /*check number of packet if is same as expected*/
            if (localSlave->packet.numberOfPacket == receivedPkt->numberOfPacket){
                /*check PRNG part*/
                if(PRNGcheckRandom(localSlave,receivedPkt)){
                    strcpy((char *)message,"\nwrong random number? even if CRC is correct ?#");
                    localSlave->numberOfReceivedPackets ++;
                    localSlave->receivedBytes += sizeof(struct PRNGrandomPacket)*2;
                    return 1;
                }else{
                    localSlave->numberOfReceivedPackets ++;
                    localSlave->receivedBytes += sizeof(struct PRNGrandomPacket)*2;
                    return 0;
                }
            }else{/*if number of packet does not match*/
                /*check order of packet*/
                if (receivedPkt->numberOfPacket >= localSlave->packet.numberOfPacket){
                    /*if order of packets is correct*/
                    missing = receivedPkt->numberOfPacket - localSlave->packet.numberOfPacket;
                    sprintf((char *)message,"\nmissing %d packets of slave %d#",missing,localSlave->packet.slave_id);
                    if (missing < MAX_DIFF)/*dont add this numnber if is higher than MAX_DIFF because probably PktReader was started after PktGenerator*/
                        localSlave->numberOfMissingPackets += missing;
                    while( missing -- ){/*generate random packet by diference*/
                        PRNGnew(localSlave);
                    }
                    /*check PRNG part*/
                    if(PRNGcheckRandom(localSlave,receivedPkt)){
                        strcat((char *)message,"\nwrong random number? even if CRC is correct ?#");
                    }
                    localSlave->numberOfReceivedPackets ++;
                    localSlave->receivedBytes += sizeof(struct PRNGrandomPacket)*2;
                    return 1;
                }else{/*if oreder of packets is switched*/
                    missing = localSlave->packet.numberOfPacket - receivedPkt->numberOfPacket;
                    if (missing < (uint16_t)((2^16) - MAX_DIFF)){/*check if packet number overflow*/
                        sprintf((char *)message,"\nnumber of packet is owerflowed and are missing %d packets of slave %d#",(2^16 - missing),receivedPkt->slave_id);
                        localSlave->numberOfMissingPackets += (2^16 - missing);
                        return 1;
                    }else if (missing > MAX_DIFF){
                        sprintf((char *)message,"\npacket number is less than expected, switched order ? diff %d of slave %d , probably PktGenerator was restarted?#",missing,receivedPkt->slave_id);
                        sprintf((char *)message,"%s\nstatistics was : %d of received packets and %d of missing packets#",(char *)message,localSlave->numberOfReceivedPackets,localSlave->numberOfMissingPackets);
                        localSlave->next = SEED;
                        localSlave->numberOfMissingPackets = 0;
                        localSlave->numberOfReceivedPackets = 0;
                        localSlave->packet.numberOfPacket = 0;
                        return 1;
                    }else{
                        sprintf((char *)message,"\npacket number is less than expected, switched order ? %d - %d diff %d of slave %d#",localSlave->packet.numberOfPacket,receivedPkt->numberOfPacket,missing,receivedPkt->slave_id);
                        localSlave->packetNumberDiff = -missing;
                        return 1;
                    }
                }
            }
        }else{
            sprintf((char *)message,"\nnumber of slave in packet is %d and is higher as total number of slaves %d#",receivedPkt->slave_id,numOfSlaves);
            return 1;
        }
    }else{
        sprintf((char *)message,"\nwrong CRC %x probably of slave %d#",checksum,receivedPkt->slave_id);
        return 1;
    }
    return 0;
}

/**
   @fn     void binToHexa(uint8_t* from, uint8_t* to, uint16_t len )
   @brief  function to converting binarz data to ASCII chars for hexadecimal system
   @param  uint8_t* from : pointer at binary data in memory
   @param  uint8_t* to : pointer at destination place in memory for hexadecimal ASCII chars
   @param  uint16_t len : source (binary) lenght of data, hexadecimal data are *2 lenght
   @note   output lenght in destination memory place is double lenght as source lenght
**/
void binToHexa(uint8_t* from, uint8_t* to, uint16_t binaryLen ){

  uint16_t i;
  for (i = 0 ; i < binaryLen ; i++){    //conversion of binary data to ascii chars 0 ... F
    *to = ((*from & 0xf0)>>4)+'0';
    if (*to > '9')         //because ASCII table is 0123456789:;<=>?@ABCDEF
      *to += 7;

    to++;                  //increment pointer

    *to = (*from & 0x0f) + '0';
    if (*to > '9')         //because ASCII table is 0123456789:;<=>?@ABCDEF
      *to += 7;

    to++;                 //increment pointer
    from++;
  }
}


/**
   @fn     void hexaToBin(uint8_t* from, uint8_t* to, uint16_t len )
   @brief  function is converting ASCII chars for hexadecimal system to binary
   @param  uint8_t* from : pointer at binary data in memory
   @param  uint8_t* to : pointer at destination place in memory for hexadecimal ASCII chars
   @param  uint16_t len : source (binary) lenght of data, hexadecimal data are *2 lenght
   @note   input lenght in source memory place is double lenght as destination lenght
**/
void hexaToBin(uint8_t* from, uint8_t* to, uint16_t binaryLen ){
    uint8_t ch1, ch2;
    uint16_t i;
    for (i=0; i<binaryLen; i++){    //conversion to binary data from ascii chars 0 ... F
      ch1 = *from - '0';
      from++;
      ch2 = *from - '0';

      if (ch1 > 9)                        //because ASCII table is 0123456789:;<=>?@ABCDEF
        ch1 -= 7;

      if (ch2 > 9)                        //because ASCII table is 0123456789:;<=>?@ABCDEF
        ch2 -= 7;

      *to = ((ch1<<4) | (ch2));
      to++;
      from++;
    }
}
/*
 * //testing code
void main(void){
    uint8_t i;
    uint8_t numberOfSlaves = 4;
    struct PRNGslave slaves[numberOfSlaves];
    struct PRNGslave testSlave, tempSlave;
    uint8_t message[50];
    uint8_t hexaBuffer[sizeof(struct PRNGrandomPacket)*2+2];
    for(i=0; i<numberOfSlaves;i++)
        PRNGinit(slaves[i],i+1);

    PRNGinit(&testSlave,1);
    PRNGinit(&tempSlave,1);


    PRNGnew(&testSlave);
    PRNGnew(&testSlave);
    PRNGnew(&testSlave);
    //memcpy((char *)&tempSlave,(char *)&testSlave,sizeof(testSlave));
    PRNGnew(&testSlave);


    binToHexa(&testSlave.packet,hexaBuffer,sizeof(tempSlave.packet));
    hexaBuffer[(sizeof(tempSlave.packet)*2)] = '$';
    hexaBuffer[(sizeof(tempSlave.packet)*2)+1] = '\0';
    printf("hexa : %s\n",hexaBuffer);

    hexaToBin(hexaBuffer,&tempSlave.packet,sizeof(tempSlave.packet));
    hexaBuffer[(sizeof(tempSlave.packet))] = '$';
    hexaBuffer[(sizeof(tempSlave.packet))+1] = '\0';

    if (PRNGcheck(slaves,&testSlave.packet,message,numberOfSlaves))
        printf("message : %s",message);
    else{

    }

    if (PRNGcheck(slaves,&tempSlave.packet,message,numberOfSlaves))
        printf("message : %s",message);
    else{

    }
}
*/
