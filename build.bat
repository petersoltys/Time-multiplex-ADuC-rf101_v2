::file     build.bat - automatic building of all tagets
::brief    batch script for windows used to build all targets and generate *.hex file
::
::
::version	  initial version
::author      Bc. Peter Soltys
::supervisor  doc. Ing. Milos Drutarovsky Phd.
::date        09.10.2016(DD.MM.YYYY)

:: build have 4 options
        :: [no option] -- build Master and Slave programs
        :: master      -- build only Master program
        :: slave       -- build all 4 Slave hex files
        :: download    -- start bootloader 4 times for any slave
        
:: second option is number of how many slaves is expected
        
        
@echo off


set slave=%~2
IF "%~2"==""  (
    SET slave=4
)

IF "%~1"==""  (

    echo staviam vsetko
    Revision_control\fnr.exe --cl --dir "%cd%" --fileMask "*.uvproj" --includeSubDirectories --caseSensitive --useEscapeChars --find "<RunDebugAfterBuild>1</RunDebugAfterBuild>" --replace "<RunDebugAfterBuild>0</RunDebugAfterBuild>"
    
    Revision_control\fnr.exe --cl --dir "%cd%\src" --fileMask "*.h" --includeSubDirectories --caseSensitive --useRegEx --useEscapeChars --find "NUMBER_OF_SLAVES [0-9]" --replace "NUMBER_OF_SLAVES %slave%"
    UV4 -b Radio.uvproj -t Master -j0 -o output/Build_Master.txt
    copy "obj\Radio.hex" "output\Master.hex"
    
    for /l %%i in (1, 1, %slave%) do (
        Revision_control\fnr.exe --cl --dir "%cd%\src" --fileMask "*.h" --includeSubDirectories --caseSensitive --useRegEx --useEscapeChars --find "SLAVE_ID [0-9]" --replace "SLAVE_ID %%i"
        UV4 -b Radio.uvproj -t Slave -j0 -o output/Build_Slave.txt
        copy "obj\Radio.hex" "output\Slave%%i.hex"
    )
    Revision_control\fnr.exe --cl --dir "%cd%" --fileMask "*.uvproj" --includeSubDirectories --caseSensitive --useEscapeChars --find "<RunDebugAfterBuild>0</RunDebugAfterBuild>" --replace "<RunDebugAfterBuild>1</RunDebugAfterBuild>"
)

IF "%~1" == "master" (
    echo staviam Master projekt
    Revision_control\fnr.exe --cl --dir "%cd%" --fileMask "*.uvproj" --includeSubDirectories --caseSensitive --useEscapeChars --find "<RunDebugAfterBuild>1</RunDebugAfterBuild>" --replace "<RunDebugAfterBuild>0</RunDebugAfterBuild>"
    Revision_control\fnr.exe --cl --dir "%cd%\src" --fileMask "*.h" --includeSubDirectories --caseSensitive --useRegEx --useEscapeChars --find "NUMBER_OF_SLAVES [0-9]" --replace "NUMBER_OF_SLAVES %slave%"
    UV4 -b Radio.uvproj -t Master -j0 -o output/Build_Master.txt
    copy "obj\Radio.hex" "output\Master.hex"
    Revision_control\fnr.exe --cl --dir "%cd%" --fileMask "*.uvproj" --includeSubDirectories --caseSensitive --useEscapeChars --find "<RunDebugAfterBuild>0</RunDebugAfterBuild>" --replace "<RunDebugAfterBuild>1</RunDebugAfterBuild>"

)
IF "%~1" == "slave" (
    echo staviam Slave projekty
    
    Revision_control\fnr.exe --cl --dir "%cd%" --fileMask "*.uvproj" --includeSubDirectories --caseSensitive --useEscapeChars --find "<RunDebugAfterBuild>1</RunDebugAfterBuild>" --replace "<RunDebugAfterBuild>0</RunDebugAfterBuild>"
    for /l %%i in (1, 1, %slave%) do (
        Revision_control\fnr.exe --cl --dir "%cd%\src" --fileMask "*.h" --includeSubDirectories --caseSensitive --useRegEx --useEscapeChars --find "SLAVE_ID [0-9]" --replace "SLAVE_ID %%i"
        UV4 -b Radio.uvproj -t Slave -j0 -o output/Build_Slave.txt
        copy "obj\Radio.hex" "output\Slave%%i.hex"
    )
    Revision_control\fnr.exe --cl --dir "%cd%" --fileMask "*.uvproj" --includeSubDirectories --caseSensitive --useEscapeChars --find "<RunDebugAfterBuild>0</RunDebugAfterBuild>" --replace "<RunDebugAfterBuild>1</RunDebugAfterBuild>"

)
IF "%~1" == "download" (
    echo spustam prednastaveny bootloader

    Revision_control\fnr.exe --cl --dir "%cd%"  --fileMask "cm3wsd.ini" --includeSubDirectories --caseSensitive --useEscapeChars --useRegEx --find "File=.*$" --replace "File=output/Master.hex"
    Integrity\CM3WSD\cm3wsd.exe
    
    for /l %%i in (1, 1, %slave%) do (
        Revision_control\fnr.exe --cl --dir "%cd%"  --fileMask "cm3wsd.ini" --includeSubDirectories --caseSensitive --useEscapeChars --useRegEx --find "File=.*$" --replace "File=output/Slave%%i.hex"
        Integrity\CM3WSD\cm3wsd.exe
    )
)


