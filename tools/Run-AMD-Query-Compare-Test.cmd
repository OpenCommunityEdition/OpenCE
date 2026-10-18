@echo off
setlocal
set "HALO_GL_DEBUG=1"
set "HALO_GPU_COMPARE_VISIBILITY=1"
set "HALO_UPDATE_AUTO=false"
set "HALO_UPDATE_ANSWER=never"
set "HALO_EXIT_AFTER=0"
set "HALO_HIDDEN_WINDOW=0"
set "HALO_NULL_RENDERER=0"
set "AMD_TEST_BIN=%~dp0"
if not exist "%AMD_TEST_BIN%halo.exe" (
    echo halo.exe was not found. Place this launcher beside the test executable.
    pause
    exit /b 1
)
set "HALO_DATA_ROOT=%AMD_TEST_BIN%"
set "HALO_SAVE_ROOT=%AMD_TEST_BIN%amd-visibility-saves"
echo Same-query mapped versus direct CPU comparison. Mapped rendering retained; comparison may affect performance.
echo Enter these in the game console, then compare the affected lights:
echo ^(set rasterizer_lens_flares_occlusion true^)
echo ^(set rasterizer_lens_flares_occlusion_debug false^)
echo ^(set rasterizer_zbias 8^)
pushd "%AMD_TEST_BIN%"
if not exist "amd-visibility-logs" mkdir "amd-visibility-logs"
set "AMD_TEST_LOG=amd-visibility-logs\query-compare-%RANDOM%-%RANDOM%.log"
echo Saving log to %AMD_TEST_LOG%
echo Same-query mapped versus direct CPU comparison> "%AMD_TEST_LOG%"
if exist "BUILD.txt" type "BUILD.txt" >> "%AMD_TEST_LOG%"
halo.exe >> "%AMD_TEST_LOG%" 2>&1
set "AMD_TEST_RESULT=%ERRORLEVEL%"
echo Game exit code: %AMD_TEST_RESULT% >> "%AMD_TEST_LOG%"
echo Saved %AMD_TEST_LOG%
popd
exit /b %AMD_TEST_RESULT%
