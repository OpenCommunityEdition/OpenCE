@echo off
rem Builds the Halo CE Universal launcher (HaloLauncher.cs, next to this file)
rem with the C# compiler that is part of Windows (.NET Framework 4), then starts
rem it. Nothing needs to be installed first. Started on its own, this file
rem fetches HaloLauncher.cs from the repository. See README.md.
setlocal
title Halo CE Universal
echo Getting Halo CE Universal ready. This window closes by itself in a moment.
set "SOURCE=%~dp0HaloLauncher.cs"
set "TARGET_DIR=%LOCALAPPDATA%\HaloCEUniversal"
set "TARGET=%TARGET_DIR%\HaloLauncher.exe"
set "CSC=%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
if not exist "%CSC%" set "CSC=%WINDIR%\Microsoft.NET\Framework\v4.0.30319\csc.exe"
if not exist "%CSC%" (
	echo.
	echo This PC is missing a part of Windows that Halo CE Universal needs:
	echo the .NET Framework 4. Install the latest Windows updates, then try again.
	pause
	exit /b 1
)
if not exist "%SOURCE%" (
	set "SOURCE=%TEMP%\HaloLauncher.cs"
	curl.exe --fail --silent --show-error --location --output "%TEMP%\HaloLauncher.cs" https://raw.githubusercontent.com/cybersecurity/halo-ce-universal/main/port/windows/launcher/HaloLauncher.cs
	if errorlevel 1 (
		echo.
		echo Couldn't download Halo CE Universal. Check your internet connection, then try again.
		pause
		exit /b 1
	)
)
if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"
"%CSC%" /nologo /target:winexe /platform:anycpu /optimize+ /out:"%TARGET%.new" /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll "%SOURCE%" >"%TEMP%\HaloLauncher-build.txt"
if errorlevel 1 (
	type "%TEMP%\HaloLauncher-build.txt"
	echo.
	echo Halo CE Universal couldn't be prepared ^(see above^).
	pause
	exit /b 1
)
rem a launcher that is already open keeps its file: start that one
move /y "%TARGET%.new" "%TARGET%" >nul 2>&1 || del "%TARGET%.new"
start "" "%TARGET%" %*
