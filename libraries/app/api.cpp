/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
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
#include <cctype>

#include <graphene/app/api.hpp>
#include <graphene/app/api_access.hpp>
#include <graphene/app/application.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/get_config.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <graphene/protocol/fee_schedule.hpp>
#include <graphene/protocol/operations_permissions.hpp>
#include <graphene/chain/confidential_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/transaction_history_object.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/worker_object.hpp>

#include <fc/crypto/hex.hpp>
#include <fc/rpc/api_connection.hpp>
#include <fc/thread/future.hpp>

template class fc::api<graphene::app::block_api>;
template class fc::api<graphene::app::network_broadcast_api>;
template class fc::api<graphene::app::network_node_api>;
template class fc::api<graphene::app::history_api>;
template class fc::api<graphene::app::crypto_api>;
template class fc::api<graphene::app::asset_api>;
template class fc::api<graphene::app::orders_api>;
template class fc::api<graphene::debug_witness::debug_api>;
template class fc::api<graphene::app::login_api>;


namespace graphene { namespace app {

   /* HELPERS begin */

   time_point_sec get_block_time(const database_api& db_api, uint32_t block_num)
   {
      const auto& header = db_api.get_block_header(block_num);
      return header.valid() ? header->timestamp : time_point_sec::min();
   }

   optional<operation> get_archived_operation(const database& db, const account_archive::account_archive_plugin& ap, const operation_archive_object& oao)
   {
      auto op = operation();
      if (oao.has_virtual_op()) {
         op = (ap.load(oao.get_virtual_op_db_index())).op;
      } else {
         const auto& block = db.fetch_block_by_number(oao.block_num);
         if (block.valid() && ((size_t)oao.trx_in_block < block->transactions.size())) {
            const auto& trx = block->transactions[oao.trx_in_block];
            if ((size_t)oao.op_in_trx < trx.operations.size())
               op = trx.operations[oao.op_in_trx];
         }
      }
      return op;
   }

   optional<operation_history_object> get_oho_without_id(const database& db, const account_archive::account_archive_plugin& ap, const operation_archive_object& oao)
   {
      auto oho = operation_history_object();
      if (oao.has_virtual_op()) {
         oho = ap.load(oao.get_virtual_op_db_index());
      } else {
         const auto& block = db.fetch_block_by_number(oao.block_num);
         if (block.valid() && ((size_t)oao.trx_in_block < block->transactions.size())) {
            const auto& trx = block->transactions[oao.trx_in_block];
            if ((size_t)oao.op_in_trx < trx.operations.size()) {
               oho.op = trx.operations[oao.op_in_trx];
               oho.result = trx.operation_results[oao.op_in_trx];
               oho.block_num = oao.block_num;
               oho.trx_in_block = oao.trx_in_block;
               oho.op_in_trx = oao.op_in_trx;
               oho.virtual_op = oao.virtual_op;
            }
         }
      }
      return oho;
   }

   bool check_query_index_input(size_t num_ops, size_t last, size_t count, size_t count_limit)
   {
      FC_ASSERT(!num_ops || (num_ops >= last + 1), "invalid request offset");
      FC_ASSERT(count <= count_limit, "invalid request count");
      FC_ASSERT(count <= last + 1, "invalid request count");
      return count > 0u;
   }

   // remove invalid operation ids
   bool check_query_opid_input(flat_set<int>& ids)
   {
      for (auto i = ids.begin(); i != ids.end(); i++) {
         if ((*i < 0) || (*i > operation::count()))
            i = ids.erase(i);
      }
      return ids.size() > 0u;
   }

   const account_archive_object* get_account_operations(const database& db, const account_id_type& account_id)
   {
      const auto& account_archive = db.get_index_type<account_archive_index>().indices().get<by_id>();
      const auto aao_id = account_archive_id_type(account_id.instance);
      const auto& finder = account_archive.find(aao_id);
      return (finder != account_archive.end()) ? &(*finder) : nullptr;
   }

   object_id_type get_archived_operation_id(const account_archive_object* archive, const account_id_type* account, size_t index)
   {
      return account ? archive->operations[index] : operation_archive_id_type(index);
   }

   fc::time_point_sec get_operation_time(
         const database_api& db_api,
         const operation_archive_index& operation_archive,
         const account_archive_object* account_operations,
         const account_id_type* account,
         size_t index)
   {
      const auto object_id = get_archived_operation_id(account_operations, account, index);
      const auto op_object = static_cast<const operation_archive_object &>(operation_archive.get(object_id));
      return get_block_time(db_api, op_object.block_num);
   }

   inline int64_t pick_pivot(int64_t begin, int64_t end)
   {
      return (end + begin + 1) >> 1; // average rounded up
   }

   // finds last operation written later than 'time'
   size_t find_operation(
         const database_api& db_api,
         const operation_archive_index& operation_archive,
         const account_archive_object* account_operations,
         const account_id_type* account,
         size_t max_op_count,
         time_point_sec time)
   {
      if ( !max_op_count )
         return 0;

      // this is edge case, when every value is > than time and algorithm would return 1 instead of 0
      auto first_time = get_operation_time(db_api, operation_archive, account_operations, account, 0);
      if ( first_time >= time )
         return 0;

      // use binary search to find specific operation id
      int64_t begin = 0;
      int64_t end = (int64_t)max_op_count - 1;
      while (begin < end) {
         const int64_t pivot = pick_pivot(begin, end);
         auto pivot_time = get_operation_time(db_api, operation_archive, account_operations, account, pivot);
         if (pivot_time < time) {
            begin = pivot;
         } else {
            end = pivot - 1;
         }
      };

      return begin + 1; // move from index to id space
   }

   asset_id_type get_asset_id(const database_api& db_api, const string& id_or_symbol)
   {
      vector<string> symbol_or_id;
      symbol_or_id.push_back(id_or_symbol);
      auto result = db_api.lookup_asset_symbols(symbol_or_id);
      FC_ASSERT(result[0].valid(), "Unable to find asset '${string}'", ("symbol", id_or_symbol));
      return result[0]->get_id();
   }

