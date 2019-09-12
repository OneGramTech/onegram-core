file(GLOB HEADERS "include/graphene/protocol/*.hpp")

list(APPEND SOURCES account.cpp
                    assert.cpp
                    asset_ops.cpp
                    block.cpp
                    confidential.cpp
                    chain_parameters.cpp
                    fee_schedule.cpp
                    operations_permissions.cpp
                    memo.cpp
                    proposal.cpp
                    transfer.cpp
                    vote.cpp
                    witness.cpp
                    address.cpp
                    asset.cpp
                    authority.cpp
                    special_authority.cpp
                    committee_member.cpp
                    custom.cpp
                    market.cpp
                    operations.cpp
                    pts_address.cpp
                    small_ops.cpp
                    transaction.cpp
                    types.cpp
                    withdraw_permission.cpp
                    worker.cpp
                    htlc.cpp)


add_library( graphene_protocol ${SOURCES} ${HEADERS} )
target_link_libraries( graphene_protocol fc )
target_include_directories( graphene_protocol PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/include" )

# OneGram specific change
#  - `operations_permissions` requires graphene_chain, graphene_db
#   - this is a contradiction to the BitShares guidelines, which state:
#       "Nothing in this directory should depend upon anything other than fc or other types defined
#	    in the protocol directory."
#   TODO: Refactor
#       - move `operations_permissions` from `protocol` to `chain` namespace
target_link_libraries( graphene_protocol graphene_chain )
target_include_directories( graphene_protocol PUBLIC "${PROJECT_SOURCE_DIR}/libraries/chain/include" )
target_include_directories( graphene_protocol PUBLIC "${PROJECT_SOURCE_DIR}/libraries/db/include" )

install( TARGETS
   graphene_protocol

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/graphene/protocol" )
