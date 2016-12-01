#include <stdio.h>
#include "RS232/rs232.h"
#include "PRNG.h"
#include <stdint.h>


uint16_t slaveID = 1;
struct PRNGslave slave;
uint16_t comPort = 7;
uint16_t baudRate = 9600;
uint16_t delay = 0;

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
    uint8_t hexaBuffer[sizeof(struct PRNGrandomPacket)*2+2];
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

    atexit(close_all );
    PRNGinit(&slave,slaveID); //initialize slave variable
        
    //inicialaizacia a otvorenie com portu
    char mode[]={'8','N','2',0};
    if(RS232_OpenComport(comPort-1, baudRate, mode))//kontrola uspesnosti otvorenia COM portu
    {
        printf("neda sa otvorit com port (com port je pravdepodobne otvoreny inou aplikaciou)");
        return(0);
    }else{
        printf("com port uspesne otvoreny\n");
        RS232_flushRXTX(comPort -1);
        while(1){
            PRNGnew(&slave);  // generate new packet

            binToHexa((uint8_t*)&slave.packet,hexaBuffer,sizeof(struct PRNGrandomPacket));
            hexaBuffer[(sizeof(struct PRNGrandomPacket)*2)] = '$';
            hexaBuffer[(sizeof(struct PRNGrandomPacket)*2)+1] = '\0';

            //RS232_SendBuf(comPort-1,(unsigned char*)&hexaBuffer,PACKET_LEN-1);//function is not waiting for transfer completing
            RS232_cputs(comPort-1,(char *)&hexaBuffer);
            Sleep(delay);
 
            //read UART buffer if avaliable
            n = RS232_PollComport(comPort-1, buf, 4095);

            if(n > 0)
            {
              buf[n] = 0;   /* always put a "\0" at the end of a string! */
              for(i=0; i < n; i++)
              {
                if(buf[i] < 32 && buf[i] != '\n' && buf[i] != '\r')  /* replace unreadable control-codes by dots */
                {
                  buf[i] = '.';
                }
                putchar(buf[i]);
              }
            }
        }
    }
    return 0;
}
