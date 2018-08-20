@echo off

REM Check if required variables are set, exit if not
if not defined CURL_ROOT goto novar


echo Setting up CURL Development Environment
echo.
echo   CUrl Root: %CURL_ROOT%
echo.

cd %CURL_ROOT%\projects\Windows\VC14

msbuild curl-all.sln /p:Platform=x64 "/p:Configuration=LIB Release - DLL Windows SSPI"
msbuild curl-all.sln /p:Platform=x64 "/p:Configuration=LIB Debug - DLL Windows SSPI"
msbuild curl-all.sln /p:Platform=x64 "/p:Configuration=DLL Release - DLL Windows SSPI"
msbuild curl-all.sln /p:Platform=x64 "/p:Configuration=DLL Debug - DLL Windows SSPI"

set CURL_INCLUDE_DIR=%CURL_ROOT%\include\

set CURL_BUILD_DIR=%CURL_ROOT%\build\Win64\VC14\

set CURL_LIBRARY_RELEASE=%CURL_BUILD_DIR%\DLL Release - DLL Windows SSPI\libcurl.lib
set CURL_LIBRARY_DEBUG=%CURL_BUILD_DIR%\DLL Debug - DLL Windows SSPI\libcurld.lib
set CURL_LIBRARY=%CURL_LIBRARY_RELEASE%

cd "%CURL_ROOT%\build\Win64\VC14\DLL Release - DLL Windows SSPI"
copy libcurl.* curl.*
del curl.dll

goto end
:novar
echo Required environment variables not set
echo Run setdev.bat to setup the required parameters
exit /B 2
:end