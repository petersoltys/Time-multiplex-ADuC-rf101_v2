
#include <chrono>
#include <ctime>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "PRNG.h"
#include "RS232/rs232.h"
#include "uwbpacketclass.hpp"


//initial default values
#define COM_PORT 3
#define BAUD_RATE 115200
#define SEED 500
#define NUM_OF_SLAVES 4


char logFile[100] = "log.txt";
int comPort = COM_PORT;
int baudRate = BAUD_RATE;
int numOfSlaves = NUM_OF_SLAVES;
bool Mikula = false;
uwbPacketRx reciever;

FILE* logFilePointer;




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
void print_output(struct PRNGslave * slaves , int slavesNum){
    unsigned long numOfValidPkt = 0;
    unsigned int numOfMissPkt = 0;
    unsigned int received = 0;
    int i;
    //summing all values to complex statistic
    for (i=0;i<slavesNum;i++){
        numOfValidPkt += slaves[i].numberOfReceivedPackets;
        numOfMissPkt += slaves[i].numberOfMissingPackets;
        received += slaves[i].receivedBytes;
    }

    system("cls");
    printf("slave || pren.rychlost(UART)|| platne p. || chybajuce p.\n");
    printf("por.c.        bajt/s          pocet           pocet\n");
    //print all statistic separately
    printf("1-4           %.5lu           %.10lu         %.5u\n",received,numOfValidPkt,numOfMissPkt);
    for (i=0;i<slavesNum;i++){
        printf("%d             %.5lu           %.10lu         %.5u\n",slaves[i].packet.slave_id,slaves[i].receivedBytes,slaves[i].numberOfReceivedPackets,slaves[i].numberOfMissingPackets);
    }
    print_time(false);

    for (i=0;i<slavesNum;i++){
        slaves[i].receivedBytes = 0;
    }
}

/**
 * @brief close_all funkcia volana pri zatvarani programu
 * ma nastarosti uzaviriet COM port a log subor
 */
void *input (void * )
{
    char inputBuffer[100];
    char ch = '\0',i=0;
    printf("\ninput\n");
    
    while(1){
        i=1;
        inputBuffer[0] = '#';
        do{
            ch = getchar();
            inputBuffer[i]=ch;
            i++;
        }while(ch != '\n');
        inputBuffer[i-1]='$';
        inputBuffer[i]='\0';
        printf(inputBuffer);
        RS232_cputs(comPort-1,(char *)&inputBuffer);
    }
    /* the function must return something - NULL will do */
    return NULL;
}
void *printing (void * slaves){
    std::chrono::time_point<std::chrono::system_clock> start, end;
    std::chrono::duration<double> elapsed_seconds;
    while(1){
        start = std::chrono::system_clock::now();
        while(1){
            end = std::chrono::system_clock::now();     // pociatanie uplynuleho casoveho useku
            elapsed_seconds = end-start;
            if (elapsed_seconds.count() >= 1) {         //after 1 sec.
                print_output((struct PRNGslave *)slaves, numOfSlaves);
                start = std::chrono::system_clock::now();
                break;
            }
        }
    }
    /* the function must return something - NULL will do */
    return NULL;
}
void listPorts(void){
    int test;
    char ComName[50] = "COM";
    TCHAR lpTargetPath[100];
    printf("\ndostupne su len tieto existujuce porty");
    for(int i=0; i<20; i++) // checking ports from COM0 to COM255
    {
        sprintf(ComName,"COM%d",i);
        test = QueryDosDevice(ComName, (LPSTR)lpTargetPath, 100);
        if (test)
            printf("\n%s",ComName);
    }
}

/**
 * @brief close_all funkcia volana pri zatvarani programu
 * ma nastarosti uzaviriet COM port a log subor
 */
