/*
 * Copyright (c) 2018 Bitshares Foundation, and contributors.
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

#include <vector>
#include <boost/test/unit_test.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/hardfork.hpp>

#include <graphene/chain/balance_object.hpp>
#include <graphene/chain/budget_record_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/worker_object.hpp>
#include <graphene/chain/asset_evaluator.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/log/log_message.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( bitasset_tests, database_fixture )

/*****
 * @brief helper method to change a backing asset to a new one
 * @param fixture the database_fixture
 * @param signing_key the signer
 * @param asset_id_to_update asset to update
 * @param new_backing_asset_id the new backing asset
 */
void change_backing_asset(database_fixture& fixture, const fc::ecc::private_key& signing_key,
      asset_id_type asset_id_to_update, asset_id_type new_backing_asset_id)
{
   asset_update_bitasset_operation ba_op;
   const asset_object& asset_to_update = asset_id_to_update(fixture.db);
   ba_op.asset_to_update = asset_id_to_update;
   ba_op.issuer = asset_to_update.issuer;
   ba_op.new_options.short_backing_asset = new_backing_asset_id;
   fixture.trx.operations.push_back(ba_op);
   fixture.sign(fixture.trx, signing_key);
   PUSH_TX(fixture.db, fixture.trx, ~0);
   fixture.generate_block();
   fixture.trx.clear();
}

/******
 * @ brief helper method to turn witness_fed_asset on and off
 * @param fixture the database_fixture
 * @param new_issuer optionally change the issuer
 * @param signing_key signer
 * @param asset_id asset we want to change
 * @param witness_fed true if you want this to be a witness fed asset
 */
void change_asset_options(database_fixture& fixture, const optional<account_id_type>& new_issuer,
      const fc::ecc::private_key& signing_key,
      asset_id_type asset_id, bool witness_fed)
{
   asset_update_operation op;
   const asset_object& obj = asset_id(fixture.db);
   op.asset_to_update = asset_id;
   op.issuer = obj.issuer;
   if (new_issuer)
      op.new_issuer = new_issuer;
   op.new_options = obj.options;
   if (witness_fed)
   {
      op.new_options.flags |= witness_fed_asset;
      op.new_options.flags &= ~committee_fed_asset;
   }
   else
   {
      op.new_options.flags &= ~witness_fed_asset; // we don't care about the committee flag here
   }
   fixture.trx.operations.push_back(op);
   fixture.sign( fixture.trx, signing_key );
   PUSH_TX( fixture.db, fixture.trx, ~0 );
   fixture.generate_block();
   fixture.trx.clear();

}

/*********
 * @brief make sure feeds still work after changing backing asset on a witness-fed asset
 */
BOOST_AUTO_TEST_CASE( reset_backing_asset_on_witness_asset )
{
   ACTORS((nathan));

   /*
       // do a maintenance block
      generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
      // generate blocks until close to hard fork
      generate_blocks( HARDFORK_CORE_868_890_TIME - fc::hours(1) );
    */

   BOOST_TEST_MESSAGE("Advance to near hard fork");
   auto maint_interval = db.get_global_properties().parameters.maintenance_interval;
   generate_blocks( HARDFORK_CORE_868_890_TIME - maint_interval);
   trx.set_expiration(HARDFORK_CORE_868_890_TIME - fc::seconds(1));

   BOOST_TEST_MESSAGE("Create USDBIT");
   asset_id_type bit_usd_id = create_bitasset("USDBIT").id;
   asset_id_type core_id = bit_usd_id(db).bitasset_data(db).options.short_backing_asset;

   {
      BOOST_TEST_MESSAGE("Update the USDBIT asset options");
      change_asset_options(*this, nathan_id, nathan_private_key, bit_usd_id, false );
   }

   BOOST_TEST_MESSAGE("Create JMJBIT based on USDBIT.");
   asset_id_type bit_jmj_id = create_bitasset("JMJBIT").id;
   {
      BOOST_TEST_MESSAGE("Update the JMJBIT asset options");
      change_asset_options(*this, nathan_id, nathan_private_key, bit_jmj_id, true );
   }
   {
      BOOST_TEST_MESSAGE("Update the JMJBIT bitasset options");
      asset_update_bitasset_operation ba_op;
      const asset_object& obj = bit_jmj_id(db);
      ba_op.asset_to_update = obj.get_id();
      ba_op.issuer = obj.issuer;
      ba_op.new_options.short_backing_asset = bit_usd_id;
      ba_op.new_options.minimum_feeds = 1;
      trx.operations.push_back(ba_op);
      sign(trx, nathan_private_key);
      PUSH_TX(db, trx, ~0);
      generate_block();
      trx.clear();
   }

   BOOST_TEST_MESSAGE("Grab active witnesses");
   auto& global_props = db.get_global_properties();
   std::vector<account_id_type> active_witnesses;
   for(const witness_id_type& wit_id : global_props.active_witnesses)
      active_witnesses.push_back(wit_id(db).witness_account);
   BOOST_REQUIRE_EQUAL(active_witnesses.size(), 10lu);

   {
      BOOST_TEST_MESSAGE("Adding price feed 1");
      publish_feed(active_witnesses[0], bit_usd_id, 1, bit_jmj_id, 300, core_id);

      const asset_bitasset_data_object& bitasset = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(bitasset.current_feed.settlement_price.to_real(), 300.0);
      BOOST_CHECK(bitasset.current_feed.maintenance_collateral_ratio == GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO);
   }
   {
      BOOST_TEST_MESSAGE("Adding price feed 2");
      publish_feed(active_witnesses[1], bit_usd_id, 1, bit_jmj_id, 100, core_id);

      const asset_bitasset_data_object& bitasset = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(bitasset.current_feed.settlement_price.to_real(), 300.0);
      BOOST_CHECK(bitasset.current_feed.maintenance_collateral_ratio == GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO);
   }
   {
      BOOST_TEST_MESSAGE("Adding price feed 3");
      publish_feed(active_witnesses[2], bit_usd_id, 1, bit_jmj_id, 1, core_id);

      const asset_bitasset_data_object& bitasset = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(bitasset.current_feed.settlement_price.to_real(), 100.0);
      BOOST_CHECK(bitasset.current_feed.maintenance_collateral_ratio == GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO);
   }
   {
      BOOST_TEST_MESSAGE("Change underlying asset of bit_jmj from bit_usd to core");
      change_backing_asset(*this, nathan_private_key, bit_jmj_id, core_id);

      BOOST_TEST_MESSAGE("Verify feed producers have not been reset");
      const asset_bitasset_data_object& jmj_obj = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(jmj_obj.feeds.size(), 3ul);
   }
   {
      BOOST_TEST_MESSAGE("With underlying bitasset changed from one to another, price feeds should still be publish-able");
      BOOST_TEST_MESSAGE("Re-Adding Witness 1 price feed");
      publish_feed(active_witnesses[0], core_id, 1, bit_jmj_id, 30, core_id);

      const asset_bitasset_data_object& bitasset = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(bitasset.current_feed.settlement_price.to_real(), 1);
      BOOST_CHECK(bitasset.current_feed.maintenance_collateral_ratio == GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO);

      BOOST_CHECK(bitasset.current_feed.core_exchange_rate.base.asset_id != bitasset.current_feed.core_exchange_rate.quote.asset_id);
   }
   {
      BOOST_TEST_MESSAGE("Re-Adding Witness 2 price feed");
      publish_feed(active_witnesses[1], core_id, 1, bit_jmj_id, 100, core_id);

      const asset_bitasset_data_object& bitasset = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(bitasset.current_feed.settlement_price.to_real(), 100);
      BOOST_CHECK(bitasset.current_feed.maintenance_collateral_ratio == GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO);
   }
   {
      BOOST_TEST_MESSAGE("Advance to after hard fork");
      generate_blocks( HARDFORK_CORE_868_890_TIME + fc::seconds(1));
      trx.set_expiration(HARDFORK_CORE_868_890_TIME + fc::hours(2));

      BOOST_TEST_MESSAGE("After hardfork, 1 feed should have been erased");
      const asset_bitasset_data_object& jmj_obj = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(jmj_obj.feeds.size(), 2ul);
   }
   {
      BOOST_TEST_MESSAGE("After hardfork, change underlying asset of bit_jmj from core to bit_usd");
      change_backing_asset(*this, nathan_private_key, bit_jmj_id, bit_usd_id);

      BOOST_TEST_MESSAGE("Verify feed producers have been reset");
      const asset_bitasset_data_object& jmj_obj = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(jmj_obj.feeds.size(), 0ul);
   }
   {
      BOOST_TEST_MESSAGE("With underlying bitasset changed from one to another, price feeds should still be publish-able");
      BOOST_TEST_MESSAGE("Re-Adding Witness 1 price feed");
      publish_feed(active_witnesses[0], bit_usd_id, 1, bit_jmj_id, 30, core_id);

      const asset_bitasset_data_object& bitasset = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(bitasset.current_feed.settlement_price.to_real(), 30);
      BOOST_CHECK(bitasset.current_feed.maintenance_collateral_ratio == GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO);

      BOOST_CHECK(bitasset.current_feed.core_exchange_rate.base.asset_id != bitasset.current_feed.core_exchange_rate.quote.asset_id);
   }
}

