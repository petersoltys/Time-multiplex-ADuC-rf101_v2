brief       this folder contain automatic version control scripts
            in *.bat script is used utility trom page https://findandreplace.codeplex.com/ 
            in Version 1.6 released on Mar 24th, 2014
version     initial version
supervisor  doc. Ing. Milos Drutarovsky Phd.
author      Bc. Peter Soltys
date        19.04.2016(DD.MM.YYYY)


version number format: 'V2.2'-5-g2d57c87
                        |     |     |
      version of software     |     |
number of commits in actual version |
                                    git commit number


file     rev - revision control bath script
brief    batch script for windows used to find and repace string in *.h, *.md and *.c files
         automaticaly changing version and date in files

files   fnr.exe  aplication for repacing strings from comand line and with gui
        rev.bat  batch script is using git to get version number for writing to files by tags
        
used tags    @version
             @date
             version:
             date: