::file     conv.bat - automatic version control script
::brief    batch script for windows used to conversion of *.hex file for ADuc rf101 
::
::
::version	  initial version
::supervisor  doc. Ing. Milos Drutarovsky Phd.
::author      Bc. Peter Soltys
::date        25.04.2016(DD.MM.YYYY)

:: %1 mean first input argument, in this case path to *.hex file

::conversion of *.hex file to other intel version of *.hex file
Integrity\srec_cat.exe %1 -intel -o %1

::filling of empty place in memory with 0xFF (errased memory patern)
Integrity\srec_cat.exe %1 -Fill 0xFF 0x0 0x20000 -o %1

::compute CRC from *.hex file and insert at last 2 places in memory
Integrity\srec_cat.exe %1 -crop 0x0000 0x1FFFE -Cyclic_Redundancy_Check_16_BIG_Endian 0x1FFFE -CCITT -BROKEN -o %1 -intel -address-length=3 -obs=16
