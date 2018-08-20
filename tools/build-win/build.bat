@echo off

echo Batch build started...

call setdev.bat

cd %SCRIPT_ROOT%
call build_boost.bat

cd %SCRIPT_ROOT%
call build_openssl.bat

cd %SCRIPT_ROOT%
call build_curl.bat

cd %SCRIPT_ROOT%
call build_cmake.bat

cd %SCRIPT_ROOT%
call build_sln.bat