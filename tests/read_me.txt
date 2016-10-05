brief       this folder is containing scripts used to check correct function of program

version     2.0
supervisor  doc. Ing. Milos Drutarovsky Phd.
author      Bc. Peter Soltys
date        26.04.2016(DD.MM.YYYY)

note        already unused matlab old scripts uncompatibile with actual version

actual programs
            PRNG packet generator/-contain PktGenerator.exe program to generate PRNG packets
                                  -using : PktGenerator 3  2 
                                                [comport^][^slaveID of packets]
                                  -for more option : PktGenerator -h
                                  -multiple_run.bat -batch file is simultaneously starting PktGenerator at different com ports 

            PRNG packet reader/   -contain PktReader.exe program is reading data from concentrator conected at com port
                                  -using : PktReader 7 
                                                    [^comport]
                                  -for more option : PktReader -h
                                  -log.txt file containing loged messages from last run
            
