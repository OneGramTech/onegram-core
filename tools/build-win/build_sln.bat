@echo off

REM Check if required variables are set, exit if not
if not defined BUILD_THREADS_COUNT goto novar
if not defined SLN_DIR goto novar
if not defined PROJECT_NAME goto novar


if not defined MSBUILD_CONFIGURATION_NAME (
	set MSBUILD_CONFIGURATION_NAME=MinSizeRel
)

echo Building %PROJECT_NAME%[%MSBUILD_CONFIGURATION_NAME%] VS Solution using %MSBUILD_THREADS% threads...

cd %SLN_DIR%
msbuild %PROJECT_NAME%.sln /p:Platform=x64 /p:Configuration=%MSBUILD_CONFIGURATION_NAME% /maxcpucount:%BUILD_THREADS_COUNT%

goto end
:novar
echo Required environment variables not set
echo Run setdev.bat to setup the required parameters
exit /B 2
:end