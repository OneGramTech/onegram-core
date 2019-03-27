@echo off

REM Check if required variables are set, exit if not
if not defined BUILD_THREADS_COUNT goto novar
if not defined BOOST_ROOT goto novar

echo.
echo   Boost Root: %BOOST_ROOT%
echo.

pushd .

REM Include build directory info
call _directories.bat
REM Check if build is needed
set __BUILD_PATH=%BOOST_BUILD_PATH%
set __BUILD_VERSION=%BOOST_BUILD_NAME%
call fnc_check_build.bat
if %ERRORLEVEL% EQU 0 (
	echo Boost already built, skipping boost build step
	goto end
)
REM Check end

cd %BOOST_ROOT%
if not exist b2.exe (    
    START /B /WAIT cmd /c bootstrap.bat
) else (
	echo No need to build Boost Build Engine [b2]
)
echo Building Boost...
b2.exe --toolset=msvc-14.0 address-model=64 --build-type=complete --without-python -j%BUILD_THREADS_COUNT%
set BUILD_RESULT=%ERRORLEVEL%

popd

REM Update version info
if %BUILD_RESULT% EQU 0 (
	set __BUILD_VERSION=%BOOST_BUILD_NAME%
	call fnc_set_build.bat
)

goto end


:novar
echo Required environment variables not set
echo Run setdev.bat to setup the required parameters
exit /B 2
:end
exit /B 0