@echo off

if not defined __BUILD_PATH goto novar

set "__GET__BUILD_VERSION="

pushd .

cd %__BUILD_PATH%
if not exist version_build goto nofile
set /p __GET__BUILD_VERSION=<version_build

popd

set __BUILD_VERSION=%__GET__BUILD_VERSION%

goto end


:novar
echo Function "set_build" not executed, invalid parameter
:nofile
set __GET__BUILD_VERSION=UNKNOWN_VERSION
set __BUILD_VERSION=%__GET__BUILD_VERSION%
echo Version file not found
EXIT /B 1
:end
EXIT /B 0