   struct fee_retrieval_visitor
   {
      typedef share_type result_type;

      const account_id_type& account_id;
      const asset_id_type& asset_id;

      fee_retrieval_visitor(const account_id_type& account_id, const asset_id_type& asset_id) : account_id(account_id), asset_id(asset_id) {}

      template<typename OpType>
      result_type operator()(OpType& op) const
      {
         return ((op.fee_payer() == account_id) && (op.fee.asset_id == asset_id)) ? op.fee.amount : 0;
      }
   };

   /* HELPERS end */

   const uint64_t archive_api::QueryLimitBase = 10;

   const struct archive_api::parameters archive_api::params = {
      5 * archive_api::QueryLimitBase,
      25 * archive_api::QueryLimitBase,
   };

   archive_api::parameters archive_api::get_archive_api_parameters() const
   {
      return params;
   }

   archive_api::query_result archive_api::get_archived_operations(uint64_t last,
                                                                  uint64_t count,
                                                                  flat_set<int> operation_id_filter) const
   {
      return my_get_archived_operations(nullptr, last, count, operation_id_filter);
   }

   archive_api::query_result archive_api::get_archived_operations_by_time(time_point_sec inclusive_from,
                                                                          time_point_sec exclusive_until,
                                                                          uint64_t skip_count,
                                                                          flat_set<int> operation_id_filter) const
   {
      return my_get_archived_operations_by_time(nullptr, inclusive_from, exclusive_until, skip_count, operation_id_filter);
   }

   archive_api::query_result archive_api::archive_api::get_archived_account_operations(const std::string account_id_or_name,
                                                                                       uint64_t last,
                                                                                       uint64_t count,
                                                                                       flat_set<int> operation_id_filter) const
   {
      const auto account_id = database_api.get_account_id_from_string(account_id_or_name);
      return my_get_archived_operations(&account_id, last, count, operation_id_filter);
   }

   archive_api::query_result archive_api::get_archived_account_operations_by_time(const std::string account_id_or_name,
                                                                                  time_point_sec inclusive_from,
                                                                                  time_point_sec exclusive_until,
                                                                                  uint64_t skip_count,
                                                                                  flat_set<int> operation_id_filter) const
   {
      const auto account_id = database_api.get_account_id_from_string(account_id_or_name);
      return my_get_archived_operations_by_time(&account_id, inclusive_from, exclusive_until, skip_count, operation_id_filter);
   }

   uint64_t archive_api::get_archived_account_operation_count(const std::string account_id_or_name) const
   {
      const auto db = _app.chain_database();

      const auto acc_id = database_api.get_account_id_from_string(account_id_or_name);
      const auto aao_id = account_archive_id_type(acc_id.instance);
      const auto& acc_archive = db->get_index_type<account_archive_index>().indices().get<by_id>();
      const auto& finder = acc_archive.find(aao_id);
      return (finder != acc_archive.end()) ? (*finder).operations.size() : 0u;
   }

   archive_api::summary_result archive_api::get_account_summary(const std::string account_id_or_name,
                                                                const std::string asset_id_or_name,
                                                                uint64_t last,
                                                                uint64_t count) const
   {
      auto result = archive_api::summary_result();

      const auto db = _app.chain_database();
      const auto ap = _app.get_plugin<account_archive::account_archive_plugin>("account_archive");

      const auto asset_id = get_asset_id(database_api, asset_id_or_name);
      const auto account_id = database_api.get_account_id_from_string(account_id_or_name);
      const auto& operation_archive = db->get_index_type<operation_archive_index>();
      const account_archive_object* account_operations = get_account_operations(*db.get(), account_id);
      if (!account_operations)
         return result; // account created in genesis without any operations yet

      size_t num_operations = account_operations->operations.size();
      if (!check_query_index_input(num_operations, last, count, params.QueryInspectLimit))
         return result;

      // inspect archived operations
      num_operations = last + 1; // number of operations left to query
      count = std::min(count, params.QueryInspectLimit);
      while (num_operations-- && count--) {
         result.num_processed++;
         const auto oao_id = get_archived_operation_id(account_operations, &account_id, num_operations);
         const auto oao = static_cast<const operation_archive_object&>(operation_archive.get(oao_id));
         const auto op = get_archived_operation(*db, *ap, oao);
         if (op.valid()) {
            FC_ASSERT((int64_t)(*op).which() == (int64_t)oao.operation_id);
            update_summary(*db.get(), account_id, asset_id, *op, result.summary);
         }
      }

      return result;
   }

   archive_api::summary_result archive_api::get_account_summary_by_time(const std::string account_id_or_name,
                                                                        const std::string asset_id_or_name,
                                                                        time_point_sec inclusive_from,
                                                                        time_point_sec exclusive_until,
                                                                        uint64_t skip_count) const
   {
      auto result = archive_api::summary_result();

      const auto db = _app.chain_database();
      const auto ap = _app.get_plugin<account_archive::account_archive_plugin>("account_archive");

      const auto asset_id = get_asset_id(database_api, asset_id_or_name);
      const auto account_id = database_api.get_account_id_from_string(account_id_or_name);
      const auto& operation_archive = db->get_index_type<operation_archive_index>();
      const account_archive_object* account_operations = get_account_operations(*db.get(), account_id);
      if (!account_operations)
         return result; // account created in genesis without any operations yet

      size_t last_op_id = account_operations->operations.size();
      size_t first_op_id = 0;

      if (!last_op_id)
         return result;

      last_op_id  = find_operation(database_api, operation_archive, account_operations, &account_id, last_op_id, exclusive_until);
      first_op_id = find_operation(database_api, operation_archive, account_operations, &account_id, last_op_id, inclusive_from);

      if ( last_op_id > skip_count )
         last_op_id -= skip_count;
      else
         last_op_id = 0;

      // inspect operations inside the time window
      while ((last_op_id > first_op_id) && (result.num_processed < params.QueryInspectLimit)) {
         result.num_processed++;
         const auto oao_id = get_archived_operation_id(account_operations, &account_id, last_op_id - 1);
         const auto oao = static_cast<const operation_archive_object&>(operation_archive.get(oao_id));
         const auto op = get_archived_operation(*db, *ap, oao);
         if (op.valid()) {
            FC_ASSERT((int64_t)(*op).which() == (int64_t)oao.operation_id);
            update_summary(*db.get(), account_id, asset_id, *op, result.summary);
         }
         last_op_id--;
      }

      return result;
   }

