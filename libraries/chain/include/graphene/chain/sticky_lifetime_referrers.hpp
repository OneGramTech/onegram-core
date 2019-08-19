/**
 * @brief Declares the sticky lifetime referrers class
 */
#pragma once

#include <graphene/protocol/types.hpp>
#include <fc/reflect/reflect.hpp>
#include <string>
#include <set>

namespace graphene
{
   namespace chain
   {
      struct sticky_lifetime_referrers_type
      {
         typedef std::string referrers_account_name_type;
         typedef std::set<referrers_account_name_type> referrer_names_type;

         referrer_names_type referrer_names;

         bool contains(const referrers_account_name_type& name) const
         {
            return !(referrer_names.find(name) == referrer_names.end());
         }
      };

      struct sticky_lifetime_referrers_ids_type
      {
         typedef std::set<account_id_type> referrer_ids_type;

         referrer_ids_type referrer_ids;
      };

   }
} // graphene::chain

FC_REFLECT(graphene::chain::sticky_lifetime_referrers_type, (referrer_names))
FC_REFLECT(graphene::chain::sticky_lifetime_referrers_ids_type, (referrer_ids))