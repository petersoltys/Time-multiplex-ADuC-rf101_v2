
#include <chrono>
#include <ctime>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "PRNG.h"
#include "settings.h"
#include "Compression.h"
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
bool terminal = false;
bool decompression = false;
uwbPacketRx reciever;

std::chrono::time_point<std::chrono::system_clock> programStart;

FILE* logFilePointer;


// Theoretical maximum (because of processing function implementation) = 20 coords (10 targets)
float test_coords[20][20];/*
test_coords[0] = { 36.04, 33.26, 37.81, 22.21, 24.73, 28.18, 40.6, 23.72, 35.73, 23.35, 30.29, 28.79, 35.55, 38.98 };
test_coords[1] = { 20.98, 27.36, 40.4, 40.16, 37.37, 29.12 };
test_coords[2] = { 28.13, 35.59, 33.92, 37.78, 33.47, 36.84 };
test_coords[3] = { 26.17, 28.62, 26.88, 33.83, 40.83, 28.61, 38.09, 28.07, 23.14, 36.21, 22.85, 26.01 };
test_coords[4] = { 26.05, 35.97, 38.94, 28.77, 36.47, 40.7, 23.1, 35.68, 37.12, 31.75, 38.14, 39.65, 21.27, 31.11, 36.48, 34.26 };
test_coords[5] = { 27.32, 34.84, 30.02, 33.73, 27.72, 29.91, 21.39, 40.21, 32.38, 39.41, 35.48, 29.63, 24.47, 35.43, 35.2, 39.96 };
test_coords[6] = { 23.02, 30.37, 28.05, 20.53, 40.45, 39.57, 31.8, 37.65, 38.27, 27.47, 22.69, 28.68, 20.79, 24.1, 35.53, 30.03, 33.64, 28.31 };
test_coords[7] = { 37.95, 24.1, 36.54, 24.26 };
test_coords[8] = { 25.95, 33.42, 21.44, 35.78, 27.67, 40.92, 23.14, 25.1, 25.22, 25.59, 40.5, 24.73 };
test_coords[9] = { 26.23, 36.78, 36.89, 33.22 };
test_coords[10] = { 24.63, 28.41, 37.49, 34.56, 37.96, 25.07 };
test_coords[11] = { 34.29, 22.07, 40.89, 24.76 };
test_coords[12] = { 22.51, 25.11, 33.68, 38.82, 35.29, 35.75, 33.67, 32.83, 35.25, 38.42, 37.95, 34.8, 37.55, 36.57 };
test_coords[13] = { 28.26, 27.39, 32.83, 21.18, 26.42, 40.76, 38.2, 40.5, 37.78, 22.32, 21.47, 26 };
test_coords[14] = { 36.77, 25.16 };
test_coords[15] = { 33.68, 29.79, 32.25, 31.55, 24.13, 27.06, 24.27, 36.49, 21.36, 21.74, 28.37, 35.69, 33.69, 39.34 };
test_coords[16] = { 21, 25.78, 22.05, 21.71, 31.73, 37.22, 34.31, 31.28 };
test_coords[17] = { 36.15, 27.16, 34.45, 34.46, 38.33, 39.13, 33.04, 31.06, 22.85, 39.69 };
test_coords[18] = { 26.5, 25.8, 40.8, 37.39, 26.69, 21.6 };
test_coords[19] = { 36.78, 29.69 };*/

void print_time(bool into_file){
    time_t timer;
    struct tm* tm_info;
    char buffer[26];

    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 26, "\n%H:%M:%S %d:%m:%Y->", tm_info);
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
        inputBuffer[i-1]=STRING_TERMINATOR;
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
   std::chrono::duration<double> elapsed_time = std::chrono::system_clock::now() - programStart;

   printf("\n program bezal %d minut ",(int)elapsed_time.count()/60);
   fprintf(logFilePointer,"\n program bezal %d minut ",(int)elapsed_time.count()/60);
    
   fclose(logFilePointer);
   RS232_CloseComport(comPort -1);
}

