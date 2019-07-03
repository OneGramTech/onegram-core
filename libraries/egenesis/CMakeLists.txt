
add_library( graphene_egenesis_none
   egenesis_none.cpp
   include/graphene/egenesis/egenesis.hpp
   )

target_link_libraries( graphene_egenesis_none graphene_chain fc )
target_include_directories( graphene_egenesis_none
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

FIND_PACKAGE(Python 3.2 COMPONENTS Interpreter)

set( embed_genesis_args
   -t "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_brief.cpp.tmpl---${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp"
   -t "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp.tmpl---${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp"
   )

if( GRAPHENE_EGENESIS_JSON )
   list( APPEND embed_genesis_args --genesis-json "${GRAPHENE_EGENESIS_JSON}" )
endif( GRAPHENE_EGENESIS_JSON )

if ( GENERATING_HELPERS_USE_PYTHON )
   MESSAGE( STATUS "embed_genesis: using Python to generate genesis header files" )

   SET( EMBED_GENESIS_TARGET_NAME embed_genesis_cpp )

   add_custom_target( embed_genesis )

   add_custom_command(
      OUTPUT
         "${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp"
         "${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp"
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/embed_genesis.py" ${embed_genesis_args}
      DEPENDS
         "${GRAPHENE_EGENESIS_JSON}"
         "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_brief.cpp.tmpl"
         "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp.tmpl"
      embed_genesis )
else()
   MESSAGE( STATUS "embed_genesis: using C++ to generate genesis header files" )

   SET( EMBED_GENESIS_TARGET_NAME embed_genesis )

   add_custom_command(
      OUTPUT
         "${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp"
         "${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp"
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND embed_genesis ${embed_genesis_args}
      DEPENDS
         "${GRAPHENE_EGENESIS_JSON}"
         "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_brief.cpp.tmpl"
         "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp.tmpl"
      embed_genesis )
endif()

add_executable( ${EMBED_GENESIS_TARGET_NAME} embed_genesis.cpp )

target_link_libraries( ${EMBED_GENESIS_TARGET_NAME} graphene_chain graphene_app graphene_egenesis_none fc )

MESSAGE( STATUS "egenesis: " ${GRAPHENE_EGENESIS_JSON} )

MESSAGE( STATUS "embed_genesis_args: " ${embed_genesis_args} )

add_library( graphene_egenesis_brief "${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp" include/graphene/egenesis/egenesis.hpp )
add_library( graphene_egenesis_full  "${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp" include/graphene/egenesis/egenesis.hpp )

target_link_libraries( graphene_egenesis_brief graphene_chain fc )
target_link_libraries( graphene_egenesis_full graphene_chain fc )

target_include_directories( graphene_egenesis_brief
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
target_include_directories( graphene_egenesis_full
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

SET( INSTALL_TARGETS )
if( NOT GENERATING_HELPERS_USE_PYTHON )
   LIST( APPEND INSTALL_TARGETS embed_genesis )
endif()
LIST( APPEND INSTALL_TARGETS graphene_egenesis_none graphene_egenesis_brief graphene_egenesis_full )

INSTALL( TARGETS
   ${INSTALL_TARGETS}

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)