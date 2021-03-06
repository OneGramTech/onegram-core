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

#include <graphene/protocol/operations.hpp>
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

      inline void     set_virtual_op_db_index(uint32_t index) { db_index = -(1 + static_cast<int32_t>(index)); }
      inline bool     has_virtual_op() const                  { return db_index < 0; }
      inline uint32_t get_virtual_op_db_index() const         { return static_cast<uint32_t>(-(db_index + 1)); }

            uint32_t block_num = 0;
      union {
         struct {
            uint16_t trx_in_block;
            uint16_t op_in_trx;
         };
            int32_t  db_index = 0;
      };
            uint16_t virtual_op = 0;
            uint16_t operation_id = 0;
};

typedef simple_index<operation_archive_object> operation_archive_index;

/**
 * @brief An index relative to a particular account referencing respective @operation_archive_object.
 *
 *  These indices are stored per account in order to speed up inspection of archived operation by account.
 *
 * !!!! NOTE !!!!
 *  Instances of this class shall never be erased from the database.
 *  In order to save memory due to the undo database storing whole
 *  objects in the remembered states, this object stores only the number
 *  of operations indexed at the moment of being stored by the undo db.
 *  Furthermore, the indices present in the @operations vector shall
 *  never be modified and/or removed. Otherwise the undo db could not work.
 *  In particular, the undo db would not restore a removed instance of
 *  this class. The merge function could fail similarly.
 */
class account_archive_object : public abstract_object<account_archive_object>
{
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id = impl_account_archive_object_type;

      vector<operation_archive_id_type> operations;

      account_id_type get_owner_account() const;

      unique_ptr<object> clone() const
      {
         auto aao = new account_archive_object();
         aao->id = this->id;
         aao->operations.emplace_back(operation_archive_id_type(this->operations.size()));
         return unique_ptr<object>(aao);
      }

      void move_from(object& obj)
      {
         auto aao = static_cast<account_archive_object&>(obj);
         auto num = static_cast<object_id_type>(aao.operations[0]);
         this->operations.erase(this->operations.begin() + num.instance(), this->operations.end());
      }
};

inline account_id_type account_archive_object::get_owner_account() const
{
   return account_id_type() + (int64_t)id.instance();
}

typedef multi_index_container<
   account_archive_object,
   indexed_by<
      ordered_unique<tag<by_id>, member<object, object_id_type, &object::id>>
   >
> account_archive_multi_index_type;

typedef generic_index<account_archive_object, account_archive_multi_index_type> account_archive_index;

} } // graphene::chain

MAP_OBJECT_ID_TO_TYPE(graphene::chain::operation_archive_object)
MAP_OBJECT_ID_TO_TYPE(graphene::chain::account_archive_object)

FC_REFLECT_TYPENAME( graphene::chain::operation_archive_object )
FC_REFLECT_TYPENAME( graphene::chain::account_archive_object )

GRAPHENE_DECLARE_EXTERNAL_SERIALIZATION( graphene::chain::operation_archive_object )
GRAPHENE_DECLARE_EXTERNAL_SERIALIZATION( graphene::chain::account_archive_object )
