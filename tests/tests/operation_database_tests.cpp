/*
 * Copyright (c) 2019 01 People, s.r.o. (01 CryptoHouse).
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

#include <graphene/account_archive/operation_database.hpp>
#include <graphene/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;
using namespace graphene::app;
using namespace graphene::account_archive;
BOOST_FIXTURE_TEST_SUITE( operation_database_tests, database_fixture )

void bump_oho_block(operation_history_object& oho, uint32_t block_inc = 1)
{
    if (block_inc) {
        oho.block_num += block_inc;
        oho.trx_in_block = 0;
        oho.op_in_trx = 0;
        oho.virtual_op = 0;
    }
}
void bump_oho_in_block(operation_history_object& oho, uint16_t trx_inc, uint16_t op_inc, uint16_t virtual_inc)
{
    oho.trx_in_block += trx_inc;
    oho.op_in_trx += op_inc;
    oho.virtual_op += virtual_inc;
}

void check_ohos_equal(const operation_history_object& a, const operation_history_object& b)
{
    BOOST_CHECK_EQUAL(true, a.op == b.op);
    BOOST_CHECK_EQUAL(true, a.result == b.result);
    BOOST_CHECK_EQUAL(a.block_num, b.block_num);
    BOOST_CHECK_EQUAL(a.trx_in_block, b.trx_in_block);
    BOOST_CHECK_EQUAL(a.op_in_trx, b.op_in_trx);
    BOOST_CHECK_EQUAL(a.virtual_op, b.virtual_op);
}

vector<operation_history_object> generate_ohos(size_t n)
{
    // fake asset
    asset a;
    {
        a.asset_id.instance = 0x12345678;
        a.amount = 0;
    }

    // fake accounts
    account_id_type from, to;
    {
        from.instance = 0x0a0a0a0a;
        to.instance   = 0x0b0b0b0b;
    }

    // operation template
    auto o = transfer_operation();
    o.fee = a;
    o.from = from;
    o.to = to;
    o.amount = a;

    // generate ohos
    vector<operation_history_object> v;
    v.reserve(n);

    operation_history_object oho;
    oho.op = o;

    while (n--) {
        if (!(n % 10))
            bump_oho_block(oho);
        else
            bump_oho_in_block(oho, 1, 1, 1);
        v.emplace_back(oho);
        oho.id.number++;
    }

    return v;
}

BOOST_AUTO_TEST_CASE(op_db_store_load_sigle) {
    try {
        const auto dir = fc::temp_directory(graphene::utilities::temp_directory_path()).path();
        operation_database opdb;

        uint32_t index;
        operation_history_object stored, loaded;

        stored.block_num = 1234;
        stored.trx_in_block = 1;
        stored.op_in_trx = 16000;
        stored.virtual_op = 1;

        // single interaction per open-load

        opdb.wipe(dir);
        opdb.open(dir);
        index = opdb.store(stored);
        opdb.close();

        opdb.open(dir);
        loaded = opdb.load(index);
        opdb.close();

        check_ohos_equal(stored, loaded);

        // multiple interactions per open-load

        opdb.wipe(dir);
        opdb.open(dir);
        index = opdb.store(stored);
        loaded = opdb.load(index);
        opdb.close();

        check_ohos_equal(stored, loaded);

    } catch (fc::exception &e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(op_db_store_load_multiple) {
    try {
        const size_t n = 1000;

        const auto dir = fc::temp_directory(graphene::utilities::temp_directory_path()).path();
        operation_database opdb;

        const auto ohos = generate_ohos(n);
        vector<uint32_t> indices;
        indices.resize(2 * n);

        // store and load in single open-close

        opdb.wipe(dir);
        opdb.open(dir);
        for (size_t i = 0; i < n; i++) {
            indices[i] = opdb.store(ohos[i]);
            const auto loaded = opdb.load(indices[i]);
            check_ohos_equal(ohos[i], loaded);
        }
        opdb.close();

        // repeat load in separate open-close

        opdb.open(dir);
        for (size_t i = 0; i < n; i++) {
            const auto loaded = opdb.load(indices[i]);
            check_ohos_equal(ohos[i], loaded);
        }
        opdb.close();

        // additional store with repeated load in single open-close

        opdb.open(dir);
        for (size_t i = 0; i < n; i++) {
            indices[n + i] = opdb.store(ohos[i]);
            const auto loaded = opdb.load(indices[n - i - 1]);
            check_ohos_equal(ohos[n - i - 1], loaded);
        }
        opdb.close();

        // load the additional store separately

        opdb.open(dir);
        for (size_t i = 0; i < n; i++) {
            const auto loaded = opdb.load(indices[n + i]);
            check_ohos_equal(ohos[i], loaded);
        }
        opdb.close();

    } catch (fc::exception &e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(op_db_truncate) {
    try {
        const size_t n = 100;

        const auto dir = fc::temp_directory(graphene::utilities::temp_directory_path()).path();
        operation_database opdb;

        const auto ohos = generate_ohos(n);
        vector<uint32_t> indices;
        indices.resize(n);

        // single truncate with store and load in one open-close

        opdb.wipe(dir);
        opdb.open(dir);
        for (size_t i = 0; i < n; i++) {
            indices[i] = opdb.store(ohos[i]);
            const auto loaded = opdb.load(indices[i]);
            check_ohos_equal(ohos[i], loaded);
        }

        const auto b = ohos[n / 2].block_num + 1;
        opdb.truncate(b);
        for (size_t i = 0; i < opdb.get_stored_operation_count(); i++) {
            const auto loaded = opdb.load(indices[i]);
            check_ohos_equal(ohos[i], loaded);
        }
        opdb.close();

        // check once again after fresh open

        opdb.open(dir);
        for (size_t i = 0; i < opdb.get_stored_operation_count(); i++) {
            const auto loaded = opdb.load(indices[i]);
            check_ohos_equal(ohos[i], loaded);
        }
        opdb.close();

        // add truncated ohos again

        opdb.open(dir);
        for (size_t i = opdb.get_stored_operation_count(); i < n; i++) {
            const auto index = opdb.store(ohos[i]);
            BOOST_CHECK_EQUAL(index, indices[i]);
        }

        // truncate multiple times

        opdb.truncate(ohos[n - 1].block_num);
        opdb.truncate(ohos[n - 2].block_num);
        opdb.truncate(b);
        opdb.truncate(b);

        for (size_t i = 0; i < opdb.get_stored_operation_count(); i++) {
            const auto loaded = opdb.load(indices[i]);
            check_ohos_equal(ohos[i], loaded);
        }
        opdb.close();

    } catch (fc::exception &e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_SUITE_END()
