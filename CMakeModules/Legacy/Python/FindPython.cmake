# Simple workaround to use `find_package(Python)` for older CMake than 3.12
#  - supports only Interpreter component

set(_PYTHON_PREFIX ${CMAKE_FIND_PACKAGE_NAME})

if (NOT (${CMAKE_FIND_PACKAGE_NAME}_FIND_COMPONENTS STREQUAL "Interpreter"))
    MESSAGE(FATAL_ERROR "Only 'Interpreter' component is supported")
endif ()

if (${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED)
    set(Python_FIND_IS_REQUIRED "REQUIRED")
endif ()

find_package(PythonInterp ${${_PYTHON_PREFIX}_FIND_VERSION} ${Python_FIND_IS_REQUIRED})

if (PYTHONINTERP_FOUND)
    set(${_PYTHON_PREFIX}_FOUND ON)
    set(${_PYTHON_PREFIX}_Interpreter_FOUND ON)
    set(${_PYTHON_PREFIX}_EXECUTABLE ${PYTHON_EXECUTABLE})
    set(${_PYTHON_PREFIX}_INTERPRETER_ID UNKNOWN-NOTFOUND)
else ()
    set(${_PYTHON_PREFIX}_FOUND OFF)
    set(${_PYTHON_PREFIX}_Interpreter_FOUND OFF)
    set(${_PYTHON_PREFIX}_EXECUTABLE "")
    set(${_PYTHON_PREFIX}_INTERPRETER_ID INTERPRETER-NOTFOUND)
endif ()

unset(_PYTHON_PREFIX)