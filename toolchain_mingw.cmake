# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)

# toolchain binaries
SET(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
SET(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

# here is the target environment located
# SET(CMAKE_FIND_ROOT_PATH  /usr/x86_64-w64-mingw32/ ...other_library_directories... )

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)


# include(GNUInstallDirs)

# example path settings

#set( BOOST_INCLUDEDIR="..../boost_1_65_1/" )
#set( OPENSSL_INCLUDE_DIR="..../openssl-1.0.2q/include" )
#set( OPENSSL_SSL_LIBRARY="..../openssl-1.0.2q/libssl.a" )
#set( OPENSSL_CRYPTO_LIBRARY="..../openssl-1.0.2q/libcrypto.a" )
#set( ZLIB_ROOT="..../zlib/" )
#set( CURL_LIBRARY="..../curl/lib/libcurl.a" )
#set( CURL_INCLUDE_DIR="..../curl-install/include/" )