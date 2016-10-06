
#include <chrono>
#include <ctime>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "RS232/rs232.h"


//initial default values
#define COM_PORT 3
#define BAUD_RATE 115200
#define SEED 500
#define NUM_OF_SLAVES 4

#define PACKET_LEN 23

char logFile[100] = "log.txt";
char packet[PACKET_LEN];
int comPort = COM_PORT;
int baudRate = BAUD_RATE;
int numOfSlaves = NUM_OF_SLAVES;

//to ensure arrangement variables in byte by byte grid structure (without "bubles")
//is used directive #pragma pack(1)
#pragma pack(1)
struct rand_pkt {
  char  slave_id;
  uint32_t  randomPktNum;
  #if WEEAK_RANDOM_FUNCTION == 1
  static unsigned long next;    //variable for PRNG
  #else
  long next;                    //variable for PRNG
  #endif
  int16_t  rnadom;
  char  pktTerminator;
} *random_packet;

FILE* logFilePointer;

/**
 * @brief The random class contain all function and variables nessesary
 * to create local pseudo-random values for comparation with received packet
 */
class random
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
    unsigned int numOfInvalidPkt = 0;
    unsigned long numOfValidPkt = 0;
    unsigned int numOfMissPkt = 0;

    void checkPacket(uint32_t packetNum,long nextin,int16_t rnd);
    void srand(unsigned int seed);
    int rand(void);
    void initNew(void);
};
/**
 * @brief random::srand function is setting initial (seed) value of PRNG
 * @param seed initial value
 */
void random::srand(unsigned int seed)
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
int random::rand(void) // RAND_MAX assumed to be 32767
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
void random::initNew(void)
{
    randomPktNum++;
    previous = next;
    rnadomnum = rand();
}

void print_time(bool into_file){
    time_t timer;
    struct tm* tm_info;
    char buffer[26];

    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);
    puts(buffer);           //vypis casovej stopy
    if (into_file == true){
        fputs(buffer,logFilePointer);
    }
}

/**
 * @brief random::checkPacket function is comparing received packet with local generated pseudo-random value.
 * @param packetNum number of received packet
 * @param nextin    seed value of packet
 * @param rnd       "random" number
 * @param fp        pointer at log file
 */
void random::checkPacket(uint32_t packetNum,long nextin,int16_t rnd){
    if (randomPktNum == packetNum){
        if (next == nextin){
            if (rnadomnum == rnd){
                //printf("\npacket valid of slave %d  ",slaveNumber);
                //fprintf(logFilePointer,"\npacket valid of slave %d  ",slaveNumber);
                //print_time(true);
                numOfValidPkt++;
                received = received+PACKET_LEN;
            }
            else{
                printf("\npacket %d invalid - slave %d  ", packetNum, slaveNumber ) ;
                fprintf(logFilePointer,"\npacket %d invalid - slave %d  ", packetNum, slaveNumber ) ;
                print_time(true);
                numOfInvalidPkt++;
            }
        }
        else{
            printf("\nout of synchronization packet %d of slave %d  ", packetNum, slaveNumber);
            fprintf(logFilePointer,"\nout of synchronization packet %d of slave %d  ", packetNum, slaveNumber);
            print_time(true);
            next = nextin;
            randomPktNum = packetNum;
            numOfMissPkt++;
        }
    }
    else{
        printf("\nmissing %u packets of slave %d from packet number %d ",(packetNum - randomPktNum),slaveNumber,randomPktNum);
        fprintf(logFilePointer,"\nmissing %u packets of slave %d from packet number %d ",(packetNum - randomPktNum),slaveNumber,randomPktNum);
        print_time(true);
        numOfMissPkt += packetNum - randomPktNum;
        next = nextin;
        randomPktNum = packetNum;
    }
}

