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
#pragma once

#include <graphene/chain/immutable_chain_parameters.hpp>
#include <graphene/chain/feeless_accounts.hpp>
#include <graphene/chain/sticky_lifetime_referrers.hpp>

namespace graphene { namespace chain {

/**
 * Contains invariants which are set at genesis and never changed.
 */
class chain_property_object : public abstract_object<chain_property_object>
{
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_chain_property_object_type;

      chain_id_type chain_id;
      immutable_chain_parameters immutable_parameters;

      // contains ids of immutable_parameters.feeless_accounts.account_names for fast search
      feeless_account_ids_type feeless_accounts;
      const feeless_account_ids_type::account_ids_type& feeless_account_ids() const { return feeless_accounts.account_ids; }

      // contains ids of immutable_parameters.eternal_committee_accounts.account_names for fast search
      eternal_committee_account_ids_type eternal_committee_accounts;
      const eternal_committee_account_ids_type::vote_ids_type& eternal_committee_account_ids() const { return eternal_committee_accounts.vote_ids; }

      /**
       * @brief The sticky lifetime referers
       *
       * List of accounts that "stick" as a lifetime referrer even when somebody upgrades his account to LTM.
       */
      sticky_lifetime_referrers_ids_type sticky_lifetime_referers;
      const sticky_lifetime_referrers_ids_type::referrer_ids_type& sticky_lifetime_referers_ids() const
      {
         return sticky_lifetime_referers.referrer_ids;
      }

      optional<account_id_type> enforced_lifetime_referrer;
};

} }

MAP_OBJECT_ID_TO_TYPE(graphene::chain::chain_property_object)

FC_REFLECT_TYPENAME( graphene::chain::chain_property_object )

GRAPHENE_DECLARE_EXTERNAL_SERIALIZATION( graphene::chain::chain_property_object )
