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
#pragma once

#include <graphene/chain/protocol/operations.hpp>
#include <graphene/db/object.hpp> 
#include <graphene/db/simple_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {

/**
 * @brief Tracks the history of all logical operations on blockchain state.
 * @ingroup object
 * @ingroup implementation
 * 
 *  All operations and virtual operations are indexed with this proxy object
 *  as they written to the blockchain. It allows for effective creation
 *  of the operation_history_object directly from the raw blocks which are
 *  stored on the disk. Each proxy operation is assigned with a number which
 *  is the same as the indexed operation_history_object's unique ID / sequence number.
 * 
 * @note This serves as a "cheap" (in terms of RAM requirements) surrogate
 * for the history_plugin's database of actual operation history objects.
 */
class operation_archive_object : public abstract_object<operation_archive_object>
{
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id = impl_operation_archive_object_type;

      uint32_t block_num = 0;
      uint16_t trx_in_block = 0;
      uint16_t op_in_trx = 0;
      uint16_t virtual_op = 0;
      uint16_t operation_id = 0;
};

typedef simple_index<operation_archive_object> operation_archive_index;

/**
 * @brief An index relative to a particular account referencing respective @operation_archive_object.
 *
 *  These indices are stored per account in order to speed up inspection of archived operation by account.
 */
class account_archive_object : public abstract_object<account_archive_object>
{
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id = impl_account_archive_object_type;

      vector<operation_archive_id_type> operations;

      account_id_type get_owner_account() const;
};

inline account_id_type account_archive_object::get_owner_account() const
{
   return account_id_type() + (int64_t)id.instance();
}

struct by_id;

typedef multi_index_container<
   account_archive_object,
   indexed_by<
      ordered_unique<tag<by_id>, member<object, object_id_type, &object::id>>
   >
> account_archive_multi_index_type;

typedef generic_index<account_archive_object, account_archive_multi_index_type> account_archive_index;

} } // graphene::chain

FC_REFLECT_DERIVED(graphene::chain::operation_archive_object, (graphene::db::object),
                   (block_num)(trx_in_block)(op_in_trx)(virtual_op)(operation_id))

FC_REFLECT_DERIVED(graphene::chain::account_archive_object, (graphene::db::object),
                   (operations))