   archive_api::query_result archive_api::my_get_archived_operations(const account_id_type* account_id,
                                                                     uint64_t last,
                                                                     uint64_t count,
                                                                     flat_set<int> operation_id_filter) const
   {
      auto result = archive_api::query_result();

      const auto db = _app.chain_database();
      const auto ap = _app.get_plugin<account_archive::account_archive_plugin>("account_archive");

      const auto& operation_archive = db->get_index_type<operation_archive_index>();
      const account_archive_object* account_operations = nullptr;
      size_t num_operations = 0;

      if (account_id) {
         account_operations = get_account_operations(*db.get(), *account_id);
         if (!account_operations)
            return result; // account created in genesis without any operations yet
         num_operations = account_operations->operations.size();
      } else {
         num_operations = operation_archive.size();
      }
      if (!check_query_index_input(num_operations, last, count, params.QueryResultLimit))
         return result;

      const auto filter_end = operation_id_filter.end();
      const bool filter = check_query_opid_input(operation_id_filter);

      result.operations.reserve(count);

      // inspect archived operations
      num_operations = last + 1; // number of operations left to query
      while (num_operations && count && (result.num_processed < params.QueryInspectLimit)) {
         result.num_processed++;
         const auto oa_id = get_archived_operation_id(account_operations, account_id, num_operations - 1);
         const auto oao = static_cast<const operation_archive_object&>(operation_archive.get(oa_id));
         if (!filter || (operation_id_filter.find((int)oao.operation_id) != filter_end)) {
            auto oho = get_oho_without_id(*db, *ap, oao);
            if (oho.valid()) {
               FC_ASSERT((int64_t)oho->op.which() == (int64_t)oao.operation_id);
               oho->id = operation_history_id_type(oa_id.instance());
               result.operations.push_back(*oho);
               count--;
            }
         }
         num_operations--;
      }

      return result;
   }

   archive_api::query_result archive_api::my_get_archived_operations_by_time(const account_id_type* account_id,
                                                                             time_point_sec inclusive_from,
                                                                             time_point_sec exclusive_until,
                                                                             uint64_t skip_count,
                                                                             flat_set<int> operation_id_filter) const
   {
      auto result = archive_api::query_result();

      if (inclusive_from >= exclusive_until)
         return result;

      const auto db = _app.chain_database();
      const auto ap = _app.get_plugin<account_archive::account_archive_plugin>("account_archive");

      const auto& operation_archive = db->get_index_type<operation_archive_index>();
      const account_archive_object* account_operations = nullptr;
      size_t last_op_id = 0;
      size_t first_op_id = 0;

      if (account_id) {
         account_operations = get_account_operations(*db.get(), *account_id);
         if (!account_operations)
            return result; // account created in genesis without any operations yet
         last_op_id = account_operations->operations.size();
      } else {
         last_op_id = operation_archive.size();
      }
      if (!last_op_id)
         return result;

      const auto filter_end = operation_id_filter.end();
      const bool filter = check_query_opid_input(operation_id_filter);

      last_op_id  = find_operation(database_api, operation_archive, account_operations, account_id, last_op_id, exclusive_until);
      first_op_id = find_operation(database_api, operation_archive, account_operations, account_id, last_op_id, inclusive_from);

      if ( last_op_id > skip_count )
         last_op_id -= skip_count;
      else
         last_op_id = 0;

      result.operations.reserve(params.QueryResultLimit);

      // inspect operations inside the time window
      while ((last_op_id > first_op_id) && (result.num_processed < params.QueryInspectLimit)) {
         const auto oa_id = get_archived_operation_id(account_operations, account_id, last_op_id - 1);
         const auto oao = static_cast<const operation_archive_object&>(operation_archive.get(oa_id));
         result.num_processed++;
         if (!filter || (operation_id_filter.find((int)oao.operation_id) != filter_end)) {
            auto oho = get_oho_without_id(*db, *ap, oao);
            if (oho.valid()) {
               FC_ASSERT((int64_t)oho->op.which() == (int64_t)oao.operation_id);
               oho->id = operation_history_id_type(oa_id.instance());
               result.operations.push_back(*oho);
               if (result.operations.size() == params.QueryResultLimit)
                  break;
            }
         }
         last_op_id--;
      }

      return result;
   }

