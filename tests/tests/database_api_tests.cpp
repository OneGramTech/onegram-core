/*
 * Copyright (c) 2017 Cryptonomex, Inc., and contributors.
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

#include <graphene/app/database_api.hpp>

#include <fc/crypto/digest.hpp>

#include <fc/crypto/hex.hpp>
#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE(database_api_tests, database_fixture)

BOOST_AUTO_TEST_CASE(is_registered) {
   try {
      /***
       * Arrange
       */
      auto nathan_private_key = generate_private_key("nathan");
      public_key_type nathan_public = nathan_private_key.get_public_key();

      auto dan_private_key = generate_private_key("dan");
      public_key_type dan_public = dan_private_key.get_public_key();

      auto unregistered_private_key = generate_private_key("unregistered");
      public_key_type unregistered_public = unregistered_private_key.get_public_key();


      /***
       * Act
       */
      create_account("dan", dan_private_key.get_public_key());
      create_account("nathan", nathan_private_key.get_public_key());
      // Unregistered key will not be registered with any account


      /***
       * Assert
       */
      graphene::app::database_api db_api(db);

      BOOST_CHECK(db_api.is_public_key_registered((string) nathan_public));
      BOOST_CHECK(db_api.is_public_key_registered((string) dan_public));
      BOOST_CHECK(!db_api.is_public_key_registered((string) unregistered_public));

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( get_potential_signatures_owner_and_active ) {
   try {
      fc::ecc::private_key nathan_key1 = fc::ecc::private_key::regenerate(fc::digest("key1"));
      fc::ecc::private_key nathan_key2 = fc::ecc::private_key::regenerate(fc::digest("key2"));
      public_key_type pub_key_active( nathan_key1.get_public_key() );
      public_key_type pub_key_owner( nathan_key2.get_public_key() );
      const account_object& nathan = create_account("nathan", nathan_key1.get_public_key() );

      try {
         account_update_operation op;
         op.account = nathan.id;
         op.active = authority(1, pub_key_active, 1);
         op.owner = authority(1, pub_key_owner, 1);
         trx.operations.push_back(op);
         sign(trx, nathan_key1);
         PUSH_TX( db, trx, database::skip_transaction_dupe_check );
         trx.operations.clear();
         trx.signatures.clear();
      } FC_CAPTURE_AND_RETHROW ((nathan.active))

      // this op requires active
      transfer_operation op;
      op.from = nathan.id;
      op.to = account_id_type();
      trx.operations.push_back(op);

      graphene::app::database_api db_api(db);
      set<public_key_type> pub_keys = db_api.get_potential_signatures( trx );

      BOOST_CHECK( pub_keys.find( pub_key_active ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( pub_key_owner ) != pub_keys.end() );

      trx.operations.clear();

      // this op requires owner
      account_update_operation auop;
      auop.account = nathan.id;
      auop.owner = authority(1, pub_key_owner, 1);
      trx.operations.push_back(auop);

      pub_keys = db_api.get_potential_signatures( trx );

      BOOST_CHECK( pub_keys.find( pub_key_active ) == pub_keys.end() ); // active key doesn't help in this case
      BOOST_CHECK( pub_keys.find( pub_key_owner ) != pub_keys.end() );

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( get_potential_signatures_other ) {
   try {
      fc::ecc::private_key priv_key1 = fc::ecc::private_key::regenerate(fc::digest("key1"));
      public_key_type pub_key1( priv_key1.get_public_key() );

      const account_object& nathan = create_account( "nathan" );

      balance_claim_operation op;
      op.deposit_to_account = nathan.id;
      op.balance_owner_key = pub_key1;
      trx.operations.push_back(op);

      graphene::app::database_api db_api(db);
      set<public_key_type> pub_keys = db_api.get_potential_signatures( trx );

      BOOST_CHECK( pub_keys.find( pub_key1 ) != pub_keys.end() );

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( get_required_signatures_owner_or_active ) {
   try {
      fc::ecc::private_key nathan_key1 = fc::ecc::private_key::regenerate(fc::digest("key1"));
      fc::ecc::private_key nathan_key2 = fc::ecc::private_key::regenerate(fc::digest("key2"));
      public_key_type pub_key_active( nathan_key1.get_public_key() );
      public_key_type pub_key_owner( nathan_key2.get_public_key() );
      const account_object& nathan = create_account("nathan", nathan_key1.get_public_key() );

      try {
         account_update_operation op;
         op.account = nathan.id;
         op.active = authority(1, pub_key_active, 1);
         op.owner = authority(1, pub_key_owner, 1);
         trx.operations.push_back(op);
         sign(trx, nathan_key1);
         PUSH_TX( db, trx, database::skip_transaction_dupe_check );
         trx.operations.clear();
         trx.signatures.clear();
      } FC_CAPTURE_AND_RETHROW ((nathan.active))

      graphene::app::database_api db_api(db);

      // prepare available keys sets
      flat_set<public_key_type> avail_keys1, avail_keys2, avail_keys3;
      avail_keys1.insert( pub_key_active );
      avail_keys2.insert( pub_key_owner );
      avail_keys3.insert( pub_key_active );
      avail_keys3.insert( pub_key_owner );

      set<public_key_type> pub_keys;

      // this op requires active
      transfer_operation op;
      op.from = nathan.id;
      op.to = account_id_type();
      trx.operations.push_back(op);

      // provides active, should be ok
      pub_keys = db_api.get_required_signatures( trx, avail_keys1 );
      BOOST_CHECK( pub_keys.find( pub_key_active ) != pub_keys.end() );

      // provides owner, should be ok
      pub_keys = db_api.get_required_signatures( trx, avail_keys2 );
      BOOST_CHECK( pub_keys.find( pub_key_owner ) != pub_keys.end() );

      // provides both active and owner, should return one of them
      pub_keys = db_api.get_required_signatures( trx, avail_keys3 );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_active ) != pub_keys.end() || pub_keys.find( pub_key_owner ) != pub_keys.end() );

      trx.operations.clear();

      // this op requires owner
      account_update_operation auop;
      auop.account = nathan.id;
      auop.owner = authority(1, pub_key_owner, 1);
      trx.operations.push_back(auop);

      // provides active, should return an empty set
      pub_keys = db_api.get_required_signatures( trx, avail_keys1 );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides owner, should return it
      pub_keys = db_api.get_required_signatures( trx, avail_keys2 );
      BOOST_CHECK( pub_keys.find( pub_key_owner ) != pub_keys.end() );

      // provides both active and owner, should return owner only
      pub_keys = db_api.get_required_signatures( trx, avail_keys3 );
      BOOST_CHECK( pub_keys.find( pub_key_active ) == pub_keys.end() );
      BOOST_CHECK( pub_keys.find( pub_key_owner ) != pub_keys.end() );

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( get_required_signatures_partially_signed_or_not ) {
   try {
      fc::ecc::private_key morgan_key = fc::ecc::private_key::regenerate(fc::digest("morgan_key"));
      fc::ecc::private_key nathan_key = fc::ecc::private_key::regenerate(fc::digest("nathan_key"));
      fc::ecc::private_key oliver_key = fc::ecc::private_key::regenerate(fc::digest("oliver_key"));
      public_key_type pub_key_morgan( morgan_key.get_public_key() );
      public_key_type pub_key_nathan( nathan_key.get_public_key() );
      public_key_type pub_key_oliver( oliver_key.get_public_key() );
      const account_object& morgan = create_account("morgan", morgan_key.get_public_key() );
      const account_object& nathan = create_account("nathan", nathan_key.get_public_key() );
      const account_object& oliver = create_account("oliver", oliver_key.get_public_key() );

      graphene::app::database_api db_api(db);

      // prepare available keys sets
      flat_set<public_key_type> avail_keys_empty, avail_keys_m, avail_keys_n, avail_keys_o;
      flat_set<public_key_type> avail_keys_mn, avail_keys_mo, avail_keys_no, avail_keys_mno;
      avail_keys_m.insert( pub_key_morgan );
      avail_keys_mn.insert( pub_key_morgan );
      avail_keys_mo.insert( pub_key_morgan );
      avail_keys_mno.insert( pub_key_morgan );
      avail_keys_n.insert( pub_key_nathan );
      avail_keys_mn.insert( pub_key_nathan );
      avail_keys_no.insert( pub_key_nathan );
      avail_keys_mno.insert( pub_key_nathan );
      avail_keys_o.insert( pub_key_oliver );
      avail_keys_mo.insert( pub_key_oliver );
      avail_keys_no.insert( pub_key_oliver );
      avail_keys_mno.insert( pub_key_oliver );

      // result set
      set<public_key_type> pub_keys;

      // make a transaction that require 1 signature (m)
      transfer_operation op;
      op.from = morgan.id;
      op.to = oliver.id;
      trx.operations.push_back(op);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

     // provides [m], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // provides [n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 0 );

       // provides [m,n], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // sign with n, but actually need m
      sign(trx, nathan_key);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // provides [n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_o );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // provides [m,o], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mo );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // provides [n,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_no );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n,o], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mno );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // sign with m, should be enough
      trx.signatures.clear();
      sign(trx, morgan_key);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 0 );

      // sign with m+n, although m only should be enough, this API won't complain
      sign(trx, nathan_key);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_o );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mo );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_no );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mno );
      BOOST_CHECK( pub_keys.size() == 0 );

      // make a transaction that require 2 signatures (m+n)
      trx.signatures.clear();
      op.from = nathan.id;
      trx.operations.push_back(op);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // provides [n], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_o );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return [m,n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 2 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [m,o], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mo );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // provides [n,o], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_no );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [m,n,o], should return [m,n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mno );
      BOOST_CHECK( pub_keys.size() == 2 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // sign with o, but actually need m+n
      sign(trx, oliver_key);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // provides [n], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_o );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return [m,n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 2 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [m,o], should return [m]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mo );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );

      // provides [n,o], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_no );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [m,n,o], should return [m,n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mno );
      BOOST_CHECK( pub_keys.size() == 2 );
      BOOST_CHECK( pub_keys.find( pub_key_morgan ) != pub_keys.end() );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // sign with m+o, but actually need m+n
      sign(trx, morgan_key);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_o );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [m,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mo );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n,o], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_no );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [m,n,o], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mno );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // sign with m, but actually need m+n
      trx.signatures.clear();
      sign(trx, morgan_key);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_o );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [m,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mo );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n,o], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_no );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // provides [m,n,o], should return [n]
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mno );
      BOOST_CHECK( pub_keys.size() == 1 );
      BOOST_CHECK( pub_keys.find( pub_key_nathan ) != pub_keys.end() );

      // sign with m+n, should be enough
      sign(trx, nathan_key);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_o );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mo );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_no );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mno );
      BOOST_CHECK( pub_keys.size() == 0 );

      // sign with m+n+o, should be enough as well
      sign(trx, oliver_key);

      // provides [], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_empty );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_m );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_n );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_o );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mn );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mo );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [n,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_no );
      BOOST_CHECK( pub_keys.size() == 0 );

      // provides [m,n,o], should return []
      pub_keys = db_api.get_required_signatures( trx, avail_keys_mno );
      BOOST_CHECK( pub_keys.size() == 0 );

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( set_subscribe_callback_disable_notify_all_test ) {
   try {
      ACTORS( (alice) );

      uint32_t objects_changed1 = 0;
      uint32_t objects_changed2 = 0;
      uint32_t objects_changed3 = 0;
      auto callback1 = [&]( const variant& v )
      {
         ++objects_changed1;
      };
      auto callback2 = [&]( const variant& v )
      {
         ++objects_changed2;
      };
      auto callback3 = [&]( const variant& v )
      {
         ++objects_changed3;
      };

      uint32_t expected_objects_changed1 = 0;
      uint32_t expected_objects_changed2 = 0;
      uint32_t expected_objects_changed3 = 0;

      graphene::app::database_api db_api1(db);

      // subscribing to all should fail
      BOOST_CHECK_THROW( db_api1.set_subscribe_callback( callback1, true ), fc::exception );

      db_api1.set_subscribe_callback( callback1, false );

      graphene::app::application_options opt;
      opt.enable_subscribe_to_all = true;

      graphene::app::database_api db_api2( db, &opt );
      db_api2.set_subscribe_callback( callback2, true );

      graphene::app::database_api db_api3( db, &opt );
      db_api3.set_subscribe_callback( callback3, false );

      vector<object_id_type> ids;
      ids.push_back( alice_id );

      db_api1.get_objects( ids ); // db_api1 subscribe to Alice
      db_api2.get_objects( ids ); // db_api2 subscribe to Alice

      generate_block();
      ++expected_objects_changed2; // subscribed to all, notify block changes

      transfer( account_id_type(), alice_id, asset(1) );
      generate_block();
      ++expected_objects_changed1; // subscribed to Alice, notify Alice balance change
      ++expected_objects_changed2; // subscribed to all, notify block changes

      fc::usleep(fc::milliseconds(200)); // sleep a while to execute callback in another thread

      BOOST_CHECK_EQUAL( expected_objects_changed1, objects_changed1 );
      BOOST_CHECK_EQUAL( expected_objects_changed2, objects_changed2 );
      BOOST_CHECK_EQUAL( expected_objects_changed3, objects_changed3 );

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( lookup_vote_ids )
{ try {
   ACTORS( (connie)(whitney)(wolverine) );

   fund(connie);
   upgrade_to_lifetime_member(connie);
   fund(whitney);
   upgrade_to_lifetime_member(whitney);
   fund(wolverine);
   upgrade_to_lifetime_member(wolverine);

   const auto& committee = create_committee_member( connie );
   const auto& witness = create_witness( whitney );
   const auto& worker = create_worker( wolverine_id );

   graphene::app::database_api db_api(db);

   std::vector<vote_id_type> votes;
   votes.push_back( committee.vote_id );
   votes.push_back( witness.vote_id );
   votes.push_back( worker.vote_for );

   const auto results = db_api.lookup_vote_ids( votes );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(get_account_limit_orders)
{ try {

   ACTORS((seller));

   const auto& bitcny = create_bitasset("CNY");
   const auto& core   = asset_id_type()(db);

   int64_t init_balance(10000000);
   transfer(committee_account, seller_id, asset(init_balance));
   BOOST_CHECK_EQUAL( 10000000, get_balance(seller, core) );

   /// Create 250 versatile orders
   for (size_t i = 0 ; i < 50 ; ++i)
   {
      BOOST_CHECK(create_sell_order(seller, core.amount(100), bitcny.amount(250)));
   }

   for (size_t i = 1 ; i < 101 ; ++i)
   {
      BOOST_CHECK(create_sell_order(seller, core.amount(100), bitcny.amount(250 + i)));
      BOOST_CHECK(create_sell_order(seller, core.amount(100), bitcny.amount(250 - i)));
   }

   graphene::app::database_api db_api(db);
   std::vector<limit_order_object> results;
   limit_order_object o;

   // query with no constraint, expected:
   // 1. up to 101 orders returned
   // 2. orders were sorted by price desendingly
   results = db_api.get_account_limit_orders(seller.name, GRAPHENE_SYMBOL, "CNY");
   BOOST_CHECK(results.size() == 101);
   for (size_t i = 0 ; i < results.size() - 1 ; ++i)
   {
      BOOST_CHECK(results[i].sell_price >= results[i+1].sell_price);
   }
   BOOST_CHECK(results.front().sell_price == price(core.amount(100), bitcny.amount(150)));
   BOOST_CHECK(results.back().sell_price == price(core.amount(100), bitcny.amount(250)));
   results.clear();

   // query with specified limit, expected:
   // 1. up to specified amount of orders returned
   // 2. orders were sorted by price desendingly
   results = db_api.get_account_limit_orders(seller.name, GRAPHENE_SYMBOL, "CNY", 50);
   BOOST_CHECK(results.size() == 50);
   for (size_t i = 0 ; i < results.size() - 1 ; ++i)
   {
      BOOST_CHECK(results[i].sell_price >= results[i+1].sell_price);
   }
   BOOST_CHECK(results.front().sell_price == price(core.amount(100), bitcny.amount(150)));
   BOOST_CHECK(results.back().sell_price == price(core.amount(100), bitcny.amount(199)));

   o = results.back();
   results.clear();

   // query with specified order id and limit, expected:
   // same as before, but also the first order's id equal to specified
   results = db_api.get_account_limit_orders(seller.name, GRAPHENE_SYMBOL, "CNY", 80,
       limit_order_id_type(o.id));
   BOOST_CHECK(results.size() == 80);
   BOOST_CHECK(results.front().id == o.id);
   for (size_t i = 0 ; i < results.size() - 1 ; ++i)
   {
      BOOST_CHECK(results[i].sell_price >= results[i+1].sell_price);
   }
   BOOST_CHECK(results.front().sell_price == price(core.amount(100), bitcny.amount(199)));
   BOOST_CHECK(results.back().sell_price == price(core.amount(100), bitcny.amount(250)));

   o = results.back();
   results.clear();

   // query with specified price and an not exists order id, expected:
   // 1. the canceled order should not exists in returned orders and first order's
   //    id should greater than specified
   // 2. returned orders sorted by price desendingly
   // 3. the first order's sell price equal to specified
   cancel_limit_order(o); // NOTE 1: this canceled order was in scope of the
                          // first created 50 orders, so with price 2.5 BTS/CNY
   results = db_api.get_account_limit_orders(seller.name, GRAPHENE_SYMBOL, "CNY", 50,
       limit_order_id_type(o.id), o.sell_price);
   BOOST_CHECK(results.size() == 50);
   BOOST_CHECK(results.front().id > o.id);
   // NOTE 2: because of NOTE 1, here should be equal
   BOOST_CHECK(results.front().sell_price == o.sell_price);
   for (size_t i = 0 ; i < results.size() - 1 ; ++i)
   {
      BOOST_CHECK(results[i].sell_price >= results[i+1].sell_price);
   }
   BOOST_CHECK(results.front().sell_price == price(core.amount(100), bitcny.amount(250)));
   BOOST_CHECK(results.back().sell_price == price(core.amount(100), bitcny.amount(279)));

   o = results.back();
   results.clear();

   cancel_limit_order(o); // NOTE 3: this time the canceled order was in scope
                          // of the lowest price 150 orders
   results = db_api.get_account_limit_orders(seller.name, GRAPHENE_SYMBOL, "CNY", 101,
       limit_order_id_type(o.id), o.sell_price);
   BOOST_CHECK(results.size() == 71);
   BOOST_CHECK(results.front().id > o.id);
   // NOTE 3: because of NOTE 1, here should be little than 
   BOOST_CHECK(results.front().sell_price < o.sell_price);
   for (size_t i = 0 ; i < results.size() - 1 ; ++i)
   {
      BOOST_CHECK(results[i].sell_price >= results[i+1].sell_price);
   }
   BOOST_CHECK(results.front().sell_price == price(core.amount(100), bitcny.amount(280)));
   BOOST_CHECK(results.back().sell_price == price(core.amount(100), bitcny.amount(350)));

   BOOST_CHECK_THROW(db_api.get_account_limit_orders(seller.name, GRAPHENE_SYMBOL, "CNY", 101,
               limit_order_id_type(o.id)), fc::exception);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( get_transaction_hex )
{ try {
   graphene::app::database_api db_api(db);
   auto test_private_key = generate_private_key("testaccount");
   public_key_type test_public = test_private_key.get_public_key();

   trx.operations.push_back(make_account("testaccount", test_public));
   trx.validate();

   // case1: not signed, get hex
   std::string hex_str = fc::to_hex( fc::raw::pack( trx ) );

   BOOST_CHECK( db_api.get_transaction_hex( trx ) == hex_str );
   BOOST_CHECK( db_api.get_transaction_hex_without_sig( trx ) + "00" == hex_str );

   // case2: signed, get hex
   sign( trx, test_private_key );
   hex_str = fc::to_hex( fc::raw::pack( trx ) );

   BOOST_CHECK( db_api.get_transaction_hex( trx ) == hex_str );
   BOOST_CHECK( db_api.get_transaction_hex_without_sig( trx ) +
                   fc::to_hex( fc::raw::pack( trx.signatures ) ) == hex_str );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(get_account_summaries)
{ try {
    ACTORS((joseph)(mario)(mark));

    share_type value;

    const auto& core = get_asset( GRAPHENE_SYMBOL );
    graphene::app::database_api db_api(db);

    // test non-existent account/asset

    {
        flat_set<asset_id_type> assets;

        auto nonexistent_id = joseph_id;
        nonexistent_id.instance = GRAPHENE_DB_MAX_INSTANCE_ID;

        const auto& result_numeric = db_api.get_account_summaries(nonexistent_id, assets);
        const auto& result_named = db_api.get_named_account_summaries(string("unicorn"), assets);

        BOOST_CHECK(result_numeric.size() == 0u);
        BOOST_CHECK(result_named.size() == 0u);
    }
    {
        flat_set<asset_id_type> assets;

        auto nonexistent_id = core.get_id();
        nonexistent_id.instance = GRAPHENE_DB_MAX_INSTANCE_ID;

        assets.insert(nonexistent_id);

        const auto& result = db_api.get_account_summaries(joseph_id, assets);

        BOOST_CHECK(result.size() == 1u);
    }

    // explicitly test summary of an account with zero transactions

    {
        flat_set<asset_id_type> assets;
        assets.insert(core.get_id());

        const auto& result = db_api.get_account_summaries(joseph_id, assets);

        BOOST_CHECK(result.size() == 1u);
        BOOST_CHECK(result[0u].asset_id == core.get_id());
        BOOST_CHECK(result[0u].debit_transfers == 0);
        BOOST_CHECK(result[0u].credit_transfers == 0);
    }

    // transfer default asset

    account_summary core_joseph, core_mario, core_mark;

    value = 1000000;
    fund(joseph, core.amount(value));
    core_joseph.credit_transfers += value;

    value = 10000;
    transfer(joseph_id, mario_id, core.amount(value));
    core_joseph.debit_transfers += value;
    core_mario.credit_transfers += value;

    value = 20000;
    transfer(joseph_id, mark_id, core.amount(value));
    core_joseph.debit_transfers += value;
    core_mark.credit_transfers += value;

    value = 5000;
    transfer(mark_id, mario_id, core.amount(value));
    core_mark.debit_transfers += value;
    core_mario.credit_transfers += value;

    generate_block();

    {
        flat_set<asset_id_type> assets;

        const auto& result_joseph = db_api.get_account_summaries(joseph_id, assets);
        const auto& result_mario = db_api.get_account_summaries(mario_id, assets);
        const auto& result_mark = db_api.get_account_summaries(mark_id, assets);

        BOOST_CHECK(result_joseph.size() == 1u);
        BOOST_CHECK(result_joseph[0u].asset_id == core.get_id());
        BOOST_CHECK(result_joseph[0u].debit_transfers == core_joseph.debit_transfers);
        BOOST_CHECK(result_joseph[0u].credit_transfers == core_joseph.credit_transfers);

        BOOST_CHECK(result_mario.size() == 1u);
        BOOST_CHECK(result_mario[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mario[0u].debit_transfers == core_mario.debit_transfers);
        BOOST_CHECK(result_mario[0u].credit_transfers == core_mario.credit_transfers);

        BOOST_CHECK(result_mark.size() == 1u);
        BOOST_CHECK(result_mark[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mark[0u].debit_transfers == core_mark.debit_transfers);
        BOOST_CHECK(result_mark[0u].credit_transfers == core_mark.credit_transfers);
    }
    {
        flat_set<asset_id_type> assets;

        // explicit names - variables defined with the ACTOR(S) macro(s) have rotated content after generating a block
        const auto& result_joseph = db_api.get_named_account_summaries(string("joseph"), assets);
        const auto& result_mario = db_api.get_named_account_summaries(string("mario"), assets);
        const auto& result_mark = db_api.get_named_account_summaries(string("mark"), assets);

        BOOST_CHECK(result_joseph.size() == 1u);
        BOOST_CHECK(result_joseph[0u].asset_id == core.get_id());
        BOOST_CHECK(result_joseph[0u].debit_transfers == core_joseph.debit_transfers);
        BOOST_CHECK(result_joseph[0u].credit_transfers == core_joseph.credit_transfers);

        BOOST_CHECK(result_mario.size() == 1u);
        BOOST_CHECK(result_mario[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mario[0u].debit_transfers == core_mario.debit_transfers);
        BOOST_CHECK(result_mario[0u].credit_transfers == core_mario.credit_transfers);

        BOOST_CHECK(result_mark.size() == 1u);
        BOOST_CHECK(result_mark[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mark[0u].debit_transfers == core_mark.debit_transfers);
        BOOST_CHECK(result_mark[0u].credit_transfers == core_mark.credit_transfers);
    }

    // create & transfer custom asset

    account_summary oil_joseph, oil_mario, oil_mark;

    value = 10000;
    const auto oil = create_user_issued_asset("OIL", get_account("joseph"), override_authority);
    issue_uia(joseph_id, oil.amount(value));
    BOOST_REQUIRE_EQUAL(get_balance(joseph_id, oil.get_id()), value.value);

    value = 1000;
    transfer(joseph_id, mario_id, oil.amount(value));
    oil_joseph.debit_transfers += value;
    oil_mario.credit_transfers += value;

    value = 2000;
    transfer(joseph_id, mark_id, oil.amount(value));
    oil_joseph.debit_transfers += value;
    oil_mark.credit_transfers += value;

    value = 500;
    transfer(mark_id, mario_id, oil.amount(value));
    oil_mark.debit_transfers += value;
    oil_mario.credit_transfers += value;

    generate_block();

    {
        flat_set<asset_id_type> assets;
        assets.insert(core.get_id());

        const auto& result_joseph = db_api.get_account_summaries(joseph_id, assets);
        const auto& result_mario = db_api.get_account_summaries(mario_id, assets);
        const auto& result_mark = db_api.get_account_summaries(mark_id, assets);

        BOOST_CHECK(result_joseph.size() == 1u);
        BOOST_CHECK(result_joseph[0u].asset_id == core.get_id());
        BOOST_CHECK(result_joseph[0u].debit_transfers == core_joseph.debit_transfers);
        BOOST_CHECK(result_joseph[0u].credit_transfers == core_joseph.credit_transfers);

        BOOST_CHECK(result_mario.size() == 1u);
        BOOST_CHECK(result_mario[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mario[0u].debit_transfers == core_mario.debit_transfers);
        BOOST_CHECK(result_mario[0u].credit_transfers == core_mario.credit_transfers);

        BOOST_CHECK(result_mark.size() == 1u);
        BOOST_CHECK(result_mark[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mark[0u].debit_transfers == core_mark.debit_transfers);
        BOOST_CHECK(result_mark[0u].credit_transfers == core_mark.credit_transfers);
    }
    {
        flat_set<asset_id_type> assets;
        assets.insert(oil.get_id());

        const auto& result_joseph = db_api.get_account_summaries(joseph_id, assets);
        const auto& result_mario = db_api.get_account_summaries(mario_id, assets);
        const auto& result_mark = db_api.get_account_summaries(mark_id, assets);

        BOOST_CHECK(result_joseph.size() == 1u);
        BOOST_CHECK(result_joseph[0u].asset_id == oil.get_id());
        BOOST_CHECK(result_joseph[0u].debit_transfers == oil_joseph.debit_transfers);
        BOOST_CHECK(result_joseph[0u].credit_transfers == oil_joseph.credit_transfers);

        BOOST_CHECK(result_mario.size() == 1u);
        BOOST_CHECK(result_mario[0u].asset_id == oil.get_id());
        BOOST_CHECK(result_mario[0u].debit_transfers == oil_mario.debit_transfers);
        BOOST_CHECK(result_mario[0u].credit_transfers == oil_mario.credit_transfers);

        BOOST_CHECK(result_mark.size() == 1u);
        BOOST_CHECK(result_mark[0u].asset_id == oil.get_id());
        BOOST_CHECK(result_mark[0u].debit_transfers == oil_mark.debit_transfers);
        BOOST_CHECK(result_mark[0u].credit_transfers == oil_mark.credit_transfers);
    }
    {
        flat_set<asset_id_type> assets;

        const auto& result_joseph = db_api.get_account_summaries(joseph_id, assets);
        const auto& result_mario = db_api.get_account_summaries(mario_id, assets);
        const auto& result_mark = db_api.get_account_summaries(mark_id, assets);

        BOOST_CHECK(result_joseph.size() == 2u);
        BOOST_CHECK(result_joseph[0u].asset_id == core.get_id());
        BOOST_CHECK(result_joseph[1u].asset_id == oil.get_id());
        BOOST_CHECK(result_joseph[0u].debit_transfers == core_joseph.debit_transfers);
        BOOST_CHECK(result_joseph[0u].credit_transfers == core_joseph.credit_transfers);
        BOOST_CHECK(result_joseph[1u].debit_transfers == oil_joseph.debit_transfers);
        BOOST_CHECK(result_joseph[1u].credit_transfers == oil_joseph.credit_transfers);

        BOOST_CHECK(result_mario.size() == 2u);
        BOOST_CHECK(result_mario[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mario[1u].asset_id == oil.get_id());
        BOOST_CHECK(result_mario[0u].debit_transfers == core_mario.debit_transfers);
        BOOST_CHECK(result_mario[0u].credit_transfers == core_mario.credit_transfers);
        BOOST_CHECK(result_mario[1u].debit_transfers == oil_mario.debit_transfers);
        BOOST_CHECK(result_mario[1u].credit_transfers == oil_mario.credit_transfers);

        BOOST_CHECK(result_mark.size() == 2u);
        BOOST_CHECK(result_mark[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mark[1u].asset_id == oil.get_id());
        BOOST_CHECK(result_mark[0u].debit_transfers == core_mark.debit_transfers);
        BOOST_CHECK(result_mark[0u].credit_transfers == core_mark.credit_transfers);
        BOOST_CHECK(result_mark[1u].debit_transfers == oil_mark.debit_transfers);
        BOOST_CHECK(result_mark[1u].credit_transfers == oil_mark.credit_transfers);
    }
    {
        flat_set<asset_id_type> assets;
        assets.insert(core.get_id());
        assets.insert(oil.get_id());

        const auto& result_joseph = db_api.get_account_summaries(joseph_id, assets);
        const auto& result_mario = db_api.get_account_summaries(mario_id, assets);
        const auto& result_mark = db_api.get_account_summaries(mark_id, assets);

        BOOST_CHECK(result_joseph.size() == 2u);
        BOOST_CHECK(result_joseph[0u].asset_id == core.get_id());
        BOOST_CHECK(result_joseph[1u].asset_id == oil.get_id());
        BOOST_CHECK(result_joseph[0u].debit_transfers == core_joseph.debit_transfers);
        BOOST_CHECK(result_joseph[0u].credit_transfers == core_joseph.credit_transfers);
        BOOST_CHECK(result_joseph[1u].debit_transfers == oil_joseph.debit_transfers);
        BOOST_CHECK(result_joseph[1u].credit_transfers == oil_joseph.credit_transfers);

        BOOST_CHECK(result_mario.size() == 2u);
        BOOST_CHECK(result_mario[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mario[1u].asset_id == oil.get_id());
        BOOST_CHECK(result_mario[0u].debit_transfers == core_mario.debit_transfers);
        BOOST_CHECK(result_mario[0u].credit_transfers == core_mario.credit_transfers);
        BOOST_CHECK(result_mario[1u].debit_transfers == oil_mario.debit_transfers);
        BOOST_CHECK(result_mario[1u].credit_transfers == oil_mario.credit_transfers);

        BOOST_CHECK(result_mark.size() == 2u);
        BOOST_CHECK(result_mark[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mark[1u].asset_id == oil.get_id());
        BOOST_CHECK(result_mark[0u].debit_transfers == core_mark.debit_transfers);
        BOOST_CHECK(result_mark[0u].credit_transfers == core_mark.credit_transfers);
        BOOST_CHECK(result_mark[1u].debit_transfers == oil_mark.debit_transfers);
        BOOST_CHECK(result_mark[1u].credit_transfers == oil_mark.credit_transfers);
    }

    // query not used asset

    const auto none = create_user_issued_asset("NONE", get_account("joseph"), override_authority);

    generate_block();

    {
        flat_set<asset_id_type> assets;

        const auto& result_joseph = db_api.get_account_summaries(joseph_id, assets);
        const auto& result_mario = db_api.get_account_summaries(mario_id, assets);
        const auto& result_mark = db_api.get_account_summaries(mark_id, assets);

        BOOST_CHECK(result_joseph.size() == 2u);
        BOOST_CHECK(result_joseph[0u].asset_id == core.get_id());
        BOOST_CHECK(result_joseph[1u].asset_id == oil.get_id());
        BOOST_CHECK(result_joseph[0u].debit_transfers == core_joseph.debit_transfers);
        BOOST_CHECK(result_joseph[0u].credit_transfers == core_joseph.credit_transfers);
        BOOST_CHECK(result_joseph[1u].debit_transfers == oil_joseph.debit_transfers);
        BOOST_CHECK(result_joseph[1u].credit_transfers == oil_joseph.credit_transfers);

        BOOST_CHECK(result_mario.size() == 2u);
        BOOST_CHECK(result_mario[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mario[1u].asset_id == oil.get_id());
        BOOST_CHECK(result_mario[0u].debit_transfers == core_mario.debit_transfers);
        BOOST_CHECK(result_mario[0u].credit_transfers == core_mario.credit_transfers);
        BOOST_CHECK(result_mario[1u].debit_transfers == oil_mario.debit_transfers);
        BOOST_CHECK(result_mario[1u].credit_transfers == oil_mario.credit_transfers);

        BOOST_CHECK(result_mark.size() == 2u);
        BOOST_CHECK(result_mark[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mark[1u].asset_id == oil.get_id());
        BOOST_CHECK(result_mark[0u].debit_transfers == core_mark.debit_transfers);
        BOOST_CHECK(result_mark[0u].credit_transfers == core_mark.credit_transfers);
        BOOST_CHECK(result_mark[1u].debit_transfers == oil_mark.debit_transfers);
        BOOST_CHECK(result_mark[1u].credit_transfers == oil_mark.credit_transfers);
    }
    {
        flat_set<asset_id_type> assets;
        assets.insert(none.get_id());

        const auto& result_joseph = db_api.get_account_summaries(joseph_id, assets);
        const auto& result_mario = db_api.get_account_summaries(mario_id, assets);
        const auto& result_mark = db_api.get_account_summaries(mark_id, assets);

        BOOST_CHECK(result_joseph.size() == 1u);
        BOOST_CHECK(result_joseph[0u].asset_id == none.get_id());
        BOOST_CHECK(result_joseph[0u].debit_transfers == 0);
        BOOST_CHECK(result_joseph[0u].credit_transfers == 0);

        BOOST_CHECK(result_mario.size() == 1u);
        BOOST_CHECK(result_mario[0u].asset_id == none.get_id());
        BOOST_CHECK(result_mario[0u].debit_transfers == 0);
        BOOST_CHECK(result_mario[0u].credit_transfers == 0);

        BOOST_CHECK(result_mark.size() == 1u);
        BOOST_CHECK(result_mark[0u].asset_id == none.get_id());
        BOOST_CHECK(result_mark[0u].debit_transfers == 0);
        BOOST_CHECK(result_mark[0u].credit_transfers == 0);
    }

    // test time-specific queries

    auto const one = uint32_t(db.block_interval());

    account_summary core_joseph_a, core_mario_a, core_mark_a;
    account_summary core_joseph_b, core_mario_b, core_mark_b;

    value = 1;

    generate_blocks(2);

    const auto start = db.head_block_time();

    transfer(joseph_id, mario_id, core.amount(value));
    core_joseph_a.debit_transfers += value;
    core_mario_a.credit_transfers += value;
    generate_block();

    transfer(joseph_id, mark_id, core.amount(value));
    core_joseph_a.debit_transfers += value;
    core_mark_a.credit_transfers += value;
    generate_block();

    const auto middle = db.head_block_time();

    transfer(mario_id, joseph_id, core.amount(value));
    core_mario_b.debit_transfers += value;
    core_joseph_b.credit_transfers += value;
    generate_block();

    transfer(joseph_id, mark_id, core.amount(value));
    core_joseph_b.debit_transfers += value;
    core_mark_b.credit_transfers += value;
    generate_block();

    const auto end = db.head_block_time();

    generate_blocks(2);

    {
        flat_set<asset_id_type> assets;
        {
            const auto& result = db_api.get_account_summaries(joseph_id, assets, start, start - one);
            BOOST_CHECK(result.size() == 0u);
        }
        {
            const auto& result = db_api.get_account_summaries(joseph_id, assets, middle, middle);
            BOOST_CHECK(result.size() == 0u);
        }
        {
            const auto& result = db_api.get_account_summaries(joseph_id, assets, end + one, end + one + one);
            BOOST_CHECK(result.size() == 0u);
        }
    }
    {
        flat_set<asset_id_type> assets;
        assets.insert(core.get_id());

        const auto& result_joseph = db_api.get_account_summaries(joseph_id, assets, start, end + one);
        const auto& result_joseph_a = db_api.get_account_summaries(joseph_id, assets, start, middle + one);
        const auto& result_joseph_b = db_api.get_account_summaries(joseph_id, assets, middle + one, end + one);

        const auto& result_mario = db_api.get_account_summaries(mario_id, assets, start, end + one);
        const auto& result_mario_a = db_api.get_account_summaries(mario_id, assets, start, middle + one);
        const auto& result_mario_b = db_api.get_account_summaries(mario_id, assets, middle + one, end + one);

        const auto& result_mark = db_api.get_account_summaries(mark_id, assets, start, end + one);
        const auto& result_mark_a = db_api.get_account_summaries(mark_id, assets, start, middle + one);
        const auto& result_mark_b = db_api.get_account_summaries(mark_id, assets, middle + one, end + one);

        BOOST_CHECK(result_joseph.size() == 1u);
        BOOST_CHECK(result_joseph[0u].asset_id == core.get_id());
        BOOST_CHECK(result_joseph[0u].debit_transfers == core_joseph_a.debit_transfers + core_joseph_b.debit_transfers);
        BOOST_CHECK(result_joseph[0u].credit_transfers == core_joseph_a.credit_transfers + core_joseph_b.credit_transfers);

        BOOST_CHECK(result_joseph_a.size() == 1u);
        BOOST_CHECK(result_joseph_a[0u].asset_id == core.get_id());
        BOOST_CHECK(result_joseph_a[0u].debit_transfers == core_joseph_a.debit_transfers);
        BOOST_CHECK(result_joseph_a[0u].credit_transfers == core_joseph_a.credit_transfers);

        BOOST_CHECK(result_joseph_b.size() == 1u);
        BOOST_CHECK(result_joseph_b[0u].asset_id == core.get_id());
        BOOST_CHECK(result_joseph_b[0u].debit_transfers == core_joseph_b.debit_transfers);
        BOOST_CHECK(result_joseph_b[0u].credit_transfers == core_joseph_b.credit_transfers);

        BOOST_CHECK(result_mario.size() == 1u);
        BOOST_CHECK(result_mario[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mario[0u].debit_transfers == core_mario_a.debit_transfers + core_mario_b.debit_transfers);
        BOOST_CHECK(result_mario[0u].credit_transfers == core_mario_a.credit_transfers + core_mario_b.credit_transfers);

        BOOST_CHECK(result_mario_a.size() == 1u);
        BOOST_CHECK(result_mario_a[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mario_a[0u].debit_transfers == core_mario_a.debit_transfers);
        BOOST_CHECK(result_mario_a[0u].credit_transfers == core_mario_a.credit_transfers);

        BOOST_CHECK(result_mario_b.size() == 1u);
        BOOST_CHECK(result_mario_b[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mario_b[0u].debit_transfers == core_mario_b.debit_transfers);
        BOOST_CHECK(result_mario_b[0u].credit_transfers == core_mario_b.credit_transfers);

        BOOST_CHECK(result_mark.size() == 1u);
        BOOST_CHECK(result_mark[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mark[0u].debit_transfers == core_mark_a.debit_transfers + core_mark_b.debit_transfers);
        BOOST_CHECK(result_mark[0u].credit_transfers == core_mark_a.credit_transfers + core_mark_b.credit_transfers);

        BOOST_CHECK(result_mark_a.size() == 1u);
        BOOST_CHECK(result_mark_a[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mark_a[0u].debit_transfers == core_mark_a.debit_transfers);
        BOOST_CHECK(result_mark_a[0u].credit_transfers == core_mark_a.credit_transfers);

        BOOST_CHECK(result_mark_b.size() == 1u);
        BOOST_CHECK(result_mark_b[0u].asset_id == core.get_id());
        BOOST_CHECK(result_mark_b[0u].debit_transfers == core_mark_b.debit_transfers);
        BOOST_CHECK(result_mark_b[0u].credit_transfers == core_mark_b.credit_transfers);
    }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
