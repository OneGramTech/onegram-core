@echo off

if not defined __BUILD_PATH goto novar
if not defined __BUILD_VERSION goto novar

pushd .

echo Setting build version info ...
if  "%__BUILD_VERSION%" == "UNKNOWN_VERSION" (
	echo UNKNOWN_VERSION cannot be stored
	goto end
)
cd %__BUILD_PATH%
echo %__BUILD_VERSION% > version_build

popd

goto end


:novar
echo Function "set_build" not executed, invalid parameter
EXIT /B 1
:end
EXIT /B 0