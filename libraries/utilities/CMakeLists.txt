set (REVISION_SHA "" CACHE STRING "git revision sha")
set (REVISION_TIMESTAMP "" CACHE STRING "git revision unix timestamp")
set (REVISION_DESCRIPTION "" CACHE STRING "git revision description")

if(REVISION_SHA STREQUAL "" OR REVISION_TIMESTAMP STREQUAL "")
	get_property(FC_ROOT_PATH GLOBAL PROPERTY FC_ROOT_PATH)
	set(FC_GIT_VERSION_GEN_PATH ${FC_ROOT_PATH}/GitVersionGen)
    list( APPEND CMAKE_MODULE_PATH ${FC_GIT_VERSION_GEN_PATH} )
    include( GetGitRevisionDescription )

	if(REVISION_SHA STREQUAL "")
		get_git_head_revision(GIT_REFSPEC REVISION_SHA)
	endif()

	if(REVISION_TIMESTAMP STREQUAL "")
		get_git_unix_timestamp(REVISION_TIMESTAMP)
	endif()

	if(REVISION_DESCRIPTION STREQUAL "")
		git_describe(REVISION_DESCRIPTION --tags)
	endif()
endif()

if(REVISION_DESCRIPTION STREQUAL "")
	set(REVISION_DESCRIPTION "unknown")
endif()

file(GLOB HEADERS "include/graphene/utilities/*.hpp")

set(sources
   key_conversion.cpp
   string_escape.cpp
   tempdir.cpp
   words.cpp
   elasticsearch.cpp
   ${HEADERS})

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/git_revision.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp" @ONLY)
list(APPEND sources "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp")

add_library( graphene_utilities
             ${sources}
             ${HEADERS} )

find_package( CURL REQUIRED )

target_link_libraries( graphene_utilities fc )
target_include_directories( graphene_utilities
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" ${CURL_INCLUDE_DIR} )
if (USE_PCH)
  set_target_properties(graphene_utilities PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
  cotire(graphene_utilities)
endif(USE_PCH)

install( TARGETS
   graphene_utilities

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/graphene/utilities" )
