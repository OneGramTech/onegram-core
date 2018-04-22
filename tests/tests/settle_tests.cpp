/*
 * Copyright (c) 2018 oxarbitrage, and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <boost/test/unit_test.hpp>

#include <graphene/chain/hardfork.hpp>

#include <graphene/chain/market_object.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( settle_tests, database_fixture )

BOOST_AUTO_TEST_CASE( settle_rounding_test )
{
   try {
      // get around Graphene issue #615 feed expiration bug
      generate_blocks(HARDFORK_615_TIME);
      generate_block();
      set_expiration( db, trx );

      ACTORS((paul)(michael)(rachel)(alice));

      // create assets
      const auto& bitusd = create_bitasset("USDBIT", paul_id);
      const auto& core   = asset_id_type()(db);
      asset_id_type bitusd_id = bitusd.id;
      asset_id_type core_id = core.id;

      // fund accounts
      transfer(committee_account, michael_id, asset( 100000000 ) );
      transfer(committee_account, paul_id, asset(10000000));
      transfer(committee_account, alice_id, asset(10000000));

      // add a feed to asset
      update_feed_producers( bitusd, {paul.id} );
      price_feed current_feed;
      current_feed.maintenance_collateral_ratio = 1750;
      current_feed.maximum_short_squeeze_ratio = 1100;
      current_feed.settlement_price = bitusd.amount( 100 ) / core.amount(5);
      publish_feed( bitusd, paul, current_feed );

      // paul gets some bitusd
      const call_order_object& call_paul = *borrow( paul, bitusd.amount(1000), core.amount(100) );
      call_order_id_type call_paul_id = call_paul.id;
      BOOST_REQUIRE_EQUAL( get_balance( paul, bitusd ), 1000 );

      // and transfer some to rachel
      transfer(paul.id, rachel.id, asset(200, bitusd.id));

      BOOST_CHECK_EQUAL(get_balance(rachel, core), 0);
      BOOST_CHECK_EQUAL(get_balance(rachel, bitusd), 200);
      BOOST_CHECK_EQUAL(get_balance(michael, bitusd), 0);
      BOOST_CHECK_EQUAL(get_balance(michael, core), 100000000);

      // michael gets some bitusd
      const call_order_object& call_michael = *borrow(michael, bitusd.amount(6), core.amount(8));
      call_order_id_type call_michael_id = call_michael.id;

      // add settle order and check rounding issue
      operation_result result = force_settle(rachel, bitusd.amount(4));

      force_settlement_id_type settle_id = result.get<object_id_type>();
      BOOST_CHECK_EQUAL( settle_id(db).balance.amount.value, 4 );

      BOOST_CHECK_EQUAL(get_balance(rachel, core), 0);
      BOOST_CHECK_EQUAL(get_balance(rachel, bitusd), 196);
      BOOST_CHECK_EQUAL(get_balance(michael, bitusd), 6);
      BOOST_CHECK_EQUAL(get_balance(michael, core), 99999992);
      BOOST_CHECK_EQUAL(get_balance(paul, core), 9999900);
      BOOST_CHECK_EQUAL(get_balance(paul, bitusd), 800);

      BOOST_CHECK_EQUAL( 1000, call_paul.debt.value );
      BOOST_CHECK_EQUAL( 100, call_paul.collateral.value );
      BOOST_CHECK_EQUAL( 6, call_michael.debt.value );
      BOOST_CHECK_EQUAL( 8, call_michael.collateral.value );

      generate_blocks( db.head_block_time() + fc::hours(20) );
      set_expiration( db, trx );

      // default feed and settlement expires at the same time
      // adding new feed so we have valid price to exit
      update_feed_producers( bitusd_id(db), {alice_id} );
      current_feed.maintenance_collateral_ratio = 1750;
      current_feed.maximum_short_squeeze_ratio = 1100;
      current_feed.settlement_price = bitusd_id(db).amount( 100 ) / core_id(db).amount(5);
      publish_feed( bitusd_id(db), alice_id(db), current_feed );

      // now yes expire settlement
      generate_blocks( db.head_block_time() + fc::hours(6) );

      // final checks
      BOOST_CHECK( !db.find( settle_id ) );
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), core_id(db)), 0); // rachel paid 4 usd and got nothing
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), bitusd_id(db)), 196);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), bitusd_id(db)), 6);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), core_id(db)), 99999992);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), core_id(db)), 9999900);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), bitusd_id(db)), 800);

      BOOST_CHECK_EQUAL( 996, call_paul_id(db).debt.value );
      BOOST_CHECK_EQUAL( 100, call_paul_id(db).collateral.value );
      BOOST_CHECK_EQUAL( 6, call_michael_id(db).debt.value );
      BOOST_CHECK_EQUAL( 8, call_michael_id(db).collateral.value );

      // settle more and check rounding issue
      // by default 20% of total supply can be settled per maintenance interval
      set_expiration( db, trx );
      operation_result result2 = force_settle(rachel_id(db), bitusd_id(db).amount(34));

      force_settlement_id_type settle_id2 = result2.get<object_id_type>();
      BOOST_CHECK_EQUAL( settle_id2(db).balance.amount.value, 34 );

      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), core_id(db)), 0);
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), bitusd_id(db)), 162); // 196-34
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), bitusd_id(db)), 6);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), core_id(db)), 99999992);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), core_id(db)), 9999900);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), bitusd_id(db)), 800);

      BOOST_CHECK_EQUAL( 996, call_paul_id(db).debt.value );
      BOOST_CHECK_EQUAL( 100, call_paul_id(db).collateral.value );
      BOOST_CHECK_EQUAL( 6, call_michael_id(db).debt.value );
      BOOST_CHECK_EQUAL( 8, call_michael_id(db).collateral.value );

      generate_blocks( db.head_block_time() + fc::hours(10) );
      set_expiration( db, trx );

      // adding new feed so we have valid price to exit
      update_feed_producers( bitusd_id(db), {alice_id} );
      current_feed.maintenance_collateral_ratio = 1750;
      current_feed.maximum_short_squeeze_ratio = 1100;
      current_feed.settlement_price = bitusd_id(db).amount( 100 ) / core_id(db).amount(5);
      publish_feed( bitusd_id(db), alice_id(db), current_feed );

      // now yes expire settlement
      generate_blocks( db.head_block_time() + fc::hours(16) );
      set_expiration( db, trx );

      // final checks
      BOOST_CHECK( !db.find( settle_id2 ) );
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), core_id(db)), 1); // rachel got 1 core and paid 34 usd
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), bitusd_id(db)), 162);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), bitusd_id(db)), 6);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), core_id(db)), 99999992);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), core_id(db)), 9999900);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), bitusd_id(db)), 800);

      BOOST_CHECK_EQUAL( 996-34, call_paul_id(db).debt.value );
      BOOST_CHECK_EQUAL( 100-1, call_paul_id(db).collateral.value );
      BOOST_CHECK_EQUAL( 6, call_michael_id(db).debt.value );
      BOOST_CHECK_EQUAL( 8, call_michael_id(db).collateral.value );

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( settle_rounding_test_after_hf_184 )
{
   try {
      auto mi = db.get_global_properties().parameters.maintenance_interval;
      generate_blocks(HARDFORK_CORE_184_TIME - mi);
      generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
      set_expiration( db, trx );

      ACTORS((paul)(michael)(rachel)(alice));

      // create assets
      const auto& bitusd = create_bitasset("USDBIT", paul_id);
      const auto& core   = asset_id_type()(db);
      asset_id_type bitusd_id = bitusd.id;
      asset_id_type core_id = core.id;

      // fund accounts
      transfer(committee_account, michael_id, asset( 100000000 ) );
      transfer(committee_account, paul_id, asset(10000000));
      transfer(committee_account, alice_id, asset(10000000));

      // add a feed to asset
      update_feed_producers( bitusd, {paul.id} );
      price_feed current_feed;
      current_feed.maintenance_collateral_ratio = 1750;
      current_feed.maximum_short_squeeze_ratio = 1100;
      current_feed.settlement_price = bitusd.amount( 100 ) / core.amount(5);
      publish_feed( bitusd, paul, current_feed );

      // paul gets some bitusd
      const call_order_object& call_paul = *borrow( paul, bitusd.amount(1000), core.amount(100) );
      call_order_id_type call_paul_id = call_paul.id;
      BOOST_REQUIRE_EQUAL( get_balance( paul, bitusd ), 1000 );

      // and transfer some to rachel
      transfer(paul.id, rachel.id, asset(200, bitusd.id));

      BOOST_CHECK_EQUAL(get_balance(rachel, core), 0);
      BOOST_CHECK_EQUAL(get_balance(rachel, bitusd), 200);
      BOOST_CHECK_EQUAL(get_balance(michael, bitusd), 0);
      BOOST_CHECK_EQUAL(get_balance(michael, core), 100000000);

      // michael gets some bitusd
      const call_order_object& call_michael = *borrow(michael, bitusd.amount(6), core.amount(8));
      call_order_id_type call_michael_id = call_michael.id;

      // add settle order and check rounding issue
      const operation_result result = force_settle(rachel, bitusd.amount(4));

      force_settlement_id_type settle_id = result.get<object_id_type>();
      BOOST_CHECK_EQUAL( settle_id(db).balance.amount.value, 4 );

      BOOST_CHECK_EQUAL(get_balance(rachel, core), 0);
      BOOST_CHECK_EQUAL(get_balance(rachel, bitusd), 196);
      BOOST_CHECK_EQUAL(get_balance(michael, bitusd), 6);
      BOOST_CHECK_EQUAL(get_balance(michael, core), 99999992);
      BOOST_CHECK_EQUAL(get_balance(paul, core), 9999900);
      BOOST_CHECK_EQUAL(get_balance(paul, bitusd), 800);

      BOOST_CHECK_EQUAL( 1000, call_paul.debt.value );
      BOOST_CHECK_EQUAL( 100, call_paul.collateral.value );
      BOOST_CHECK_EQUAL( 6, call_michael.debt.value );
      BOOST_CHECK_EQUAL( 8, call_michael.collateral.value );

      generate_blocks( db.head_block_time() + fc::hours(20) );
      set_expiration( db, trx );

      // default feed and settlement expires at the same time
      // adding new feed so we have valid price to exit
      update_feed_producers( bitusd_id(db), {alice_id} );
      current_feed.maintenance_collateral_ratio = 1750;
      current_feed.maximum_short_squeeze_ratio = 1100;
      current_feed.settlement_price = bitusd_id(db).amount( 101 ) / core_id(db).amount(5);
      publish_feed( bitusd_id(db), alice_id(db), current_feed );

      // now yes expire settlement
      generate_blocks( db.head_block_time() + fc::hours(6) );

      // final checks
      BOOST_CHECK( !db.find( settle_id ) );
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), core_id(db)), 0);
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), bitusd_id(db)), 200); // rachel's settle order is cancelled and he get refunded
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), bitusd_id(db)), 6);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), core_id(db)), 99999992);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), core_id(db)), 9999900);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), bitusd_id(db)), 800);

      BOOST_CHECK_EQUAL( 1000, call_paul_id(db).debt.value );
      BOOST_CHECK_EQUAL( 100, call_paul_id(db).collateral.value );
      BOOST_CHECK_EQUAL( 6, call_michael_id(db).debt.value );
      BOOST_CHECK_EQUAL( 8, call_michael_id(db).collateral.value );

      // settle more and check rounding issue
      // by default 20% of total supply can be settled per maintenance interval
      set_expiration( db, trx );
      const operation_result result2 = force_settle(rachel_id(db), bitusd_id(db).amount(34));

      force_settlement_id_type settle_id2 = result2.get<object_id_type>();
      BOOST_CHECK_EQUAL( settle_id2(db).balance.amount.value, 34 );

      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), core_id(db)), 0);
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), bitusd_id(db)), 166); // 200-34
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), bitusd_id(db)), 6);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), core_id(db)), 99999992);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), core_id(db)), 9999900);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), bitusd_id(db)), 800);

      BOOST_CHECK_EQUAL( 1000, call_paul_id(db).debt.value );
      BOOST_CHECK_EQUAL( 100, call_paul_id(db).collateral.value );
      BOOST_CHECK_EQUAL( 6, call_michael_id(db).debt.value );
      BOOST_CHECK_EQUAL( 8, call_michael_id(db).collateral.value );

      generate_blocks( db.head_block_time() + fc::hours(10) );
      set_expiration( db, trx );

      // adding new feed so we have valid price to exit
      update_feed_producers( bitusd_id(db), {alice_id} );
      current_feed.maintenance_collateral_ratio = 1750;
      current_feed.maximum_short_squeeze_ratio = 1100;
      current_feed.settlement_price = bitusd_id(db).amount( 101 ) / core_id(db).amount(5);
      publish_feed( bitusd_id(db), alice_id(db), current_feed );

      // now yes expire settlement
      generate_blocks( db.head_block_time() + fc::hours(16) );
      set_expiration( db, trx );

      // final checks
      BOOST_CHECK( !db.find( settle_id2 ) );
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), core_id(db)), 1); // rachel got 1 core
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), bitusd_id(db)), 179); // paid 21 usd since 1 core worths a little more than 20 usd
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), bitusd_id(db)), 6);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), core_id(db)), 99999992);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), core_id(db)), 9999900);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), bitusd_id(db)), 800);

      BOOST_CHECK_EQUAL( 1000-21, call_paul_id(db).debt.value );
      BOOST_CHECK_EQUAL( 100-1, call_paul_id(db).collateral.value );
      BOOST_CHECK_EQUAL( 6, call_michael_id(db).debt.value );
      BOOST_CHECK_EQUAL( 8, call_michael_id(db).collateral.value );
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( global_settle_rounding_test )
{
   try {
      // get around Graphene issue #615 feed expiration bug
      generate_blocks(HARDFORK_615_TIME);
      generate_block();
      set_expiration( db, trx );

      ACTORS((paul)(michael)(rachel)(alice));

      // create assets
      const auto& bitusd = create_bitasset("USDBIT", paul_id);
      const auto& core   = asset_id_type()(db);
      asset_id_type bitusd_id = bitusd.id;
      asset_id_type core_id = core.id;

      // fund accounts
      transfer(committee_account, michael_id, asset( 100000000 ) );
      transfer(committee_account, paul_id,    asset(  10000000 ) );
      transfer(committee_account, alice_id,   asset(  10000000 ) );

      // allow global settle in bitusd
      asset_update_operation op;
      op.issuer = bitusd.issuer;
      op.asset_to_update = bitusd.id;
      op.new_options.issuer_permissions = global_settle;
      op.new_options.flags = bitusd.options.flags;
      op.new_options.core_exchange_rate = price( asset(1,bitusd_id), asset(1,core_id) );
      trx.operations.push_back(op);
      sign(trx, paul_private_key);
      PUSH_TX(db, trx);
      generate_block();
      trx.clear();

      // add a feed to asset
      update_feed_producers( bitusd_id(db), {paul_id} );
      price_feed current_feed;
      current_feed.maintenance_collateral_ratio = 1750;
      current_feed.maximum_short_squeeze_ratio = 1100;
      current_feed.settlement_price = bitusd_id(db).amount( 100 ) / core_id(db).amount(5);
      publish_feed( bitusd_id(db), paul_id(db), current_feed );

      BOOST_CHECK_EQUAL(get_balance(paul_id(db), bitusd_id(db)), 0);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), core_id(db)), 10000000);

      // paul gets some bitusd
      const call_order_object& call_paul = *borrow( paul_id(db), bitusd_id(db).amount(1001), core_id(db).amount(101));
      call_order_id_type call_paul_id = call_paul.id;
      BOOST_REQUIRE_EQUAL( get_balance( paul_id(db), bitusd_id(db) ), 1001 );
      BOOST_REQUIRE_EQUAL( get_balance( paul_id(db), core_id(db) ), 10000000-101);

      // and transfer some to rachel
      transfer(paul_id, rachel_id, asset(200, bitusd_id));

      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), core_id(db)), 0);
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), bitusd_id(db)), 200);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), bitusd_id(db)), 0);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), core_id(db)), 100000000);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), core_id(db)), 9999899);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), bitusd_id(db)), 801);

      // michael borrow some bitusd
      const call_order_object& call_michael = *borrow(michael_id(db), bitusd_id(db).amount(6), core_id(db).amount(8));
      call_order_id_type call_michael_id = call_michael.id;

      BOOST_CHECK_EQUAL(get_balance(michael_id(db), bitusd_id(db)), 6);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), core_id(db)), 100000000-8);

      // add global settle
      force_global_settle(bitusd_id(db), bitusd_id(db).amount(10) / core_id(db).amount(1));
      generate_block();

      BOOST_CHECK( bitusd_id(db).bitasset_data(db).settlement_price
                   == price( bitusd_id(db).amount(1007), core_id(db).amount(100) ) );
      BOOST_CHECK_EQUAL( bitusd_id(db).bitasset_data(db).settlement_fund.value, 100 ); // 100 from paul, and 0 from michael
      BOOST_CHECK_EQUAL( bitusd_id(db).dynamic_data(db).current_supply.value, 1007 );

      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), core_id(db)), 0);
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), bitusd_id(db)), 200);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), bitusd_id(db)), 6);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), core_id(db)), 100000000); // michael paid nothing for 6 usd
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), core_id(db)), 9999900); // paul paid 100 core for 1001 usd
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), bitusd_id(db)), 801);

      // all call orders are gone after global settle
      BOOST_CHECK( !db.find_object(call_paul_id) );
      BOOST_CHECK( !db.find_object(call_michael_id) );

      // add settle order and check rounding issue
      force_settle(rachel_id(db), bitusd_id(db).amount(4));
      generate_block();

      BOOST_CHECK( bitusd_id(db).bitasset_data(db).settlement_price
                   == price( bitusd_id(db).amount(1007), core_id(db).amount(100) ) );
      BOOST_CHECK_EQUAL( bitusd_id(db).bitasset_data(db).settlement_fund.value, 100 ); // paid nothing
      BOOST_CHECK_EQUAL( bitusd_id(db).dynamic_data(db).current_supply.value, 1003 ); // settled 4 usd

      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), core_id(db)), 0);
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), bitusd_id(db)), 196); // rachel paid 4 usd and got nothing
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), bitusd_id(db)), 6);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), core_id(db)), 100000000);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), core_id(db)), 9999900);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), bitusd_id(db)), 801);

      // rachel settle more than 1 core
      force_settle(rachel_id(db), bitusd_id(db).amount(13));
      generate_block();

      BOOST_CHECK( bitusd_id(db).bitasset_data(db).settlement_price
                   == price( bitusd_id(db).amount(1007), core_id(db).amount(100) ) );
      BOOST_CHECK_EQUAL( bitusd_id(db).bitasset_data(db).settlement_fund.value, 99 ); // paid 1 core
      BOOST_CHECK_EQUAL( bitusd_id(db).dynamic_data(db).current_supply.value, 990 ); // settled 13 usd

      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), core_id(db)), 1);
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), bitusd_id(db)), 183); // rachel paid 13 usd and got 1 core
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), bitusd_id(db)), 6);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), core_id(db)), 100000000);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), core_id(db)), 9999900);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), bitusd_id(db)), 801);

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( global_settle_rounding_test_after_hf_184 )
{
   try {
      auto mi = db.get_global_properties().parameters.maintenance_interval;
      generate_blocks(HARDFORK_CORE_184_TIME - mi); // assume that hard fork core-184 and core-342 happen at same time
      generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
      set_expiration( db, trx );

      ACTORS((paul)(michael)(rachel)(alice));

      // create assets
      const auto& bitusd = create_bitasset("USDBIT", paul_id);
      const auto& core   = asset_id_type()(db);
      asset_id_type bitusd_id = bitusd.id;
      asset_id_type core_id = core.id;

      // fund accounts
      transfer(committee_account, michael_id, asset( 100000000 ) );
      transfer(committee_account, paul_id,    asset(  10000000 ) );
      transfer(committee_account, alice_id,   asset(  10000000 ) );

      // allow global settle in bitusd
      asset_update_operation op;
      op.issuer = bitusd_id(db).issuer;
      op.asset_to_update = bitusd_id;
      op.new_options.issuer_permissions = global_settle;
      op.new_options.flags = bitusd.options.flags;
      op.new_options.core_exchange_rate = price( asset(1,bitusd_id), asset(1,core_id) );
      trx.operations.push_back(op);
      sign(trx, paul_private_key);
      PUSH_TX(db, trx);
      generate_block();
      trx.clear();

      // add a feed to asset
      update_feed_producers( bitusd_id(db), {paul_id} );
      price_feed current_feed;
      current_feed.maintenance_collateral_ratio = 1750;
      current_feed.maximum_short_squeeze_ratio = 1100;
      current_feed.settlement_price = bitusd_id(db).amount( 100 ) / core_id(db).amount(5);
      publish_feed( bitusd_id(db), paul_id(db), current_feed );

      BOOST_CHECK_EQUAL(get_balance(paul_id(db), bitusd_id(db)), 0);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), core_id(db)), 10000000);

      // paul gets some bitusd
      const call_order_object& call_paul = *borrow( paul_id(db), bitusd_id(db).amount(1001), core_id(db).amount(101));
      call_order_id_type call_paul_id = call_paul.id;
      BOOST_REQUIRE_EQUAL( get_balance( paul_id(db), bitusd_id(db) ), 1001 );
      BOOST_REQUIRE_EQUAL( get_balance( paul_id(db), core_id(db) ), 10000000-101);

      // and transfer some to rachel
      transfer(paul_id, rachel_id, asset(200, bitusd_id));

      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), core_id(db)), 0);
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), bitusd_id(db)), 200);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), bitusd_id(db)), 0);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), core_id(db)), 100000000);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), core_id(db)), 9999899);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), bitusd_id(db)), 801);

      // michael borrow some bitusd
      const call_order_object& call_michael = *borrow(michael_id(db), bitusd_id(db).amount(6), core_id(db).amount(8));
      call_order_id_type call_michael_id = call_michael.id;

      BOOST_CHECK_EQUAL(get_balance(michael_id(db), bitusd_id(db)), 6);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), core_id(db)), 100000000-8);

      // add global settle
      force_global_settle(bitusd_id(db), bitusd_id(db).amount(10) / core_id(db).amount(1));
      generate_block();

      BOOST_CHECK( bitusd_id(db).bitasset_data(db).settlement_price
                   == price( bitusd_id(db).amount(1007), core_id(db).amount(102) ) );
      BOOST_CHECK_EQUAL( bitusd_id(db).bitasset_data(db).settlement_fund.value, 102 ); // 101 from paul, and 1 from michael
      BOOST_CHECK_EQUAL( bitusd_id(db).dynamic_data(db).current_supply.value, 1007 );

      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), core_id(db)), 0);
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), bitusd_id(db)), 200);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), bitusd_id(db)), 6);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), core_id(db)), 99999999); // michael paid 1 core for 6 usd
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), core_id(db)), 9999899); // paul paid 101 core for 1001 usd
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), bitusd_id(db)), 801);

      // all call orders are gone after global settle
      BOOST_CHECK( !db.find_object(call_paul_id));
      BOOST_CHECK( !db.find_object(call_michael_id));

      // settle order will not execute after HF due to too small
      GRAPHENE_REQUIRE_THROW( force_settle(rachel_id(db), bitusd_id(db).amount(4)), fc::exception );

      generate_block();

      // balances unchanged
      BOOST_CHECK( bitusd_id(db).bitasset_data(db).settlement_price
                   == price( bitusd_id(db).amount(1007), core_id(db).amount(102) ) );
      BOOST_CHECK_EQUAL( bitusd_id(db).bitasset_data(db).settlement_fund.value, 102 );
      BOOST_CHECK_EQUAL( bitusd_id(db).dynamic_data(db).current_supply.value, 1007 );

      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), core_id(db)), 0);
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), bitusd_id(db)), 200);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), bitusd_id(db)), 6);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), core_id(db)), 99999999);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), core_id(db)), 9999899);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), bitusd_id(db)), 801);

      // rachel settle more than 1 core
      force_settle(rachel_id(db), bitusd_id(db).amount(13));
      generate_block();

      BOOST_CHECK( bitusd_id(db).bitasset_data(db).settlement_price
                   == price( bitusd_id(db).amount(1007), core_id(db).amount(102) ) );
      BOOST_CHECK_EQUAL( bitusd_id(db).bitasset_data(db).settlement_fund.value, 101 ); // paid 1 core
      BOOST_CHECK_EQUAL( bitusd_id(db).dynamic_data(db).current_supply.value, 997 ); // settled 10 usd

      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), core_id(db)), 1);
      BOOST_CHECK_EQUAL(get_balance(rachel_id(db), bitusd_id(db)), 190); // rachel paid 10 usd and got 1 core, 3 usd returned
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), bitusd_id(db)), 6);
      BOOST_CHECK_EQUAL(get_balance(michael_id(db), core_id(db)), 99999999);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), core_id(db)), 9999899);
      BOOST_CHECK_EQUAL(get_balance(paul_id(db), bitusd_id(db)), 801);


   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