/****
 * @brief make sure feeds work correctly after changing the backing asset on a non-witness-fed asset
 */
BOOST_AUTO_TEST_CASE( reset_backing_asset_on_non_witness_asset )
{
   ACTORS((nathan)(dan)(ben)(vikram));

   BOOST_TEST_MESSAGE("Advance to near hard fork");
   auto maint_interval = db.get_global_properties().parameters.maintenance_interval;
   generate_blocks( HARDFORK_CORE_868_890_TIME - maint_interval);
   trx.set_expiration(HARDFORK_CORE_868_890_TIME - fc::seconds(1));


   BOOST_TEST_MESSAGE("Create USDBIT");
   asset_id_type bit_usd_id = create_bitasset("USDBIT").id;
   asset_id_type core_id = bit_usd_id(db).bitasset_data(db).options.short_backing_asset;

   {
      BOOST_TEST_MESSAGE("Update the USDBIT asset options");
      change_asset_options(*this, nathan_id, nathan_private_key, bit_usd_id, false );
   }

   BOOST_TEST_MESSAGE("Create JMJBIT based on USDBIT.");
   asset_id_type bit_jmj_id = create_bitasset("JMJBIT").id;
   {
      BOOST_TEST_MESSAGE("Update the JMJBIT asset options");
      change_asset_options(*this, nathan_id, nathan_private_key, bit_jmj_id, false );
   }
   {
      BOOST_TEST_MESSAGE("Update the JMJBIT bitasset options");
      asset_update_bitasset_operation ba_op;
      const asset_object& obj = bit_jmj_id(db);
      ba_op.asset_to_update = obj.get_id();
      ba_op.issuer = obj.issuer;
      ba_op.new_options.short_backing_asset = bit_usd_id;
      ba_op.new_options.minimum_feeds = 1;
      trx.operations.push_back(ba_op);
      sign(trx, nathan_private_key);
      PUSH_TX(db, trx, ~0);
      generate_block();
      trx.clear();
   }
   {
      BOOST_TEST_MESSAGE("Set feed producers for JMJBIT");
      asset_update_feed_producers_operation op;
      op.asset_to_update = bit_jmj_id;
      op.issuer = nathan_id;
      op.new_feed_producers = {dan_id, ben_id, vikram_id};
      trx.operations.push_back(op);
      sign( trx, nathan_private_key );
      PUSH_TX( db, trx, ~0 );
      generate_block();
      trx.clear();
   }
   {
      BOOST_TEST_MESSAGE("Verify feed producers are registered for JMJBIT");
      const asset_bitasset_data_object& obj = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(obj.feeds.size(), 3ul);
      BOOST_CHECK(obj.current_feed == price_feed());


      BOOST_CHECK_EQUAL("1", std::to_string(obj.options.short_backing_asset.space_id));
      BOOST_CHECK_EQUAL("3", std::to_string(obj.options.short_backing_asset.type_id));
      BOOST_CHECK_EQUAL("1", std::to_string(obj.options.short_backing_asset.instance.value));

      BOOST_CHECK_EQUAL("1", std::to_string(bit_jmj_id.space_id));
      BOOST_CHECK_EQUAL("3", std::to_string(bit_jmj_id.type_id));
      BOOST_CHECK_EQUAL("2", std::to_string(bit_jmj_id.instance.value));
   }
   {
      BOOST_TEST_MESSAGE("Adding Vikram's price feed");
      publish_feed(vikram_id, bit_usd_id, 1, bit_jmj_id, 300, core_id);

      const asset_bitasset_data_object& bitasset = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(bitasset.current_feed.settlement_price.to_real(), 300.0);
      BOOST_CHECK(bitasset.current_feed.maintenance_collateral_ratio == GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO);
   }
   {
      BOOST_TEST_MESSAGE("Adding Ben's pricing to JMJBIT");
      publish_feed(ben_id, bit_usd_id, 1, bit_jmj_id, 100, core_id);

      const asset_bitasset_data_object& bitasset = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(bitasset.current_feed.settlement_price.to_real(), 300);
      BOOST_CHECK(bitasset.current_feed.maintenance_collateral_ratio == GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO);
   }
   {
      BOOST_TEST_MESSAGE("Adding Dan's pricing to JMJBIT");
      publish_feed(dan_id, bit_usd_id, 1, bit_jmj_id, 1, core_id);

      const asset_bitasset_data_object& bitasset = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(bitasset.current_feed.settlement_price.to_real(), 100);
      BOOST_CHECK(bitasset.current_feed.maintenance_collateral_ratio == GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO);
      generate_block();
      trx.clear();

      BOOST_CHECK(bitasset.current_feed.core_exchange_rate.base.asset_id != bitasset.current_feed.core_exchange_rate.quote.asset_id);
   }
   {
      BOOST_TEST_MESSAGE("Change underlying asset of bit_jmj from bit_usd to core");
      change_backing_asset(*this, nathan_private_key, bit_jmj_id, core_id);

      BOOST_TEST_MESSAGE("Verify feed producers have not been reset");
      const asset_bitasset_data_object& jmj_obj = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(jmj_obj.feeds.size(), 3ul);
      for(const auto& feed : jmj_obj.feeds) {
         BOOST_CHECK(!feed.second.second.settlement_price.is_null());
      }
   }
   {
      BOOST_TEST_MESSAGE("Add a new (and correct) feed price for 1 feed producer");
      publish_feed(vikram_id, core_id, 1, bit_jmj_id, 300, core_id);
   }
   {
      BOOST_TEST_MESSAGE("Advance to past hard fork");
      generate_blocks( HARDFORK_CORE_868_890_TIME + maint_interval);
      trx.set_expiration(HARDFORK_CORE_868_890_TIME + fc::hours(48));

      BOOST_TEST_MESSAGE("Verify that the incorrect feeds have been corrected");
      const asset_bitasset_data_object& jmj_obj = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(jmj_obj.feeds.size(), 3ul);
      int nan_count = 0;
      for(const auto& feed : jmj_obj.feeds)
      {
         if (feed.second.second.settlement_price.is_null())
            nan_count++;
      }
      BOOST_CHECK_EQUAL(nan_count, 2);
      // the settlement price will be NaN until 50% of price feeds are valid
      //BOOST_CHECK_EQUAL(jmj_obj.current_feed.settlement_price.to_real(), 300);
   }
   {
      BOOST_TEST_MESSAGE("After hardfork, change underlying asset of bit_jmj from core to bit_usd");
      change_backing_asset(*this, nathan_private_key, bit_jmj_id, bit_usd_id);

      BOOST_TEST_MESSAGE("Verify feed producers have been reset");
      const asset_bitasset_data_object& jmj_obj = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(jmj_obj.feeds.size(), 3ul);
      for(const auto& feed : jmj_obj.feeds)
      {
         BOOST_CHECK(feed.second.second.settlement_price.is_null());
      }
   }
   {
      BOOST_TEST_MESSAGE("With underlying bitasset changed from one to another, price feeds should still be publish-able");
      BOOST_TEST_MESSAGE("Adding Vikram's price feed");
      publish_feed(vikram_id, bit_usd_id, 1, bit_jmj_id, 30, core_id);

      const asset_bitasset_data_object& bitasset = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(bitasset.current_feed.settlement_price.to_real(), 30);
      BOOST_CHECK(bitasset.current_feed.maintenance_collateral_ratio == GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO);

      BOOST_TEST_MESSAGE("Adding Ben's pricing to JMJBIT");
      publish_feed(ben_id, bit_usd_id, 1, bit_jmj_id, 25, core_id);

      BOOST_CHECK_EQUAL(bitasset.current_feed.settlement_price.to_real(), 30);
      BOOST_CHECK(bitasset.current_feed.maintenance_collateral_ratio == GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO);

      BOOST_TEST_MESSAGE("Adding Dan's pricing to JMJBIT");
      publish_feed(dan_id, bit_usd_id, 1, bit_jmj_id, 10, core_id);

      BOOST_CHECK_EQUAL(bitasset.current_feed.settlement_price.to_real(), 25);
      BOOST_CHECK(bitasset.current_feed.maintenance_collateral_ratio == GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO);
      generate_block();
      trx.clear();

      BOOST_CHECK(bitasset.current_feed.core_exchange_rate.base.asset_id != bitasset.current_feed.core_exchange_rate.quote.asset_id);
   }
}

