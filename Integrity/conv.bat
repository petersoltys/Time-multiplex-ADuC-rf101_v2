
Integrity\srec_cat.exe %1 -intel -o %1
::srec_cat.exe ..\obj\Radio.hex -intel -o ..\obj\Radio.hex

Integrity\srec_cat.exe %1 -Fill 0xFF 0x0 0x20000 -o %1
::srec_cat.exe ..\obj\Radio.hex -Fill 0xFF 0x0 0x20000 -o ..\obj\Radio.hex

::srec_cat.exe radio.bin -binary -Cyclic_Redundancy_Check_16_Big_Endian 0x20000 -Most_To_Least -BROKEN -o crc_BROKEN.bin -binary -address-length=2 -obs=16
::srec_cat radio.bin -Cyclic_Redundancy_Check_16_Big_Endian 0x0FFE -Most_To_Least -BROKEN -o crc_BROKEN.hex -address-length=2 -obs=16 
Integrity\srec_cat.exe %1 -crop 0x0000 0x1FFFE -Cyclic_Redundancy_Check_16_Big_Endian 0x1FFFE -Most_To_Least -BROKEN -o %1 -intel -address-length=3 -obs=16
::srec_cat ..\obj\Radio.hex -crop 0x0000 0x1FFFE -Cyclic_Redundancy_Check_16_Big_Endian 0x1FFFE -Most_To_Least -BROKEN -o ..\obj\Radio.hex -intel -address-length=3 -obs=16