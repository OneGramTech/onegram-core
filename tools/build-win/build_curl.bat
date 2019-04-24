@echo off

REM Check if required variables are set, exit if not
if not defined CURL_ROOT goto novar


echo Setting up CURL Development Environment
echo.
echo   CUrl Root: %CURL_ROOT%
echo.

if not defined CURL_BUILD_WITH_SSPI (
	if EXIST %OPENSSL_ROOT%\include (
		echo.
		echo Building cURL with OpenSSL
		echo.

		if EXIST %CURL_ROOT%\include\openssl (
			RMDIR /S /Q "%CURL_ROOT%\include\openssl"
		)

		if EXIST %CURL_ROOT%\include\internal (
			RMDIR /S /Q "%CURL_ROOT%\include\internal"
		)

		xcopy /qs %OPENSSL_ROOT%\include %CURL_ROOT%\include

		if %ERRORLEVEL% EQU 0 (
			set CURL_BUILD_CONFIG_RELEASE=LIB Release - LIB OpenSSL
			set CURL_BUILD_CONFIG_DEBUG=LIB Debug - LIB OpenSSL
			echo Setting OpenSSL to build cURL
		)
	)
)

if not defined CURL_BUILD_CONFIG_RELEASE (
	echo Build cURL Release LIB with Windows SSPI
	set CURL_BUILD_CONFIG_RELEASE=LIB Release - DLL Windows SSPI
)

if not defined CURL_BUILD_CONFIG_DEBUG (
	echo Build cURL Debug LIB with Windows SSPI
	set CURL_BUILD_CONFIG_DEBUG=LIB Debug - DLL Windows SSPI
)

set CURL_CHECK_VERSION=%CURL_BUILD_NAME%-%CURL_BUILD_CONFIG_RELEASE%-%CURL_BUILD_CONFIG_DEBUG%

set CURL_INCLUDE_DIR=%CURL_ROOT%\include\

set CURL_BUILD_DIR=%CURL_ROOT%\build\Win64\VC14\

set CURL_BUILD_DIR_RELEASE=%CURL_BUILD_DIR%\%CURL_BUILD_CONFIG_RELEASE%\
set CURL_BUILD_DIR_DEBUG=%CURL_BUILD_DIR%\%CURL_BUILD_CONFIG_DEBUG%\

set CURL_LIBRARY_RELEASE=%CURL_BUILD_DIR_RELEASE%libcurl.lib
set CURL_LIBRARY_DEBUG=%CURL_BUILD_DIR_DEBUG%libcurld.lib
set CURL_LIBRARY=%CURL_LIBRARY_RELEASE%

pushd .

REM Include build directory info
call _directories.bat
REM Check if build is needed
set __BUILD_PATH=%CURL_BUILD_PATH%
set __BUILD_VERSION=%CURL_CHECK_VERSION%
call fnc_check_build.bat
if %ERRORLEVEL% EQU 0 (
	echo cURL already built, skipping cURL build step
	goto end
)
REM Check end

cd %CURL_ROOT%\projects\Windows\VC14

msbuild curl-all.sln /t:libcurl /p:Platform=x64 "/p:Configuration=%CURL_BUILD_CONFIG_RELEASE%"
set BUILD_RESULT_1=%ERRORLEVEL%
msbuild curl-all.sln /t:libcurl /p:Platform=x64 "/p:Configuration=%CURL_BUILD_CONFIG_DEBUG%"
set BUILD_RESULT_2=%ERRORLEVEL%

set /a "BUILD_RESULT=%BUILD_RESULT_1%+%BUILD_RESULT_2%"

echo cURL build result: %BUILD_RESULT% ( %BUILD_RESULT_1%, %BUILD_RESULT_2% )

cd %CURL_BUILD_DIR_RELEASE%
copy libcurl.* curl.*
del curl.dll

popd

REM Update version info
if %BUILD_RESULT% EQU 0 (
	set __BUILD_VERSION=%CURL_CHECK_VERSION%
	call fnc_set_build.bat
)


goto end
:novar
echo Required environment variables not set
echo Run setdev.bat to setup the required parameters
exit /B 2
:end
exit /B 0