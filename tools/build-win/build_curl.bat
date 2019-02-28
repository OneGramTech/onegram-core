@echo off

REM Check if required variables are set, exit if not
if not defined CURL_ROOT goto novar


echo Setting up CURL Development Environment
echo.
echo   CUrl Root: %CURL_ROOT%
echo.

set CURL_INCLUDE_DIR=%CURL_ROOT%\include\

set CURL_BUILD_DIR=%CURL_ROOT%\build\Win64\VC14\

set CURL_BUILD_DIR_RELEASE=%CURL_BUILD_DIR%\DLL Release - DLL Windows SSPI\
set CURL_BUILD_DIR_DEBUG=%CURL_BUILD_DIR%\DLL Debug - DLL Windows SSPI\

set CURL_LIBRARY_RELEASE=%CURL_BUILD_DIR_RELEASE%libcurl.lib
set CURL_LIBRARY_DEBUG=%CURL_BUILD_DIR_DEBUG%libcurld.lib
set CURL_LIBRARY=%CURL_LIBRARY_RELEASE%

pushd .

REM Include build directory info
call _directories.bat
REM Check if build is needed
set __BUILD_PATH=%CURL_BUILD_PATH%
set __BUILD_VERSION=%CURL_BUILD_NAME%
call fnc_check_build.bat
if %ERRORLEVEL% EQU 0 (
	echo cURL already built, skipping cURL build step
	goto end
)
REM Check end

cd %CURL_ROOT%\projects\Windows\VC14

msbuild curl-all.sln /p:Platform=x64 "/p:Configuration=LIB Release - DLL Windows SSPI"
set BUILD_RESULT_1=%ERRORLEVEL%
msbuild curl-all.sln /p:Platform=x64 "/p:Configuration=LIB Debug - DLL Windows SSPI"
set BUILD_RESULT_2=%ERRORLEVEL%
msbuild curl-all.sln /p:Platform=x64 "/p:Configuration=DLL Release - DLL Windows SSPI"
set BUILD_RESULT_3=%ERRORLEVEL%
msbuild curl-all.sln /p:Platform=x64 "/p:Configuration=DLL Debug - DLL Windows SSPI"
set BUILD_RESULT_4=%ERRORLEVEL%

set /a "BUILD_RESULT=%BUILD_RESULT_1%+%BUILD_RESULT_2%+%BUILD_RESULT_3%+%BUILD_RESULT_4%"

echo cURL build result: %BUILD_RESULT% ( %BUILD_RESULT_1%, %BUILD_RESULT_2%, %BUILD_RESULT_3%, %BUILD_RESULT_4% )

cd %CURL_BUILD_DIR_RELEASE%
copy libcurl.* curl.*
del curl.dll

popd

REM Update version info
if %BUILD_RESULT% EQU 0 (
	set __BUILD_VERSION=%CURL_BUILD_NAME%
	call fnc_set_build.bat
)


goto end
:novar
echo Required environment variables not set
echo Run setdev.bat to setup the required parameters
exit /B 2
:end
exit /B 0