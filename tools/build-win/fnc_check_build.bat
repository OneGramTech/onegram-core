@echo off

if not defined __BUILD_PATH goto novar
if not defined __BUILD_VERSION goto novar

set __CHECK_BUILD_VERSION=%__BUILD_VERSION%

call fnc_get_build.bat
echo Checking "%__BUILD_PATH%" for version info

echo Found version: %__BUILD_VERSION: =% ( return: %ERRORLEVEL% ), check against: %__CHECK_BUILD_VERSION: =%

if %ERRORLEVEL% NEQ 0 (
	goto end_false
)

if /I %__BUILD_VERSION: =% == %__CHECK_BUILD_VERSION: =% (	
	goto end
)

echo Version does not equal
goto end_false


:novar
echo [Check-build] Version string to compare is not set, make sure \%__BUILD_VERSION\% is set
EXIT /B 1
:end_false
set "__CHECK_BUILD_VERSION="
EXIT /B 1
:end
set "__CHECK_BUILD_VERSION="
EXIT /B 0