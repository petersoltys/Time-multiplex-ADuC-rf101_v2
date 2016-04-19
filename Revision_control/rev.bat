::file     rev.bat - automatic version control script
::brief    batch script for windows used to find and repace string in *.h, *.md and *.c files
::         automaticaly changing version and date in files
::
::
::version	  initial version
::supervisor  doc. Ing. Milos Drutarovsky Phd.
::author      Bc. Peter Soltys
::date        19.04.2016(DD.MM.YYYY)


::version of software is editing by git
::get version from git and write to temporary file
git describe --tags --long > temp.txt

::read temoprary file as variable
set /p VERSION=<temp.txt
del temp.txt

::find and repalace @version tag with new version number
Revision_control\fnr.exe --cl --dir %cd%\src --fileMask "*.c, *.h" --excludeFileMask "*.dll, *.exe, *.bat" --useRegEx --useEscapeChars --find "@version[0-9.' a-z,A-Z,-]*" --replace "@version     %VERSION%"
::find and repalace @date tag with actual date
Revision_control\fnr.exe --cl --dir %cd%\src --fileMask "*.c, *.h" --excludeFileMask "*.dll, *.exe, *.bat" --useRegEx --useEscapeChars --find "@date[0-9. ]*" --replace "@date        %DATE%"

::find and repalace version tag (for front page of doxygen) with new version number
Revision_control\fnr.exe --cl --dir %cd%\src --fileMask "*.c, *.h" --excludeFileMask "*.dll, *.exe, *.bat" --useRegEx --caseSensitive --useEscapeChars --find "Date:[0-9. ]*" --replace "Date:       %DATE%"
::find and repalace date tag (for front page of doxygen) with actual date
Revision_control\fnr.exe --cl --dir %cd%\src --fileMask "*.c, *.h" --excludeFileMask "*.dll, *.exe, *.bat" --useRegEx --caseSensitive --useEscapeChars --find "Version:[0-9.' a-z,A-Z,-]*" --replace "Version:  %VERSION%"

::find and repalace version tag (for readme.md) with new version number
Revision_control\fnr.exe --cl --dir %cd% --fileMask "*.c, *.h, *.MD" --useRegEx --caseSensitive --useEscapeChars --find "Date:[0-9. ]*" --replace "Date:         %DATE%"
::find and repalace date tag (for readme.md) with actual date
Revision_control\fnr.exe --cl --dir %cd% --fileMask "*.c, *.h, *.MD" --useRegEx --caseSensitive --useEscapeChars --find "Version:[0-9.' a-z,A-Z,-]*" --replace "Version:      %VERSION%"
