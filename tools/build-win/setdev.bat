@echo off

if not defined SCRIPT_ROOT (
	set SCRIPT_ROOT=%~dp0
)

if not defined DEV_ROOT (
	set "DEV_ROOT_GUESS=%SCRIPT_ROOT:\OneGramDev\=" & set "DEV_ROOT_GUESS_R=%"
)

if exist %DEV_ROOT_GUESS%\ (
	set DEV_ROOT=%DEV_ROOT_GUESS%
) else (
	set DEV_ROOT=C:\Dev
)

if not defined PROJECT_NAME (
    set PROJECT_NAME=OneGram
)

if exist _directories.bat (
    call _directories.bat

    if not defined BOOST_ROOT (
        if defined BOOST_BUILD_PATH (
            set BOOST_ROOT=%BOOST_BUILD_PATH%
        )
    )

    if not defined OPENSSL_ROOT (
        if defined OPENSSL_BUILD_PATH (
            set OPENSSL_ROOT=%OPENSSL_BUILD_PATH%
        )
    )

    if not defined CURL_ROOT (
        if defined CURL_BUILD_PATH (
            set CURL_ROOT=%CURL_BUILD_PATH%
        )
    )

    REM TODO: Add 'CMake' and 'nmake' to %PATH%
    REM  + check if not already in %PATH%

)

rem Default directories
if not defined BOOST_ROOT (
    set BOOST_ROOT=%DEV_ROOT%\boost_1_63_0
)

if not defined OPENSSL_ROOT (
    set OPENSSL_ROOT=%DEV_ROOT%\openssl-1.0.2o
)

if not defined CURL_ROOT (
    set CURL_ROOT=%DEV_ROOT%\curl-7.61.0
)
rem Default directories end


if not defined PROJECT_DIR (
    set PROJECT_DIR=%DEV_ROOT%\OneGramDev
)

if not defined SLN_DIR (
    set SLN_DIR=%PROJECT_DIR%\x64
)

if not defined BOOST_INCLUDEDIR (
	set BOOST_INCLUDEDIR=%BOOST_ROOT%
)

if not defined BOOST_LIBRARYDIR (
	set BOOST_LIBRARYDIR=%BOOST_ROOT%\stage\lib
)

if not defined OPENSSL_ROOT_DIR (
	set OPENSSL_ROOT_DIR=%OPENSSL_ROOT%
)

if not defined OPENSSL_LIBRARIES (
	set OPENSSL_LIBRARIES=%OPENSSL_ROOT%\lib
)

if not defined BUILD_THREADS_COUNT (
    if defined NUMBER_OF_PROCESSORS (
        set BUILD_THREADS_COUNT=%NUMBER_OF_PROCESSORS%
    )
    if not defined NUMBER_OF_PROCESSORS (
        set BUILD_THREADS_COUNT=4
    )
)


echo ============================
echo  Setting Development Environment
echo ============================
echo.
echo   Dev Root: %DEV_ROOT%
echo.
echo   Boost Root: %BOOST_ROOT%
echo.
echo   OpenSSL Root: %OPENSSL_ROOT%
echo.
echo   Curl Root: %CURL_ROOT%
echo.
echo Extra folders:
echo   Boost Include Directory: %BOOST_INCLUDEDIR%
echo   Boost Library Directory: %BOOST_LIBRARYDIR%
echo   OpenSSL Root Directory:  %OPENSSL_ROOT_DIR%
echo   OpenSSL Libraries:       %OPENSSL_LIBRARIES%
echo.

echo Setting up VS2015 environment...
call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64