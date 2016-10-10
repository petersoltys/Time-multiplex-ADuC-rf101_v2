REM brief       This bat file is strating simultaneously PktGenerator at different com ports with different slave id

REM author      Bc. Peter Soltys
REM supervisor  doc. Ing. Milos Drutarovsky Phd.
REM version     initial
REM date        06.10.2016(DD.MM.YYYY)

@echo off
start PktGenerator.exe 7 1 
::com 7 slave 1

start PktGenerator.exe 11 2 
::com 9 slave 2

start PktGenerator.exe 12 3
::com 2 slave 3

start PktGenerator.exe 13 4
::com 5 slave 4