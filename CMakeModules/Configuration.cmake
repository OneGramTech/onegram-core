set (TARGET_ENV "TESTNET" CACHE STRING "target build MAINNET or TESTNET (default)")

if (NOT ((TARGET_ENV STREQUAL "MAINNET") OR (TARGET_ENV STREQUAL "TESTNET")))
	message (FATAL_ERROR "TARGET_ENV is ${TARGET_ENV}, valid values are MAINNET or TESTNET")
endif ()

message ("TARGET_ENV is ${TARGET_ENV}")

if (TARGET_ENV STREQUAL "MAINNET")
	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/mainnet.genesis.json.in"
		"${CMAKE_CURRENT_BINARY_DIR}/runtime/genesis.json"
		IMMEDIATE @ONLY)
		
	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/mainnet.seed-nodes.h.in"
		"${CMAKE_CURRENT_BINARY_DIR}/runtime/seed-nodes.h"
		IMMEDIATE @ONLY)
else()
	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/testnet.genesis.json.in"
		"${CMAKE_CURRENT_BINARY_DIR}/runtime/genesis.json"
		IMMEDIATE @ONLY)
		
	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/testnet.seed-nodes.h.in"
		"${CMAKE_CURRENT_BINARY_DIR}/runtime/seed-nodes.h"
		IMMEDIATE @ONLY)
endif()
