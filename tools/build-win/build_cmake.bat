@echo off

REM Check if required variables are set, exit if not
if not defined SLN_DIR goto novar
if not defined PROJECT_DIR goto novar
if not defined CURL_LIBRARY goto novar
if not defined CURL_INCLUDE_DIR goto novar

echo =====================================
echo  Running CMake solution generation 
echo =====================================
echo

md %SLN_DIR%
cd %SLN_DIR%
cmake -G "Visual Studio 14 Win64" %PROJECT_DIR% -DTARGET_ENV="MAINNET" "-DCURL_LIBRARY=%CURL_LIBRARY%" "-DCURL_INCLUDE_DIR=%CURL_INCLUDE_DIR%"


goto end
:novar
echo Required environment variables not set
echo Run setdev.bat to setup the required parameters
exit /B 2
:end
exit /B 0