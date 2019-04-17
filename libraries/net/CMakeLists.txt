if (TARGET_ENV STREQUAL "MAINNET")
	SET(ENABLE_P2P_DEBUGGING_API 0)
	SET(GRAPHENE_NET_TEST_P2P_PORT 18540)
	SET(GRAPHENE_NET_DEFAULT_P2P_PORT 8540)
	SET(GRAPHENE_NET_DEFAULT_DESIRED_CONNECTIONS 10)
	SET(GRAPHENE_NET_DEFAULT_MAX_CONNECTIONS 500)
else()
	SET(ENABLE_P2P_DEBUGGING_API 1)
	SET(GRAPHENE_NET_TEST_P2P_PORT 1700)
	SET(GRAPHENE_NET_DEFAULT_P2P_PORT 1776)
	SET(GRAPHENE_NET_DEFAULT_DESIRED_CONNECTIONS 20)
	SET(GRAPHENE_NET_DEFAULT_MAX_CONNECTIONS 200)
endif()

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/include/graphene/net/config.hpp.in"
	"${CMAKE_CURRENT_BINARY_DIR}/include/graphene/net/config.hpp"
	IMMEDIATE @ONLY)

file(GLOB HEADERS "include/graphene/net/*.hpp")

set(SOURCES node.cpp
            stcp_socket.cpp
            core_messages.cpp
            peer_database.cpp
            peer_connection.cpp
            message_oriented_connection.cpp)

add_library( graphene_net ${SOURCES} ${HEADERS} )

target_link_libraries( graphene_net 
  PUBLIC fc graphene_db )

target_include_directories( graphene_net 
  PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	"${CMAKE_CURRENT_BINARY_DIR}/include"
  PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/../chain/include"
)

if(MSVC)
  set_source_files_properties( node.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

if (USE_PCH)
  set_target_properties(graphene_net PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
  cotire(graphene_net)
endif(USE_PCH)

install( TARGETS
   graphene_net

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/graphene/net" )
