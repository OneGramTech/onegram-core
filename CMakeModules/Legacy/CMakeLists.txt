# If you are running recent enough version of CMake you shall consider removing these lines ...
MESSAGE(STATUS "Configuring legacy build scripts for main project")

if ((${CMAKE_VERSION} VERSION_LESS "3.12.0") OR (${USE_BUNDLED_FIND_PYTHON}))
    MESSAGE(STATUS "Configuring for legacy FindPython (CMake version ${CMAKE_VERSION} older than 3.12)")

    # CMake 3.12 comes with bundled FindPython routines
    # For older versions use FindPython-FindPythonInterp wrapper

    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Python")

    # Update Module Path for parent scope
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)
endif ()

if (${CMAKE_VERSION} VERSION_LESS "3.4.0")
    MESSAGE(STATUS "Configuring for legacy CMake (CMake version ${CMAKE_VERSION} older than 3.4.0)")
    MESSAGE(STATUS "  Target propertes not supported (SOURCE_DIR, BINARY_DIR)")

    set(GRAPHENE_CHAIN_BIN_LEGACY "${PROJECT_BINARY_DIR}/libraries/chain" PARENT_SCOPE)
    set(GRAPHENE_CHAIN_SOURCE_LEGACY "${PROJECT_SOURCE_DIR}/libraries/chain" PARENT_SCOPE)
endif ()
