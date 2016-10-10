#include <stdio.h>
#include "RS232/rs232.h"
#include <stdint.h>

#define PACKET_LEN 24

uint16_t slaveID = 1;
uint16_t comPort = 7;
uint16_t baudRate = 9600;
uint16_t delay = 0;


//to ensure arrangement variables in byte by byte grid structure (without "bubles")
//is used directive #pragma pack(1)
#pragma pack(1)
struct rand_pkt {
  uint8_t  slave_id;
  uint32_t  randomPktNum;
  #if WEEAK_RANDOM_FUNCTION == 1
  static unsigned long next;    //variable for PRNG
  #else
  long next;                    //variable for PRNG
  #endif
  int16_t  rnadom;
  uint8_t  pktTerminator;
} random_packet;

/**
 * @brief The random class contain all function and variables nessesary
 * to create local pseudo-random values for comparation with received packet
 */
class randomGenerator
 {
    public:
    uint32_t  randomPktNum = 0;
    #if WEEAK_RANDOM_FUNCTION == 1
    static unsigned long next, previous;//variable for PRNG
    #else
    long next,previous;//variable for PRNG
    #endif
    int16_t  rnadomnum = 0;
    unsigned long received = 0;
    int slaveNumber = 0;

    void srand(unsigned int seed);
    int rand(void);
    void initNew(void);
};
/**
 * @brief random::srand function is setting initial (seed) value of PRNG
 * @param seed initial value
 */
void randomGenerator::srand(unsigned int seed)
{
  #if WEEAK_RANDOM_FUNCTION == 1
  next = seed;
  #else
  next = (long)seed;
  #endif
}

/**
 * @brief random::rand function generating pseudo-random value
 * @return int pseudo-random value
 */
int randomGenerator::rand(void) // RAND_MAX assumed to be 32767
{
  #if WEEAK_RANDOM_FUNCTION == 1
  //official ANSI implementation
    next = next * 1103515245 + 12345;
    return (unsigned int)(next/65536) % 32768;
  #else
  //official random implementation for C from CodeGuru forum
    return(((next = next * 214013L + 2531011L) >> 16) & 0x7fff);
  #endif
}

/**
 * @brief random::initNew initializing values at local machine to compare with received
 */
void randomGenerator::initNew(void)
{
    randomPktNum++;
    previous = next;
    rnadomnum = rand();
}