   void archive_api::update_summary(const database& db,
                                    account_id_type account_id,
                                    asset_id_type asset_id,
                                    const operation& op,
                                    account_archive::account_summary& sum) const
   {
      switch(op.which()) {
         case operation::tag<transfer_operation>::value:
         {
            const auto& o = op.get<transfer_operation>();
            FC_ASSERT((o.from == account_id) || (o.to == account_id));
            if (o.amount.asset_id == asset_id) {
               if (o.from == account_id)
                  sum.debits += o.amount.amount;
               if (o.to == account_id)
                  sum.credits += o.amount.amount;
            }
            break;
         }
         case operation::tag<limit_order_create_operation>::value:
         {
            const auto& o = op.get<limit_order_create_operation>();
            FC_ASSERT(o.seller == account_id);
            if (o.amount_to_sell.asset_id == asset_id) {
               sum.debits += o.amount_to_sell.amount;
            }
            break;
         }
         case operation::tag<limit_order_cancel_operation>::value:
         {
            const auto& o = op.get<limit_order_cancel_operation>();
            const auto& order = o.order(db);
            FC_ASSERT(order.seller == account_id);
            FC_ASSERT(order.sell_price.base.asset_id == asset_id);
            sum.credits += order.sell_price.base.amount;
            break;
         }
         case operation::tag<call_order_update_operation>::value:
         {
            const auto& o = op.get<call_order_update_operation>();
            FC_ASSERT(o.funding_account == account_id);
            if (o.delta_collateral.asset_id == asset_id) {
               if (o.delta_collateral.amount > 0)
                  sum.debits += o.delta_collateral.amount;
               if (o.delta_collateral.amount < 0)
                  sum.credits += o.delta_collateral.amount;
            }
            if (o.delta_debt.asset_id == asset_id) {
               if (o.delta_debt.amount > 0)
                  sum.credits += o.delta_debt.amount;
               if (o.delta_debt.amount < 0)
                  sum.debits += o.delta_debt.amount;
            }
            break;
         }
         case operation::tag<fill_order_operation>::value: // VIRTUAL
         {
            const auto& o = op.get<fill_order_operation>();
            FC_ASSERT(o.account_id == account_id);
            // seems to be accounted in the limit_order_create_operation
            //if (o.pays.asset_id == asset_id)
            //   sum.debits += o.pays.amount;
            if (o.receives.asset_id == asset_id)
               sum.credits += o.receives.amount;
            break;
         }
         case operation::tag<asset_issue_operation>::value:
         {
            const auto& o = op.get<asset_issue_operation>();
            FC_ASSERT((o.issuer == account_id) || (o.issue_to_account == account_id));
            if ((o.asset_to_issue.asset_id == asset_id) && (o.issue_to_account == account_id))
               sum.credits += o.asset_to_issue.amount;
            break;
         }
         case operation::tag<asset_reserve_operation>::value:
         {
            const auto& o = op.get<asset_reserve_operation>();
            FC_ASSERT(o.payer == account_id);
            if (o.amount_to_reserve.asset_id == asset_id)
               sum.debits += o.amount_to_reserve.amount;
            break;
         }
         case operation::tag<asset_fund_fee_pool_operation>::value:
         {
            const auto& o = op.get<asset_fund_fee_pool_operation>();
            FC_ASSERT(o.from_account == account_id);
            if (o.asset_id == asset_id)
               sum.debits += o.amount;
            break;
         }
         case operation::tag<asset_settle_operation>::value:
         {
            const auto& o = op.get<asset_settle_operation>();
            FC_ASSERT(o.account == account_id);
            if (o.amount.asset_id == asset_id)
               sum.debits += o.amount.amount;
            break;
         }
         case operation::tag<withdraw_permission_claim_operation>::value:
         {
            const auto& o = op.get<withdraw_permission_claim_operation>();
            FC_ASSERT((o.withdraw_from_account == account_id) || (o.withdraw_to_account == account_id));
            if (o.amount_to_withdraw.asset_id == asset_id) {
               if (o.withdraw_from_account == account_id)
                  sum.debits += o.amount_to_withdraw.amount;
               if (o.withdraw_to_account == account_id)
                  sum.credits += o.amount_to_withdraw.amount;
            }
            break;
         }
         case operation::tag<vesting_balance_create_operation>::value:
         {
            const auto& o = op.get<vesting_balance_create_operation>();
            if ((o.creator == account_id) && (o.amount.asset_id == asset_id))
               sum.debits += o.amount.amount;
            break;
         }
         case operation::tag<vesting_balance_withdraw_operation>::value:
         {
            const auto& o = op.get<vesting_balance_withdraw_operation>();
            FC_ASSERT(o.owner == account_id);
            if (o.amount.asset_id == asset_id)
               sum.credits += o.amount.amount;
            break;
         }
         case operation::tag<balance_claim_operation>::value:
         {
            const auto& o = op.get<balance_claim_operation>();
            FC_ASSERT(o.deposit_to_account == account_id);
            if (o.total_claimed.asset_id == asset_id)
               sum.credits += o.total_claimed.amount;
            break;
         }
         case operation::tag<override_transfer_operation>::value:
         {
            const auto& o = op.get<override_transfer_operation>();
            FC_ASSERT((o.issuer == account_id) || (o.from == account_id) || (o.to == account_id));
            if (o.amount.asset_id == asset_id) {
               if (o.from == account_id)
                  sum.debits += o.amount.amount;
               if (o.to == account_id)
                  sum.credits += o.amount.amount;
            }
            break;
         }
         case operation::tag<transfer_to_blind_operation>::value:
         {
            const auto& o = op.get<transfer_to_blind_operation>();
            if ((o.from == account_id) && (o.amount.asset_id == asset_id))
               sum.debits += o.amount.amount;
            break;
         }
         case operation::tag<transfer_from_blind_operation>::value:
         {
            const auto& o = op.get<transfer_from_blind_operation>();
            FC_ASSERT(o.to == account_id);
            if (o.amount.asset_id == asset_id)
               sum.credits += o.amount.amount;
            break;
         }
         case operation::tag<asset_settle_cancel_operation>::value: // VIRTUAL
         {
            const auto& o = op.get<asset_settle_cancel_operation>();
            FC_ASSERT(o.account == account_id);
            if (o.amount.asset_id == asset_id)
               sum.credits += o.amount.amount;
            break;
         }
         case operation::tag<asset_claim_fees_operation>::value:
         {
            const auto& o = op.get<asset_claim_fees_operation>();
            FC_ASSERT(o.issuer == account_id);
            if (o.amount_to_claim.asset_id == asset_id)
               sum.credits += o.amount_to_claim.amount;
            break;
         }
         case operation::tag<bid_collateral_operation>::value:
         {
            const auto& o = op.get<bid_collateral_operation>();
            FC_ASSERT(o.bidder == account_id);
            if (o.additional_collateral.asset_id == asset_id)
               sum.debits += o.additional_collateral.amount;
            break;
         }
         case operation::tag<execute_bid_operation>::value: // VIRTUAL
         {
            const auto& o = op.get<execute_bid_operation>();
            FC_ASSERT(o.bidder == account_id);
            if (o.debt.asset_id == asset_id)
               sum.credits += o.debt.amount;
            break;
         }
         case operation::tag<asset_claim_pool_operation>::value:
         {
            const auto& o = op.get<asset_claim_pool_operation>();
            FC_ASSERT(o.issuer == account_id);
            if (o.amount_to_claim.asset_id == asset_id)
               sum.credits += o.amount_to_claim.amount;
            break;
         }
         default: // These don't modify balances directly.
            // account_create_operation
            // account_update_operation
            // account_whitelist_operation
            // account_upgrade_operation
            // account_transfer_operation
            // asset_create_operation
            // asset_update_operation
            // asset_update_bitasset_operation
            // asset_update_feed_producers_operation
            // asset_global_settle_operation
            // asset_publish_feed_operation
            // witness_create_operation
            // witness_update_operation
            // proposal_create_operation
            // proposal_update_operation
            // proposal_delete_operation
            // withdraw_permission_create_operation
            // withdraw_permission_update_operation
            // withdraw_permission_delete_operation
            // committee_member_create_operation
            // committee_member_update_operation
            // committee_member_update_global_parameters_operation
            // worker_create_operation
            // custom_operation
            // assert_operation
            // blind_transfer_operation
            // fba_distribute_operation // VIRTUAL
            // asset_update_issuer_operation
            break;
      }
      sum.fees += op.visit(fee_retrieval_visitor(account_id, asset_id));
   }

