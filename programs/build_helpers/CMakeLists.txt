FIND_PACKAGE(Python 3.6 COMPONENTS Interpreter)

get_target_property(GRAPHENE_CHAIN_SOURCE graphene_chain SOURCE_DIR)
get_target_property(GRAPHENE_CHAIN_BIN graphene_chain BINARY_DIR)

if ( GENERATING_HELPERS_USE_PYTHON )
  MESSAGE( STATUS "cat-parts: using Python to generate hardfork header file")
  SET( CAT_PARTS_TARGET_NAME cat-parts-cpp )

  add_custom_target(cat-parts)

  add_custom_target( build_hardfork_hpp
    COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/cat-parts.py" "${GRAPHENE_CHAIN_SOURCE}/hardfork.d" "${GRAPHENE_CHAIN_BIN}/include/graphene/chain/hardfork.hpp" )
else()
  MESSAGE( STATUS "cat-parts: using C++ to generate hardfork header file")
  SET( CAT_PARTS_TARGET_NAME cat-parts )

  add_custom_target( build_hardfork_hpp
    COMMAND cat-parts "${GRAPHENE_CHAIN_SOURCE}/hardfork.d" "${GRAPHENE_CHAIN_BIN}/include/graphene/chain/hardfork.hpp" )
endif()

add_dependencies( build_hardfork_hpp cat-parts )

add_executable( ${CAT_PARTS_TARGET_NAME} cat-parts.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

# we only actually need Boost, but link against FC for now so we don't duplicate it.
target_link_libraries( ${CAT_PARTS_TARGET_NAME} PRIVATE fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

add_executable( member_enumerator member_enumerator.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

# we only actually need Boost, but link against FC for now so we don't duplicate it.
target_link_libraries( member_enumerator PRIVATE fc graphene_app graphene_net graphene_chain graphene_egenesis_brief graphene_utilities graphene_wallet ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