/**
   @fn     void binToHexa(uint8_t* from, uint8_t* to, uint16_t len )
   @brief  function to converting binarz data to ASCII chars for hexadecimal system
   @param  uint8_t* from : pointer at binary data in memory
   @param  uint8_t* to : pointer at destination place in memory for hexadecimal ASCII chars
   @param  uint16_t len : source (binary) lenght of data, hexadecimal data are *2 lenght
   @note   output lenght in destination memory place is double lenght as source lenght
**/
void binToHexa(uint8_t* from, uint8_t* to, uint16_t len ){

  uint16_t i;
  for (i = 0 ; i < len ; i++){    //conversion of binary data to ascii chars 0 ... F
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
 * @brief close_all funkcia volana pri zatvarani programu
 * ma nastarosti uzaviriet COM port a log subor
 */
void close_all ()
{
   printf("\nzatvaram port\n");
   RS232_CloseComport(comPort-1);
}

/**
 * @brief main function of program
 * @return
 */
int main(int argc, char *argv[])
{
    char *ptr;
    uint8_t packetBufferHexa[PACKET_LEN*2];
    int16_t i,n;
    uint8_t buf[4095]; // buffer for reading message
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
           if (argv[i][1] == 'd') // -d delay parameter
           {
               i++;
               delay = strtod(argv[i],&ptr);
               if (delay == 0){
                   printf("zle zadany baudrate\n");
                   printf("pouzitie: 'PktGenerator [cislo portu] [slaveID 1-9] [-b, -d, -h]'\n");
                   printf("priklad: 'PktGenerator 3 1' baudRate je prednastaveny 9600 a nemusi byt zadavany\n");
                   return 0;
               }
           }else
           if (argv[i][1] == 'b') // -b baudrate parameter
           {
               i++;
               baudRate = strtod(argv[i],&ptr);
               if (baudRate == 0){
                   printf("zle zadany baudrate\n");
                   printf("pouzitie: 'PktGenerator [cislo portu] [slaveID 1-9] [-b, -d, -h]'\n");
                   printf("priklad: 'PktGenerator 3 1' baudRate je prednastaveny 9600 a nemusi byt zadavany\n");
                   return 0;
               }
           }else
           if (argv[i][1] == 'h') // -h help
           {
               printf("pouzitie: 'PktGenerator [cislo portu] [slaveID 1-9] [-b, -d, -h]'\n");
               printf("priklad: 'PktGenerator 3 1' baudRate je prednastaveny 9600 a nemusi byt zadavany\n");
               printf("parametre: -b baudrate [9600]\n");
               printf("           -d delay oneskorenie medzi paketmi [0ms]\n");
               printf("\n");
               return 0;
           }else{
               printf("Invalid option %s", argv[i]);
               return 2;
           }

        } else {
            if (i == 1){
                comPort = strtod(argv[1],&ptr);
                if (comPort == 0){
                    printf("zle zadane cilo com portu\n");
                    printf("pouzitie: 'PktGenerator [cislo portu] [slaveID 1-9] [-b, -d, -h]'\n");
                    printf("priklad: 'PktGenerator 3 1' baudRate je prednastaveny 9600 a nemusi byt zadavany\n");
                    return 0;
                }
            }else{
                if (i == 2){
                    slaveID = strtod(argv[2],&ptr);
                    if (slaveID >= 10 || slaveID <= 0){
                        printf("slaveID musi byt v rozmedzi 1-9\n");
                        printf("pouzitie: 'PktGenerator [cislo portu] [slaveID 1-9] [-b, -d, -h]'\n");
                        printf("priklad: 'PktGenerator 3 1' baudRate je prednastaveny 9600 a nemusi byt zadavany\n");
                        return 0;
                    }
                }
                else{
                    printf("Invalid option %s",argv[i]);
                    return 0;
                }
            }
        }
      }

    printf("com port %d\nslaveID = %d,\nbaudRate = %d\n",comPort,slaveID,baudRate);

    randomGenerator slave;
    slave.srand(500);
    slave.slaveNumber = slaveID;
    atexit(close_all );

    //inicialaizacia a otvorenie com portu
    char mode[]={'8','N','1',0};
    if(RS232_OpenComport(comPort-1, baudRate, mode))//kontrola uspesnosti otvorenia COM portu
    {
        printf("neda sa otvorit com port (com port je pravdepodobne otvoreny inou aplikaciou)");
        return(0);
    }else{
        printf("com port uspesne otvoreny");
        while(1){
            slave.initNew();
            random_packet.next =            slave.next;
            random_packet.slave_id =        slave.slaveNumber;
            random_packet.randomPktNum =    slave.randomPktNum;
            random_packet.rnadom =          slave.rnadomnum;
            random_packet.pktTerminator =   '$';

            binToHexa((unsigned char*)&random_packet, packetBufferHexa, (PACKET_LEN/2)-1 );
            packetBufferHexa[PACKET_LEN-2] = '$';
            packetBufferHexa[PACKET_LEN-1] = '\0';

            //RS232_SendBuf(comPort-1,(unsigned char*)&packetBufferHexa,PACKET_LEN-1);//function is not waiting for transfer completing
            RS232_cputs(comPort-1,(char *)&packetBufferHexa);
            Sleep(delay);
            
            //read UART buffer if avaliable
            n = RS232_PollComport(comPort-1, buf, 4095);

            if(n > 0)
            {
              buf[n] = 0;   /* always put a "null" at the end of a string! */

              for(i=0; i < n; i++)
              {
                if(buf[i] < 32)  /* replace unreadable control-codes by dots */
                {
                  buf[i] = '.';
                }
                printf("%s", n, (char *)buf);
              }
            }
        }
    }
    return 0;
}
