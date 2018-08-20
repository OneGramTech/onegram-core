@echo off

REM Check if required variables are set, exit if not
if not defined BUILD_THREADS_COUNT goto novar
if not defined BOOST_ROOT goto novar

echo.
echo   Boost Root: %BOOST_ROOT%
echo.

cd %BOOST_ROOT%
if not exist b2.exe (    
    START /B /WAIT cmd /c bootstrap.bat
) else (
	echo No need to build Boost Build Engine [b2]
)
echo Building Boost...
b2.exe address-model=64 -j%BUILD_THREADS_COUNT%

goto end
:novar
echo Required environment variables not set
echo Run setdev.bat to setup the required parameters
exit /B 2
:end
