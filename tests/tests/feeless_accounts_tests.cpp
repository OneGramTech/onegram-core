//
// Created by Lubos Ilcik on 19/02/2018.
//

#include <boost/test/unit_test.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/utilities/tempdir.hpp>
#include <graphene/chain/chain_property_object.hpp>
#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE(feeless_accounts_tests, database_fixture)

BOOST_AUTO_TEST_CASE(check_set_fee_with_feeless_account_ids_argument_when_transfer_from_feeless_account)
{
   try {

      // create test accounts
      create_account("nathan", public_key_type());
      create_account("alice", public_key_type());

      // fee payer
      const account_object &nathan_account = *db.get_index_type<account_index>().indices().get<by_name>().find("nathan");

      // include fee payer as the feeless account
      db.modify(chain_property_id_type()(db), [&](chain_property_object &cpo) {
         cpo.immutable_parameters.feeless_accounts.account_names.emplace("nathan");
         cpo.feeless_accounts.account_ids.insert(nathan_account.id);
      });

      BOOST_CHECK(db.get_chain_properties().immutable_parameters.feeless_accounts.contains("nathan"));

      // input parameters
      uint64_t transfer_amount = 10000 * GRAPHENE_BLOCKCHAIN_PRECISION;
      // set default fees
      enable_fees();

      // transfer an amount from committee account to nathan
      trx.clear();

      account_id_type committee_account;
      asset committee_balance = db.get_balance(account_id_type(), asset_id_type());
      transfer_operation top_from_committee;
      top_from_committee.from = committee_account;
      top_from_committee.to = nathan_account.id;
      top_from_committee.amount = asset(transfer_amount);

      trx.operations.push_back(top_from_committee);
      for (auto &op : trx.operations) db.current_fee_schedule().set_fee(op, price::unit_price(), db.get_chain_properties().feeless_account_ids());

      asset fee = trx.operations.front().get<transfer_operation>().fee;
      // verify the preconditions - fee non null
      auto flat_fee = db.get_global_properties().parameters.current_fees->get<transfer_operation>().fee;
      BOOST_CHECK_EQUAL(fee.amount.value, flat_fee);

      int64_t pre_transfer_to_account_balance = get_balance(nathan_account, asset_id_type()(db));
      // verify the preconditions - "to" account balance state
      BOOST_CHECK_EQUAL(pre_transfer_to_account_balance, 0);

      trx.validate();
      PUSH_TX(db, trx, ~0);

      share_type &pre_transfer_from_account_balance = committee_balance.amount;
      int64_t post_transfer_from_account_balance = get_balance(account_id_type()(db), asset_id_type()(db));
      // verify the postconditions - the "from" balance
      BOOST_CHECK_EQUAL(post_transfer_from_account_balance, (pre_transfer_from_account_balance - transfer_amount - fee.amount).value);

      int64_t post_transfer_to_account_balance = get_balance(nathan_account, asset_id_type()(db));
      // verify the postconditions - the "to" balance
      BOOST_CHECK_EQUAL(post_transfer_to_account_balance, transfer_amount);

      // transfer a half of amount from nathan (feeless account) to alice
      trx.clear();
      const account_object &alice_account = *db.get_index_type<account_index>().indices().get<by_name>().find("alice");
      transfer_operation top_from_nathan;
      top_from_nathan.from = nathan_account.id;
      top_from_nathan.to = alice_account.id;
      top_from_nathan.amount = asset(transfer_amount / 2);

      trx.operations.push_back(top_from_nathan);
      for (auto &op : trx.operations) db.current_fee_schedule().set_fee(op, price::unit_price(), db.get_chain_properties().feeless_account_ids());

      // verify the preconditions - fee is zero
      fee = trx.operations.front().get<transfer_operation>().fee;
      BOOST_CHECK_EQUAL(fee.amount.value, 0);

      pre_transfer_to_account_balance = get_balance(alice_account, asset_id_type()(db));
      // verify the preconditions - "to" account balance state
      BOOST_CHECK_EQUAL(pre_transfer_to_account_balance, 0);

      pre_transfer_from_account_balance = post_transfer_to_account_balance;

      trx.validate();
      PUSH_TX(db, trx, ~0);

      post_transfer_from_account_balance = get_balance(nathan_account, asset_id_type()(db));
      // verify the postconditions - the "from" balance
      BOOST_CHECK_EQUAL(post_transfer_from_account_balance, (pre_transfer_from_account_balance - transfer_amount / 2).value);

      post_transfer_to_account_balance = get_balance(alice_account, asset_id_type()(db));
      // verify the postconditions - the "to" balance
      BOOST_CHECK_EQUAL(post_transfer_to_account_balance, transfer_amount / 2);


   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