    login_api::login_api(application& a)
    :_app(a)
    {
    }

    login_api::~login_api()
    {
    }

    bool login_api::login(const string& user, const string& password)
    {
       optional< api_access_info > acc = _app.get_api_access_info( user );
       if( !acc.valid() )
          return false;
       if( acc->password_hash_b64 != "*" )
       {
          std::string password_salt = fc::base64_decode( acc->password_salt_b64 );
          std::string acc_password_hash = fc::base64_decode( acc->password_hash_b64 );

          fc::sha256 hash_obj = fc::sha256::hash( password + password_salt );
          if( hash_obj.data_size() != acc_password_hash.length() )
             return false;
          if( memcmp( hash_obj.data(), acc_password_hash.c_str(), hash_obj.data_size() ) != 0 )
             return false;
       }

       for( const std::string& api_name : acc->allowed_apis )
          enable_api( api_name );
       return true;
    }

    void login_api::enable_api( const std::string& api_name )
    {
       if( api_name == "database_api" )
       {
          _database_api = std::make_shared< database_api >( std::ref( *_app.chain_database() ), &( _app.get_options() ) );
       }
       else if( api_name == "block_api" )
       {
          _block_api = std::make_shared< block_api >( std::ref( *_app.chain_database() ) );
       }
       else if( api_name == "network_broadcast_api" )
       {
          _network_broadcast_api = std::make_shared< network_broadcast_api >( std::ref( _app ) );
       }
       else if( api_name == "archive_api" )
       {
          _archive_api = std::make_shared< archive_api >( _app );
       }
       else if( api_name == "history_api" )
       {
          _history_api = std::make_shared< history_api >( _app );
       }
       else if( api_name == "network_node_api" )
       {
          _network_node_api = std::make_shared< network_node_api >( std::ref(_app) );
       }
       else if( api_name == "crypto_api" )
       {
          _crypto_api = std::make_shared< crypto_api >();
       }
       else if( api_name == "asset_api" )
       {
          _asset_api = std::make_shared< asset_api >( _app );
       }
       else if( api_name == "orders_api" )
       {
          _orders_api = std::make_shared< orders_api >( std::ref( _app ) );
       }
       else if( api_name == "debug_api" )
       {
          // can only enable this API if the plugin was loaded
          if( _app.get_plugin( "debug_witness" ) )
             _debug_api = std::make_shared< graphene::debug_witness::debug_api >( std::ref(_app) );
       }
       return;
    }

    // block_api
    block_api::block_api(graphene::chain::database& db) : _db(db) { }
    block_api::~block_api() { }

    vector<optional<signed_block>> block_api::get_blocks(uint32_t block_num_from, uint32_t block_num_to)const
    {
       FC_ASSERT( block_num_to >= block_num_from );
       vector<optional<signed_block>> res;
       for(uint32_t block_num=block_num_from; block_num<=block_num_to; block_num++) {
          res.push_back(_db.fetch_block_by_number(block_num));
       }
       return res;
    }

    network_broadcast_api::network_broadcast_api(application& a):_app(a)
    {
       _applied_block_connection = _app.chain_database()->applied_block.connect([this](const signed_block& b){ on_applied_block(b); });
    }

    void network_broadcast_api::on_applied_block( const signed_block& b )
    {
       if( _callbacks.size() )
       {
          /// we need to ensure the database_api is not deleted for the life of the async operation
          auto capture_this = shared_from_this();
          for( uint32_t trx_num = 0; trx_num < b.transactions.size(); ++trx_num )
          {
             const auto& trx = b.transactions[trx_num];
             auto id = trx.id();
             auto itr = _callbacks.find(id);
             if( itr != _callbacks.end() )
             {
                auto block_num = b.block_num();
                auto& callback = _callbacks.find(id)->second;
                auto v = fc::variant( transaction_confirmation{ id, block_num, trx_num, trx }, GRAPHENE_MAX_NESTED_OBJECTS );
                fc::async( [capture_this,v,callback]() {
                   callback(v);
                } );
             }
          }
       }
    }

    void network_broadcast_api::broadcast_transaction(const precomputable_transaction& trx)
    {
       _app.chain_database()->precompute_parallel( trx ).wait();
       _app.chain_database()->push_transaction(trx);
       if( _app.p2p_node() != nullptr )
          _app.p2p_node()->broadcast_transaction(trx);
    }

    fc::variant network_broadcast_api::broadcast_transaction_synchronous(const precomputable_transaction& trx)
    {
       fc::promise<fc::variant>::ptr prom( new fc::promise<fc::variant>() );
       broadcast_transaction_with_callback( [prom]( const fc::variant& v ){
        prom->set_value(v);
       }, trx );

       return fc::future<fc::variant>(prom).wait();
    }

