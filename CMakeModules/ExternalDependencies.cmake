include(FetchContent)

# Fetch onegram-fc external dependency which depends on target environment
if (TARGET_ENV STREQUAL "MAINNET")
    FetchContent_Declare (
	libFC
	GIT_REPOSITORY https://gitlab.com/onegram-developers/onegram-fc.git
	GIT_TAG 666585ba
    )
else ()
    FetchContent_Declare (
	libFC
	GIT_REPOSITORY https://gitlab.com/onegram-developers/onegram-fc.git
	GIT_TAG 666585ba
    )
endif ()

FetchContent_GetProperties(libFC)
if (NOT libFC_POPULATED)
	message ("Fetch onegram-fc library")
    FetchContent_Populate(libFC)
    add_subdirectory(${libfc_SOURCE_DIR} ${libfc_BINARY_DIR})
endif()

# Fetch docs external resource
FetchContent_Declare (
docs
GIT_REPOSITORY https://github.com/bitshares/bitshares-core.wiki.git
GIT_TAG 00bd507
)

FetchContent_GetProperties(docs)
if (NOT docs_POPULATED)
	message ("Fetch docs dependency")
    FetchContent_Populate(docs)
endif()