/*********
 * @brief Update median feeds after feed_lifetime_sec changed
 */
BOOST_AUTO_TEST_CASE( hf_890_test )
{
   uint32_t skip = database::skip_witness_signature
                 | database::skip_transaction_signatures
                 | database::skip_transaction_dupe_check
                 | database::skip_block_size_check
                 | database::skip_tapos_check
                 | database::skip_authority_check
                 | database::skip_merkle_check
                 ;
   generate_blocks(HARDFORK_615_TIME, true, skip); // get around Graphene issue #615 feed expiration bug
   generate_blocks(db.get_dynamic_global_properties().next_maintenance_time, true, skip);

   for( int i=0; i<2; ++i )
   {
      int blocks = 0;
      auto mi = db.get_global_properties().parameters.maintenance_interval;

      if( i == 1 ) // go beyond hard fork
      {
         generate_blocks(HARDFORK_CORE_868_890_TIME - mi, true, skip);
         blocks += 2;
         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time, true, skip);
         blocks += 2;
      }
      set_expiration( db, trx );

      ACTORS((buyer)(seller)(borrower)(feedproducer));

      int64_t init_balance(1000000);

      transfer(committee_account, buyer_id, asset(init_balance));
      transfer(committee_account, borrower_id, asset(init_balance));

      const auto& bitusd = create_bitasset("USDBIT", feedproducer_id);
      asset_id_type usd_id = bitusd.id;

      {
         // change feed lifetime
         const asset_object& asset_to_update = usd_id(db);
         asset_update_bitasset_operation ba_op;
         ba_op.asset_to_update = usd_id;
         ba_op.issuer = asset_to_update.issuer;
         ba_op.new_options = asset_to_update.bitasset_data(db).options;
         ba_op.new_options.feed_lifetime_sec = 600;
         trx.operations.push_back(ba_op);
         PUSH_TX(db, trx, ~0);
         trx.clear();
      }

      // prepare feed data
      price_feed current_feed;
      current_feed.maintenance_collateral_ratio = 1750;
      current_feed.maximum_short_squeeze_ratio = 1100;

      // set price feed
      update_feed_producers( usd_id(db), {feedproducer_id} );
      current_feed.settlement_price = asset(100, usd_id) / asset(5);
      publish_feed( usd_id, feedproducer_id, current_feed );

      // Place some collateralized orders
      // start out with 300% collateral, call price is 15/175 CORE/USD = 60/700
      borrow( borrower_id, asset(100, usd_id), asset(15) );

      transfer( borrower_id, seller_id, asset(100, usd_id) );

      // Adjust price feed to get call order into margin call territory
      current_feed.settlement_price = asset(100, usd_id) / asset(10);
      publish_feed( usd_id, feedproducer_id, current_feed );
      // settlement price = 100 USD / 10 CORE, mssp = 100/11 USD/CORE

      // let the feed expire
      generate_blocks( db.head_block_time() + 1200, true, skip );
      blocks += 2;
      set_expiration( db, trx );

      // check: median feed should be null
      BOOST_CHECK( usd_id(db).bitasset_data(db).current_feed.settlement_price.is_null() );

      // place a sell order, it won't be matched with the call order
      limit_order_id_type sell_id = create_sell_order(seller_id, asset(10, usd_id), asset(1))->id;

      {
         // change feed lifetime to longer
         const asset_object& asset_to_update = usd_id(db);
         asset_update_bitasset_operation ba_op;
         ba_op.asset_to_update = usd_id;
         ba_op.issuer = asset_to_update.issuer;
         ba_op.new_options = asset_to_update.bitasset_data(db).options;
         ba_op.new_options.feed_lifetime_sec = HARDFORK_CORE_868_890_TIME.sec_since_epoch()
                                             - db.head_block_time().sec_since_epoch()
                                             + mi
                                             + 1800;
         trx.operations.push_back(ba_op);
         PUSH_TX(db, trx, ~0);
         trx.clear();
      }

      // check
      if( i == 0 ) // before hard fork, median feed is still null, and limit order is still there
      {
         BOOST_CHECK( usd_id(db).bitasset_data(db).current_feed.settlement_price.is_null() );
         BOOST_CHECK( db.find<limit_order_object>( sell_id ) );

         // go beyond hard fork
         generate_blocks(HARDFORK_CORE_868_890_TIME - mi, true, skip);
         blocks += 2;
         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time, true, skip);
         blocks += 2;
      }

      // after hard fork, median feed should become valid, and the limit order should be filled
      {
         BOOST_CHECK( usd_id(db).bitasset_data(db).current_feed.settlement_price == current_feed.settlement_price );
         BOOST_CHECK( !db.find<limit_order_object>( sell_id ) );
      }

      // undo above tx's and reset
      generate_block( skip );
      ++blocks;
      while( blocks > 0 )
      {
         db.pop_block();
         --blocks;
      }
   }
}

