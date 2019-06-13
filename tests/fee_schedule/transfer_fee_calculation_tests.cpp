//
// Created by Lubos Ilcik on 27/01/2018.
//

#include <boost/test/unit_test.hpp>

#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/protocol/operations_permissions.hpp>

using namespace graphene::chain;

BOOST_AUTO_TEST_SUITE(transfer_fee_calculation_tests)

BOOST_AUTO_TEST_CASE(calculate_flat_transfer_fee)
{
    // fee calculation input parameters
    uint64_t fee = 1000;
    uint32_t scale = GRAPHENE_100_PERCENT; // the rationale for scale is unknown
                                           // fee value is multiplied by (scale / GRAPHENE_100_PERCENT)
    uint64_t base_amount  = 2; // the base asset is the asset being sold
    uint64_t quote_amount = 1; // the quote asset is the asset being purchased
                              // exchange_rate = quote_amount / base_amount

    fee_schedule schedule;

    // set fee parameter
    transfer_operation::fee_parameters_type transfer_fee;
    transfer_fee.fee = fee;
    schedule.parameters.insert(transfer_fee); // overwrite default value

    // set scale parameter
    schedule.scale = scale; // schedule.scale default value is GRAPHENE_100_PERCENT

    // set price parameters
    price price(asset(base_amount, asset_id_type(0)), asset(quote_amount, asset_id_type(1)));

    // calculate operation fee with given input parameters
    auto operation = transfer_operation();
    auto calculatedFee = schedule.calculate_fee(operation, price);

    // expected value
    uint64_t expected_fee = fee * quote_amount * scale / (base_amount * GRAPHENE_100_PERCENT);
    BOOST_CHECK_EQUAL(static_cast<uint64_t>(calculatedFee.amount.value), expected_fee);
}

BOOST_AUTO_TEST_CASE(when_transfer_amount_is_0_calculated_fee_is_flat_fee)
{
    uint64_t flat_fee = 1000;

    fee_schedule schedule;

    // set fee parameter
    transfer_operation::fee_parameters_type transfer_fee;
    transfer_fee.fee = flat_fee;
    transfer_fee.percentage = (GRAPHENE_100_PERCENT/1000); // 0.1%
    transfer_fee.percentage_max_fee = 300 * GRAPHENE_BLOCKCHAIN_PRECISION;
    schedule.parameters.insert(transfer_fee); // overwrite default value

    // calculate operation fee with given input parameters
    auto operation = transfer_operation();
    operation.amount = asset();
    auto calculatedFee = schedule.calculate_fee(operation, price::unit_price());

    BOOST_CHECK_EQUAL(static_cast<uint64_t>(calculatedFee.amount.value), flat_fee);
}

BOOST_AUTO_TEST_CASE(when_percentage_is_0_calculated_fee_is_flat_fee)
{
    uint64_t flat_fee = 1000;

    fee_schedule schedule;

    // set fee parameter
    transfer_operation::fee_parameters_type transfer_fee;
    transfer_fee.fee = flat_fee;
    transfer_fee.percentage = 0;
    transfer_fee.percentage_max_fee = 300 * GRAPHENE_BLOCKCHAIN_PRECISION;
    schedule.parameters.insert(transfer_fee); // overwrite default value

    // calculate operation fee with given input parameters
    auto operation = transfer_operation();
    operation.amount = asset(1000 * GRAPHENE_BLOCKCHAIN_PRECISION);
    auto calculatedFee = schedule.calculate_fee(operation, price::unit_price());

    BOOST_CHECK_EQUAL(static_cast<uint64_t>(calculatedFee.amount.value), flat_fee);
}

BOOST_AUTO_TEST_CASE(when_percentage_and_transfer_amount_are_not_0_calculated_fee_is_percentage_fee)
{
    uint64_t min_fee = 1000;
    uint16_t percentage = GRAPHENE_1_PERCENT / 10; // 0.1%
    uint64_t max_fee = 300 * GRAPHENE_BLOCKCHAIN_PRECISION;
    uint64_t amount = 1000 * GRAPHENE_BLOCKCHAIN_PRECISION;

    fee_schedule schedule;

    // set fee parameter
    transfer_operation::fee_parameters_type transfer_fee;
    transfer_fee.fee = min_fee;
    transfer_fee.percentage = percentage;
    transfer_fee.percentage_max_fee = max_fee;
    schedule.parameters.insert(transfer_fee); // overwrite default value

    // calculate operation fee with given input parameters
    auto operation = transfer_operation();
    operation.amount = asset(amount);
    auto calculatedFee = schedule.calculate_fee(operation, price::unit_price());

    uint64_t expected_fee = amount * percentage / GRAPHENE_100_PERCENT;
    BOOST_CHECK_EQUAL(static_cast<uint64_t>(calculatedFee.amount.value), expected_fee);
}

BOOST_AUTO_TEST_CASE(when_calculated_percentage_fee_is_more_than_max_fee_return_max_fee)
{
    uint64_t min_fee = 1000;
    uint16_t percentage = GRAPHENE_1_PERCENT;
    uint64_t max_fee = 300 * GRAPHENE_BLOCKCHAIN_PRECISION;
    uint64_t amount = 100000 * GRAPHENE_BLOCKCHAIN_PRECISION;

    fee_schedule schedule;

    // set fee parameter
    transfer_operation::fee_parameters_type transfer_fee;
    transfer_fee.fee = min_fee;
    transfer_fee.percentage = percentage;
    transfer_fee.percentage_max_fee = max_fee;
    schedule.parameters.insert(transfer_fee); // overwrite default value

    // calculate operation fee with given input parameters
    auto operation = transfer_operation();
    operation.amount = asset(amount);
    auto calculatedFee = schedule.calculate_fee(operation, price::unit_price());

    uint64_t expected_fee = max_fee;
    BOOST_CHECK_EQUAL(static_cast<uint64_t>(calculatedFee.amount.value), expected_fee);
}

BOOST_AUTO_TEST_CASE(when_calculated_percentage_fee_is_less_than_min_fee_return_min_fee)
{
    uint64_t min_fee = 10000000;
    uint16_t percentage = GRAPHENE_1_PERCENT / 100; // 0.0001
    uint64_t max_fee = 300 * GRAPHENE_BLOCKCHAIN_PRECISION;
    uint64_t amount = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;

    fee_schedule schedule;

    // set fee parameter
    transfer_operation::fee_parameters_type transfer_fee;
    transfer_fee.fee = min_fee;
    transfer_fee.percentage = percentage;
    transfer_fee.percentage_max_fee = max_fee;
    schedule.parameters.insert(transfer_fee); // overwrite default value

    // calculate operation fee with given input parameters
    auto operation = transfer_operation();
    operation.amount = asset(amount);
    auto calculatedFee = schedule.calculate_fee(operation, price::unit_price());

    uint64_t expected_fee = min_fee;
    BOOST_CHECK_EQUAL(static_cast<uint64_t>(calculatedFee.amount.value), expected_fee);
}

BOOST_AUTO_TEST_SUITE_END()