void close_all ()
{
   printf("\nzatvaram port a log subor\n");
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
                   printf("pouzitie: 'PktReader [cislo portu] [-b, -s, -h, -p,-M ]'\n");
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
                   printf("pouzitie: 'PktReader [cislo portu] [-b, -s, -h, -p, -M ]'\n");
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
                   printf("pouzitie: 'PktReader [cislo portu] [-b, -s, -h, -p, -M ]'\n");
                   printf("priklad: 'PktReader 3 -b 9600' com 3, baudRate 9600 \n");
                   return 0;
               }
           }else
           if (argv[i][1] == 'M') // M (Mikula) extracting data from UWB formated packet
           {
               //i++;
               Mikula = true;
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
               printf("pouzitie: 'PktReader [cislo portu] [-b, -s, -h, -p, -M ]'\n");
               printf("priklad: 'PktReader 3 -b 9600' com 3, baudRate 9600 \n");
               printf("parametre: -b baudrate [115200]\n");
               printf("           -p comPort [3]\n");
               printf("           -s pocet slave zariadeni [4]\n");
               printf("           -o output vystupny log subor [log.txt]\n");
               printf("           -M (Mikula) extracting data from UWB formated packet \n");
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
    struct PRNGslave slaves[numOfSlaves];
    char message[100];
    unsigned char buffer[250];
    int bufferPointer = 0;

    logFilePointer = fopen(logFile,"w+");
    atexit(close_all);

    //inicializovanie hodnot pre slave zariadenia
    for(i=0; i<numOfSlaves;i++)
        PRNGinit(&slaves[i],i+1);

    //inicialaizacia a otvorenie com portu
    char mode[]={'8','N','1',0};
    if(RS232_OpenComport(comPort -1, BAUD_RATE, mode))
    {
      printf("Can not open comport\n");
      printf("neda sa otvorit com port (com port je pravdepodobne otvoreny inou aplikaciou)\n");
      listPorts();
      fclose(logFilePointer);
      RS232_CloseComport(comPort -1);
      return 0;
    }
    else{
        printf ( "seriovy com%d port uspesne otvoreny",comPort);
        RS232_flushRXTX(comPort -1);

        pthread_t inputThread,printThread;
        /* create a second thread reading input*/
        if(pthread_create(&inputThread, NULL, input,NULL)) {
            fprintf(stderr, "Error creating thread\n");
            return 1;
        }
    
        if(Mikula != true){
            /* create a third thread for printing output*/
            if(pthread_create(&printThread, NULL, printing,(void*)slaves)) {
                fprintf(stderr, "Error creating thread\n");
                return 1;
            }
        }
        


        //nekonecna slucka primajuca data
        unsigned char c;

        while(1){

            while ( 0 == RS232_PollComport(comPort -1, &c, 1));

            //if message
            if (c == '\n')//begining of message
            {
                while(c != '#' && c != '$')//packet
                {
                    fputc(c,logFilePointer);
                    putchar(c);
                    while ( 0 == RS232_PollComport(comPort -1, &c, 1));
                }
                fputc(c,logFilePointer);
                putchar(c);
                while ( 0 == RS232_PollComport(comPort -1, &c, 1));
            }
            //skontroluj ci je prijimanie synchronizovane
            if (synchronizeFlag == true){

                if (c <= '9' || c >= '0' || c >= 'A' || c <= 'F' || c == '$'){//if hexadecimal chars
                    buffer[bufferPointer] = c;
                    bufferPointer++;
                }
                else{
                    printf("\nunexpected char %c ",c);
                }
            }
            else{//cakaj na synchronizacny znak
                if(c == '$')//packet end identifier
                {
                    bufferPointer = 0;
                    synchronizeFlag = true;
                }else{
                    fputc(c,logFilePointer);
                    putchar(c);
                }
            }
            
            if (Mikula == true){
                if (bufferPointer >= sizeof(struct PRNGrandomPacket)*2 && synchronizeFlag == true){
                    reciever.setPacket( buffer );
                    reciever.setPacketLength( bufferPointer );
                    reciever.readPacket();
                    
                    printf("Radar ID: %d",reciever.getRadarId());
                    printf("Sent at time: %d", reciever.getRadarTime());
                    printf("DATA [ x, y ]:");
                    for( int i = 0; i < reciever.getDataCount()/2; i++ )
                        printf(" [ %f ; %f ]",reciever.getData()[ i*2 ], reciever.getData()[ i*2 + 1 ]) ;
                    printf("\n");
    
                    bufferPointer -= ((sizeof(struct PRNGrandomPacket)*2) +1);
                }
            }else{
                if (bufferPointer >= sizeof(struct PRNGrandomPacket)*2 && synchronizeFlag == true){

                    //konvertuj paket z hexadecimalnej do binarnej podoby
                    hexaToBin(&buffer[bufferPointer - sizeof(struct PRNGrandomPacket)*2],buffer,sizeof(struct PRNGrandomPacket));
                    buffer[(sizeof(struct PRNGrandomPacket))] = '$';
                    buffer[(sizeof(struct PRNGrandomPacket))+1] = '\0';

                    if (PRNGcheck(slaves,(struct PRNGrandomPacket*)buffer,(uint8_t *)message,numOfSlaves)){
                        puts(message);
                        fputs(message,logFilePointer);
                        synchronizeFlag = false;
                    }
                    else{
                        //printf("packet is valid");
                    }
                    bufferPointer -= ((sizeof(struct PRNGrandomPacket)*2) +1);
                }
            }
        }
    }
}