class bitasset_evaluator_wrapper : public asset_update_bitasset_evaluator
{
public:
   void set_db(database& db)
   {
      this->trx_state = new transaction_evaluation_state(&db);
   }
};

struct assets_922_931
{
   const asset_object* bit_usd;
   const asset_object* bit_usdbacked;
   const asset_object* bit_usdbacked2;
   const asset_object* bit_child_user_issued;
   const asset_object* bit_parent;
   const asset_object* user_issued;
   const asset_object* six_precision;
   const asset_object* prediction;
};

assets_922_931 create_assets_922_931(database_fixture* fixture)
{
   assets_922_931 asset_objs;
   BOOST_TEST_MESSAGE( "Create USDBIT" );
   asset_objs.bit_usd = &fixture->create_bitasset( "USDBIT", GRAPHENE_COMMITTEE_ACCOUNT );

   BOOST_TEST_MESSAGE( "Create USDBACKED" );
   asset_objs.bit_usdbacked = &fixture->create_bitasset( "USDBACKED", GRAPHENE_COMMITTEE_ACCOUNT,
         100, charge_market_fee, 2, asset_objs.bit_usd->get_id() );

   BOOST_TEST_MESSAGE( "Create USDBACKEDII" );
   asset_objs.bit_usdbacked2 = &fixture->create_bitasset( "USDBACKEDII", GRAPHENE_WITNESS_ACCOUNT,
         100, charge_market_fee, 2, asset_objs.bit_usd->get_id() );

   BOOST_TEST_MESSAGE( "Create PARENT" );
   asset_objs.bit_parent = &fixture->create_bitasset( "PARENT", GRAPHENE_WITNESS_ACCOUNT);

   BOOST_TEST_MESSAGE( "Create CHILDUSER" );
   asset_objs.bit_child_user_issued = &fixture->create_bitasset( "CHILDUSER", GRAPHENE_WITNESS_ACCOUNT,
         100, charge_market_fee, 2, asset_objs.bit_parent->get_id() );

   BOOST_TEST_MESSAGE( "Create user issued USERISSUED" );
   asset_objs.user_issued = &fixture->create_user_issued_asset( "USERISSUED", GRAPHENE_WITNESS_ACCOUNT(fixture->db), charge_market_fee );

   BOOST_TEST_MESSAGE( "Create a user-issued asset with a precision of 6" );
   asset_objs.six_precision = &fixture->create_user_issued_asset( "SIXPRECISION", GRAPHENE_WITNESS_ACCOUNT(fixture->db),
         charge_market_fee, price(asset(1, asset_id_type(1)), asset(1)), 6 );

   BOOST_TEST_MESSAGE( "Create Prediction market with precision of 6, backed by SIXPRECISION" );
   asset_objs.prediction = &fixture->create_prediction_market( "PREDICTION", GRAPHENE_WITNESS_ACCOUNT,
         100, charge_market_fee, 6, asset_objs.six_precision->get_id() );

   return asset_objs;
}
/******
 * @brief Test various bitasset asserts within the asset_evaluator before the HF 922 / 931
 */