void print_output(random * slaves , int slavesNum){
    unsigned long numOfValidPkt = 0;
    unsigned int numOfInvalidPkt = 0;
    unsigned int numOfMissPkt = 0;
    unsigned int received = 0;
    int i;
    //summing all values to complex statistic
    for (i=0;i<slavesNum;i++){
        numOfValidPkt += slaves[i].numOfValidPkt;
        numOfInvalidPkt += slaves[i].numOfInvalidPkt;
        numOfMissPkt += slaves[i].numOfMissPkt;
        received += slaves[i].received;;
    }

    system("cls");
    printf("slave || pren.rychlost(UART)|| platne p. || neplatne p. || chybajuce p.\n");
    printf("por.c.        bajt/s          pocet             pocet        pocet\n");
    //print all statistic separately
    printf("1-4           %.5lu           %.10lu        %.3u          %.5u\n",received,numOfValidPkt,numOfInvalidPkt,numOfMissPkt);
    for (i=0;i<slavesNum;i++){
        printf("%d             %.5lu           %.10lu        %.3u          %.5u\n",slaves[i].slaveNumber,slaves[i].received,slaves[i].numOfValidPkt,slaves[i].numOfInvalidPkt,slaves[i].numOfMissPkt);
    }
    print_time(false);

    for (i=0;i<slavesNum;i++){
        slaves[i].received = 0;
    }
}

/**
 * @brief close_all funkcia volana pri zatvarani programu
 * ma nastarosti uzaviriet COM port a log subor
 */
void close_all ()
{
   printf("zatvaram port a log subor\n");
   fclose(logFilePointer);
   RS232_CloseComport(comPort -1);
}

/**
 * @brief main function of program
 * @return
 */

