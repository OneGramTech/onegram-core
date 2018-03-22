//
// Created by Lubos Ilcik on 22/03/2018.
//

#include <boost/test/unit_test.hpp>
#include "../common/database_fixture.hpp"
#include <graphene/chain/chain_property_object.hpp>
using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE(enforced_lifetime_referrer_tests, database_fixture)

BOOST_AUTO_TEST_CASE(given_enforced_lifetime_referrer_when_account_is_created_its_lifetime_referrer_is_enforced)
{
   try {
      /// create test lifetime referrer account
      create_account("enforced-lt-referrer");
      const account_object& lifetime_referrer = get_account("enforced-lt-referrer");
      /// set the chain property
      db.modify(chain_property_id_type()(db), [&](chain_property_object &cpo) {
         cpo.enforced_lifetime_referrer = optional<account_id_type>(lifetime_referrer.id);
      });
      /// create account with given registrar and referrer
      const account_object& registrar = get_account("init0");
      const account_object& referrer = get_account("init1");
      create_account("alice", registrar, referrer);
      const account_object& alice = get_account("alice");

      BOOST_CHECK(alice.registrar == account_id_type(registrar.id));
      BOOST_CHECK(alice.referrer == account_id_type(referrer.id));
      BOOST_CHECK(alice.lifetime_referrer == account_id_type(lifetime_referrer.id));
      BOOST_CHECK(alice.lifetime_referrer != account_id_type(referrer.lifetime_referrer));

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(given_referrer_when_account_is_created_its_lifetime_referrer_is_referrer_lifetime_referrer)
{
   try {
      /// create account with given registrar and referrer
      const account_object& registrar = get_account("init0");
      const account_object& referrer = get_account("init1");
      create_account("alice", registrar, referrer);
      const account_object& alice = get_account("alice");

      BOOST_CHECK(alice.registrar == account_id_type(registrar.id));
      BOOST_CHECK(alice.referrer == account_id_type(referrer.id));
      BOOST_CHECK(alice.lifetime_referrer == account_id_type(referrer.lifetime_referrer));

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