BOOST_AUTO_TEST_CASE( bitasset_evaluator_test_before_922_931 )
{
   BOOST_TEST_MESSAGE("Advance to near hard fork 922 / 931");
   auto global_params = db.get_global_properties().parameters;
   generate_blocks( HARDFORK_CORE_922_931_TIME - global_params.maintenance_interval );
   trx.set_expiration( HARDFORK_CORE_922_931_TIME - global_params.maintenance_interval + global_params.maximum_time_until_expiration );

   ACTORS( (nathan) (john) );

   assets_922_931 asset_objs = create_assets_922_931( this );
   const asset_id_type bit_usd_id = asset_objs.bit_usd->get_id();

   // make a generic operation
   bitasset_evaluator_wrapper evaluator;
   evaluator.set_db(db);
   asset_update_bitasset_operation op;
   op.asset_to_update = bit_usd_id;
   op.issuer = asset_objs.bit_usd->issuer;
   op.new_options = asset_objs.bit_usd->bitasset_data(db).options;

   // this should pass
   BOOST_TEST_MESSAGE( "Evaluating a good operation" );
   BOOST_CHECK( evaluator.evaluate(op) == void_result() );

   // test with a market issued asset
   BOOST_TEST_MESSAGE( "Sending a non-bitasset." );
   op.asset_to_update = asset_objs.user_issued->get_id();
   REQUIRE_EXCEPTION_WITH_TEXT( evaluator.evaluate(op), "on a non-BitAsset." );
   op.asset_to_update = bit_usd_id;

   // test changing issuer
   BOOST_TEST_MESSAGE( "Test changing issuer." );
   account_id_type original_issuer = op.issuer;
   op.issuer = john_id;
   REQUIRE_EXCEPTION_WITH_TEXT( evaluator.evaluate(op), "Only asset issuer can update" );
   op.issuer = original_issuer;

   // bad backing_asset
   BOOST_TEST_MESSAGE( "Non-existent backing asset." );
   asset_id_type correct_asset_id = op.new_options.short_backing_asset;
   op.new_options.short_backing_asset = asset_id_type();
   op.new_options.short_backing_asset.instance = 123;
   REQUIRE_EXCEPTION_WITH_TEXT( evaluator.evaluate(op), "Unable to find Object" );
   op.new_options.short_backing_asset = correct_asset_id;

   // now check the things that are wrong, but still pass before HF 922 / 931
   BOOST_TEST_MESSAGE( "Now check the things that are wrong, but still pass before HF 922 / 931" );

   // back by self
   BOOST_TEST_MESSAGE( "Message should contain: op.new_options.short_backing_asset == asset_obj.get_id()" );
   op.new_options.short_backing_asset = bit_usd_id;
   BOOST_CHECK( evaluator.evaluate(op) == void_result() );
   op.new_options.short_backing_asset = correct_asset_id;

   // prediction market with different precision
   BOOST_TEST_MESSAGE( "Message should contain: for a PM, asset_obj.precision != new_backing_asset.precision" );
   op.asset_to_update = asset_objs.prediction->get_id();
   op.issuer = asset_objs.prediction->issuer;
   BOOST_CHECK( evaluator.evaluate(op) == void_result() );
   op.asset_to_update = bit_usd_id;
   op.issuer = asset_objs.bit_usd->issuer;

   // checking old backing asset instead of new backing asset
   BOOST_TEST_MESSAGE( "Message should contain: to be backed by an asset which is not market issued asset nor CORE" );
   op.new_options.short_backing_asset = asset_objs.six_precision->get_id();
   BOOST_CHECK( evaluator.evaluate(op) == void_result() );
   BOOST_TEST_MESSAGE( "Message should contain: modified a blockchain-controlled market asset to be backed by an asset "
         "which is not backed by CORE" );
   op.new_options.short_backing_asset = asset_objs.prediction->get_id();
   BOOST_CHECK( evaluator.evaluate(op) == void_result() );
   op.new_options.short_backing_asset = correct_asset_id;

   // CHILDUSER is a non-committee asset backed by PARENT which is backed by CORE
   // Cannot change PARENT's backing asset from CORE to something that is not [CORE | UIA]
   // because that will make CHILD be backed by an asset that is not itself backed by CORE or a UIA.
   BOOST_TEST_MESSAGE( "Message should contain: but this asset is a backing asset for another MPA, which would cause MPA "
         "backed by MPA backed by MPA." );
   op.asset_to_update = asset_objs.bit_parent->get_id();
   op.issuer = asset_objs.bit_parent->issuer;
   op.new_options.short_backing_asset = asset_objs.bit_usdbacked->get_id();
   // this should generate a warning in the log, but not fail.
   BOOST_CHECK( evaluator.evaluate(op) == void_result() );
   // changing the backing asset to a UIA should work
   BOOST_TEST_MESSAGE( "Switching to a backing asset that is a UIA should work. No warning should be produced." );
   op.new_options.short_backing_asset = asset_objs.user_issued->get_id();
   BOOST_CHECK( evaluator.evaluate(op) == void_result() );
   // A -> B -> C, change B to be backed by A (circular backing)
   BOOST_TEST_MESSAGE( "Message should contain: A cannot be backed by B which is backed by A." );
   op.new_options.short_backing_asset = asset_objs.bit_child_user_issued->get_id();
   BOOST_CHECK( evaluator.evaluate(op) == void_result() );
   op.new_options.short_backing_asset = asset_objs.user_issued->get_id();
   BOOST_TEST_MESSAGE( "Message should contain: but this asset is a backing asset for a committee-issued asset." );
   // CHILDCOMMITTEE is a committee asset backed by PARENT which is backed by CORE
   // Cannot change PARENT's backing asset from CORE to something else because that will make CHILD be backed by
   // an asset that is not itself backed by CORE
   create_bitasset( "CHILDCOMMITTEE", GRAPHENE_COMMITTEE_ACCOUNT, 100, charge_market_fee, 2,
         asset_objs.bit_parent->get_id() );
   // it should again work, generating 2 warnings in the log. 1 for the above, and 1 new one.
   BOOST_CHECK( evaluator.evaluate(op) == void_result() );
   op.asset_to_update = asset_objs.bit_usd->get_id();
   op.issuer = asset_objs.bit_usd->issuer;
   op.new_options.short_backing_asset = correct_asset_id;

   // Feed lifetime must exceed block interval
   BOOST_TEST_MESSAGE( "Message should contain: op.new_options.feed_lifetime_sec <= chain_parameters.block_interval" );
   const auto good_feed_lifetime = op.new_options.feed_lifetime_sec;
   op.new_options.feed_lifetime_sec = db.get_global_properties().parameters.block_interval;
   BOOST_CHECK( evaluator.evaluate(op) == void_result() );
   BOOST_TEST_MESSAGE( "Message should contain: op.new_options.feed_lifetime_sec <= chain_parameters.block_interval" );
   op.new_options.feed_lifetime_sec = db.get_global_properties().parameters.block_interval - 1; // default interval > 1
   BOOST_CHECK( evaluator.evaluate(op) == void_result() );
   op.new_options.feed_lifetime_sec = good_feed_lifetime;

   // Force settlement delay must exceed block interval.
   BOOST_TEST_MESSAGE( "Message should contain: op.new_options.force_settlement_delay_sec <= chain_parameters.block_interval" );
   const auto good_force_settlement_delay_sec = op.new_options.force_settlement_delay_sec;
   op.new_options.force_settlement_delay_sec = db.get_global_properties().parameters.block_interval;
   BOOST_CHECK( evaluator.evaluate(op) == void_result() );
   BOOST_TEST_MESSAGE( "Message should contain: op.new_options.force_settlement_delay_sec <= chain_parameters.block_interval" );
   op.new_options.force_settlement_delay_sec = db.get_global_properties().parameters.block_interval - 1; // default interval > 1
   BOOST_CHECK( evaluator.evaluate(op) == void_result() );
   op.new_options.force_settlement_delay_sec = good_force_settlement_delay_sec;

   // this should pass
   BOOST_TEST_MESSAGE( "We should be all good again." );
   BOOST_CHECK( evaluator.evaluate(op) == void_result() );
}

