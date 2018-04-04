//
// Created by Lubos Ilcik on 30/03/2018.
//

#include <boost/test/unit_test.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
using namespace graphene::chain;

BOOST_AUTO_TEST_SUITE(coin_days_vesting_balance_tests)

struct vesting_balance_fixture {
   vesting_balance_object vesting_balance;
   const asset initial_balance;
   const fc::time_point_sec beginning;
   const uint32_t vesting_seconds;

   vesting_balance_fixture() : vesting_balance(), initial_balance(100), beginning(), vesting_seconds(100) {
      vesting_balance.owner = account_id_type();
      vesting_balance.balance = initial_balance;
      cdd_vesting_policy policy;
      policy.vesting_seconds = vesting_seconds;
      policy.coin_seconds_earned = 0;
      policy.coin_seconds_earned_last_update = beginning;
      vesting_balance.policy = policy;
   }
};

BOOST_FIXTURE_TEST_CASE(when_vesting_seconds_passed_withdraw_full_balance_is_allowed, vesting_balance_fixture)
{
   BOOST_CHECK(!vesting_balance.is_withdraw_allowed(beginning, initial_balance));
   for (int sec = 1; sec < vesting_seconds; ++sec) {
      BOOST_CHECK(!vesting_balance.is_withdraw_allowed(beginning + sec, initial_balance));
   }
   BOOST_CHECK(vesting_balance.is_withdraw_allowed(beginning + vesting_seconds, initial_balance));
}

BOOST_FIXTURE_TEST_CASE(when_seconds_passes_allowed_withdraw_increases_proportionally, vesting_balance_fixture)
{
   BOOST_CHECK_EQUAL(vesting_balance.get_allowed_withdraw(beginning).amount.value, 0);
   // initial vesting balance and vesting seconds set to 100 for easy testing
   for (int i = 1; i < vesting_seconds; ++i) {
      BOOST_CHECK_EQUAL(vesting_balance.get_allowed_withdraw(beginning + i).amount.value, i);
   }
   BOOST_CHECK_EQUAL(vesting_balance.get_allowed_withdraw(beginning + vesting_seconds).amount.value, initial_balance.amount.value);
}

BOOST_FIXTURE_TEST_CASE(when_deposit_the_respective_portion_is_vested_as_time_passes, vesting_balance_fixture)
{
   share_type vested_amount;
   uint32_t delta_seconds = 50;
   share_type allowed_withdraw_after_delta_seconds = delta_seconds; // initial vesting balance and vesting seconds set to 100 for easy testing
   share_type deposit_amount = 100;

   /// we deposit after delta_seconds
   //    precondition
   BOOST_CHECK_EQUAL(vesting_balance.get_allowed_withdraw(beginning + delta_seconds).amount.value, allowed_withdraw_after_delta_seconds.value);
   vesting_balance.deposit(beginning + delta_seconds, asset(deposit_amount));
   //    postcondition
   BOOST_CHECK_EQUAL(vesting_balance.balance.amount.value, initial_balance.amount.value + deposit_amount.value);
   BOOST_CHECK_EQUAL(vesting_balance.get_allowed_withdraw(beginning + delta_seconds).amount.value, allowed_withdraw_after_delta_seconds.value);

   /// after some more seconds portion of initial balance and portion of deposit amount are vested
   uint32_t check_point = delta_seconds + 10;
   vested_amount = initial_balance.amount * check_point / vesting_seconds;
   vested_amount += deposit_amount * (check_point - delta_seconds) / vesting_seconds;
   BOOST_CHECK_EQUAL(vesting_balance.get_allowed_withdraw(beginning + check_point).amount.value, vested_amount.value);

   /// after vesting seconds the initial balance is vested + portion of the new deposit
   vested_amount = initial_balance.amount + (vesting_seconds - delta_seconds) * deposit_amount / vesting_seconds;
   BOOST_CHECK_EQUAL(vesting_balance.get_allowed_withdraw(beginning + vesting_seconds).amount.value, vested_amount.value);

   /// after vesting seconds + delta_seconds the total balance is vested
   BOOST_CHECK_EQUAL(vesting_balance.get_allowed_withdraw(beginning + vesting_seconds + delta_seconds).amount.value, (initial_balance.amount + deposit_amount).value);
}

