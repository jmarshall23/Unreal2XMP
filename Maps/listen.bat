@echo off

set XMPDIR=C:\U2XMP

echo Starting listen server for %1%...

%XMPDIR%\System\U2XMP.exe %1%?listen -log=listen.log

