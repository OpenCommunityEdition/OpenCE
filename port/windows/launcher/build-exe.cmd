@echo off
rem Builds HaloLauncher.exe, with its icon, next to this file: one file to hand
rem out (a download link, a shared drive). Uses the C# compiler that is part of
rem Windows (.NET Framework 4). See README.md.
setlocal
set "HERE=%~dp0"
set "CSC=%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
if not exist "%CSC%" set "CSC=%WINDIR%\Microsoft.NET\Framework\v4.0.30319\csc.exe"
set "WORK=%TEMP%\HaloLauncher-build"
set "FLAGS=/nologo /target:winexe /platform:anycpu /optimize+ /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll"
if exist "%WORK%" rmdir /s /q "%WORK%"
mkdir "%WORK%"
rem the icon is drawn by the launcher itself: build it once to draw it
"%CSC%" %FLAGS% /out:"%WORK%\HaloLauncher.exe" "%HERE%HaloLauncher.cs"
if errorlevel 1 goto failed
"%WORK%\HaloLauncher.exe" --write-icon "%WORK%\HaloLauncher.ico"
if errorlevel 1 goto failed
"%CSC%" %FLAGS% /win32icon:"%WORK%\HaloLauncher.ico" /out:"%HERE%HaloLauncher.exe" "%HERE%HaloLauncher.cs"
if errorlevel 1 goto failed
rmdir /s /q "%WORK%"
echo Built %HERE%HaloLauncher.exe
exit /b 0
:failed
echo Building HaloLauncher.exe failed.
exit /b 1
