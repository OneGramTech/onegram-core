/*
 * Copyright (c) 2018 01 People, s.r.o. (01 CryptoHouse).
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

#include <graphene/app/api.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;
using namespace graphene::app;
BOOST_FIXTURE_TEST_SUITE( archive_api_tests, database_fixture )

const int asset_create_op_id = operation::tag<asset_create_operation>::value;
const int asset_issue_op_id = operation::tag<asset_issue_operation>::value;
const int account_create_op_id = operation::tag<account_create_operation>::value;
const int transfer_op_id = operation::tag<transfer_operation>::value;
const int balance_claim_op_id = operation::tag<balance_claim_operation>::value;
const int publish_feed_op_id = operation::tag<asset_publish_feed_operation>::value;
const int update_feed_producers_op_id = operation::tag<asset_update_feed_producers_operation>::value;
const int update_call_order_op_id = operation::tag<call_order_update_operation>::value;
const int create_limit_order_op_id = operation::tag<limit_order_create_operation>::value;
const int fill_order_op_id = operation::tag<fill_order_operation>::value;

BOOST_AUTO_TEST_CASE(get_archived_operations) {
    try {
        graphene::app::archive_api arch_api(app);

        const auto eur = create_user_issued_asset("EUR"); // op 0
        issue_uia(account_id_type(), eur.amount(1000000)); // op 1
        generate_block();

        const auto mario = create_account("mario"); // op 2
        const auto marek = create_account("marek"); // op 3
        generate_block();

        fund(mario, eur.amount(1000)); // op 4
        transfer(mario, marek, eur.amount(500)); // op 5
        generate_block();

        auto opid_filter = flat_set<int>();
        auto result = archive_api::query_result();
        auto& archived = result.operations;
        auto& nprocessed = result.num_processed;
        
        /* Test empty query. */

        result = arch_api.get_archived_operations(0u, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 0u);
        BOOST_CHECK_EQUAL(archived.size(), 0u);

        /* Query each operation alone. */

        // op 0
        result = arch_api.get_archived_operations(0u, 1u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 0u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), asset_create_op_id);

        // op 1
        result = arch_api.get_archived_operations(1u, 1u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 1u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), asset_issue_op_id);

        // op 2
        result = arch_api.get_archived_operations(2u, 1u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);

        // op 3
        result = arch_api.get_archived_operations(3u, 1u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);

        // op 4
        result = arch_api.get_archived_operations(4u, 1u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);

        // op 5
        result = arch_api.get_archived_operations(5u, 1u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);

        /* Query operations by pairs. */

        // ops 0 1
        result = arch_api.get_archived_operations(1u, 2u);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 1u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 0u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), asset_issue_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), asset_create_op_id);

        // ops 1 2
        result = arch_api.get_archived_operations(2u, 2u);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 1u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), asset_issue_op_id);

        // ops 2 3
        result = arch_api.get_archived_operations(3u, 2u);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), account_create_op_id);

        // ops 3 4
        result = arch_api.get_archived_operations(4u, 2u);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), account_create_op_id);

        // ops 4 5
        result = arch_api.get_archived_operations(5u, 2u);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), transfer_op_id);

        /* Query operations by quadruples. */

        // ops 0 1 2 3
        result = arch_api.get_archived_operations(3u, 4u);
        BOOST_CHECK_EQUAL(archived.size(), 4u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[2].id.instance(), 1u);
        BOOST_CHECK_EQUAL(archived[3].id.instance(), 0u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[2].op.which(), asset_issue_op_id);
        BOOST_CHECK_EQUAL(archived[3].op.which(), asset_create_op_id);

        // ops 3 4 5
        result = arch_api.get_archived_operations(5u, 3u);
        BOOST_CHECK_EQUAL(nprocessed, 3u);
        BOOST_CHECK_EQUAL(archived.size(), 3u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[2].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[2].op.which(), account_create_op_id);

        /* Query all operations at once. */

        // ops 0 1 2 3 4 5
        result = arch_api.get_archived_operations(5u, 6u);
        BOOST_CHECK_EQUAL(nprocessed, 6u);
        BOOST_CHECK_EQUAL(archived.size(), 6u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[2].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[3].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[4].id.instance(), 1u);
        BOOST_CHECK_EQUAL(archived[5].id.instance(), 0u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[2].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[3].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[4].op.which(), asset_issue_op_id);
        BOOST_CHECK_EQUAL(archived[5].op.which(), asset_create_op_id);

        /* Query all transfer operations at once. */

        opid_filter.clear();
        opid_filter.insert(transfer_op_id);
        result = arch_api.get_archived_operations(5u, 6u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 6u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), transfer_op_id);

        /* Extend environment for more complex tests. */

        fund(mario, eur.amount(archive_api::params.QueryResultLimit + archive_api::params.QueryInspectLimit + 1u)); // op 6
        generate_block();

        size_t last_op_index = 6; // can't use last id from global table because of the fixture

        /* Test queries with incorrect parameters. */
        {
            const auto n = archive_api::params.QueryResultLimit;

            GRAPHENE_REQUIRE_THROW(arch_api.get_archived_operations(last_op_index + 1u, 1u), fc::exception);
            GRAPHENE_REQUIRE_THROW(arch_api.get_archived_operations(last_op_index, last_op_index + 2u), fc::exception);
            GRAPHENE_REQUIRE_THROW(arch_api.get_archived_operations(last_op_index + 1u, last_op_index + 2u), fc::exception);
            GRAPHENE_REQUIRE_THROW(arch_api.get_archived_operations(last_op_index, n + 1u), fc::exception);
            GRAPHENE_REQUIRE_THROW(arch_api.get_archived_operations(last_op_index + 1u, n + 1u), fc::exception);

            for (size_t i = 0; i <= n; i++)
                transfer(mario, marek, eur.amount(1));
            generate_block();

            GRAPHENE_REQUIRE_THROW(arch_api.get_archived_operations(last_op_index + n + 1u, n + 1u), fc::exception);

            last_op_index += n + 1u;
        }

        /* Test query requiring to inspect more operations than the inspect limit per call. */
        {
            const auto n = archive_api::params.QueryInspectLimit;

            for (size_t i = 0; i < n; i++)
                transfer(mario, marek, eur.amount(1));
            generate_block();

            opid_filter.clear();
            opid_filter.insert(asset_create_op_id);

            result = arch_api.get_archived_operations(last_op_index + n, 1u, opid_filter);
            BOOST_CHECK_EQUAL(nprocessed, archive_api::params.QueryInspectLimit);
            BOOST_CHECK_EQUAL(archived.size(), 0u);
            result = arch_api.get_archived_operations(last_op_index + n - nprocessed, 1u, opid_filter);
            BOOST_CHECK_EQUAL(nprocessed, last_op_index + 1);
            BOOST_CHECK_EQUAL(archived.size(), 1u);
            BOOST_CHECK_EQUAL(archived[0].id.instance(), 0u);
            BOOST_CHECK_EQUAL(archived[0].op.which(), asset_create_op_id);

            last_op_index += n;
        }

    } catch (fc::exception &e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(get_archived_account_operations) {
    try {
        graphene::app::archive_api arch_api(app);

        const auto eur = create_user_issued_asset("EUR"); // op 0
        issue_uia(account_id_type(), eur.amount(1000000)); // op 1
        generate_block();

        const auto mario = create_account("mario"); // op 2, mario 0
        const auto marek = create_account("marek"); // op 3, marek 0
        generate_block();

        fund(mario, eur.amount(1000)); // op 4, mario 1
        transfer(mario, marek, eur.amount(500)); // op 5, mario 2, marek 1
        generate_block();

        fund(mario, eur.amount(1000)); // op 6, mario 3
        transfer(mario, marek, eur.amount(500)); // op 7, mario 4, marek 2
        generate_block();

        auto opid_filter = flat_set<int>();
        auto result = archive_api::query_result();
        auto& archived = result.operations;
        auto& nprocessed = result.num_processed;

        /* Test empty query. */

        result = arch_api.get_archived_account_operations(mario.name, 0u, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 0u);
        BOOST_CHECK_EQUAL(archived.size(), 0u);

        /* Query each operation alone. */

        // mario 0
        result = arch_api.get_archived_account_operations(mario.name, 0u, 1u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);

        // mario 1
        result = arch_api.get_archived_account_operations(mario.name, 1u, 1u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);

        // mario 2
        result = arch_api.get_archived_account_operations(mario.name, 2u, 1u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);

        // mario 3
        result = arch_api.get_archived_account_operations(mario.name, 3u, 1u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 6u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);

        // mario 4
        result = arch_api.get_archived_account_operations(mario.name, 4u, 1u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 7u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);

        // marek 0
        result = arch_api.get_archived_account_operations(marek.name, 0u, 1u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);

        // marek 1
        result = arch_api.get_archived_account_operations(marek.name, 1u, 1u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);

        // marek 2
        result = arch_api.get_archived_account_operations(marek.name, 2u, 1u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 7u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);

        /* Query all transfer operations at once. */

        opid_filter.clear();
        opid_filter.insert(transfer_op_id);

        // mario
        result = arch_api.get_archived_account_operations(mario.name, 4u, 5u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 5u);
        BOOST_CHECK_EQUAL(archived.size(), 4u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 7u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 6u);
        BOOST_CHECK_EQUAL(archived[2].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[3].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[2].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[3].op.which(), transfer_op_id);

        /* Query first two transfer and create_account operations. */

        opid_filter.clear();
        opid_filter.insert(transfer_op_id);
        opid_filter.insert(account_create_op_id);

        // marek
        result = arch_api.get_archived_account_operations(marek.name, 1u, 2u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), account_create_op_id);

        /* Test query requesting unexisting operations - specific test for an underflow. */

        opid_filter.clear();
        opid_filter.insert(balance_claim_op_id);
        result = arch_api.get_archived_account_operations(mario.name, 4u, 5u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 5u);
        BOOST_CHECK_EQUAL(archived.size(), 0u);

        /* Extend environment for more complex tests. */

        const auto jozef = create_account("jozef"); // op 8, jozef 0
        const auto franz = create_account("franz"); // op 9, franz 0
        generate_block();
        fund(mario, eur.amount(archive_api::params.QueryResultLimit + archive_api::params.QueryInspectLimit + 1u)); // op 10, mario 5
        fund(marek, eur.amount(archive_api::params.QueryResultLimit + archive_api::params.QueryInspectLimit + 1u)); // op 11
        fund(franz, eur.amount(archive_api::params.QueryResultLimit + archive_api::params.QueryInspectLimit + 1u)); // op 12, franz 1
        generate_block();

        const size_t franz_create_index = 9;
        size_t last_op_index = 12; // can't use last id from global table because of the fixture

        /* Test queries with incorrect parameters. */
        {
            const auto anonym = std::string("anonym");
            const auto n = archive_api::params.QueryResultLimit;
            const size_t base_index_franz = arch_api.get_archived_account_operation_count(franz.name) - 1u;

            GRAPHENE_REQUIRE_THROW(arch_api.get_archived_account_operations(anonym, 0u, 1u), fc::exception);
            GRAPHENE_REQUIRE_THROW(arch_api.get_archived_account_operations(franz.name, base_index_franz + 1u, 1u), fc::exception);
            GRAPHENE_REQUIRE_THROW(arch_api.get_archived_account_operations(franz.name, base_index_franz, base_index_franz + 2u), fc::exception);
            GRAPHENE_REQUIRE_THROW(arch_api.get_archived_account_operations(franz.name, base_index_franz + 1u, base_index_franz + 2u), fc::exception);
            GRAPHENE_REQUIRE_THROW(arch_api.get_archived_account_operations(franz.name, base_index_franz, n + 1u), fc::exception);
            GRAPHENE_REQUIRE_THROW(arch_api.get_archived_account_operations(franz.name, base_index_franz + 1u, n + 1u), fc::exception);

            for (size_t i = 0; i <= n; i++)
                transfer(mario, marek, eur.amount(1));
            generate_block();

            GRAPHENE_REQUIRE_THROW(arch_api.get_archived_account_operations(mario.name, last_op_index + n + 1u, n + 1u), fc::exception);

            last_op_index += n + 1u;
        }

        /* Test query requiring to inspect more operations than the inspect limit per call. */
        {
            const auto n = archive_api::params.QueryInspectLimit;

            const size_t base_index_franz = arch_api.get_archived_account_operation_count(franz.name) - 1u;

            for (size_t i = 0; i < n; i++)
                transfer(franz, mario, eur.amount(1));
            generate_block();

            opid_filter.clear();
            opid_filter.insert(account_create_op_id);

            result = arch_api.get_archived_account_operations(franz.name, base_index_franz + n, 1u, opid_filter);
            BOOST_CHECK_EQUAL(nprocessed, archive_api::params.QueryInspectLimit);
            BOOST_CHECK_EQUAL(archived.size(), 0u);
            result = arch_api.get_archived_account_operations(franz.name, base_index_franz + n - nprocessed, 1u, opid_filter);
            BOOST_CHECK_EQUAL(nprocessed, base_index_franz + 1);
            BOOST_CHECK_EQUAL(archived.size(), 1u);
            BOOST_CHECK_EQUAL(archived[0].id.instance(), franz_create_index);
            BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);

            last_op_index += n;
        }

    } catch (fc::exception &e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(get_archived_operations_by_time) {
    try {
        graphene::app::archive_api arch_api(app);

        generate_block();
        const auto time_1 = db.head_block_time();

        const auto eur = create_user_issued_asset("EUR"); // op 0
        issue_uia(account_id_type(), eur.amount(1000000)); // op 1
        generate_block();

        const auto mario = create_account("mario"); // op 2, mario 0
        const auto marek = create_account("marek"); // op 3, marek 0
        generate_block();

        fund(mario, eur.amount(1000)); // op 4, mario 1
        transfer(mario, marek, eur.amount(500)); // op 5, mario 2, marek 1
        generate_block();
        const auto time_2 = db.head_block_time();
        generate_block(); // make a time gap
        const auto time_3 = db.head_block_time();

        fund(mario, eur.amount(1000)); // op 6, mario 3
        transfer(mario, marek, eur.amount(500)); // op 7, mario 4, marek 2
        generate_block();
        const auto time_4 = db.head_block_time();

        auto opid_filter = flat_set<int>();
        auto result = archive_api::query_result();
        auto& archived = result.operations;
        auto& nprocessed = result.num_processed;
    
        /* Query invalid time window. */

        // [time_1 + 1,time_1)
        result = arch_api.get_archived_operations_by_time(time_1 + 1u, time_1, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 0u);
        BOOST_CHECK_EQUAL(archived.size(), 0u);

        /* Query empty time window. */

        // [time_1 - 1,time_1)
        result = arch_api.get_archived_operations_by_time(time_1 - 1u, time_1, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 0u);
        BOOST_CHECK_EQUAL(archived.size(), 0u);

        /* Query zero-length time window. */

        // [time_1,time_1)
        result = arch_api.get_archived_operations_by_time(time_1, time_1, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 0u);
        BOOST_CHECK_EQUAL(archived.size(), 0u);

        /* Query first time window. */

        // [time_1,time_2)
        result = arch_api.get_archived_operations_by_time(time_1, time_2, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 4u);
        BOOST_CHECK_EQUAL(archived.size(), 4u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[2].id.instance(), 1u);
        BOOST_CHECK_EQUAL(archived[3].id.instance(), 0u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[2].op.which(), asset_issue_op_id);
        BOOST_CHECK_EQUAL(archived[3].op.which(), asset_create_op_id);

        // [time_1,time_2]
        result = arch_api.get_archived_operations_by_time(time_1, time_2 + 1u, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 6u);
        BOOST_CHECK_EQUAL(archived.size(), 6u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[2].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[3].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[4].id.instance(), 1u);
        BOOST_CHECK_EQUAL(archived[5].id.instance(), 0u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[2].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[3].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[4].op.which(), asset_issue_op_id);
        BOOST_CHECK_EQUAL(archived[5].op.which(), asset_create_op_id);

        // [time_1,time_2) + filter
        opid_filter.clear();
        opid_filter.insert(account_create_op_id);
        result = arch_api.get_archived_operations_by_time(time_1, time_2, 0u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 4u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), account_create_op_id);

        // [time_1,time_2] + skip
        result = arch_api.get_archived_operations_by_time(time_1, time_2 + 1u, 4u);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 1u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 0u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), asset_issue_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), asset_create_op_id);

        // [time_1,time_2) + filter & skip
        opid_filter.clear();
        opid_filter.insert(account_create_op_id);
        result = arch_api.get_archived_operations_by_time(time_1, time_2, 1u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 3u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);

        /* Query second time window. */

        // [time_2,time_3)
        result = arch_api.get_archived_operations_by_time(time_2, time_3, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), transfer_op_id);

        /* Query second and third time windows. */

        // [time_2,time_4)
        result = arch_api.get_archived_operations_by_time(time_2, time_4, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), transfer_op_id);

        // [time_2,time_4]
        result = arch_api.get_archived_operations_by_time(time_2, time_4 + 1u, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 4u);
        BOOST_CHECK_EQUAL(archived.size(), 4u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 7u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 6u);
        BOOST_CHECK_EQUAL(archived[2].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[3].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[2].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[3].op.which(), transfer_op_id);

        // [time_2,time_4) + filter
        opid_filter.clear();
        opid_filter.insert(account_create_op_id);
        result = arch_api.get_archived_operations_by_time(time_2, time_4, 0u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 0u);

        // [time_2,time_4) + filter
        opid_filter.clear();
        opid_filter.insert(transfer_op_id);
        result = arch_api.get_archived_operations_by_time(time_2, time_4, 0u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), transfer_op_id);

        // [time_2,time_4] + skip
        result = arch_api.get_archived_operations_by_time(time_2, time_4 + 1u, 3u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);

        // [time_2,time_4) + skip & filter
        opid_filter.clear();
        opid_filter.insert(transfer_op_id);
        result = arch_api.get_archived_operations_by_time(time_2, time_4, 1u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);

        /* Query whole time window. */

        // [time_1,time_4]
        result = arch_api.get_archived_operations_by_time(time_1, time_4 + 1u, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 8u);
        BOOST_CHECK_EQUAL(archived.size(), 8u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 7u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 6u);
        BOOST_CHECK_EQUAL(archived[2].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[3].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[4].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[5].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[6].id.instance(), 1u);
        BOOST_CHECK_EQUAL(archived[7].id.instance(), 0u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[2].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[3].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[4].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[5].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[6].op.which(), asset_issue_op_id);
        BOOST_CHECK_EQUAL(archived[7].op.which(), asset_create_op_id);

        // [time_1,time_4] + skip
        result = arch_api.get_archived_operations_by_time(time_1, time_4 + 1u, 4u);
        BOOST_CHECK_EQUAL(nprocessed, 4u);
        BOOST_CHECK_EQUAL(archived.size(), 4u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[2].id.instance(), 1u);
        BOOST_CHECK_EQUAL(archived[3].id.instance(), 0u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[2].op.which(), asset_issue_op_id);
        BOOST_CHECK_EQUAL(archived[3].op.which(), asset_create_op_id);

        // [time_1,time_4] + filter
        opid_filter.clear();
        opid_filter.insert(account_create_op_id);
        result = arch_api.get_archived_operations_by_time(time_1, time_4 + 1u, 0u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 8u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), account_create_op_id);

        // [time_1,time_4] + skip & filter
        opid_filter.clear();
        opid_filter.insert(account_create_op_id);
        result = arch_api.get_archived_operations_by_time(time_1, time_4 + 1u, 1u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 7u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), account_create_op_id);

        /* Extend environment for more complex tests. */

        fund(mario, eur.amount(archive_api::params.QueryResultLimit + archive_api::params.QueryInspectLimit + 1u)); // op 8, mario 5
        generate_block();

        size_t last_op_index = 8; // can't use last id from global table because of the fixture

        /* Test query requiring to inspect more operations than the inspect limit per call. */
        {
            const auto n = archive_api::params.QueryInspectLimit;

            for (size_t i = 0; i < n; i++)
                transfer(mario, marek, eur.amount(1));
            generate_block();

            const auto time_a = time_1;
            const auto time_b = db.head_block_time() + 1u;

            opid_filter.clear();
            opid_filter.insert(asset_create_op_id);

            result = arch_api.get_archived_operations_by_time(time_a, time_b, 0u, opid_filter);
            BOOST_CHECK_EQUAL(nprocessed, archive_api::params.QueryInspectLimit);
            BOOST_CHECK_EQUAL(archived.size(), 0u);
            result = arch_api.get_archived_operations_by_time(time_a, time_b, nprocessed, opid_filter);
            BOOST_CHECK_EQUAL(nprocessed, last_op_index + 1);
            BOOST_CHECK_EQUAL(archived.size(), 1u);
            BOOST_CHECK_EQUAL(archived[0].id.instance(), 0u);
            BOOST_CHECK_EQUAL(archived[0].op.which(), asset_create_op_id);

            last_op_index += n;
        }

    } catch (fc::exception &e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(get_archived_account_operations_by_time) {
    try {
        graphene::app::archive_api arch_api(app);

        generate_block();
        const auto time_1 = db.head_block_time();

        const auto eur = create_user_issued_asset("EUR"); // op 0
        issue_uia(account_id_type(), eur.amount(1000000)); // op 1
        generate_block();

        const auto mario = create_account("mario"); // op 2, mario 0
        const auto marek = create_account("marek"); // op 3, marek 0
        generate_block();

        fund(mario, eur.amount(1000)); // op 4, mario 1
        transfer(mario, marek, eur.amount(500)); // op 5, mario 2, marek 1
        generate_block();
        const auto time_2 = db.head_block_time();
        generate_block(); // make a time gap
        const auto time_3 = db.head_block_time();

        fund(mario, eur.amount(1000)); // op 6, mario 3
        transfer(mario, marek, eur.amount(500)); // op 7, mario 4, marek 2
        generate_block();
        const auto time_4 = db.head_block_time();

        auto opid_filter = flat_set<int>();
        auto result = archive_api::query_result();
        auto& archived = result.operations;
        auto& nprocessed = result.num_processed;
        
        /* Query invalid time window. */

        // mario [time_1 + 1,time_1)
        result = arch_api.get_archived_account_operations_by_time(mario.name, time_1 + 1u, time_1, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 0u);
        BOOST_CHECK_EQUAL(archived.size(), 0u);
        BOOST_CHECK_EQUAL(nprocessed, 0u);

        /* Query empty time window. */

        // marek [time_1 - 1,time_1)
        result = arch_api.get_archived_account_operations_by_time(marek.name, time_1 - 1u, time_1, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 0u);
        BOOST_CHECK_EQUAL(archived.size(), 0u);
        BOOST_CHECK_EQUAL(nprocessed, 0u);

        /* Query zero-length time window. */

        // mario [time_1,time_1)
        result = arch_api.get_archived_account_operations_by_time(mario.name, time_1, time_1, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 0u);
        BOOST_CHECK_EQUAL(archived.size(), 0u);
        BOOST_CHECK_EQUAL(nprocessed, 0u);

        /* Query first time window. */

        // marek [time_1,time_2)
        result = arch_api.get_archived_account_operations_by_time(marek.name, time_1, time_2, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);

        // mario [time_1,time_2]
        result = arch_api.get_archived_account_operations_by_time(mario.name, time_1, time_2 + 1u, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 3u);
        BOOST_CHECK_EQUAL(archived.size(), 3u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[2].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[2].op.which(), account_create_op_id);

        // marek [time_1,time_2) + filter
        opid_filter.clear();
        opid_filter.insert(account_create_op_id);
        result = arch_api.get_archived_account_operations_by_time(marek.name, time_1, time_2, 0u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);

        // mario [time_1,time_2] + skip
        result = arch_api.get_archived_account_operations_by_time(mario.name, time_1, time_2 + 1u, 1u);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), account_create_op_id);

        // marek [time_1,time_2) + filter & skip
        opid_filter.clear();
        opid_filter.insert(account_create_op_id);
        result = arch_api.get_archived_account_operations_by_time(marek.name, time_1, time_2, 1u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 0u);
        BOOST_CHECK_EQUAL(archived.size(), 0u);

        /* Query second time window. */

        // mario [time_2,time_3)
        result = arch_api.get_archived_account_operations_by_time(mario.name, time_2, time_3, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), transfer_op_id);

        /* Query second and third time windows. */

        // marek [time_2,time_4)
        result = arch_api.get_archived_account_operations_by_time(marek.name, time_2, time_4, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);

        // mario [time_2,time_4]
        result = arch_api.get_archived_account_operations_by_time(mario.name, time_2, time_4 + 1u, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 4u);
        BOOST_CHECK_EQUAL(archived.size(), 4u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 7u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 6u);
        BOOST_CHECK_EQUAL(archived[2].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[3].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[2].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[3].op.which(), transfer_op_id);

        // marek [time_2,time_4) + filter
        opid_filter.clear();
        opid_filter.insert(account_create_op_id);
        result = arch_api.get_archived_account_operations_by_time(marek.name, time_2, time_4, 0u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 0u);

        // mario [time_2,time_4) + filter
        opid_filter.clear();
        opid_filter.insert(transfer_op_id);
        result = arch_api.get_archived_account_operations_by_time(mario.name, time_2, time_4, 0u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), transfer_op_id);

        // marek [time_2,time_4] + skip
        result = arch_api.get_archived_account_operations_by_time(marek.name, time_2, time_4 + 1u, 1u);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);

        // mario [time_2,time_4) + skip & filter
        opid_filter.clear();
        opid_filter.insert(transfer_op_id);
        result = arch_api.get_archived_account_operations_by_time(mario.name, time_2, time_4, 1u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 1u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);

        /* Query whole time window. */

        // marek [time_1,time_4]
        result = arch_api.get_archived_account_operations_by_time(marek.name, time_1, time_4 + 1u, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 3u);
        BOOST_CHECK_EQUAL(archived.size(), 3u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 7u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 5u);
        BOOST_CHECK_EQUAL(archived[2].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[2].op.which(), account_create_op_id);

        // mario [time_1,time_4] + skip
        result = arch_api.get_archived_account_operations_by_time(mario.name, time_1, time_4 + 1u, 3u);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 4u);
        BOOST_CHECK_EQUAL(archived[1].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[1].op.which(), account_create_op_id);

        // marek [time_1,time_4] + filter
        opid_filter.clear();
        opid_filter.insert(account_create_op_id);
        result = arch_api.get_archived_account_operations_by_time(marek.name, time_1, time_4 + 1u, 0u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 3u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 3u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);

        // mario [time_1,time_4] + skip & filter
        opid_filter.clear();
        opid_filter.insert(account_create_op_id);
        result = arch_api.get_archived_account_operations_by_time(mario.name, time_1, time_4 + 1u, 2u, opid_filter);
        BOOST_CHECK_EQUAL(nprocessed, 3u);
        BOOST_CHECK_EQUAL(archived.size(), 1u);
        BOOST_CHECK_EQUAL(archived[0].id.instance(), 2u);
        BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);

        /* Extend environment for more complex tests. */

        const auto jozef = create_account("jozef"); // op 8, jozef 0
        const auto franz = create_account("franz"); // op 9, franz 0
        generate_block();
        fund(mario, eur.amount(archive_api::params.QueryResultLimit + archive_api::params.QueryInspectLimit + 1u)); // op 10, mario 5
        fund(marek, eur.amount(archive_api::params.QueryResultLimit + archive_api::params.QueryInspectLimit + 1u)); // op 11
        fund(franz, eur.amount(archive_api::params.QueryResultLimit + archive_api::params.QueryInspectLimit + 1u)); // op 12, franz 1
        generate_block();

        const size_t franz_create_index = 9;
        size_t last_op_index = 12; // can't use last id from global table because of the fixture

        /* Test queries with incorrect parameters. */
        {
            const auto anonym = std::string("anonym");
            GRAPHENE_REQUIRE_THROW(arch_api.get_archived_account_operations_by_time(anonym, time_1, time_4 + 1u, 0u), fc::exception);
        }

        /* Test query requiring to inspect more operations than the inspect limit per call. */
        {
            const auto n = archive_api::params.QueryInspectLimit;

            const size_t base_index_franz = arch_api.get_archived_account_operation_count(franz.name) - 1u;

            for (size_t i = 0; i < n; i++)
                transfer(franz, mario, eur.amount(1));
            generate_block();

            const auto time_a = time_1;
            const auto time_b = db.head_block_time() + 1u;

            opid_filter.clear();
            opid_filter.insert(account_create_op_id);

            result = arch_api.get_archived_account_operations_by_time(franz.name, time_a, time_b, 0u, opid_filter);
            BOOST_CHECK_EQUAL(nprocessed, archive_api::params.QueryInspectLimit);
            BOOST_CHECK_EQUAL(archived.size(), 0u);
            result = arch_api.get_archived_account_operations_by_time(franz.name, time_a, time_b, nprocessed, opid_filter);
            BOOST_CHECK_EQUAL(nprocessed, base_index_franz + 1);
            BOOST_CHECK_EQUAL(archived.size(), 1u);
            BOOST_CHECK_EQUAL(archived[0].id.instance(), franz_create_index);
            BOOST_CHECK_EQUAL(archived[0].op.which(), account_create_op_id);

            last_op_index += n;
        }

    } catch (fc::exception &e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(get_archived_account_operation_count) {
    try {
        graphene::app::archive_api arch_api(app);

        const auto eur = create_user_issued_asset("EUR"); // op 0
        issue_uia(account_id_type(), eur.amount(1000000)); // op 1
        generate_block();

        const auto mario = create_account("mario"); // op 2, mario 0
        const auto marek = create_account("marek"); // op 3, marek 0
        generate_block();

        fund(mario, eur.amount(1000)); // op 4, mario 1
        transfer(mario, marek, eur.amount(500)); // op 5, mario 2, marek 1
        generate_block();

        fund(mario, eur.amount(1000)); // op 6, mario 3
        transfer(mario, marek, eur.amount(500)); // op 7, mario 4, marek 2
        generate_block();

        /* Test get_archived_account_operation_count(). */

        BOOST_CHECK_EQUAL(arch_api.get_archived_account_operation_count(mario.name), 5u);
        BOOST_CHECK_EQUAL(arch_api.get_archived_account_operation_count(marek.name), 3u);

        /* Test queries with incorrect parameters. */

        const auto anonym = std::string("anonym");
        GRAPHENE_REQUIRE_THROW(arch_api.get_archived_account_operation_count(anonym), fc::exception);

    } catch (fc::exception &e) {
        edump((e.to_detail_string()));
        throw;
    }
}

void assert_summary(const graphene::app::database_api& db_api,
                    const graphene::app::archive_api& arch_api,
                    const account_object& account,
                    const vector<const asset_object*>& assets)
{
    const auto op_count = arch_api.get_archived_account_operation_count(account.name);
    for (size_t i = 0; i < assets.size(); i++) {
        const auto& asset = *assets[i];
        auto asset_filter = flat_set<asset_id_type>();
        asset_filter.insert(asset.get_id());
        const auto given_balances = db_api.get_account_balances(account.name, asset_filter);
        BOOST_CHECK_EQUAL(given_balances.size(), 1u);
        BOOST_CHECK_EQUAL((const int64_t)given_balances[0u].asset_id.instance, (const int64_t)asset.get_id().instance);
        const auto answer = arch_api.get_account_summary(account.name, asset.symbol, op_count - 1u, op_count);
        const auto accid = account.get_id();
        const auto assid = asset.get_id();
        const auto asset_is_cra = (accid == account_id_type()) && assid == asset_id_type();
        const auto calcd_balance = answer.summary.credits - answer.summary.debits - answer.summary.fees
                                 + (asset_is_cra ? GRAPHENE_MAX_SHARE_SUPPLY : 0);
        BOOST_CHECK_EQUAL(calcd_balance.value, given_balances[0u].amount.value);
        BOOST_CHECK(calcd_balance.value >= 0);
    }
}

BOOST_AUTO_TEST_CASE(get_account_summary) {

    const auto generate_block_with_fees = [&](const flat_set<fee_parameters>& fees) {
        enable_fees();
        change_fees(fees);
        generate_block();
    };

    try {
        const auto supply = share_type(1000000u);
        const auto fraction = share_type(1000u);

        graphene::app::database_api db_api(db);
        graphene::app::archive_api arch_api(app);
        const auto cmmtt = account_id_type()(db); // committee account
        const auto cra = asset_id_type()(db); // core asset

        flat_set<fee_parameters> fees;
        fees.insert(transfer_operation::fee_parameters_type());
        fees.insert(account_create_operation::fee_parameters_type());

        auto assets = vector<const asset_object*>();
        assets.push_back(&cra);

        //

        const auto time_1 = db.head_block_time();

        const auto eur = create_user_issued_asset("EUR");
        assert_summary(db_api, arch_api, cmmtt, assets);
        assets.push_back(&eur);

        issue_uia(cmmtt.get_id(), eur.amount(supply));
        generate_block_with_fees(fees);
        assert_summary(db_api, arch_api, cmmtt, assets);

        const auto mario = create_account("mario");
        const auto marek = create_account("marek");
        fund(mario, cra.amount(fraction * fraction));
        generate_block_with_fees(fees);
        assert_summary(db_api, arch_api, cmmtt, assets);

        fund(mario, eur.amount(supply - fraction));
        generate_block_with_fees(fees);
        assert_summary(db_api, arch_api, cmmtt, assets);
        assert_summary(db_api, arch_api, mario, assets);

        transfer(mario, marek, eur.amount(fraction));
        generate_block_with_fees(fees);
        assert_summary(db_api, arch_api, mario, assets);
        assert_summary(db_api, arch_api, marek, assets);

        const auto usd = create_bitasset("USD", mario.get_id());
        update_feed_producers(usd, { mario.get_id() });
        assets.push_back(&usd);

        price_feed feed;
        feed.settlement_price = usd.amount(100u) / cra.amount(100u);
        feed.maintenance_collateral_ratio = 1500u;
        feed.maximum_short_squeeze_ratio = 1100u;
        publish_feed(usd, mario, feed);

        borrow(mario, usd.amount(fraction), cra.amount(2u * fraction));
        generate_block_with_fees(fees);
        assert_summary(db_api, arch_api, mario, assets);

        transfer(mario, marek, usd.amount(fraction));
        generate_block_with_fees(fees);
        assert_summary(db_api, arch_api, mario, assets);
        assert_summary(db_api, arch_api, marek, assets);

        const auto time_2 = db.head_block_time();

        // check the _by_time variant
        {
            const size_t nops = arch_api.get_archived_account_operation_count(mario.name);

            const auto result_by_index = arch_api.get_account_summary(mario.name, usd.symbol, nops - 1u, nops);
            const auto result_by_time = arch_api.get_account_summary_by_time(mario.name, usd.symbol, time_1, time_2 + 1u, 0u);

            BOOST_CHECK_EQUAL(result_by_index.summary.credits.value, result_by_time.summary.credits.value);
            BOOST_CHECK_EQUAL(result_by_index.summary.debits.value, result_by_time.summary.debits.value);
            BOOST_CHECK_EQUAL(result_by_index.summary.fees.value, result_by_time.summary.fees.value);
        }
        {
            const size_t nops = arch_api.get_archived_account_operation_count(mario.name);

            const auto result_by_index = arch_api.get_account_summary(mario.name, usd.symbol, nops - 2u, nops - 1u);
            const auto result_by_time = arch_api.get_account_summary_by_time(mario.name, usd.symbol, time_1, time_2 + 1u, 1u);

            BOOST_CHECK_EQUAL(result_by_index.summary.credits.value, result_by_time.summary.credits.value);
            BOOST_CHECK_EQUAL(result_by_index.summary.debits.value, result_by_time.summary.debits.value);
            BOOST_CHECK_EQUAL(result_by_index.summary.fees.value, result_by_time.summary.fees.value);
        }
        {
            const size_t nops = arch_api.get_archived_account_operation_count(marek.name);

            const auto result_by_index = arch_api.get_account_summary(marek.name, usd.symbol, nops - 2u, nops - 1u);
            const auto result_by_time = arch_api.get_account_summary_by_time(marek.name, usd.symbol, time_1, time_2, 0u);

            BOOST_CHECK_EQUAL(result_by_index.summary.credits.value, result_by_time.summary.credits.value);
            BOOST_CHECK_EQUAL(result_by_index.summary.debits.value, result_by_time.summary.debits.value);
            BOOST_CHECK_EQUAL(result_by_index.summary.fees.value, result_by_time.summary.fees.value);
        }

    } catch (fc::exception &e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(including_virtual_operations) {
    try {
        graphene::app::database_api db_api(db);
        graphene::app::archive_api arch_api(app);

        auto opid_filter = flat_set<int>();
        auto result = archive_api::query_result();
        auto& archived = result.operations;
        auto& nprocessed = result.num_processed;

        //

        const auto time_1 = db.head_block_time();

        const auto franz = create_account("franz"); // op 0
        const auto mario = create_account("mario"); // op 1
        const auto marek = create_account("marek"); // op 2
        generate_block();

        const auto cra = asset_id_type()(db); // core asset
        const auto eur = create_bitasset("EUR", franz.id); // op 3
        fund(mario, cra.amount(10000)); // op 4
        fund(marek, cra.amount(10000)); // op 5
        generate_block();

        price_feed feed;
        feed.settlement_price = eur.amount(10) / cra.amount(1);
        feed.maintenance_collateral_ratio = 2000;
        update_feed_producers(eur, {franz.id}); // op 6
        publish_feed(eur, franz, feed); // op 7
        generate_block();

        const auto time_2 = db.head_block_time();

        borrow(mario, eur.amount(2), cra.amount(4)); // op 8
        create_sell_order(mario, eur.amount(1), cra.amount(1)); // op 9
        create_sell_order(marek, cra.amount(1), eur.amount(1)); // op 10
        generate_block(); // ops 11 + 12

        fund(franz, cra.amount(1)); // op 13
        transfer(mario, franz, cra.amount(1)); // op 14
        generate_block();

        const auto time_3 = db.head_block_time();

        BOOST_CHECK_EQUAL(db.get_balance(franz, cra).amount.value, 2);
        BOOST_CHECK_EQUAL(db.get_balance(mario, cra).amount.value, 9996);
        BOOST_CHECK_EQUAL(db.get_balance(marek, cra).amount.value, 9999);
        BOOST_CHECK_EQUAL(db.get_balance(mario, eur).amount.value, 1);
        BOOST_CHECK_EQUAL(db.get_balance(marek, eur).amount.value, 1);

        /* Check generic query. */

        // all at once

        result = arch_api.get_archived_operations(14u, 15u);
        BOOST_CHECK_EQUAL(nprocessed, 15u);
        BOOST_CHECK_EQUAL(archived.size(), 15u);
        BOOST_CHECK_EQUAL(archived[ 0].id.instance(), 14u);
        BOOST_CHECK_EQUAL(archived[ 1].id.instance(), 13u);
        BOOST_CHECK_EQUAL(archived[ 2].id.instance(), 12u);
        BOOST_CHECK_EQUAL(archived[ 3].id.instance(), 11u);
        BOOST_CHECK_EQUAL(archived[ 4].id.instance(), 10u);
        BOOST_CHECK_EQUAL(archived[ 5].id.instance(),  9u);
        BOOST_CHECK_EQUAL(archived[ 6].id.instance(),  8u);
        BOOST_CHECK_EQUAL(archived[ 7].id.instance(),  7u);
        BOOST_CHECK_EQUAL(archived[ 8].id.instance(),  6u);
        BOOST_CHECK_EQUAL(archived[ 9].id.instance(),  5u);
        BOOST_CHECK_EQUAL(archived[10].id.instance(),  4u);
        BOOST_CHECK_EQUAL(archived[11].id.instance(),  3u);
        BOOST_CHECK_EQUAL(archived[12].id.instance(),  2u);
        BOOST_CHECK_EQUAL(archived[13].id.instance(),  1u);
        BOOST_CHECK_EQUAL(archived[14].id.instance(),  0u);
        BOOST_CHECK_EQUAL(archived[ 0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[ 1].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[ 2].op.which(), fill_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 3].op.which(), fill_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 4].op.which(), create_limit_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 5].op.which(), create_limit_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 6].op.which(), update_call_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 7].op.which(), publish_feed_op_id);
        BOOST_CHECK_EQUAL(archived[ 8].op.which(), update_feed_producers_op_id);
        BOOST_CHECK_EQUAL(archived[ 9].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[10].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[11].op.which(), asset_create_op_id);
        BOOST_CHECK_EQUAL(archived[12].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[13].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[14].op.which(), account_create_op_id);

        // just some

        result = arch_api.get_archived_operations(11u, 2u);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[ 0].id.instance(), 11u);
        BOOST_CHECK_EQUAL(archived[ 1].id.instance(), 10u);
        BOOST_CHECK_EQUAL(archived[ 0].op.which(), fill_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 1].op.which(), create_limit_order_op_id);

        /* Check by account query. */

        BOOST_CHECK_EQUAL(arch_api.get_archived_account_operation_count(mario.name), 6u);

        // all at once

        result = arch_api.get_archived_account_operations(mario.name, 5u, 6u);
        BOOST_CHECK_EQUAL(nprocessed, 6u);
        BOOST_CHECK_EQUAL(archived.size(), 6u);
        BOOST_CHECK_EQUAL(archived[ 0].id.instance(), 14u);
        BOOST_CHECK_EQUAL(archived[ 1].id.instance(), 12u);
        BOOST_CHECK_EQUAL(archived[ 2].id.instance(),  9u);
        BOOST_CHECK_EQUAL(archived[ 3].id.instance(),  8u);
        BOOST_CHECK_EQUAL(archived[ 4].id.instance(),  4u);
        BOOST_CHECK_EQUAL(archived[ 5].id.instance(),  1u);
        BOOST_CHECK_EQUAL(archived[ 0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[ 1].op.which(), fill_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 2].op.which(), create_limit_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 3].op.which(), update_call_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 4].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[ 5].op.which(), account_create_op_id);

        // just some

        result = arch_api.get_archived_account_operations(mario.name, 4u, 2u);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[ 0].id.instance(), 12u);
        BOOST_CHECK_EQUAL(archived[ 1].id.instance(),  9u);
        BOOST_CHECK_EQUAL(archived[ 0].op.which(), fill_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 1].op.which(), create_limit_order_op_id);

        /* Check by time query. */

        // full time window

        result = arch_api.get_archived_operations_by_time(time_1, time_3 + 1, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 15u);
        BOOST_CHECK_EQUAL(archived.size(), 15u);
        BOOST_CHECK_EQUAL(archived[ 0].id.instance(), 14u);
        BOOST_CHECK_EQUAL(archived[ 1].id.instance(), 13u);
        BOOST_CHECK_EQUAL(archived[ 2].id.instance(), 12u);
        BOOST_CHECK_EQUAL(archived[ 3].id.instance(), 11u);
        BOOST_CHECK_EQUAL(archived[ 4].id.instance(), 10u);
        BOOST_CHECK_EQUAL(archived[ 5].id.instance(),  9u);
        BOOST_CHECK_EQUAL(archived[ 6].id.instance(),  8u);
        BOOST_CHECK_EQUAL(archived[ 7].id.instance(),  7u);
        BOOST_CHECK_EQUAL(archived[ 8].id.instance(),  6u);
        BOOST_CHECK_EQUAL(archived[ 9].id.instance(),  5u);
        BOOST_CHECK_EQUAL(archived[10].id.instance(),  4u);
        BOOST_CHECK_EQUAL(archived[11].id.instance(),  3u);
        BOOST_CHECK_EQUAL(archived[12].id.instance(),  2u);
        BOOST_CHECK_EQUAL(archived[13].id.instance(),  1u);
        BOOST_CHECK_EQUAL(archived[14].id.instance(),  0u);
        BOOST_CHECK_EQUAL(archived[ 0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[ 1].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[ 2].op.which(), fill_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 3].op.which(), fill_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 4].op.which(), create_limit_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 5].op.which(), create_limit_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 6].op.which(), update_call_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 7].op.which(), publish_feed_op_id);
        BOOST_CHECK_EQUAL(archived[ 8].op.which(), update_feed_producers_op_id);
        BOOST_CHECK_EQUAL(archived[ 9].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[10].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[11].op.which(), asset_create_op_id);
        BOOST_CHECK_EQUAL(archived[12].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[13].op.which(), account_create_op_id);
        BOOST_CHECK_EQUAL(archived[14].op.which(), account_create_op_id);

        // partial time window

        result = arch_api.get_archived_operations_by_time(time_2, time_3 + 1, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 9u);
        BOOST_CHECK_EQUAL(archived.size(), 9u);
        BOOST_CHECK_EQUAL(archived[ 0].id.instance(), 14u);
        BOOST_CHECK_EQUAL(archived[ 1].id.instance(), 13u);
        BOOST_CHECK_EQUAL(archived[ 2].id.instance(), 12u);
        BOOST_CHECK_EQUAL(archived[ 3].id.instance(), 11u);
        BOOST_CHECK_EQUAL(archived[ 4].id.instance(), 10u);
        BOOST_CHECK_EQUAL(archived[ 5].id.instance(),  9u);
        BOOST_CHECK_EQUAL(archived[ 6].id.instance(),  8u);
        BOOST_CHECK_EQUAL(archived[ 7].id.instance(),  7u);
        BOOST_CHECK_EQUAL(archived[ 8].id.instance(),  6u);
        BOOST_CHECK_EQUAL(archived[ 0].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[ 1].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[ 2].op.which(), fill_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 3].op.which(), fill_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 4].op.which(), create_limit_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 5].op.which(), create_limit_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 6].op.which(), update_call_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 7].op.which(), publish_feed_op_id);
        BOOST_CHECK_EQUAL(archived[ 8].op.which(), update_feed_producers_op_id);

        /* Check by account and by time query. */

        BOOST_CHECK_EQUAL(arch_api.get_archived_account_operation_count(marek.name), 4u);

        // full time window

        result = arch_api.get_archived_account_operations_by_time(marek.name, time_1, time_3 + 1u, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 4u);
        BOOST_CHECK_EQUAL(archived.size(), 4u);
        BOOST_CHECK_EQUAL(archived[ 0].id.instance(), 11u);
        BOOST_CHECK_EQUAL(archived[ 1].id.instance(), 10u);
        BOOST_CHECK_EQUAL(archived[ 2].id.instance(),  5u);
        BOOST_CHECK_EQUAL(archived[ 3].id.instance(),  2u);
        BOOST_CHECK_EQUAL(archived[ 0].op.which(), fill_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 1].op.which(), create_limit_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 2].op.which(), transfer_op_id);
        BOOST_CHECK_EQUAL(archived[ 3].op.which(), account_create_op_id);

        // partial time window

        result = arch_api.get_archived_account_operations_by_time(marek.name, time_2, time_3 + 1u, 0u);
        BOOST_CHECK_EQUAL(nprocessed, 2u);
        BOOST_CHECK_EQUAL(archived.size(), 2u);
        BOOST_CHECK_EQUAL(archived[ 0].id.instance(), 11u);
        BOOST_CHECK_EQUAL(archived[ 1].id.instance(), 10u);
        BOOST_CHECK_EQUAL(archived[ 0].op.which(), fill_order_op_id);
        BOOST_CHECK_EQUAL(archived[ 1].op.which(), create_limit_order_op_id);

        /* Check account summary query. */

        auto assets = vector<const asset_object*>();
        assets.push_back(&cra);
        assets.push_back(&eur);

        // full raw check

        assert_summary(db_api, arch_api, mario, assets);
        assert_summary(db_api, arch_api, marek, assets);

        // by time check
        {
            const size_t nops = arch_api.get_archived_account_operation_count(mario.name);

            const auto result_by_index = arch_api.get_account_summary(mario.name, cra.symbol, nops - 1u, nops);
            const auto result_by_time = arch_api.get_account_summary_by_time(mario.name, cra.symbol, time_1, time_3 + 1u, 0u);

            BOOST_CHECK_EQUAL(result_by_index.summary.credits.value, result_by_time.summary.credits.value);
            BOOST_CHECK_EQUAL(result_by_index.summary.debits.value, result_by_time.summary.debits.value);
            BOOST_CHECK_EQUAL(result_by_index.summary.fees.value, result_by_time.summary.fees.value);
        }
        {
            const size_t nops = arch_api.get_archived_account_operation_count(mario.name);

            const auto result_by_index = arch_api.get_account_summary(mario.name, eur.symbol, nops - 1u, nops);
            const auto result_by_time = arch_api.get_account_summary_by_time(mario.name, eur.symbol, time_2, time_3 + 1u, 0u);

            BOOST_CHECK_EQUAL(result_by_index.summary.credits.value, result_by_time.summary.credits.value);
            BOOST_CHECK_EQUAL(result_by_index.summary.debits.value, result_by_time.summary.debits.value);
            BOOST_CHECK_EQUAL(result_by_index.summary.fees.value, result_by_time.summary.fees.value);
        }
        {
            const size_t nops = arch_api.get_archived_account_operation_count(marek.name);

            const auto result_by_index = arch_api.get_account_summary(marek.name, cra.symbol, nops - 1u, nops);
            const auto result_by_time = arch_api.get_account_summary_by_time(marek.name, cra.symbol, time_1, time_3 + 1u, 0u);

            BOOST_CHECK_EQUAL(result_by_index.summary.credits.value, result_by_time.summary.credits.value);
            BOOST_CHECK_EQUAL(result_by_index.summary.debits.value, result_by_time.summary.debits.value);
            BOOST_CHECK_EQUAL(result_by_index.summary.fees.value, result_by_time.summary.fees.value);
        }
        {
            const size_t nops = arch_api.get_archived_account_operation_count(marek.name);

            const auto result_by_index = arch_api.get_account_summary(marek.name, eur.symbol, nops - 1u, nops);
            const auto result_by_time = arch_api.get_account_summary_by_time(marek.name, eur.symbol, time_2, time_3 + 1u, 0u);

            BOOST_CHECK_EQUAL(result_by_index.summary.credits.value, result_by_time.summary.credits.value);
            BOOST_CHECK_EQUAL(result_by_index.summary.debits.value, result_by_time.summary.debits.value);
            BOOST_CHECK_EQUAL(result_by_index.summary.fees.value, result_by_time.summary.fees.value);
        }

    } catch (fc::exception &e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_SUITE_END()