    void network_broadcast_api::broadcast_block( const signed_block& b )
    {
       _app.chain_database()->precompute_parallel( b ).wait();
       _app.chain_database()->push_block(b);
       if( _app.p2p_node() != nullptr )
          _app.p2p_node()->broadcast( net::block_message( b ));
    }

    void network_broadcast_api::broadcast_transaction_with_callback(confirmation_callback cb, const precomputable_transaction& trx)
    {
       _app.chain_database()->precompute_parallel( trx ).wait();
       _callbacks[trx.id()] = cb;
       _app.chain_database()->push_transaction(trx);
       if( _app.p2p_node() != nullptr )
          _app.p2p_node()->broadcast_transaction(trx);
    }

    network_node_api::network_node_api( application& a ) : _app( a )
    {
    }

    fc::variant_object network_node_api::get_info() const
    {
       fc::mutable_variant_object result = _app.p2p_node()->network_get_info();
       result["connection_count"] = _app.p2p_node()->get_connection_count();
       return result;
    }

    void network_node_api::add_node(const fc::ip::endpoint& ep)
    {
       _app.p2p_node()->add_node(ep);
    }

    std::vector<net::peer_status> network_node_api::get_connected_peers() const
    {
       return _app.p2p_node()->get_connected_peers();
    }

    std::vector<net::potential_peer_record> network_node_api::get_potential_peers() const
    {
       return _app.p2p_node()->get_potential_peers();
    }

    fc::variant_object network_node_api::get_advanced_node_parameters() const
    {
       return _app.p2p_node()->get_advanced_node_parameters();
    }

    void network_node_api::set_advanced_node_parameters(const fc::variant_object& params)
    {
       return _app.p2p_node()->set_advanced_node_parameters(params);
    }

    fc::api<network_broadcast_api> login_api::network_broadcast()const
    {
       FC_ASSERT(_network_broadcast_api);
       return *_network_broadcast_api;
    }

    fc::api<block_api> login_api::block()const
    {
       FC_ASSERT(_block_api);
       return *_block_api;
    }

    fc::api<network_node_api> login_api::network_node()const
    {
       FC_ASSERT(_network_node_api);
       return *_network_node_api;
    }

    fc::api<database_api> login_api::database()const
    {
       FC_ASSERT(_database_api);
       return *_database_api;
    }

    fc::api<archive_api> login_api::archive()const
    {
       FC_ASSERT(_archive_api);
       return *_archive_api;
    }

    fc::api<history_api> login_api::history() const
    {
       FC_ASSERT(_history_api);
       return *_history_api;
    }

    fc::api<crypto_api> login_api::crypto() const
    {
       FC_ASSERT(_crypto_api);
       return *_crypto_api;
    }

    fc::api<asset_api> login_api::asset() const
    {
       FC_ASSERT(_asset_api);
       return *_asset_api;
    }

    fc::api<orders_api> login_api::orders() const
    {
       FC_ASSERT(_orders_api);
       return *_orders_api;
    }

    fc::api<graphene::debug_witness::debug_api> login_api::debug() const
    {
       FC_ASSERT(_debug_api);
       return *_debug_api;
    }

    vector<order_history_object> history_api::get_fill_order_history( std::string asset_a, std::string asset_b, uint32_t limit  )const
    {
       FC_ASSERT(_app.chain_database());
       const auto& db = *_app.chain_database();
       asset_id_type a = database_api.get_asset_id_from_string( asset_a );
       asset_id_type b = database_api.get_asset_id_from_string( asset_b );
       if( a > b ) std::swap(a,b);
       const auto& history_idx = db.get_index_type<graphene::market_history::history_index>().indices().get<by_key>();
       history_key hkey;
       hkey.base = a;
       hkey.quote = b;
       hkey.sequence = std::numeric_limits<int64_t>::min();

       uint32_t count = 0;
       auto itr = history_idx.lower_bound( hkey );
       vector<order_history_object> result;
       while( itr != history_idx.end() && count < limit)
       {
          if( itr->key.base != a || itr->key.quote != b ) break;
          result.push_back( *itr );
          ++itr;
          ++count;
       }

       return result;
    }

    vector<operation_history_object> history_api::get_account_history( const std::string account_id_or_name,
                                                                       operation_history_id_type stop,
                                                                       unsigned limit,
                                                                       operation_history_id_type start ) const
    {
       FC_ASSERT( _app.chain_database() );
       const auto& db = *_app.chain_database();
       uint64_t api_limit_get_account_history=_app.get_options().api_limit_get_account_history;
       FC_ASSERT( limit <= api_limit_get_account_history );
       vector<operation_history_object> result;
       account_id_type account;
       try {
          account = database_api.get_account_id_from_string(account_id_or_name);
          const account_transaction_history_object& node = account(db).statistics(db).most_recent_op(db);
          if(start == operation_history_id_type() || start.instance.value > node.operation_id.instance.value)
             start = node.operation_id;
       } catch(...) { return result; }

       const auto& hist_idx = db.get_index_type<account_transaction_history_index>();
       const auto& by_op_idx = hist_idx.indices().get<by_op>();
       auto index_start = by_op_idx.begin();
       auto itr = by_op_idx.lower_bound(boost::make_tuple(account, start));

       while(itr != index_start && itr->account == account && itr->operation_id.instance.value > stop.instance.value && result.size() < limit)
       {
          if(itr->operation_id.instance.value <= start.instance.value)
             result.push_back(itr->operation_id(db));
          --itr;
       }
       if(stop.instance.value == 0 && result.size() < limit && itr->account == account) {
         result.push_back(itr->operation_id(db));
       }

       return result;
    }

