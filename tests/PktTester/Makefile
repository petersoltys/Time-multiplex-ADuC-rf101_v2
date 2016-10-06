#brief       Makefile rule for compilation of packet generator and reader of PRNG packets

#version     initial
#author      bc. peter soltys
#supervisor  doc. ing. milos drutarovsky phd.
#date        06.10.2016(dd.mm.yyyy)
#note        already unused matlab old scripts uncompatibile with actual version

#compilator
CC=g++

# options I'll pass to the compiler.
CFLAGS=-std=c++11

# common files RS232
CRS232= RS232\rs232.c

all: PktGenerator PktReader

PktGenerator: 
	$(CC) $(CFLAGS) $(CRS232) PktGenerator.cpp -o PktGenerator

PktReader: 
	$(CC) $(CFLAGS) $(CRS232) PktReader.cpp -o PktReader

clean:
	del *exe