BOOST_FIXTURE_TEST_CASE(when_deposit_is_added_allowed_withdraw_increases, vesting_balance_fixture)
{
   uint32_t delta_seconds = 10;
   share_type deposit_amount = 100;
   asset previous_allowed_withdraw = asset(0);
   /// reset initial balance
   vesting_balance.balance = asset(0);
   for (int step = delta_seconds; step < vesting_seconds;) {
      fc::time_point_sec point = beginning + step;
      vesting_balance.deposit(point, asset(deposit_amount));
      if (step > delta_seconds)
         BOOST_CHECK_GT(vesting_balance.get_allowed_withdraw(point).amount.value, previous_allowed_withdraw.amount.value);
      previous_allowed_withdraw = vesting_balance.get_allowed_withdraw(point);
      step += delta_seconds;
   }
}

BOOST_FIXTURE_TEST_CASE(when_witdraw_total_vested_amount_withdraw_not_allowed, vesting_balance_fixture)
{
   uint32_t delta_seconds = 10;
   share_type withdraw_amount = vesting_balance.get_allowed_withdraw(beginning + delta_seconds).amount;
   vesting_balance.withdraw(beginning + delta_seconds, asset(withdraw_amount));
   BOOST_CHECK_EQUAL(vesting_balance.balance.amount.value, initial_balance.amount.value - withdraw_amount.value);
   BOOST_CHECK_GT(vesting_balance.balance.amount.value, 0);
   BOOST_CHECK_EQUAL(vesting_balance.get_allowed_withdraw(beginning + delta_seconds).amount.value, 0);
}

BOOST_FIXTURE_TEST_CASE(when_vested_portion_withdrawn_no_exception, vesting_balance_fixture)
{
   uint32_t delta_seconds = 10;
   for (int step = delta_seconds; step < vesting_seconds;) {
      share_type withdraw_amount = vesting_balance.get_allowed_withdraw(beginning + step).amount;
      vesting_balance.withdraw(beginning + step, asset(withdraw_amount));
      step += delta_seconds;
   }
   BOOST_CHECK_GT(vesting_balance.balance.amount.value, 0);
}

BOOST_FIXTURE_TEST_CASE(test, vesting_balance_fixture)
{
   share_type withdraw_amount = 50;
   share_type deposit_amount = 10;
   uint32_t withdraw_delta_seconds = 50;
   uint32_t deposit_delta_seconds = 10;
   uint32_t check_point_delta_seconds = 10;

   /// we withdraw at point +withdraw_delta_seconds
   vesting_balance.withdraw(beginning + withdraw_delta_seconds, asset(withdraw_amount));
   /// we deposit at point +(withdraw_delta_seconds + deposit_delta_seconds)
   vesting_balance.deposit(beginning + withdraw_delta_seconds + deposit_delta_seconds, asset(deposit_amount));
   /// we check at point +(withdraw_delta_seconds + deposit_delta_seconds + check_point_delta_seconds)
   auto allowed_withdraw = vesting_balance.get_allowed_withdraw(beginning + withdraw_delta_seconds + deposit_delta_seconds + check_point_delta_seconds);
   /// we expect initial balance - withdrawal portion to be vested
   share_type expected_allowed_withdraw = (initial_balance.amount - withdraw_amount) * (deposit_delta_seconds + check_point_delta_seconds) / vesting_seconds;
   /// we expect deposit portion to be vested
   expected_allowed_withdraw += deposit_amount * check_point_delta_seconds / vesting_seconds;

   BOOST_CHECK_EQUAL(allowed_withdraw.amount.value, expected_allowed_withdraw.value);
}

BOOST_AUTO_TEST_SUITE_END()