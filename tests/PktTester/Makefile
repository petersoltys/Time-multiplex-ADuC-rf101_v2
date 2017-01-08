#brief       Makefile rule for compilation of packet generator and reader of PRNG packets

#version     initial
#author      bc. peter soltys
#supervisor  doc. ing. milos drutarovsky phd.
#date        08.01.2017(dd.mm.yyyy)
#note        already unused matlab old scripts uncompatibile with actual version

#compilator
CC=g++

# options I'll pass to the compiler.
CFLAGS=-std=c++11 -m32

# common files RS232
CRS232= RS232/rs232.c
CPRNG= PRNG.c uwbpacketclass.cpp
#paht to base folder
BASE_PATH= ../../
CCRC= $(BASE_PATH)Integrity/crc.c 
INCLUDE_PATH= $(BASE_PATH)src/include/
COMPR= $(INCLUDE_PATH)Compression.c 

INCLUDE= -I$(INCLUDE_PATH) -I$(INCLUDE_PATH)../ 

all: PktGenerator PktReader

PktGenerator: 
	$(CC) $(CFLAGS) $(CRS232) $(CPRNG) $(CCRC) $(COMPR) PktGenerator.cpp -o PktGenerator $(INCLUDE)

PktReader: 
	$(CC) $(CFLAGS) $(CRS232) $(CPRNG) $(CCRC) $(COMPR) PktReader.cpp -o PktReader $(INCLUDE)

clean:
	del *exe