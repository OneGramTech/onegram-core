@echo off

REM Check if required variables are set, exit if not
if not defined BUILD_THREADS_COUNT goto novar
if not defined SLN_DIR goto novar
if not defined PROJECT_NAME goto novar


if not defined MSBUILD_CONFIGURATION_NAME (
	set MSBUILD_CONFIGURATION_NAME=Release
)

echo Building %PROJECT_NAME%[%MSBUILD_CONFIGURATION_NAME%] VS Solution using %BUILD_THREADS_COUNT% threads...

REM Build using MSBUILD
REM Use "-target:witness_node" to build a particular target ( in this example: witness_node )
cd %SLN_DIR%
msbuild %PROJECT_NAME%.sln /p:Platform=x64 /p:Configuration=%MSBUILD_CONFIGURATION_NAME% /maxcpucount:%BUILD_THREADS_COUNT% /verbosity:minimal /clp:ErrorsOnly


goto end
:novar
echo Required environment variables not set
echo Run setdev.bat to setup the required parameters
exit /B 2
:end
exit /B 0