/******
 * @brief Test various bitasset asserts within the asset_evaluator before the HF 922 / 931
 */
BOOST_AUTO_TEST_CASE( bitasset_evaluator_test_after_922_931 )
{
   BOOST_TEST_MESSAGE("Advance to after hard fork 922 / 931");
   auto global_params = db.get_global_properties().parameters;
   generate_blocks( HARDFORK_CORE_922_931_TIME + global_params.maintenance_interval );
   trx.set_expiration( HARDFORK_CORE_922_931_TIME + global_params.maintenance_interval + global_params.maximum_time_until_expiration );

   ACTORS( (nathan) (john) );

   assets_922_931 asset_objs = create_assets_922_931( this );
   const asset_id_type& bit_usd_id = asset_objs.bit_usd->get_id();

   // make a generic operation
   bitasset_evaluator_wrapper evaluator;
   evaluator.set_db( db );
   asset_update_bitasset_operation op;
   op.asset_to_update = bit_usd_id;
   op.issuer = asset_objs.bit_usd->issuer;
   op.new_options = asset_objs.bit_usd->bitasset_data(db).options;

   // this should pass
   BOOST_TEST_MESSAGE( "Evaluating a good operation" );
   BOOST_CHECK( evaluator.evaluate(op) == void_result() );

   // test with a market issued asset
   BOOST_TEST_MESSAGE( "Sending a non-bitasset." );
   op.asset_to_update = asset_objs.user_issued->get_id();
   REQUIRE_EXCEPTION_WITH_TEXT( evaluator.evaluate(op), "Cannot update BitAsset-specific settings on a non-BitAsset" );
   op.asset_to_update = bit_usd_id;

   // test changing issuer
   BOOST_TEST_MESSAGE( "Test changing issuer." );
   account_id_type original_issuer = op.issuer;
   op.issuer = john_id;
   REQUIRE_EXCEPTION_WITH_TEXT( evaluator.evaluate(op), "Only asset issuer can update" );
   op.issuer = original_issuer;

   // bad backing_asset
   BOOST_TEST_MESSAGE( "Non-existent backing asset." );
   asset_id_type correct_asset_id = op.new_options.short_backing_asset;
   op.new_options.short_backing_asset = asset_id_type();
   op.new_options.short_backing_asset.instance = 123;
   REQUIRE_EXCEPTION_WITH_TEXT( evaluator.evaluate(op), "Unable to find" );
   op.new_options.short_backing_asset = correct_asset_id;

   // now check the things that are wrong and won't pass after HF 922 / 931
   BOOST_TEST_MESSAGE( "Now check the things that are wrong and won't pass after HF 922 / 931" );

   // back by self
   BOOST_TEST_MESSAGE( "Back by itself" );
   op.new_options.short_backing_asset = bit_usd_id;
   REQUIRE_EXCEPTION_WITH_TEXT( evaluator.evaluate(op), "Cannot update an asset to be backed by itself" );
   op.new_options.short_backing_asset = correct_asset_id;

   // prediction market with different precision
   BOOST_TEST_MESSAGE( "Prediction market with different precision" );
   op.asset_to_update = asset_objs.prediction->get_id();
   op.issuer = asset_objs.prediction->issuer;
   REQUIRE_EXCEPTION_WITH_TEXT( evaluator.evaluate(op), "The precision of the asset and backing asset must" );
   op.asset_to_update = bit_usd_id;
   op.issuer = asset_objs.bit_usd->issuer;

   // checking old backing asset instead of new backing asset
   BOOST_TEST_MESSAGE( "Correctly checking new backing asset rather than old backing asset" );
   op.new_options.short_backing_asset = asset_objs.six_precision->get_id();
   REQUIRE_EXCEPTION_WITH_TEXT( evaluator.evaluate(op), "which is not market issued asset nor CORE." );
   op.new_options.short_backing_asset = asset_objs.prediction->get_id();
   REQUIRE_EXCEPTION_WITH_TEXT( evaluator.evaluate(op), "which is not backed by CORE" );
   op.new_options.short_backing_asset = correct_asset_id;

   // CHILD is a non-committee asset backed by PARENT which is backed by CORE
   // Cannot change PARENT's backing asset from CORE to something that is not [CORE | UIA]
   // because that will make CHILD be backed by an asset that is not itself backed by CORE or a UIA.
   BOOST_TEST_MESSAGE( "Attempting to change PARENT to be backed by a non-core and non-user-issued asset" );
   op.asset_to_update = asset_objs.bit_parent->get_id();
   op.issuer = asset_objs.bit_parent->issuer;
   op.new_options.short_backing_asset = asset_objs.bit_usdbacked->get_id();
   REQUIRE_EXCEPTION_WITH_TEXT( evaluator.evaluate(op), "A non-blockchain controlled BitAsset would be invalidated" );
   // changing the backing asset to a UIA should work
   BOOST_TEST_MESSAGE( "Switching to a backing asset that is a UIA should work." );
   op.new_options.short_backing_asset = asset_objs.user_issued->get_id();
   BOOST_CHECK( evaluator.evaluate(op) == void_result() );
   // A -> B -> C, change B to be backed by A (circular backing)
   BOOST_TEST_MESSAGE( "Check for circular backing. This should generate an exception" );
   op.new_options.short_backing_asset = asset_objs.bit_child_user_issued->get_id();
   REQUIRE_EXCEPTION_WITH_TEXT( evaluator.evaluate(op), "'A' backed by 'B' backed by 'A'" );
   op.new_options.short_backing_asset = asset_objs.user_issued->get_id();
   BOOST_TEST_MESSAGE( "Creating CHILDCOMMITTEE" );
   // CHILDCOMMITTEE is a committee asset backed by PARENT which is backed by CORE
   // Cannot change PARENT's backing asset from CORE to something else because that will make CHILD be backed by
   // an asset that is not itself backed by CORE
   create_bitasset( "CHILDCOMMITTEE", GRAPHENE_COMMITTEE_ACCOUNT, 100, charge_market_fee, 2,
         asset_objs.bit_parent->get_id() );
   // it should again not work
   REQUIRE_EXCEPTION_WITH_TEXT( evaluator.evaluate(op), "A blockchain-controlled market asset would be invalidated" );
   op.asset_to_update = asset_objs.bit_usd->get_id();
   op.issuer = asset_objs.bit_usd->issuer;
   op.new_options.short_backing_asset = correct_asset_id;

   // Feed lifetime must exceed block interval
   BOOST_TEST_MESSAGE( "Feed lifetime less than or equal to block interval" );
   const auto good_feed_lifetime = op.new_options.feed_lifetime_sec;
   op.new_options.feed_lifetime_sec = db.get_global_properties().parameters.block_interval;
   REQUIRE_EXCEPTION_WITH_TEXT( evaluator.evaluate(op), "Feed lifetime must exceed block" );
   op.new_options.feed_lifetime_sec = db.get_global_properties().parameters.block_interval - 1; // default interval > 1
   REQUIRE_EXCEPTION_WITH_TEXT( evaluator.evaluate(op), "Feed lifetime must exceed block" );
   op.new_options.feed_lifetime_sec = good_feed_lifetime;

   // Force settlement delay must exceed block interval.
   BOOST_TEST_MESSAGE( "Force settlement delay less than or equal to block interval" );
   const auto good_force_settlement_delay_sec = op.new_options.force_settlement_delay_sec;
   op.new_options.force_settlement_delay_sec = db.get_global_properties().parameters.block_interval;
   REQUIRE_EXCEPTION_WITH_TEXT( evaluator.evaluate(op), "Force settlement delay must" );
   op.new_options.force_settlement_delay_sec = db.get_global_properties().parameters.block_interval - 1; // default interval > 1
   REQUIRE_EXCEPTION_WITH_TEXT( evaluator.evaluate(op), "Force settlement delay must" );
   op.new_options.force_settlement_delay_sec = good_force_settlement_delay_sec;

   // this should pass
   BOOST_TEST_MESSAGE( "We should be all good again." );
   BOOST_CHECK( evaluator.evaluate(op) == void_result() );

}


