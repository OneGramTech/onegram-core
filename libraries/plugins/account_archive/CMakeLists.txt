file(GLOB HEADERS "include/graphene/account_archive/*.hpp")

add_library( graphene_account_archive 
             account_archive_plugin.cpp
             operation_database.cpp
           )

target_link_libraries( graphene_account_archive graphene_chain graphene_app )
target_include_directories( graphene_account_archive
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( account_archive_plugin.cpp operation_database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   graphene_account_archive

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/graphene/account_archive" )
