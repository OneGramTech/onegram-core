//
// Created by Lubos Ilcik on 29/01/2018.
//

#include <boost/test/unit_test.hpp>
#include <boost/assign/list_of.hpp>

#include <graphene/chain/database.hpp>

#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/witness_object.hpp>

#include <graphene/market_history/market_history_plugin.hpp>
#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( transfer_operation_fee_tests, database_fixture )

BOOST_AUTO_TEST_CASE(when_transfer_percentage_fee_is_set_the_from_account_balance_is_less_percentage_fee)
{
    try {
        create_account("nathan", public_key_type());
        trx.clear();

        // input parameters
        uint64_t transfer_amount = 10000 * GRAPHENE_BLOCKCHAIN_PRECISION;
        uint64_t min_fee_amount = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
        uint64_t max_fee_amount = 300 * GRAPHENE_BLOCKCHAIN_PRECISION;
        uint16_t percentage_fee = 1 * GRAPHENE_1_PERCENT;
        uint64_t expected_fee_amount = percentage_fee * transfer_amount / GRAPHENE_100_PERCENT;

        // overwrite default fee parameters
        db.modify(global_property_id_type()(db), [&](global_property_object& gpo)
        {
            gpo.parameters.get_mutable_fees() = fee_schedule::get_default();
            gpo.parameters.get_mutable_fees().get<transfer_operation>().fee = min_fee_amount;
            gpo.parameters.get_mutable_fees().get<transfer_operation>().percentage = percentage_fee;
            gpo.parameters.get_mutable_fees().get<transfer_operation>().percentage_max_fee = max_fee_amount;
        });

        // transfer an amount from committee account to nathan
        account_id_type committee_account;
        asset committee_balance = db.get_balance(account_id_type(), asset_id_type());
        const account_object &nathan_account = *db.get_index_type<account_index>().indices().get<by_name>().find(
        "nathan");
        transfer_operation top;
        top.from = committee_account;
        top.to = nathan_account.id;
        top.amount = asset(transfer_amount);
        trx.operations.push_back(top);
        for (auto &op : trx.operations) db.current_fee_schedule().set_fee(op);

        asset fee = trx.operations.front().get<transfer_operation>().fee;
        // verify the preconditions - fee is calculated as expected
        BOOST_CHECK_EQUAL(static_cast<uint64_t>(fee.amount.value), expected_fee_amount);

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
        BOOST_CHECK_EQUAL(static_cast<uint64_t>(post_transfer_to_account_balance), transfer_amount);
    } catch (fc::exception& e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(when_default_transfer_fee_is_used_the_from_account_balance_is_less_flat_fee)
{
   try {
      create_account("nathan", public_key_type());
      trx.clear();

      // input parameters
      uint64_t transfer_amount = 10000 * GRAPHENE_BLOCKCHAIN_PRECISION;
      // set default fees
      enable_fees();

      // transfer an amount from committee account to nathan
      account_id_type committee_account;
      asset committee_balance = db.get_balance(account_id_type(), asset_id_type());
      const account_object &nathan_account = *db.get_index_type<account_index>().indices().get<by_name>().find(
      "nathan");
      transfer_operation top;
      top.from = committee_account;
      top.to = nathan_account.id;
      top.amount = asset(transfer_amount);
      trx.operations.push_back(top);
      for (auto &op : trx.operations) db.current_fee_schedule().set_fee(op);

      asset fee = trx.operations.front().get<transfer_operation>().fee;
      // verify the preconditions - fee non null
      auto flat_fee = db.get_global_properties().parameters.current_fees->get<transfer_operation>().fee;
      BOOST_CHECK_EQUAL(static_cast<uint64_t>(fee.amount.value), flat_fee);

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
      BOOST_CHECK_EQUAL(static_cast<uint64_t>(post_transfer_to_account_balance), transfer_amount);
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_SUITE_END()