/*****
 * @brief make sure feeds work correctly after changing from non-witness-fed to witness-fed before the 868 fork
 * NOTE: This test case is a different issue than what is currently being worked on, and fails. Hopefully it
 * will help when the fix for that issue is being coded.
 */
/*
BOOST_AUTO_TEST_CASE( reset_backing_asset_switching_to_witness_fed )
{
   ACTORS((nathan)(dan)(ben)(vikram));

   BOOST_TEST_MESSAGE("Advance to near hard fork");
   auto maint_interval = db.get_global_properties().parameters.maintenance_interval;
   generate_blocks( HARDFORK_CORE_868_890_TIME - maint_interval);
   trx.set_expiration(HARDFORK_CORE_868_890_TIME - fc::seconds(1));


   BOOST_TEST_MESSAGE("Create USDBIT");
   asset_id_type bit_usd_id = create_bitasset("USDBIT").id;
   asset_id_type core_id = bit_usd_id(db).bitasset_data(db).options.short_backing_asset;

   {
      BOOST_TEST_MESSAGE("Update the USDBIT asset options");
      change_asset_options(*this, nathan_id, nathan_private_key, bit_usd_id, false );
   }

   BOOST_TEST_MESSAGE("Create JMJBIT based on USDBIT.");
   asset_id_type bit_jmj_id = create_bitasset("JMJBIT").id;
   {
      BOOST_TEST_MESSAGE("Update the JMJBIT asset options");
      change_asset_options(*this, nathan_id, nathan_private_key, bit_jmj_id, false );
   }
   {
      BOOST_TEST_MESSAGE("Update the JMJBIT bitasset options");
      asset_update_bitasset_operation ba_op;
      const asset_object& obj = bit_jmj_id(db);
      ba_op.asset_to_update = obj.get_id();
      ba_op.issuer = obj.issuer;
      ba_op.new_options.short_backing_asset = bit_usd_id;
      ba_op.new_options.minimum_feeds = 1;
      trx.operations.push_back(ba_op);
      sign(trx, nathan_private_key);
      PUSH_TX(db, trx, ~0);
      generate_block();
      trx.clear();
   }
   {
      BOOST_TEST_MESSAGE("Set feed producers for JMJBIT");
      asset_update_feed_producers_operation op;
      op.asset_to_update = bit_jmj_id;
      op.issuer = nathan_id;
      op.new_feed_producers = {dan_id, ben_id, vikram_id};
      trx.operations.push_back(op);
      sign( trx, nathan_private_key );
      PUSH_TX( db, trx, ~0 );
      generate_block();
      trx.clear();
   }
   {
      BOOST_TEST_MESSAGE("Verify feed producers are registered for JMJBIT");
      const asset_bitasset_data_object& obj = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(obj.feeds.size(), 3ul);
      BOOST_CHECK(obj.current_feed == price_feed());


      BOOST_CHECK_EQUAL("1", std::to_string(obj.options.short_backing_asset.space_id));
      BOOST_CHECK_EQUAL("3", std::to_string(obj.options.short_backing_asset.type_id));
      BOOST_CHECK_EQUAL("1", std::to_string(obj.options.short_backing_asset.instance.value));

      BOOST_CHECK_EQUAL("1", std::to_string(bit_jmj_id.space_id));
      BOOST_CHECK_EQUAL("3", std::to_string(bit_jmj_id.type_id));
      BOOST_CHECK_EQUAL("2", std::to_string(bit_jmj_id.instance.value));
   }
   {
      BOOST_TEST_MESSAGE("Adding Vikram's price feed");
      add_price_feed(*this, vikram_id, bit_usd_id, 1, bit_jmj_id, 300, core_id);

      const asset_bitasset_data_object& bitasset = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(bitasset.current_feed.settlement_price.to_real(), 300.0);
      BOOST_CHECK(bitasset.current_feed.maintenance_collateral_ratio == GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO);
   }
   {
      BOOST_TEST_MESSAGE("Change JMJBIT to be witness_fed");
      optional<account_id_type> noone;
      change_asset_options(*this, noone, nathan_private_key, bit_jmj_id, true );
   }
   {
      BOOST_TEST_MESSAGE("Change underlying asset of bit_jmj from bit_usd to core");
      change_backing_asset(*this, nathan_private_key, bit_jmj_id, core_id);

      BOOST_TEST_MESSAGE("Verify feed producers have not been reset");
      const asset_bitasset_data_object& jmj_obj = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(jmj_obj.feeds.size(), 3ul);
      int nan_count = 0;
      for(const auto& feed : jmj_obj.feeds) {
         if(feed.second.second.settlement_price.is_null())
            ++nan_count;
      }
      BOOST_CHECK_EQUAL(nan_count, 2);
   }
   {
      BOOST_TEST_MESSAGE("Add a new (and correct) feed price from a witness");
      auto& global_props = db.get_global_properties();
      std::vector<account_id_type> active_witnesses;
      const witness_id_type& first_witness_id = (*global_props.active_witnesses.begin());
      const account_id_type witness_account_id = first_witness_id(db).witness_account;
      add_price_feed(*this, witness_account_id, core_id, 1, bit_jmj_id, 300, core_id);

      // we should have 2 feeds nan, 1 old feed with wrong asset, and 1 witness feed
      const asset_bitasset_data_object& jmj_obj = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(jmj_obj.feeds.size(), 4ul);
      int nan_count = 0;
      for(const auto& feed : jmj_obj.feeds) {
         if ( feed.second.second.settlement_price.is_null() )
            ++nan_count;
      }
      BOOST_CHECK_EQUAL(nan_count, 2);
   }
   {
      BOOST_TEST_MESSAGE("Advance to past hard fork");
      generate_blocks( HARDFORK_CORE_868_890_TIME + maint_interval);
      trx.set_expiration(HARDFORK_CORE_868_890_TIME + fc::hours(48));

      BOOST_TEST_MESSAGE("Verify that the incorrect feeds have been removed");
      const asset_bitasset_data_object& jmj_obj = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(jmj_obj.feeds.size(), 1ul);
      BOOST_CHECK( ! (*jmj_obj.feeds.begin()).second.second.settlement_price.is_null() );
      // the settlement price will be NaN until 50% of price feeds are valid
      //BOOST_CHECK_EQUAL(jmj_obj.current_feed.settlement_price.to_real(), 300);
   }
   {
      BOOST_TEST_MESSAGE("After hardfork, change underlying asset of bit_jmj from core to bit_usd");
      change_backing_asset(*this, nathan_private_key, bit_jmj_id, bit_usd_id);

      BOOST_TEST_MESSAGE("Verify feed producers have been reset");
      const asset_bitasset_data_object& jmj_obj = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(jmj_obj.feeds.size(), 0ul);
   }
   {
      BOOST_TEST_MESSAGE("With underlying bitasset changed from one to another, price feeds should still be publish-able");
      auto& global_props = db.get_global_properties();
      std::vector<account_id_type> active_witnesses;
      for(const auto& witness_id : global_props.active_witnesses)
      {
         active_witnesses.push_back(witness_id(db).witness_account);
      }
      BOOST_TEST_MESSAGE("Adding Witness 0's price feed");
      add_price_feed(*this, active_witnesses[0], bit_usd_id, 1, bit_jmj_id, 30, core_id);

      const asset_bitasset_data_object& bitasset = bit_jmj_id(db).bitasset_data(db);
      BOOST_CHECK_EQUAL(bitasset.current_feed.settlement_price.to_real(), 30);
      BOOST_CHECK(bitasset.current_feed.maintenance_collateral_ratio == GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO);

      BOOST_TEST_MESSAGE("Adding Witness 1's pricing to JMJBIT");
      add_price_feed(*this, active_witnesses[0], bit_usd_id, 1, bit_jmj_id, 25, core_id);

      BOOST_CHECK_EQUAL(bitasset.current_feed.settlement_price.to_real(), 30);
      BOOST_CHECK(bitasset.current_feed.maintenance_collateral_ratio == GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO);

      BOOST_TEST_MESSAGE("Adding Witness 2's pricing to JMJBIT");
      add_price_feed(*this, active_witnesses[2], bit_usd_id, 1, bit_jmj_id, 10, core_id);

      BOOST_CHECK_EQUAL(bitasset.current_feed.settlement_price.to_real(), 25);
      BOOST_CHECK(bitasset.current_feed.maintenance_collateral_ratio == GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO);
      generate_block();
      trx.clear();

      BOOST_CHECK(bitasset.current_feed.core_exchange_rate.base.asset_id != bitasset.current_feed.core_exchange_rate.quote.asset_id);
   }
}
*/

BOOST_AUTO_TEST_SUITE_END()
