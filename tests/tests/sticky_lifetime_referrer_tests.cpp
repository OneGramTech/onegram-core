#include <boost/test/unit_test.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/chain_property_object.hpp>
#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE(sticky_lifetime_referrer_tests, database_fixture)

BOOST_AUTO_TEST_CASE(test)
{
   try 
   {
      string lifetimeReferrerName("lifetime-referrer");

      // create registrar account and update to LTM
      const auto& registrar_account = create_account("registrar", public_key_type());
      upgrade_to_lifetime_member(registrar_account);

      // create lifetime referrer account and update to LTM
      const auto& lifetime_referrer_account = create_account(lifetimeReferrerName, public_key_type());
      upgrade_to_lifetime_member(lifetime_referrer_account);
      BOOST_CHECK(lifetime_referrer_account.lifetime_referrer == lifetime_referrer_account.get_id());

      // set lifetime_refferer account as sticky_lifetime_referrer
      db.modify(chain_property_id_type()(db), [&](chain_property_object &cpo)
      {
         cpo.immutable_parameters.sticky_lifetime_referrers.referrer_names.emplace(lifetimeReferrerName);
         cpo.sticky_lifetime_referers.referrer_ids.insert(lifetime_referrer_account.id);
      });

      BOOST_CHECK(db.get_chain_properties().immutable_parameters.sticky_lifetime_referrers.contains(lifetimeReferrerName));
      
      // validate alice account => lifetime_referrer is changed after account upgrade to LTM
      const auto& alice_account = create_account("alice", public_key_type());
      BOOST_CHECK(alice_account.lifetime_referrer == account_id_type());
      upgrade_to_lifetime_member(alice_account);
      BOOST_CHECK(alice_account.lifetime_referrer == alice_account.get_id());

      // validate nathan account => lifetime_referrer sticks to lifetime_referrer_account after upgrade to LTM
      const auto& nathan_account = create_account("nathan", registrar_account, lifetime_referrer_account, 100, public_key_type());
      BOOST_CHECK(nathan_account.lifetime_referrer == lifetime_referrer_account.get_id());
      upgrade_to_lifetime_member(nathan_account);
      BOOST_CHECK(nathan_account.lifetime_referrer == lifetime_referrer_account.get_id());

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(basic_sticky_lifetime_referrer_tests)

BOOST_AUTO_TEST_CASE(check_serialization)
{
   sticky_lifetime_referrers_type accounts;
   auto json = fc::json::to_string(accounts);
   BOOST_CHECK_EQUAL("{\"referrer_names\":[]}", json);

   accounts.referrer_names.emplace("nathan");
   accounts.referrer_names.emplace("ltm");
   json = fc::json::to_string(accounts);
   BOOST_CHECK_EQUAL("{\"referrer_names\":[\"ltm\",\"nathan\"]}", json);

   auto from_json = fc::json::from_string(json).as<sticky_lifetime_referrers_type>();
   BOOST_CHECK(from_json.referrer_names == accounts.referrer_names);
}

BOOST_AUTO_TEST_CASE(when_referrer_names_empty_contains_false)
{
   sticky_lifetime_referrers_type accounts;

   BOOST_CHECK(accounts.referrer_names.empty());
   BOOST_CHECK(!accounts.contains("nathan"));
}

BOOST_AUTO_TEST_CASE(when_referrer_names_contains_true)
{
   sticky_lifetime_referrers_type accounts;
   accounts.referrer_names.emplace("nathan");

   BOOST_CHECK(accounts.contains("nathan"));
   BOOST_CHECK(!accounts.contains("ltm"));
}

BOOST_AUTO_TEST_SUITE_END()