void shortHelp(void){
    printf("pouzitie: 'PktReader [cislo portu] [-b, -s, -h, -p,-M ]'\n");
    printf("priklad: 'PktReader 3 -b 9600' com 3, baudRate 9600 \n");
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
           if (strcmp(&argv[i][1],"s") == 0 || strcmp(&argv[i][1],"slaves") == 0) // -s number of slaves
           {
                i++;
                numOfSlaves = strtol(argv[i],&ptr,10);
                if (numOfSlaves >= 10 || numOfSlaves <= 0){
                    printf("pocet slave zariadeni musi byt v rozmedzi 1-9\n");
                    shortHelp();
                    return 0;
                }
           }else
           if (strcmp(&argv[i][1],"b") == 0 || strcmp(&argv[i][1],"baud") == 0) // -b baudrate parameter
           {
                i++;
                baudRate = strtol(argv[i],&ptr,10);
                if (baudRate == 0){
                    printf("zle zadany baudrate\n");
                    shortHelp();
                    return 0;
                }
            }else
            if (strcmp(&argv[i][1],"p") == 0 || strcmp(&argv[i][1],"port") == 0) // -p comport number parameter
            {
                i++;
                comPort = strtol(argv[1],&ptr,10);
                if (comPort == 0){
                    printf("zle zadane cilo com portu\n");
                    shortHelp();
                    return 0;
                }
            }else
            if (strcmp(&argv[i][1],"M") == 0 || strcmp(&argv[i][1],"Mikula") == 0) // M (Mikula) extracting data from UWB formated packet
            {
                Mikula = true;
            }else
            if (strcmp(&argv[i][1],"t") == 0 || strcmp(&argv[i][1],"terminal") == 0) // spusti seriovy terminal/konzolu
            {
                terminal = true;
            }else
            if (strcmp(&argv[i][1],"decompress") == 0) // spusti dekompresiu dat z binarnej podoby
            {   //velmi citlive na chyby v prenosovom kanaly
                decompression = true;
            }else
            if (strcmp(&argv[i][1],"o") == 0 || strcmp(&argv[i][1],"output") == 0) // -o output log file
            {
               i++;
               strcpy(logFile,argv[i]);
            }else
            if (strcmp(&argv[i][1],"h") == 0 || strcmp(&argv[i][1],"help") == 0) // -h help
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
                printf("           -t -terminal funguje len ako seriovy termial");
                printf("           -h help \n");
                printf("\n");
                return 0;
            }else
            {
                printf("Invalid option %s", argv[i]);
                shortHelp();
                return 2;
            }

        } else {
            if (i == 1){
                comPort = strtol(argv[1],&ptr,10);
                if (comPort == 0){
                    printf("zle zadane cilo com portu\n");
                    shortHelp();
                    return 0;
                }
            }else{
                printf("Invalid option %s",argv[i]);
                shortHelp();
                return 0;
            }
        }
    }

    bool synchronizeFlag = true;
    struct PRNGslave slaves[numOfSlaves];
    char message[100];
    unsigned char buffer[250];
    int bufferPointer = 0;

    programStart = std::chrono::system_clock::now();

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
    
        if(Mikula == false && terminal == false){
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

            if (terminal == true)
                putchar(c);
            else{
                if (decompression == false)
                {
                    //if message
                    if (c == '\n')//begining of message
                    {
                        while(c != '#' && c != STRING_TERMINATOR)//packet
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

                        if (c <= '9' || c >= '0' || c >= 'A' || c <= 'F' || c == STRING_TERMINATOR){//if hexadecimal chars
                            buffer[bufferPointer] = c;
                            bufferPointer++;
                        }
                        else{
                            printf("\nunexpected char %c ",c);
                        }
                    }
                    else{//cakaj na synchronizacny znak
                        if(c == STRING_TERMINATOR)//packet end identifier
                        {
                            bufferPointer = 0;
                            synchronizeFlag = true;
                        }else{
                            fputc(c,logFilePointer);
                            putchar(c);
                        }
                    }
                    
                    if (Mikula == true && decompression == false){
                        if (bufferPointer >= sizeof(struct PRNGrandomPacket)*2 && synchronizeFlag == true){
                            reciever.setPacket( buffer );
                            reciever.setPacketLength( bufferPointer );
                            reciever.readPacket();
                            
                            printf("Radar ID: %d",reciever.getRadarId());
                            printf("Sent at time: %d", reciever.getRadarTime());
                            printf("DATA [ x, y ]:");
                            for( int i = 0; i < reciever.getDataCount()/2; i++ )
                                printf(" [ %f ; %f ]",reciever.getData()[ i*2 ], reciever.getData()[ i*2 + 1 ]) ;
                                if((int)test_coords[reciever.getRadarId()][i*2] == (int)reciever.getData()[ i*2 ] &&
                                    (int)test_coords[reciever.getRadarId()][i*2 +1] == (int)reciever.getData()[ i*2 +1 ]){
                                        printf(" OK " );
                                }
                                else{
                                    printf(" zle " );
                                }
                            printf("\n");
            
                            bufferPointer -= ((sizeof(struct PRNGrandomPacket)*2) +1);
                        }
                    }else{
                        if (bufferPointer >= sizeof(struct PRNGrandomPacket)*2 && synchronizeFlag == true){
                            //konvertuj paket z hexadecimalnej do binarnej podoby
                            hexaToBin(&buffer[bufferPointer - sizeof(struct PRNGrandomPacket)*2],buffer,sizeof(struct PRNGrandomPacket));
                            buffer[(sizeof(struct PRNGrandomPacket))] = STRING_TERMINATOR;
                            buffer[(sizeof(struct PRNGrandomPacket))+1] = '\0';
                            

                            if (PRNGcheck(slaves,(struct PRNGrandomPacket*)buffer,(uint8_t *)message,numOfSlaves)){
                                print_time(true);
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
                }else{
                    
                    buffer[bufferPointer] = c;
                    bufferPointer++;
                    
                    if (bufferPointer >= sizeof(struct PRNGrandomPacket)+1){
                        uint8_t locBuf[250];
                        uint16_t len;
                        
                        len = binaryToHexaDecompression( buffer, 
                                    locBuf, sizeof(struct PRNGrandomPacket)+1);
                                    
                        //konvertuj paket z hexadecimalnej do binarnej podoby
                        hexaToBin(locBuf,buffer,sizeof(struct PRNGrandomPacket));
                        buffer[(sizeof(struct PRNGrandomPacket))] = STRING_TERMINATOR;
                        buffer[(sizeof(struct PRNGrandomPacket))+1] = '\0';
                        
                        if (PRNGcheck(slaves,(struct PRNGrandomPacket*)buffer,(uint8_t *)message,numOfSlaves)){
                            print_time(true);
                            puts(message);
                            fputs(message,logFilePointer);
                            while(c != sizeof(struct PRNGrandomPacket)+1)
                                while ( 0 == RS232_PollComport(comPort -1, &c, 1));
                            buffer[bufferPointer] = c;
                            bufferPointer -= (sizeof(struct PRNGrandomPacket));
                        }
                        else{
                            //printf("packet is valid");
                            bufferPointer -= (sizeof(struct PRNGrandomPacket) +1);
                        }
                        
                    }
                }
            }
        }
    }
}
