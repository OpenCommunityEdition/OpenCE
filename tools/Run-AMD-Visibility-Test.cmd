@echo off
setlocal
set "HALO_GPU_SYNC_VISIBILITY=true"
if /I "%~1"=="--baseline" set "HALO_GPU_SYNC_VISIBILITY=false"
set "HALO_UPDATE_AUTO=false"
set "HALO_UPDATE_ANSWER=never"
set "HALO_EXIT_AFTER=0"
set "HALO_HIDDEN_WINDOW=0"
set "HALO_NULL_RENDERER=0"
set "AMD_TEST_BIN=%~dp0"
if not exist "%AMD_TEST_BIN%halo.exe" set "AMD_TEST_BIN=%~dp0..\build\windows\"
if not exist "%AMD_TEST_BIN%halo.exe" (
    echo halo.exe was not found. Place this launcher beside the diagnostic executable.
    pause
    exit /b 1
)
set "HALO_SAVE_ROOT=%AMD_TEST_BIN%amd-visibility-saves"
echo Synchronized visibility: %HALO_GPU_SYNC_VISIBILITY%
echo Keep lens-flare occlusion enabled in the game console:
echo ^(set rasterizer_lens_flares_occlusion true^)
pushd "%AMD_TEST_BIN%"
halo.exe
set "AMD_TEST_RESULT=%ERRORLEVEL%"
popd
exit /b %AMD_TEST_RESULT%
