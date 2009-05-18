echo off

REM batch file for distribution of "subcrawler"
REM $Header: /comm/subcrawler/dist.bat 2     09/05/19 4:25 tsupo $

chmod -R +w dist

rm -Rf dist\src

mkdir dist\src
mkdir dist\src\include
mkdir dist\src\lib

copy *.c            dist\src
copy subcrawler.dsw dist\src
copy subcrawler.dsp dist\src

copy Release\subcrawler.exe dist
copy Release\xmlRPC.dll     dist
copy README                 dist

copy ..\xmlRPC\xmlRPC.h dist\src\include
copy ..\xmlRPC\Release\xmlRPC.lib dist\src\lib
