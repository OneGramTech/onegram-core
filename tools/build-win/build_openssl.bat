@echo off

REM Check if required variables are set, exit if not
if not defined OPENSSL_ROOT goto novar


echo Setting up OpenSSL Development Environment
echo.
echo   OpenSSL: %OPENSSL_ROOT%
echo.

echo Setting up VS2015 environment...
call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64

echo Building OpenSSL...
cd %OPENSSL_ROOT%
perl Configure VC-WIN64A --prefix=%OPENSSL_ROOT% --openssldir=%OPENSSL_ROOT%
call ms\do_win64a.bat
nmake -f ms\ntdll.mak
nmake -f ms\ntdll.mak install

goto end
:novar
echo Required environment variables not set
echo Run setdev.bat to setup the required parameters
exit /B 2
:end