int main(int argc, char *argv[])
{

    char *ptr;
    int i;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
           if (argv[i][1] == 's') // -s number of slaves
           {
               i++;
               numOfSlaves = strtol(argv[i],&ptr,10);
               if (numOfSlaves >= 10 || numOfSlaves <= 0){
                   printf("pocet slave zariadeni musi byt v rozmedzi 1-9\n");
                   printf("pouzitie: 'PktReader [cislo portu] [-b, -s, -h, -p]'\n");
                   printf("priklad: 'PktReader 3 -b 9600' com 3, baudRate 9600 \n");
                   return 0;
               }
           }else
           if (argv[i][1] == 'b') // -b baudrate parameter
           {
               i++;
               baudRate = strtol(argv[i],&ptr,10);
               if (baudRate == 0){
                   printf("zle zadany baudrate\n");
                   printf("pouzitie: 'PktReader [cislo portu] [-b, -s, -h, -p]'\n");
                   printf("priklad: 'PktReader 3 -b 9600' com 3, baudRate 9600 \n");
                   return 0;
               }
           }else
           if (argv[i][1] == 'p') // -p comport number parameter
           {
               i++;
               comPort = strtol(argv[1],&ptr,10);
               if (comPort == 0){
                   printf("zle zadane cilo com portu\n");
                   printf("pouzitie: 'PktReader [cislo portu] [-b, -s, -h, -p]'\n");
                   printf("priklad: 'PktReader 3 -b 9600' com 3, baudRate 9600 \n");
                   return 0;
               }
           }else
           if (argv[i][1] == 'o') // -o output log file
           {
               i++;
               strcpy(logFile,argv[i]);
           }else
           if (argv[i][1] == 'h') // -h help
           {
               printf("\nPkrReader je program spolupracujuci s programom PktGenerator\n");
               printf("Program cita pakety generovane programom PktGenerator\n");
               printf("Data cita z UARTu\n");
               printf("pouzitie: 'PktReader [cislo portu] [-b, -s, -h, -p]'\n");
               printf("priklad: 'PktReader 3 -b 9600' com 3, baudRate 9600 \n");
               printf("parametre: -b baudrate [115200]\n");
               printf("           -p comPort [3]\n");
               printf("           -s pocet slave zariadeni [4]\n");
               printf("           -o output vystupny log subor [log.txt]\n");
               printf("           -h help \n");
               printf("\n");
               return 0;
           }else{
               printf("Invalid option %s", argv[i]);
               return 2;
           }

        } else {
            if (i == 1){
                comPort = strtol(argv[1],&ptr,10);
                if (comPort == 0){
                    printf("zle zadane cilo com portu\n");
                    printf("pouzitie: 'PktReader [cislo portu] [-b, -s, -h, -p]'\n");
                    printf("priklad: 'PktReader 3 -b 9600' com 3, baudRate 9600 \n");
                    return 0;
                }
            }else{
                printf("Invalid option %s",argv[i]);
                return 0;
            }
        }
      }

    bool synchronizeFlag = true;
    random slaves[numOfSlaves];
    unsigned char buffer[250];
    int bufferPointer = 0;

    logFilePointer = fopen(logFile,"w+");
    atexit(close_all);

    //inicializovanie hodnot pre slave zariadenia
    for (i=0;i<numOfSlaves;i++){
        slaves[i].srand(500);
        slaves[i].slaveNumber = i+1;
    }

    //inicialaizacia a otvorenie com portu
    char mode[]={'8','N','1',0};
    if(RS232_OpenComport(comPort -1, BAUD_RATE, mode))
    {
      printf("Can not open comport\n");
      printf("neda sa otvorit com port (com port je pravdepodobne otvoreny inou aplikaciou)\n");
      fclose(logFilePointer);
      RS232_CloseComport(comPort -1);
      return 0;
    }
    else{
        printf ( "seriovy com%d port uspesne otvoreny",comPort);
        RS232_flushRXTX(comPort -1);

        std::chrono::time_point<std::chrono::system_clock> start, end;
        start = std::chrono::system_clock::now();

        //nekonecna slucka primajuca data
        unsigned char c;

        while(1){

            while ( 0 == RS232_PollComport(comPort -1, &c, 1));

            //if message
            if (c == '\n')//begining of message
            {
                while(c != '#')//packet
                {
                    fputc(c,logFilePointer);
                    putchar(c);
                    while ( 0 == RS232_PollComport(comPort -1, &c, 1));
                }
                fputc(c,logFilePointer);
                putchar(c);
                while ( 0 == RS232_PollComport(comPort -1, &c, 1));
            }
            if (synchronizeFlag == true){

                if (c <= '9' || c >= '0' || c >= 'A' || c <= 'F' || c == '$'){//if hexadecimal chars
                    buffer[bufferPointer] = c;
                    bufferPointer++;
                }
                else{
                    printf("\nunexpected char %c ",c);
                }
            }
            else{
                if(c == '$')//packet end identifier
                {
                    bufferPointer = 0;
                    synchronizeFlag = true;
                }else{
                    fputc(c,logFilePointer);
                    putchar(c);
                }
            }

            end = std::chrono::system_clock::now();     // pociatanie uplynuleho casoveho useku
            std::chrono::duration<double> elapsed_seconds = end-start;
            if (elapsed_seconds.count() >= 1) {         //after 1 sec.
                print_output(slaves, numOfSlaves);
                start = std::chrono::system_clock::now();
            }

            if (bufferPointer >= PACKET_LEN){

                //converting packet from hexa to binary
                unsigned char ch1, ch2;
                for (i=0;i<PACKET_LEN/2;i++){    //conversion to binary data from ascii chars 0 ... F
                  ch1 = buffer[(i*2)]-'0';
                  ch2 = buffer[((i*2)+1)]-'0';

                  if (ch1 > 9)                        //because ASCII table is 0123456789:;<=>?@ABCDEF
                    ch1 -= 7;

                  if (ch2 > 9)                        //because ASCII table is 0123456789:;<=>?@ABCDEF
                    ch2 -= 7;

                  buffer[i] = ((ch1<<4) | (ch2));
                }
                buffer[11] = buffer[22];

                random_packet = (rand_pkt*) buffer;
                //check packet by identificator
                if (random_packet->pktTerminator == '$'){
                    //check slave number
                    if ((random_packet->slave_id > 0) && (random_packet->slave_id <= numOfSlaves)) {
                        slaves[random_packet->slave_id-1].initNew();
                        //check packet
                        slaves[random_packet->slave_id-1].checkPacket(random_packet->randomPktNum,random_packet->next,random_packet->rnadom);

                        bufferPointer -= PACKET_LEN;
                    }else{
                        printf ("\nchybne cislo slave zariadenia");
                        fprintf(logFilePointer,"\nchybne cislo slave zariadenia");
                        synchronizeFlag = false;
                        bufferPointer -= PACKET_LEN;
                    }
                }
                else{
                    printf("\nchyba identifikatora '%c' namiesto '$' ",random_packet->pktTerminator);
                    fprintf(logFilePointer,"\n chyba indentifikatora '%c' namiesto '$' ",random_packet->pktTerminator);
                    synchronizeFlag = false;
                    bufferPointer -= PACKET_LEN;
                }
            }
        }
    }
}
