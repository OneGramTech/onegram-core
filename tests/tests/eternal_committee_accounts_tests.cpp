#include <boost/test/unit_test.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/chain_property_object.hpp>
#include <graphene/chain/committee_member_object.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE(eternal_committee_tests, database_fixture)

BOOST_AUTO_TEST_CASE(test)
{
   try 
   {
      // create eternal committee account
      const auto committee_member_name = string("eternal-committee-member");
      auto committee_key = init_account_priv_key;
      auto committee_member_key = fc::ecc::private_key::generate();

      const auto& account = create_account(committee_member_name, committee_member_key.get_public_key());
      upgrade_to_lifetime_member(account);
      auto eternal_committee_account = create_committee_member(account);

      // create account without eternal voting account
      const auto& without_eternal_committee = create_account("without-eternal-committee", public_key_type());

      // validate votes => must not contain the eternal committee acount vote id
      const auto& votes_without_eternal = without_eternal_committee.options.votes;
      BOOST_CHECK(votes_without_eternal.find(eternal_committee_account.vote_id) == votes_without_eternal.end());

      auto global_eternal_committee_members = db.get_chain_properties().immutable_parameters.eternal_committee_members;

      BOOST_CHECK(global_eternal_committee_members.account_names.empty() == true);

      // include eternal_committee_account as the eternal voting account
      db.modify(chain_property_id_type()(db), [&](chain_property_object &cpo)
      {
         cpo.immutable_parameters.eternal_committee_members.account_names.emplace(committee_member_name);
         cpo.eternal_committee_accounts.vote_ids.insert(eternal_committee_account.vote_id);
      });

      global_eternal_committee_members = db.get_chain_properties().immutable_parameters.eternal_committee_members;
      auto eternal_committee_accounts = db.get_chain_properties().eternal_committee_accounts;

      // validate that the eternal_committee_account is set as the eternal voting account
      BOOST_CHECK(global_eternal_committee_members.account_names.find(committee_member_name) != global_eternal_committee_members.account_names.end());
      BOOST_CHECK(eternal_committee_accounts.vote_ids.find(eternal_committee_account.vote_id) != eternal_committee_accounts.vote_ids.end());

      // create account including eternal committee members account as default voting accounts
      const auto& with_eternal_committee = create_account("with-eternal-committee", public_key_type());
      
      // validate votes => must contain the eternal committee acount vote id
      const auto& votes_with_eternal = with_eternal_committee.options.votes;
      BOOST_CHECK(votes_with_eternal.find(eternal_committee_account.vote_id) != votes_with_eternal.end());

      // update votes => must contain eternal as default + user's custom votes
      {
         ACTORS((selfvoting));
         upgrade_to_lifetime_member(selfvoting_id);
         const committee_member_id_type another_committee_member = create_committee_member(selfvoting_id(db)).id; 
         transfer(account_id_type(), selfvoting_id, asset(100));

         account_update_operation op;
         op.account = selfvoting_id;
         op.new_options = selfvoting_id(db).options;
         op.new_options->voting_account = selfvoting_id;
         op.new_options->votes = flat_set<vote_id_type>{another_committee_member(db).vote_id};
         op.new_options->num_committee = 1;
         trx.operations.emplace_back(op);
         sign( trx, selfvoting_private_key );
         PUSH_TX( db, trx );
         trx.clear();

         // verify if the eternal committee is included in account's votes
         const account_object& another_committee_account = get_account("selfvoting"); 
         BOOST_CHECK(another_committee_account.options.votes.find(another_committee_member(db).vote_id) != another_committee_account.options.votes.end());
         BOOST_CHECK(another_committee_account.options.votes.find(eternal_committee_account.vote_id) != another_committee_account.options.votes.end());
      }
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
