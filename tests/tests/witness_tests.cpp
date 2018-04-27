//
// Created by Lubos Ilcik on 11/03/2018.
//

#include <boost/test/unit_test.hpp>
#include "../common/database_fixture.hpp"
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE(witness_tests, database_fixture)

struct witness_tests_fixture: public database_fixture {

   const int64_t core_max_supply = GRAPHENE_MAX_SHARE_SUPPLY;

   const share_type core_precision = asset::scaled_precision( asset_id_type()(db).precision );
   const uint8_t block_interval = db.get_global_properties().parameters.block_interval;
   const uint32_t maintenance_interval = db.get_global_properties().parameters.maintenance_interval;
   const uint32_t blocks_to_maintain = maintenance_interval / block_interval;


   const time_point_sec next_maintenance_time() const { return db.get_dynamic_global_properties().next_maintenance_time; }

   const uint64_t membership_lifetime_fee() const { return db.current_fee_schedule().get<account_upgrade_operation>().membership_lifetime_fee; }

   const asset_object& core() const { return asset_id_type()(db); }
   const share_type core_reserve() const { return core().reserved(db); }
   const share_type core_current_supply() const { return core().dynamic_data(db).current_supply; }
   const share_type core_accumulated_fees() const { return core().dynamic_asset_data_id(db).accumulated_fees; }

   const share_type witness_budget() const { return db.get_dynamic_global_properties().witness_budget; }
   const share_type witness_pay() const { return db.get_dynamic_global_properties().witness_pay; }

   const asset nathan_init_assets = asset(20000 * core_precision);

   const account_object& nathan_account() const { return get_account("nathan"); }
   const account_object& committee_account() const { return account_id_type()(db); }

   const uint64_t nathan_lifetime_fee_network_cut() const { return lifetime_fee_network_cut_for_account(nathan_account()); }
   const uint64_t nathan_lifetime_fee_referral_cut() const { return lifetime_fee_referral_cut_for_account(nathan_account()); }

   const uint64_t lifetime_fee_network_cut_for_account(const account_object& account) const { return account.network_fee_percentage * membership_lifetime_fee() / GRAPHENE_100_PERCENT; }
   const uint64_t lifetime_fee_referral_cut_for_account(const account_object &account) const { return account.lifetime_referrer(db).lifetime_referrer_fee_percentage * membership_lifetime_fee() / GRAPHENE_100_PERCENT; }

   const share_type pending_fees_for_account(const account_object& account) const {
      const account_statistics_object &statistics = account.statistics(db);
      return statistics.pending_vested_fees + statistics.pending_fees;
   }
   const share_type fees_paid_from_account(const account_object& account) const { return account.statistics(db).lifetime_fees_paid; }
   
   void upgrade_nathan_account_to_lifetime_member() {
      BOOST_TEST_MESSAGE( "Upgrading nathan account to LTM" );
      BOOST_CHECK_GT(membership_lifetime_fee(), 0); // fee must be set

      account_upgrade_operation uop;
      uop.account_to_upgrade = nathan_account().get_id();
      uop.upgrade_to_lifetime_member = true;
      set_expiration(db, trx);
      trx.operations.push_back(uop);
      for(auto& op : trx.operations) db.current_fee_schedule().set_fee(op);
      trx.validate();
      sign(trx, init_account_priv_key);
      PUSH_TX(db, trx);
      trx.clear();

      BOOST_CHECK(nathan_account().is_lifetime_member());
      BOOST_CHECK_EQUAL(get_balance(nathan_account(), core()), nathan_init_assets.amount.value - membership_lifetime_fee());
   }

   void create_nathan_account_and_transfer_assets(const asset &nathan_init_assets) {
      BOOST_TEST_MESSAGE("Creating nathan account");
      create_account("nathan", init_account_pub_key);

      BOOST_TEST_MESSAGE("Transfering init assets to nathan account");
      transfer(committee_account(), nathan_account(), nathan_init_assets);

      BOOST_CHECK_EQUAL(get_balance(nathan_account(), core()), nathan_init_assets.amount.value);
   }

   const share_type signing_witness_balance() const {
      const witness_object& wit = db.fetch_block_by_number(db.head_block_num())->witness(db);
      if( !wit.pay_vb.valid() )
         return 0;
      return (*wit.pay_vb)(db).balance.amount;
   }