    vector<operation_history_object> history_api::get_account_history_operations( const std::string account_id_or_name,
                                                                       int operation_type,
                                                                       operation_history_id_type start,
                                                                       operation_history_id_type stop,
                                                                       unsigned limit) const
    {
       FC_ASSERT( _app.chain_database() );
       const auto& db = *_app.chain_database();
       uint64_t api_limit_get_account_history_operations=_app.get_options().api_limit_get_account_history_operations;
       FC_ASSERT(limit <= api_limit_get_account_history_operations);
       vector<operation_history_object> result;
       account_id_type account;
       try {
          account = database_api.get_account_id_from_string(account_id_or_name);
       } catch(...) { return result; }
       const auto& stats = account(db).statistics(db);
       if( stats.most_recent_op == account_transaction_history_id_type() ) return result;
       const account_transaction_history_object* node = &stats.most_recent_op(db);
       if( start == operation_history_id_type() )
          start = node->operation_id;

       while(node && node->operation_id.instance.value > stop.instance.value && result.size() < limit)
       {
          if( node->operation_id.instance.value <= start.instance.value ) {

             if(node->operation_id(db).op.which() == operation_type)
               result.push_back( node->operation_id(db) );
          }
          if( node->next == account_transaction_history_id_type() )
             node = nullptr;
          else node = &node->next(db);
       }
       if( stop.instance.value == 0 && result.size() < limit ) {
          auto head = db.find(account_transaction_history_id_type());
          if (head != nullptr && head->account == account && head->operation_id(db).op.which() == operation_type)
            result.push_back(head->operation_id(db));
       }
       return result;
    }


    vector<operation_history_object> history_api::get_relative_account_history( const std::string account_id_or_name,
                                                                                uint64_t stop,
                                                                                unsigned limit,
                                                                                uint64_t start) const
    {
       FC_ASSERT( _app.chain_database() );
       const auto& db = *_app.chain_database();
       uint64_t api_limit_get_relative_account_history=_app.get_options().api_limit_get_relative_account_history;
       FC_ASSERT(limit <= api_limit_get_relative_account_history);
       vector<operation_history_object> result;
       account_id_type account;
       try {
          account = database_api.get_account_id_from_string(account_id_or_name);
       } catch(...) { return result; }
       const auto& stats = account(db).statistics(db);
       if( start == 0 )
          start = stats.total_ops;
       else
          start = std::min( stats.total_ops, start );

       if( start >= stop && start > stats.removed_ops && limit > 0 )
       {
          const auto& hist_idx = db.get_index_type<account_transaction_history_index>();
          const auto& by_seq_idx = hist_idx.indices().get<by_seq>();

          auto itr = by_seq_idx.upper_bound( boost::make_tuple( account, start ) );
          auto itr_stop = by_seq_idx.lower_bound( boost::make_tuple( account, stop ) );

          do
          {
             --itr;
             result.push_back( itr->operation_id(db) );
          }
          while ( itr != itr_stop && result.size() < limit );
       }
       return result;
    }

    flat_set<uint32_t> history_api::get_market_history_buckets()const
    {
       auto hist = _app.get_plugin<market_history_plugin>( "market_history" );
       FC_ASSERT( hist );
       return hist->tracked_buckets();
    }

    history_operation_detail history_api::get_account_history_by_operations(const std::string account_id_or_name, vector<uint16_t> operation_types, uint32_t start, unsigned limit)
    {
       uint64_t api_limit_get_account_history_by_operations=_app.get_options().api_limit_get_account_history_by_operations;
       FC_ASSERT(limit <= api_limit_get_account_history_by_operations);
       history_operation_detail result;
       vector<operation_history_object> objs = get_relative_account_history(account_id_or_name, start, limit, limit + start - 1);
       std::for_each(objs.begin(), objs.end(), [&](const operation_history_object &o) {
                    if (operation_types.empty() || find(operation_types.begin(), operation_types.end(), o.op.which()) != operation_types.end()) {
                        result.operation_history_objs.push_back(o);
                     }
                 });

        result.total_count = objs.size();
        return result;
    }

    vector<bucket_object> history_api::get_market_history( std::string asset_a, std::string asset_b,
                                                           uint32_t bucket_seconds, fc::time_point_sec start, fc::time_point_sec end )const
    { try {
       FC_ASSERT(_app.chain_database());
       const auto& db = *_app.chain_database();
       asset_id_type a = database_api.get_asset_id_from_string( asset_a );
       asset_id_type b = database_api.get_asset_id_from_string( asset_b );
       vector<bucket_object> result;
       result.reserve(200);

       if( a > b ) std::swap(a,b);

       const auto& bidx = db.get_index_type<bucket_index>();
       const auto& by_key_idx = bidx.indices().get<by_key>();

       auto itr = by_key_idx.lower_bound( bucket_key( a, b, bucket_seconds, start ) );
       while( itr != by_key_idx.end() && itr->key.open <= end && result.size() < 200 )
       {
          if( !(itr->key.base == a && itr->key.quote == b && itr->key.seconds == bucket_seconds) )
          {
            return result;
          }
          result.push_back(*itr);
          ++itr;
       }
       return result;
    } FC_CAPTURE_AND_RETHROW( (asset_a)(asset_b)(bucket_seconds)(start)(end) ) }

    vector<operation_history_object> history_api::get_last_operations_history(unsigned limit) const
    {
        FC_ASSERT(limit <= OperationHistoryObjectsLimit);
        FC_ASSERT(_app.chain_database());

        const auto& db = *_app.chain_database();
        const auto& history_by_id_idx = db.get_index_type<operation_history_index>().indices().get<by_id>();

        vector<operation_history_object> result;
        result.reserve(limit);

        // copy the operation history elements in reverse order from newest to oldest
        auto itr = history_by_id_idx.rbegin();
        auto itr_stop = history_by_id_idx.rend();

        while ((itr != itr_stop) && (result.size() < limit))
        {
            result.push_back( *itr );
            ++itr;
        }

        return result;
    }

    crypto_api::crypto_api(){};

    commitment_type crypto_api::blind( const blind_factor_type& blind, uint64_t value )
    {
       return fc::ecc::blind( blind, value );
    }

    blind_factor_type crypto_api::blind_sum( const std::vector<blind_factor_type>& blinds_in, uint32_t non_neg )
    {
       return fc::ecc::blind_sum( blinds_in, non_neg );
    }

