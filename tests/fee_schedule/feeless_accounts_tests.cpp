//
// Created by Lubos Ilcik on 19/02/2018.
//

#include <boost/test/unit_test.hpp>

#include <fc/io/json.hpp>
#include <graphene/chain/feeless_accounts.hpp>
#include <graphene/chain/immutable_chain_parameters.hpp>

using namespace graphene::chain;

BOOST_AUTO_TEST_SUITE(feeless_accounts_tests)

BOOST_AUTO_TEST_CASE(check_serialization)
{
   feeless_accounts_type accounts;
   auto json = fc::json::to_string(accounts);
   BOOST_CHECK_EQUAL("{\"account_names\":[]}", json);

   accounts.account_names.emplace("nathan");
   accounts.account_names.emplace("ltm");
   json = fc::json::to_string(accounts);
   BOOST_CHECK_EQUAL("{\"account_names\":[\"ltm\",\"nathan\"]}", json);

   auto from_json = fc::json::from_string(json).as<feeless_accounts_type>(20); // TODO: magic number?!
   BOOST_CHECK(from_json.account_names == accounts.account_names);
}

BOOST_AUTO_TEST_CASE(when_feeless_accounts_empty_contains_false)
{
   feeless_accounts_type accounts;

   BOOST_CHECK(!accounts.contains("nathan"));
}

BOOST_AUTO_TEST_CASE(when_feeless_accounts_contains_true)
{
   feeless_accounts_type accounts;
   accounts.account_names.emplace("nathan");

   BOOST_CHECK(accounts.contains("nathan"));
   BOOST_CHECK(!accounts.contains("ltm"));
}

BOOST_AUTO_TEST_CASE(check_container_serialization)
{
   immutable_chain_parameters parameters;
   parameters.min_committee_member_count = 11;
   parameters.min_witness_count = 11;
   parameters.num_special_accounts = 0;
   parameters.num_special_assets = 0;

   auto json = fc::json::to_string(parameters);
   BOOST_CHECK_EQUAL("{\"min_committee_member_count\":11,\"min_witness_count\":11,\"num_special_accounts\":0,\"num_special_assets\":0,\"feeless_accounts\":{\"account_names\":[]},\"eternal_committee_members\":{\"account_names\":[]},\"sticky_lifetime_referrers\":{\"referrer_names\":[]}}", json);

   parameters.feeless_accounts.account_names.emplace("nathan");
   parameters.feeless_accounts.account_names.emplace("ltm1");
   parameters.feeless_accounts.account_names.emplace("ltm1"); // account_names guarantee uniqueness
   parameters.feeless_accounts.account_names.emplace("ltm2");
   json = fc::json::to_string(parameters);
   BOOST_CHECK_EQUAL("{\"min_committee_member_count\":11,\"min_witness_count\":11,\"num_special_accounts\":0,\"num_special_assets\":0,\"feeless_accounts\":{\"account_names\":[\"ltm1\",\"ltm2\",\"nathan\"]},\"eternal_committee_members\":{\"account_names\":[]},\"sticky_lifetime_referrers\":{\"referrer_names\":[]}}", json);

   parameters.enforced_lifetime_referrer = "enforced_ltm";
   json = fc::json::to_string(parameters);
   BOOST_CHECK_EQUAL("{\"min_committee_member_count\":11,\"min_witness_count\":11,\"num_special_accounts\":0,\"num_special_assets\":0,\"feeless_accounts\":{\"account_names\":[\"ltm1\",\"ltm2\",\"nathan\"]},\"eternal_committee_members\":{\"account_names\":[]},\"sticky_lifetime_referrers\":{\"referrer_names\":[]},\"enforced_lifetime_referrer\":\"enforced_ltm\"}", json);
}

BOOST_AUTO_TEST_SUITE_END()