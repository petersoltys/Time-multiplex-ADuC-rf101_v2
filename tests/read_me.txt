brief       this folder is containing scripts used to check correct function of program

version     2.0
author      Bc. Peter Soltys
supervisor  doc. Ing. Milos Drutarovsky Phd.
date        26.10.2016(DD.MM.YYYY)

note        already are all matlab scripts old and uncompatibile with actual version

actual programs
            PktTester   -Makefile is file with compilation rules of PktGenerator.cpp and PktReader
                                -esamples : make PktGenerator
                                            make PktReader
                                            make all
            
                        -PktReader.exe program is reading data from concentrator conected at com port
                                -using : PktReader 7 
                                                  [^comport]
                                -for more option : PktReader -h
                                
                        -PktGenerator.exe program to generate PRNG packets
                                -using : PktGenerator 3  2 
                                            [comport^][^slaveID of packets]
                                -for more option : PktGenerator -h
                          
                        -multiple_run.bat -batch file is simultaneously starting PktGenerator at different com ports 

                        -log.txt file containing loged messages from last run
                        
                        sources for makefile
                            -PktGenerator.cpp
                            -PktReader.cpp
                            -RS232/rs232.c
                            -RS232/rs232.h
            
