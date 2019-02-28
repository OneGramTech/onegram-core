@echo off

REM Check if required variables are set, exit if not
if not defined OPENSSL_ROOT goto novar


echo Setting up OpenSSL Development Environment
echo.
echo   OpenSSL: %OPENSSL_ROOT%
echo.

pushd .

REM Include build directory info
call _directories.bat
REM Check if build is needed
set __BUILD_PATH=%OPENSSL_BUILD_PATH%
set __BUILD_VERSION=%OPENSSL_BUILD_NAME%
call fnc_check_build.bat
if %ERRORLEVEL% EQU 0 (
	echo OpenSSL already built, skipping OpenSSL build step
	goto end
)
REM Check end

echo Setting up VS2015 environment...
call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64

echo Building OpenSSL...
cd %OPENSSL_ROOT%
perl Configure VC-WIN64A --prefix=%OPENSSL_ROOT% --openssldir=%OPENSSL_ROOT%
call ms\do_win64a.bat
nmake -f ms\ntdll.mak
set BUILD_RESULT=%ERRORLEVEL%
nmake -f ms\ntdll.mak install

popd

REM Update version info
if %BUILD_RESULT% EQU 0 (
	set __BUILD_VERSION=%OPENSSL_BUILD_NAME%
	call fnc_set_build.bat
)


goto end
:novar
echo Required environment variables not set
echo Run setdev.bat to setup the required parameters
exit /B 2
:end
exit /B 0