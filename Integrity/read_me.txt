brief       this folder is containing all files required for integritz check of firmware

version     initial version
supervisor  doc. Ing. Milos Drutarovsky Phd.
author      Bc. Peter Soltys
date        26.04.2016(DD.MM.YYYY)

note        to properly working CRC program is required executing of Integrity.bat and downloading
            firmware to microcontroller via bootlader containded in folder CM3WD


file     Integrity.bat 
brief    batch script is using Srecord program to generate CRC and append in *.hex file

files    srec_cat.exe, srec_cmp.exe, srec_info.exe,
brief    little utility to for working with *.hex, *.bin etc. files
         for more information see http://srecord.sourceforge.net/
         
files    crc.c, crc.h
brief    source code of functions to calculate CRC on ADuc rf101

folder   CM3WD
brief    folder containing little utility from Analog Devides using bootlader to download firmware
         to microcontroller
