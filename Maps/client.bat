@echo off

set XMPDIR=C:\U2XMP

echo Connecting to local server...

%XMPDIR%\System\U2XMP.exe 127.0.0.1 -newwindow -log=client.log