   void schedule_maint() {
      BOOST_TEST_MESSAGE("Scheduling maintenance");
      db.modify(db.get_dynamic_global_properties(), [&](dynamic_global_property_object& _dpo)
      {
         _dpo.next_maintenance_time = db.head_block_time() + 1;
      });
   }

   void generate_block_times(int block_count) {
      BOOST_TEST_MESSAGE("Generating some blocks");
      const auto start_head_block_time = db.head_block_time().sec_since_epoch();
      const int target_interval = block_count * block_interval;
      while(db.head_block_time().sec_since_epoch() - start_head_block_time < target_interval)
      {
         generate_block();
      }
      const uint32_t elapsed_interval = db.head_block_time().sec_since_epoch() - start_head_block_time;
      BOOST_CHECK_EQUAL(elapsed_interval, target_interval);
   }

   void generate_block() {
      BOOST_TEST_MESSAGE("Generating block");
      database_fixture::generate_block();
   }

   witness_tests_fixture() {
      BOOST_TEST_MESSAGE("Generating initial block");
      // there is an immediate maintenance interval in the first block
      //   which will initialize last_budget_time
      database_fixture::generate_block();
   }

};

BOOST_FIXTURE_TEST_CASE(check_witness_budget_and_pay_per_block_calculations, witness_tests_fixture)
{
   try {
      /// 1. create a test account and give him some money
      // initial state
      BOOST_CHECK_EQUAL(core_current_supply().value, core_max_supply);
      BOOST_CHECK_EQUAL(core_reserve().value, 0);
      BOOST_CHECK_EQUAL(witness_budget().value, 0);
      BOOST_CHECK_EQUAL(core_accumulated_fees().value, 0);

      create_nathan_account_and_transfer_assets(nathan_init_assets);

      generate_block(); // no witness payment yet
      BOOST_CHECK_EQUAL(signing_witness_balance().value, 0);

      /// 2. spend some fees to create funds for witness pay
      enable_fees(); // enable fees to pay LTM membership fee
      upgrade_nathan_account_to_lifetime_member();

      // lifetime fee network cut goes to  accumulated_fees
      BOOST_CHECK_EQUAL(core_accumulated_fees().value, nathan_lifetime_fee_network_cut());
      // lifetime referral fee cut went to the committee account, which burned it -> goes to reserve
      BOOST_CHECK_EQUAL(core_reserve().value, nathan_lifetime_fee_referral_cut());
      BOOST_CHECK_LT(core_current_supply().value, core_max_supply);

      // the 1st maintenance interval pending - no witness budget yet
      BOOST_CHECK_EQUAL(witness_budget().value, 0);

      generate_block(); // no witness payment yet
      BOOST_CHECK_EQUAL(signing_witness_balance().value, 0);

      /// 3. trigger maintenance to calculate witness pay budget
      // store pre-maintenance state
      auto prev_accumulated_fees = core_accumulated_fees();
      auto prev_core_reserve = core_reserve();

      // schedule maintenance so that the next block triggers maintenance processing
      //    we expect following witness budget (unspent witness budget is 0)
      auto expected_witness_budget = prev_accumulated_fees + prev_core_reserve;
      schedule_maint();
      generate_block(); // => maintenance interval

      // post-maintenance state
      BOOST_CHECK_EQUAL(witness_budget().value, expected_witness_budget.value);
      //    accumulated fees are consumed now
      BOOST_CHECK_EQUAL(core_accumulated_fees().value, 0);
      //    core reserve becomes 0
      auto core_reserve_delta = expected_witness_budget.value - prev_accumulated_fees.value; // (unspent witness budget is 0)
      BOOST_CHECK_EQUAL(core_reserve().value, prev_core_reserve.value - core_reserve_delta);
      BOOST_CHECK_EQUAL(core_reserve().value, 0);

      // after the maintenance block the witness is paid from old budget (=0, so no pay)
      BOOST_CHECK_EQUAL(signing_witness_balance().value, 0);

      /// 4. generate blocks for which the witnesses should get paid
      // second witness finally gets paid!
      generate_block();
      auto witness_pay_per_block = expected_witness_budget - witness_budget();
      BOOST_CHECK_EQUAL(signing_witness_balance().value, witness_pay_per_block.value);

      // after each block the witness budget is reduced by the witness pay
      generate_block();
      BOOST_CHECK_EQUAL(signing_witness_balance().value, witness_pay_per_block.value);
      BOOST_CHECK_EQUAL(witness_budget().value, expected_witness_budget.value - 2 * witness_pay_per_block.value);

      // verify witness pay value
      BOOST_CHECK_EQUAL(witness_pay_per_block.value, expected_witness_budget.value / blocks_to_maintain);

   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(when_witness_budget_is_consumed_the_witnesses_are_not_paid, witness_tests_fixture)
{
   try {
      /// create a test account and transfer money
      create_nathan_account_and_transfer_assets(nathan_init_assets);
      generate_block();
      BOOST_CHECK_EQUAL(signing_witness_balance().value, 0);
      /// create some accumulated fees by paying account upgrade fee
      enable_fees();
      upgrade_nathan_account_to_lifetime_member();
      generate_block();
      BOOST_CHECK_EQUAL(signing_witness_balance().value, 0);
      /// schedule maintenance
      schedule_maint();
      generate_block(); // => maintenance interval
      BOOST_CHECK_EQUAL(signing_witness_balance().value, 0);
      auto init_witness_budget = witness_budget();
      /// check the 1st witness is paid
      generate_block(); // => 1st pay
      auto witness_pay_per_block = init_witness_budget - witness_budget();
      BOOST_CHECK_EQUAL(signing_witness_balance().value, witness_pay_per_block.value);
      /// simulate budget consumption prior to the next maintenance interval
      auto left_witness_budget = witness_budget();
      db.modify(db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& _dpo) {
         _dpo.witness_budget = witness_pay_per_block;
      });
      // we must put left budget back to the accumulated fees to silence assert in database_fixture::verify_asset_supplies
      db.modify(asset_id_type(0)(db).dynamic_asset_data_id(db), [&]( asset_dynamic_data_object& _core) {
         _core.accumulated_fees = left_witness_budget - witness_pay_per_block;
      });
      BOOST_CHECK_EQUAL(witness_budget().value, witness_pay_per_block.value);
      /// consume left budget
      generate_block(); // => last pay
      BOOST_CHECK_EQUAL(signing_witness_balance().value, witness_pay_per_block.value);
      BOOST_CHECK_EQUAL(witness_budget().value, 0);
      generate_block(); // => no pay
      BOOST_CHECK_EQUAL(signing_witness_balance().value, 0);
      BOOST_CHECK_EQUAL(witness_budget().value, 0);

   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(when_witness_budget_is_not_consumed_the_left_budget_is_used_in_next_period, witness_tests_fixture)
{
   try {
      /// create a test account and transfer money
      create_nathan_account_and_transfer_assets(nathan_init_assets);
      generate_block();
      /// create some accumulated fees by paying account upgrade fee
      enable_fees();
      upgrade_nathan_account_to_lifetime_member();
      generate_block();
      /// schedule maintenance
      schedule_maint();
      generate_block(); // => maintenance interval
      auto init_witness_budget = witness_budget();
      /// the 1st witness is paid
      generate_block(); // => 1st pay
      int paid_block_count = 1;
      auto witness_pay_per_block = init_witness_budget - witness_budget();
      /// generate some more blocks
      generate_block_times(10);
      paid_block_count += 10;

      /// schedule next maintenance interval
      //    store pre-maintenance state
      auto left_witness_budget = witness_budget();
      auto accumulated_fees = core_accumulated_fees();
      auto reserve = core_reserve();
      BOOST_CHECK_EQUAL(left_witness_budget.value, init_witness_budget.value - paid_block_count * witness_pay_per_block.value);
      BOOST_CHECK_EQUAL(accumulated_fees.value, 0);
      BOOST_CHECK_EQUAL(reserve.value, 0);

      schedule_maint();
      generate_block(); // => maintenance interval + last pay from previous budget
      auto unused_witness_budget = left_witness_budget - witness_pay_per_block;
      //    we expect the new budget will be unused_witness_budget (accumulated_fees = reserve = 0)
      auto expected_witness_budget = unused_witness_budget;
      share_type expected_witness_pay = expected_witness_budget / blocks_to_maintain;
      BOOST_CHECK_EQUAL(witness_budget().value, expected_witness_budget.value);
      BOOST_CHECK_EQUAL(witness_pay().value, expected_witness_pay.value);
      BOOST_CHECK_EQUAL(accumulated_fees.value, 0);
      BOOST_CHECK_EQUAL(reserve.value, 0);

   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(when_witness_budget_is_not_consumed_and_fees_accumulated_the_funds_are_used_in_next_period, witness_tests_fixture)
{
   try {
      /// create a test account and transfer money
      create_nathan_account_and_transfer_assets(nathan_init_assets);
      create_account("alice"); // will be used later
      create_account("bob"); // will be used later
      generate_block();
      /// create some accumulated fees by paying account upgrade fee
      enable_fees();
      upgrade_nathan_account_to_lifetime_member();
      generate_block();
      /// schedule maintenance
      schedule_maint();
      generate_block(); // => maintenance interval
      auto init_witness_budget = witness_budget();
      /// the 1st witness is paid
      generate_block(); // => 1st pay
      int paid_block_count = 1;
      auto witness_pay_per_block = init_witness_budget - witness_budget();
      /// generate some more blocks
      generate_block_times(10);
      paid_block_count += 10;

      /// make some transaction to accumulate fees
      const account_object &committee = committee_account();
      const account_object &alice_account = get_account("alice");
      const account_object &bob_account = get_account("bob");
      //    fund alice from committee account
      transfer(committee, alice_account, asset(2000 * core_precision));
      //    committee pays transfer fee which is split into
      //    - network cut => goes to accumulated fees
      //    - lifetime cut => goes to reserve since LTM referer is also committee account
      auto committee_pending_fees = pending_fees_for_account(committee);
      auto committee_network_cut = committee_pending_fees * committee.network_fee_percentage / GRAPHENE_100_PERCENT;
      auto committee_reserve_cut = committee_pending_fees - committee_network_cut;
      //    transfer some money from alice to bob
      transfer(alice_account, bob_account, asset(1000 * core_precision));
      //    alice pays transfer fee which is split into
      //    - network cut => goes to accumulated fees
      //    - lifetime cut => goes to reserve since LTM referer is committee account
      //    - registrar cut => goes to reserve since registrar is committee account
      // (referer cut = 0 because referer reward percentage = 0)
      auto alice_pending_fees = pending_fees_for_account(alice_account);
      auto alice_network_cut = alice_pending_fees * alice_account.network_fee_percentage / GRAPHENE_100_PERCENT;
      auto alice_reserve_cut = alice_pending_fees - alice_network_cut;

      generate_block();
      paid_block_count += 1;

      /// schedule next maintenance interval
      //    store pre-maintenance state
      auto left_witness_budget = witness_budget();
      //    pre-conditions
      BOOST_CHECK_EQUAL(left_witness_budget.value, init_witness_budget.value - paid_block_count * witness_pay_per_block.value);
      BOOST_CHECK_EQUAL(core_accumulated_fees().value, 0);
      BOOST_CHECK_EQUAL(core_reserve().value, 0);

      //    trigger maintenance processing
      schedule_maint();
      generate_block(); // => maintenance interval + last pay from previous budget
      auto unused_witness_budget = left_witness_budget - witness_pay_per_block;
      //    we expect the new budget will be unused_witness_budget + accumulated_fees + reserve
      auto accumulated_fees = alice_network_cut + committee_network_cut;
      auto reserve = alice_reserve_cut + committee_reserve_cut;
      auto expected_witness_budget = unused_witness_budget + accumulated_fees + reserve;
      auto expected_witness_pay = expected_witness_budget / blocks_to_maintain;
      //    post-conditions
      BOOST_CHECK_EQUAL(witness_budget().value, expected_witness_budget.value);
      BOOST_CHECK_EQUAL(witness_pay().value, expected_witness_pay.value);
      //    accumulated fees are moved to the new witness budget
      BOOST_CHECK_EQUAL(core_accumulated_fees().value, 0);
      //    reserve is moved to the new witness budget
      BOOST_CHECK_EQUAL(core_reserve().value, 0);
      BOOST_CHECK_EQUAL(fees_paid_from_account(alice_account).value, alice_pending_fees.value);
      BOOST_CHECK_EQUAL(fees_paid_from_account(committee).value, committee_pending_fees.value);

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

