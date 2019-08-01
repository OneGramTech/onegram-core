//
// Created by Lubos Ilcik on 19/02/2018.
//

#pragma once

#include <graphene/protocol/types.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/optional.hpp>
#include <string>
#include <set>

namespace graphene { namespace chain {

using graphene::protocol::account_id_type;

struct feeless_accounts_type {
   typedef std::string account_name_type;
   typedef std::set<account_name_type> account_names_type;

   account_names_type account_names;

   bool contains(account_name_type name) const {
      return !(account_names.find(name) == account_names.end());
   }
};

struct feeless_account_ids_type {
   typedef std::set<account_id_type> account_ids_type;

   account_ids_type account_ids;
};

}} // graphene::chain

FC_REFLECT(graphene::chain::feeless_accounts_type, (account_names))
FC_REFLECT(graphene::chain::feeless_account_ids_type, (account_ids))
