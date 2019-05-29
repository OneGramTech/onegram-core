file(GLOB HEADERS "include/graphene/elasticsearch/*.hpp")

add_library( graphene_elasticsearch
        elasticsearch_plugin.cpp
           )

find_package( CURL REQUIRED )
include_directories(${CURL_INCLUDE_DIRS})

target_link_libraries( graphene_elasticsearch graphene_chain graphene_app ${CURL_LIBRARY} ${CURL_ADDITIONAL_LIBS} )
target_include_directories( graphene_elasticsearch
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" ${CURL_INCLUDE_DIR} )

if(MSVC)
  set_source_files_properties(elasticsearch_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

if(CURL_STATICLIB)
  SET_TARGET_PROPERTIES(graphene_elasticsearch PROPERTIES
  COMPILE_DEFINITIONS "CURL_STATICLIB")
endif(CURL_STATICLIB)


install( TARGETS
   graphene_elasticsearch

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/graphene/elasticsearch" )

