file(GLOB COMMON_SOURCES "common/*.cpp")

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling tests with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

file(GLOB UNIT_TESTS "tests/*.cpp")
add_executable( chain_test ${COMMON_SOURCES} ${UNIT_TESTS} )
target_link_libraries( chain_test graphene_chain graphene_app graphene_witness graphene_account_history graphene_elasticsearch graphene_es_objects graphene_egenesis_none fc graphene_wallet ${PLATFORM_SPECIFIC_LIBS} )

file(GLOB PERFORMANCE_TESTS "performance/*.cpp")
add_executable( performance_test ${COMMON_SOURCES} ${PERFORMANCE_TESTS} )
target_link_libraries( performance_test graphene_chain graphene_app graphene_account_history graphene_elasticsearch graphene_es_objects graphene_egenesis_none fc ${PLATFORM_SPECIFIC_LIBS} )

file(GLOB BENCH_MARKS "benchmarks/*.cpp")
add_executable( chain_bench ${COMMON_SOURCES} ${BENCH_MARKS} )
target_link_libraries( chain_bench graphene_chain graphene_app graphene_account_history graphene_elasticsearch graphene_es_objects graphene_egenesis_none fc ${PLATFORM_SPECIFIC_LIBS} )

file(GLOB APP_SOURCES "app/*.cpp")
add_executable( app_test ${APP_SOURCES} )
target_link_libraries( app_test graphene_app graphene_account_history graphene_net graphene_witness graphene_chain graphene_egenesis_none fc ${PLATFORM_SPECIFIC_LIBS} )

file(GLOB CLI_SOURCES "cli/*.cpp")
add_executable( cli_test ${CLI_SOURCES} )
target_link_libraries( cli_test graphene_app graphene_wallet graphene_witness graphene_account_history graphene_net graphene_chain graphene_egenesis_none fc ${PLATFORM_SPECIFIC_LIBS} )

file(GLOB ES_SOURCES "elasticsearch/*.cpp")
add_executable( es_test ${COMMON_SOURCES} ${ES_SOURCES} )
target_link_libraries( es_test graphene_chain graphene_app graphene_account_history graphene_elasticsearch graphene_es_objects graphene_egenesis_none fc ${PLATFORM_SPECIFIC_LIBS} )

file(GLOB FEE_SCHEDULE_SOURCES "fee_schedule/*.cpp")
add_executable( fee_schedule_test ${FEE_SCHEDULE_SOURCES} )
target_link_libraries( fee_schedule_test graphene_chain fc ${PLATFORM_SPECIFIC_LIBS} )

file(GLOB VESTING_BALANCE_SOURCES "vesting_balance/*.cpp")
add_executable( vesting_balance_test ${VESTING_BALANCE_SOURCES} )
target_link_libraries( vesting_balance_test graphene_chain fc ${PLATFORM_SPECIFIC_LIBS} )

add_subdirectory( generate_empty_blocks )

if(WIN32)
    list(APPEND PLATFORM_SPECIFIC_LIBS ws2_32)
endif()

if(MSVC)
	# Souce code level configuration
	set_source_files_properties(
			tests/serialization_tests.cpp
			tests/block_tests.cpp
			app/main.cpp
			common/database_fixture.cpp
			PROPERTIES COMPILE_FLAGS "/bigobj"
            cli/main.cpp
            PROPERTIES COMPILE_FLAGS "/bigobj")

	# Project level configruation
	set_target_properties(
		chain_test
		cli_test
		PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)