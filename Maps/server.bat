@echo off

set XMPDIR=C:\U2XMP

echo Starting dedicated server for %1%...

%XMPDIR%\System\U2XMP.exe %1% -server -log=server.log

