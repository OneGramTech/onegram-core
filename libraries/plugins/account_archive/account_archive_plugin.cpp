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

#include <boost/filesystem.hpp>
#include <fc/filesystem.hpp>
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

         void                     init(const boost::program_options::variables_map& options);
         operation_history_object load(uint32_t index) const;

      private:
         operation_database _operation_db;

         flat_set<account_id_type> get_impacted_accounts(const operation_history_object& op, const object_database& db);
   };

   account_archive_plugin_impl::~account_archive_plugin_impl() {}

   void account_archive_plugin_impl::process_block(const signed_block& b)
   {
      auto& db = database();
      auto& account_archives = db.get_index_type<account_archive_index>().indices().get<by_id>();

      // comply with undo if applied
      _operation_db.truncate(b.block_num());

      // index enchained operations
      const auto numtrxs = static_cast<uint16_t>(b.transactions.size());
      const auto& operations = db.get_applied_operations();
      for (const auto& op : operations) {
         if (!op.valid())
            continue;

         uint32_t virtual_op_db_index = 0;
         const bool op_is_virtual = ((*op).trx_in_block == numtrxs) || ((*op).virtual_op);

         // store virtual operation in custom database
         if (op_is_virtual) {
            virtual_op_db_index = _operation_db.store((*op).op);
         }

         // index in operation archive
         auto initialize_operation = [&](operation_archive_object& o) {
            o.block_num = op->block_num;
            if (op_is_virtual) {
               o.set_virtual_op_db_index(virtual_op_db_index);
            } else {
               o.trx_in_block = op->trx_in_block;
               o.op_in_trx = op->op_in_trx;
            }
            o.virtual_op = op->virtual_op;
            o.operation_id = static_cast<uint16_t>(op->op.which());
         };
         auto& indexed_operation = db.create<operation_archive_object>(initialize_operation); // THIS OBJECT SHALL NOT BE REMOVED FROM THE DB

         // index in account archives
         auto append_operation = [&](account_archive_object& o) { o.operations.push_back(indexed_operation.id); }; // THE operations VECTOR SHALL NEVER BE MODIFIED EXCEPT BEING EXPANDED
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

      // flush all virtual operations at once
      _operation_db.flush();
   }

   void account_archive_plugin_impl::init(const boost::program_options::variables_map& options)
   {
      fc::path data_dir;
      if (options.count("data-dir"))
      {
         data_dir = options["data-dir"].as<boost::filesystem::path>();
         if(data_dir.is_relative())
            data_dir = fc::current_path() / data_dir;
      }

      if (options.count("resync-blockchain") || options.count("replay-blockchain") || options.count("revalidate-blockchain"))
         _operation_db.wipe(data_dir);
      _operation_db.open(data_dir);
   }

   operation_history_object account_archive_plugin_impl::load(uint32_t index) const
   {
      return _operation_db.load(index);
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

} // graphene::account_archive::detail

   account_archive_plugin::account_archive_plugin()
   {
      impl = unique_ptr<detail::account_archive_plugin_impl>{ new detail::account_archive_plugin_impl(*this) };
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

      impl->init(options);
   }

   void account_archive_plugin::plugin_startup()
   {
   }

   void account_archive_plugin::plugin_set_program_options(
      boost::program_options::options_description& cli,
      boost::program_options::options_description& cfg)
   {
   }

   operation_history_object account_archive_plugin::load(uint32_t index) const
   {
      return impl->load(index);
   }

} } // graphene::account_archive
