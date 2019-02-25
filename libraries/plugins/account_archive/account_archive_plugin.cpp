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

#include <graphene/account_archive/account_archive_plugin.hpp>

#include <graphene/chain/impacted.hpp>

#include <graphene/chain/account_evaluator.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/config.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/operation_archive_object.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

namespace graphene { namespace account_archive {

namespace detail {

   class account_archive_plugin_impl
   {
      public:
         account_archive_plugin_impl(account_archive_plugin& _plugin) : _self(_plugin) {}
         virtual ~account_archive_plugin_impl();

         graphene::chain::database& database() { return _self.database(); }

         /** This updates indices after a block is enchained. */
         void process_block(const signed_block& b);

         account_archive_plugin& _self;

      private:
         flat_set<account_id_type> get_impacted_accounts(const operation_history_object& op, const object_database& db);
   };

   account_archive_plugin_impl::~account_archive_plugin_impl() {}

   void account_archive_plugin_impl::process_block(const signed_block& b)
   {
      auto& db = database();
      auto& account_archives = db.get_index_type<account_archive_index>().indices().get<by_id>();

      // index enchained operations
      const auto& operations = db.get_applied_operations();
      for (const auto& op : operations) {
         if (!op.valid())
            continue;

         // TODO: virtual operations have to be stored in a separate database; for now, do not index them;
         if (((*op).op.which() == operation::tag<fill_order_operation>::value)
           ||((*op).op.which() == operation::tag<asset_settle_cancel_operation>::value)
           ||((*op).op.which() == operation::tag<fba_distribute_operation>::value)
           ||((*op).op.which() == operation::tag<execute_bid_operation>::value)) {
              continue;
         }

         // index in operation archive
         auto initialize_operation = [&](operation_archive_object& o) {
            o.block_num = op->block_num;
            o.trx_in_block = op->trx_in_block;
            o.op_in_trx = op->op_in_trx;
            o.virtual_op = op->virtual_op;
            o.operation_id = static_cast<uint16_t>(op->op.which());
         };
         auto& indexed_operation = db.create<operation_archive_object>(initialize_operation);

         // index in account archives
         auto append_operation = [&](account_archive_object& o) { o.operations.push_back(indexed_operation.id); };
         flat_set<account_id_type> impacted_accounts = get_impacted_accounts(*op, db);
         for (auto& acc : impacted_accounts) {
            object_id_type id = account_archive_id_type(acc.instance);
            const account_archive_object* archive = nullptr;
            const auto& finder = account_archives.find(id);
            if (finder != account_archives.end())
               archive = &(*finder);
            else
               archive = &db.create<account_archive_object>([&](account_archive_object& aao) { aao.id = id; });
            FC_ASSERT(archive != nullptr);
            db.modify(*archive, append_operation);
         }
      }
   }

   flat_set<account_id_type> account_archive_plugin_impl::get_impacted_accounts(const operation_history_object& op, const object_database& db)
   {
      flat_set<account_id_type> impacted;
      vector<authority> other;

      operation_get_required_authorities(op.op, impacted, impacted, other);

      if (op.op.which() == operation::tag<account_create_operation>::value)
         impacted.insert(op.result.get<object_id_type>());
      else
         operation_get_impacted_accounts(op.op, impacted);

      for (auto& a : other)
         for (auto& item : a.account_auths)
            impacted.insert(item.first);

      return impacted;
   }

} // graphene::account_history::detail

   account_archive_plugin::account_archive_plugin()
   {
      impl = unique_ptr<detail::account_archive_plugin_impl>{ new detail::account_archive_plugin_impl(*this)};
   }

   account_archive_plugin::~account_archive_plugin()
   {
   }

   std::string account_archive_plugin::plugin_name() const
   {
      return "account_archive";
   }

   void account_archive_plugin::plugin_initialize(const boost::program_options::variables_map& options)
   {
      database().add_index<primary_index<account_archive_index>>();
      database().add_index<primary_index<operation_archive_index>>();
      database().applied_block.connect( [&](const signed_block& b){ impl->process_block(b); } );
   }

   void account_archive_plugin::plugin_startup()
   {
   }

   void account_archive_plugin::plugin_set_program_options(
      boost::program_options::options_description& cli,
      boost::program_options::options_description& cfg)
   {
   }

} } // graphene::account_history
