::file     build.bat - automatic building of all tagets
::brief    batch script for windows used to build all targets and generate *.hex file
::
::
::version	  initial version
::author      Bc. Peter Soltys
::supervisor  doc. Ing. Milos Drutarovsky Phd.
::date        09.10.2016(DD.MM.YYYY)

REM build have 4 options
        REM [no option] -- build Master and Slave programs
        REM master      -- build only Master program
        REM slave       -- build all 4 Slave hex files
        REM download    -- start bootloader 4 times for any slave
        
        
@echo off
SET slave=4

IF "%~1"==""  (

    echo staviam vsetko
    
    UV4 -b Radio.uvproj -t Master -j0 -o output/Build_Master.txt
    copy "obj\Radio.hex" "output\Master.hex"
    
    for /l %%i in (1, 1, %slave%) do (
        Revision_control\fnr.exe --cl --dir %cd%\src --fileMask "*.h" --includeSubDirectories --caseSensitive --useRegEx --useEscapeChars --find "SLAVE_ID [0-9]" --replace "SLAVE_ID %%i"
        UV4 -b Radio.uvproj -t Slave -j0 -o output/Build_Slave.txt
        copy "obj\Radio.hex" "output\Slave%%i.hex"
    )

)

IF "%~1" == "master" (
    echo staviam Master projekt
    UV4 -b Radio.uvproj -t Master -j0 -o output/Build_Master.txt
    copy "obj\Radio.hex" "output\Master.hex"
)
IF "%~1" == "slave" (
    echo staviam Slave projekty

    for /l %%i in (1, 1, %slave%) do (
        Revision_control\fnr.exe --cl --dir %cd%\src --fileMask "*.h" --includeSubDirectories --caseSensitive --useRegEx --useEscapeChars --find "SLAVE_ID [0-9]" --replace "SLAVE_ID %%i"
        UV4 -b Radio.uvproj -t Slave -j0 -o output/Build_Slave.txt
        copy "obj\Radio.hex" "output\Slave%%i.hex"
    )
)
IF "%~1" == "download" (
    echo spustam prednastaveny bootloader

    Revision_control\fnr.exe --cl --dir %cd%  --fileMask "cm3wsd.ini" --includeSubDirectories --caseSensitive --useEscapeChars --useRegEx --find "File=.*$" --replace "File=output/Master.hex"
    Integrity\CM3WSD\cm3wsd.exe
    
    for /l %%i in (1, 1, %slave%) do (
        Revision_control\fnr.exe --cl --dir %cd%  --fileMask "cm3wsd.ini" --includeSubDirectories --caseSensitive --useEscapeChars --useRegEx --find "File=.*$" --replace "File=output/Slave%%i.hex"
        Integrity\CM3WSD\cm3wsd.exe
    )
)


