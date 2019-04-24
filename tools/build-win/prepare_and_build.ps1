Import-Module BitsTransfer

# TODO: Error handling
# TODO: OneGramDev string const
# TODO: OpenSSL tar.gz fails to extract ( maybe problem with symbolic links )

function Expand-Archive($archive, $dest) {

    $pathToModule = ".\7Zip4Powershell\1.9.0\7Zip4PowerShell.psd1"

    if (-not (Get-Command Expand-7Zip -ErrorAction Ignore)) {
        Import-Module $pathToModule
    }    

    Expand-7Zip $archive $dest

    if ( $archive -like "*.tar.gz" ) {
        # last Expand-Archive created .tar
        $subarchive = [System.IO.Path]::GetDirectoryName($dest+"\") + "\" + [System.IO.Path]::GetFileNameWithoutExtension($archive)

        Expand-7Zip $subarchive $dest

        Remove-Item -Path $subarchive -Force
    }
}

$BUILD_ROOT = Resolve-Path -Path .

# Gues OneGramDev Root
$GUESS_BUILD_ROOT = $BUILD_ROOT -replace "\\OneGramDev\b.*","\"

if ( !($BUILD_ROOT -eq $GUESS_BUILD_ROOT) ) {
    if ( Test-Path -Path $GUESS_BUILD_ROOT ) {
        $BUILD_ROOT = $GUESS_BUILD_ROOT
    } 
}

$BOOST_VERSION = "1.65.1"
$OPENSSL_VERSION = "1.1.1b"
#$OPENSSL_VERSION = "1.1.0j"
$CURL_VERSION = "7.64.1"
$CMAKE_VERSION = "3.10.3"
$CMAKE_SHORT_VERSION = "3.10"
$NASM_VERSION = "2.14.02"

$DLPREFIX = "Downloads"

$BOOST_VERSION_US = $BOOST_VERSION -replace "\.","_"

$BOOST_NAME = "boost_$BOOST_VERSION_US"
$OPENSSL_NAME = "openssl-$OPENSSL_VERSION"
$CURL_NAME = "curl-$CURL_VERSION"
$CMAKE_NAME = "cmake-$CMAKE_VERSION-win32-x86"
$NASM_NAME = "nasm-$NASM_VERSION"

$BOOST_FILENAME = "$BOOST_NAME.7z"
$OPENSSL_FILENAME = "$OPENSSL_NAME.tar.gz"
$CURL_FILENAME = "$CURL_NAME.zip"
$CMAKE_FILENAME = "$CMAKE_NAME.zip"
$NASM_FILENAME = "$NASM_NAME-win64.zip"

# https://sourceforge.net/projects/boost/files/boost/1.63.0/boost_1_63_0.zip/download
# https://www.openssl.org/source/openssl-1.0.2o.tar.gz
# https://curl.haxx.se/download/curl-7.61.0.zip
# https://cmake.org/files/v3.1/cmake-3.1.3-win32-x86.zip

$BOOST_URL = "https://sourceforge.net/projects/boost/files/boost/$BOOST_VERSION/$BOOST_FILENAME/download"
$OPENNSL_URL = "https://www.openssl.org/source/$OPENSSL_FILENAME"
$CURL_URL = "https://curl.haxx.se/download/$CURL_FILENAME"
$CMAKE_URL = "https://cmake.org/files/v$CMAKE_SHORT_VERSION/$CMAKE_FILENAME"
$NASM_URL = "https://www.nasm.us/pub/nasm/releasebuilds/$NASM_VERSION/win64/$NASM_FILENAME"

$BOOST_FILEDEST = "$BUILD_ROOT\$DLPREFIX\$BOOST_FILENAME"
$OPENSSL_FILEDEST = "$BUILD_ROOT\$DLPREFIX\$OPENSSL_FILENAME"
$CURL_FILEDEST = "$BUILD_ROOT\$DLPREFIX\$CURL_FILENAME"
$CMAKE_FILEDEST = "$BUILD_ROOT\$DLPREFIX\$CMAKE_FILENAME"
$NASM_FILEDEST = "$BUILD_ROOT\$DLPREFIX\$NASM_FILENAME"

if (!(Test-Path "$BUILD_ROOT\$DLPREFIX" )) {
    new-item "$BUILD_ROOT\$DLPREFIX" -itemtype directory
}

if (!(Test-Path "$BUILD_ROOT\$BOOST_NAME" ) -And !( Test-Path $BOOST_FILEDEST )) {
    Start-BitsTransfer -Source $BOOST_URL -Destination $BOOST_FILEDEST
}

if (!(Test-Path "$BUILD_ROOT\$OPENSSL_NAME") -And !(Test-Path $OPENSSL_FILEDEST)) {
    Start-BitsTransfer -Source $OPENNSL_URL -Destination $OPENSSL_FILEDEST
}

if (!(Test-Path "$BUILD_ROOT\$CURL_NAME" ) -And !(Test-Path $CURL_FILEDEST )) {
    Start-BitsTransfer -Source $CURL_URL -Destination $CURL_FILEDEST
}

if (!(Test-Path "$BUILD_ROOT\$CMAKE_NAME" ) -And !(Test-Path $CMAKE_FILEDEST )) {
    Start-BitsTransfer -Source $CMAKE_URL -Destination $CMAKE_FILEDEST
}

if (!(Test-Path "$BUILD_ROOT\$NASM_NAME" ) -And !(Test-Path $NASM_FILEDEST )) {
    Start-BitsTransfer -Source $NASM_URL -Destination $NASM_FILEDEST
}

# Initialize Directories

if (!(Test-Path "$BUILD_ROOT\$BOOST_NAME" )) {    
    Expand-Archive $BOOST_FILEDEST $BUILD_ROOT
}

if (!(Test-Path "$BUILD_ROOT\$OPENSSL_NAME" )) {    
    Expand-Archive $OPENSSL_FILEDEST $BUILD_ROOT
}

if (!(Test-Path "$BUILD_ROOT\$CURL_NAME" )) {    
    Expand-Archive $CURL_FILEDEST $BUILD_ROOT
}

if (!(Test-Path "$BUILD_ROOT\$CMAKE_NAME" )) {    
    Expand-Archive $CMAKE_FILEDEST $BUILD_ROOT    
}

if (!(Test-Path "$BUILD_ROOT\$NASM_NAME" )) {    
    Expand-Archive $NASM_FILEDEST $BUILD_ROOT
}

# add CMake and NASM to path
$ENV:PATH = "$BUILD_ROOT\$CMAKE_NAME\bin;$BUILD_ROOT\$NASM_NAME\;" + $ENV:PATH

$ENV:BOOST_ROOT = Resolve-Path -Path "$BUILD_ROOT\$BOOST_NAME"
$ENV:OPENSSL_ROOT = Resolve-Path -Path "$BUILD_ROOT\$OPENSSL_NAME"
$ENV:CURL_ROOT = Resolve-Path -Path "$BUILD_ROOT\$CURL_NAME"
#$ENV:CURL_BUILD_WITH_SSPI = "TRUE"

# store directory info
"@echo off`r`n`r`n" | Out-File _directories.bat -encoding ascii
"set BOOST_BUILD_NAME=$BOOST_NAME`r`nset OPENSSL_BUILD_NAME=$OPENSSL_NAME`r`nset CURL_BUILD_NAME=$CURL_NAME`r`nset CMAKE_BUILD_NAME=$CMAKE_NAME`r`nset NASM_NAME=$NASM_NAME`r`n" | Out-File _directories.bat -encoding ascii -Append
"set BOOST_BUILD_PATH=$ENV:BOOST_ROOT`r`nset OPENSSL_BUILD_PATH=$ENV:OPENSSL_ROOT`r`nset CURL_BUILD_PATH=$ENV:CURL_ROOT`r`nset CMAKE_BUILD_PATH=$BUILD_ROOT\$CMAKE_NAME`r`nset NASM_BUILD_PATH=$BUILD_ROOT\$NASM_NAME`r`n" | Out-File _directories.bat -encoding ascii -Append

$PROJECT_NAME = "BitShares"

$SLN_DIR = "$BUILD_ROOT\$PROJECT_NAME-build-x64-win"

if (!(Test-Path $SLN_DIR )) {
    new-item $SLN_DIR -itemtype directory    
}

$SLN_DIR = Resolve-Path -Path $SLN_DIR

$ENV:SLN_DIR = Resolve-Path -Path $SLN_DIR
$ENV:PROJECT_NAME = "$PROJECT_NAME"


# all downloaded, build it baby!

& .\build.bat