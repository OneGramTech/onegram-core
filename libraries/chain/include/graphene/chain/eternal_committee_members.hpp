/**
 * @brief Declares the eternal committee members class
 */
#pragma once

#include <graphene/protocol/types.hpp>
#include <graphene/protocol/vote.hpp>
#include <fc/reflect/reflect.hpp>
#include <string>
#include <set>

namespace graphene
{
   namespace chain
   {

      struct eternal_committee_accounts_type
      {
         typedef std::string account_name_type;
         typedef std::set<account_name_type> account_names_type;

         account_names_type account_names;
      };

      struct eternal_committee_account_ids_type
      {
         typedef std::set<vote_id_type> vote_ids_type;

         vote_ids_type vote_ids;
      };

   }
} // graphene::chain

FC_REFLECT(graphene::chain::eternal_committee_accounts_type, (account_names))
FC_REFLECT(graphene::chain::eternal_committee_account_ids_type, (vote_ids))