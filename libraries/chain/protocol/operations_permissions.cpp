#include <algorithm>
#include <graphene/chain/database.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/protocol/operations_permissions.hpp>
#include <fc/smart_ref_impl.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/worker_object.hpp>

namespace graphene { namespace chain {

   typedef fc::smart_ref<operations_permissions> smart_operations_permissions;

   static smart_operations_permissions smart_operations_permissions_tmp;

   bool account_has_comittee(const database& db, const account_id_type& account)
   {
      const auto& idx = db.get_index_type<committee_member_index>().indices().get<by_account>();
      
      return idx.find(account) != idx.end();
   }

   bool account_has_witness(const database& db, const account_id_type& account)
   {
      const auto& idx = db.get_index_type<witness_index>().indices().get<by_account>();

      return idx.find(account) != idx.end();
   }

   bool account_has_workers(const database& db, const account_id_type& account)
   {
       vector<optional<worker_object>> result;
       const auto& workers_idx = db.get_index_type<worker_index>().indices().get<by_account>();

       for (const auto& w : workers_idx)
       {
           if (w.worker_account == account)
           {
               return true;
           }
       }

       return false;
   }

   struct is_allowed_visitor
   {
      typedef bool result_type;

      const operations_permissions& param;
      const int current_op;
      transaction_evaluation_state* trx_state;

      is_allowed_visitor( const operations_permissions& p, const operation& op, transaction_evaluation_state* trx_state ):param(p), current_op(op.which()), trx_state(trx_state){}

      template<typename OpType>
      result_type operator()(const OpType& op )const
      {
         try
         {
            optional<account_object> account;
            auto account_id = op.fee_payer();
            auto rules = param.get<OpType>().rules;

            for (const auto& rule : rules)
            {
               // generic rule => match always if no matching condition is defined
               if (!rule.account_name.valid() &&
                   !rule.object_type.valid() &&
                   !rule.object_id_type.valid())
               {
                  return rule.allowed;
               }

               // try to match by object type (witness, comittee, worker)
               if (rule.object_type.valid() &&
                  protocol_ids == account->space_id)
               {
                  switch (*rule.object_type)
                  {
                     case committee_member_object_type:
                        {                           
                           if (account_has_comittee(trx_state->db(), account_id))
                           {
                              return rule.allowed;
                           }

                           break;
                        }
                     case witness_object_type:
                        {
                           if (account_has_witness(trx_state->db(), account_id))
                           {
                              return rule.allowed; 
                           }

                           break;
                        }
                     case worker_object_type:
                        {
                           if (account_has_workers(trx_state->db(), account_id))
                           {
                              return rule.allowed;
                           }

                           break;
                        }
                     default: return false;
                  }
               }

               // try to match by account object id
               if(rule.object_id_type.valid() &&
                  rule.object_id_type == account_id)
               {
                  return rule.allowed;
               }

               // lazy retrieval of account object
               if (!account.valid())
               {
                  account = account_id(trx_state->db());
               }

               // try to match by account name
               if(rule.account_name.valid() &&
                  rule.account_name == account->name)
               {
                  return rule.allowed;
               }
            }

            return false;
         }
         catch (const fc::assert_exception& e)
         {
            return false;
         }
      }
   };

   bool operations_permissions::is_allowed(transaction_evaluation_state* trx_state, const operation& op) const
   {
      return op.visit( is_allowed_visitor( *this, op, trx_state ) );
   }

   struct enable_operation
   {
      typedef void result_type;

      template<typename ParamType>
      result_type operator()(  ParamType& op )const
      {
         // clear previous rules to avoid duplicates, if enable_operation is called multiple times
         op.rules.clear();

         // add default operation enabled rule
         op.rules.emplace_back(operation_permission_type());
      }
   };

   operations_permissions operations_permissions::get_default()
   {
      operations_permissions result;
      for(auto i = 0; i < operations_permissions_parameters().count(); ++i )
      {
         operations_permissions_parameters x;
         x.set_which(i);
         x.visit(enable_operation());
         result.parameters.insert(x);
      }

      return result;
   }

   struct operations_permissions_validate_visitor
   {
      typedef void result_type;

      template<typename T>
      void operator()( const T& op )const
      {
         for(const auto& rule : op.rules)
         {
            auto rules_count = rule.account_name.valid() + rule.object_type.valid() + rule.object_id_type.valid();

            // number of defined rules must be at most one or zero
            FC_ASSERT(rules_count <= 1);
         }
      }
   };

   void operations_permissions::validate()const
   {
      for( const auto& operation_permissions : parameters )
      {
         operation_permissions.visit( operations_permissions_validate_visitor() );
      }
   }

   void operations_permissions::enable_all_operations()
   {
      for(auto& operation_permissions : parameters )
      {
         operation_permissions.visit( enable_operation() );
      }
   }
} } // graphene::chain