    bool crypto_api::verify_sum( const std::vector<commitment_type>& commits_in, const std::vector<commitment_type>& neg_commits_in, int64_t excess )
    {
       return fc::ecc::verify_sum( commits_in, neg_commits_in, excess );
    }

    verify_range_result crypto_api::verify_range( const commitment_type& commit, const std::vector<char>& proof )
    {
       verify_range_result result;
       result.success = fc::ecc::verify_range( result.min_val, result.max_val, commit, proof );
       return result;
    }

    std::vector<char> crypto_api::range_proof_sign( uint64_t min_value,
                                                    const commitment_type& commit,
                                                    const blind_factor_type& commit_blind,
                                                    const blind_factor_type& nonce,
                                                    int8_t base10_exp,
                                                    uint8_t min_bits,
                                                    uint64_t actual_value )
    {
       return fc::ecc::range_proof_sign( min_value, commit, commit_blind, nonce, base10_exp, min_bits, actual_value );
    }

    verify_range_proof_rewind_result crypto_api::verify_range_proof_rewind( const blind_factor_type& nonce,
                                                                            const commitment_type& commit,
                                                                            const std::vector<char>& proof )
    {
       verify_range_proof_rewind_result result;
       result.success = fc::ecc::verify_range_proof_rewind( result.blind_out,
                                                            result.value_out,
                                                            result.message_out,
                                                            nonce,
                                                            result.min_val,
                                                            result.max_val,
                                                            const_cast< commitment_type& >( commit ),
                                                            proof );
       return result;
    }

    range_proof_info crypto_api::range_get_info( const std::vector<char>& proof )
    {
       return fc::ecc::range_get_info( proof );
    }

    // asset_api
    asset_api::asset_api(graphene::app::application& app) :
          _app(app),
          _db( *app.chain_database()),
          database_api( std::ref(*app.chain_database()), &(app.get_options())
          ) { }
    asset_api::~asset_api() { }

    vector<account_asset_balance> asset_api::get_asset_holders( std::string asset, uint32_t start, uint32_t limit ) const {
       uint64_t api_limit_get_asset_holders=_app.get_options().api_limit_get_asset_holders;
       FC_ASSERT(limit <= api_limit_get_asset_holders);
       asset_id_type asset_id = database_api.get_asset_id_from_string( asset );
       const auto& bal_idx = _db.get_index_type< account_balance_index >().indices().get< by_asset_balance >();
       auto range = bal_idx.equal_range( boost::make_tuple( asset_id ) );

       vector<account_asset_balance> result;

       uint32_t index = 0;
       for( const account_balance_object& bal : boost::make_iterator_range( range.first, range.second ) )
       {
          if( result.size() >= limit )
             break;

          if( bal.balance.value == 0 )
             continue;

          if( index++ < start )
             continue;

          const auto account = _db.find(bal.owner);

          account_asset_balance aab;
          aab.name       = account->name;
          aab.account_id = account->id;
          aab.amount     = bal.balance.value;

          result.push_back(aab);
       }

       return result;
    }
    // get number of asset holders.
    int asset_api::get_asset_holders_count( std::string asset ) const {
       const auto& bal_idx = _db.get_index_type< account_balance_index >().indices().get< by_asset_balance >();
       asset_id_type asset_id = database_api.get_asset_id_from_string( asset );
       auto range = bal_idx.equal_range( boost::make_tuple( asset_id ) );

       int count = boost::distance(range) - 1;

       return count;
    }
    // function to get vector of system assets with holders count.
    vector<asset_holders> asset_api::get_all_asset_holders() const {
       vector<asset_holders> result;
       vector<asset_id_type> total_assets;
       for( const asset_object& asset_obj : _db.get_index_type<asset_index>().indices() )
       {
          const auto& dasset_obj = asset_obj.dynamic_asset_data_id(_db);

          asset_id_type asset_id;
          asset_id = dasset_obj.id;

          const auto& bal_idx = _db.get_index_type< account_balance_index >().indices().get< by_asset_balance >();
          auto range = bal_idx.equal_range( boost::make_tuple( asset_id ) );

          int count = boost::distance(range) - 1;

          asset_holders ah;
          ah.asset_id       = asset_id;
          ah.count     = count;

          result.push_back(ah);
       }

       return result;
    }

   // orders_api
   flat_set<uint16_t> orders_api::get_tracked_groups()const
   {
      auto plugin = _app.get_plugin<grouped_orders_plugin>( "grouped_orders" );
      FC_ASSERT( plugin );
      return plugin->tracked_groups();
   }

   vector< limit_order_group > orders_api::get_grouped_limit_orders( std::string base_asset,
                                                               std::string quote_asset,
                                                               uint16_t group,
                                                               optional<price> start,
                                                               uint32_t limit )const
   {
      uint64_t api_limit_get_grouped_limit_orders=_app.get_options().api_limit_get_grouped_limit_orders;
      FC_ASSERT( limit <= api_limit_get_grouped_limit_orders );
      auto plugin = _app.get_plugin<graphene::grouped_orders::grouped_orders_plugin>( "grouped_orders" );
      FC_ASSERT( plugin );
      const auto& limit_groups = plugin->limit_order_groups();
      vector< limit_order_group > result;

      asset_id_type base_asset_id = database_api.get_asset_id_from_string( base_asset );
      asset_id_type quote_asset_id = database_api.get_asset_id_from_string( quote_asset );

      price max_price = price::max( base_asset_id, quote_asset_id );
      price min_price = price::min( base_asset_id, quote_asset_id );
      if( start.valid() && !start->is_null() )
         max_price = std::max( std::min( max_price, *start ), min_price );

      auto itr = limit_groups.lower_bound( limit_order_group_key( group, max_price ) );
      // use an end itrator to try to avoid expensive price comparison
      auto end = limit_groups.upper_bound( limit_order_group_key( group, min_price ) );
      while( itr != end && result.size() < limit )
      {
         result.emplace_back( *itr );
         ++itr;
      }
      return result;
   }

} } // graphene::app
