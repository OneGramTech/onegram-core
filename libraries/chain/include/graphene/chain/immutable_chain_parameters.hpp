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

#include <fc/reflect/reflect.hpp>

#include <cstdint>

#include <graphene/chain/config.hpp>
#include <graphene/chain/feeless_accounts.hpp>
#include <graphene/chain/eternal_committee_members.hpp>
#include  <graphene/chain/sticky_lifetime_referrers.hpp>

namespace graphene { namespace chain {

struct immutable_chain_parameters
{
   uint16_t min_committee_member_count = GRAPHENE_DEFAULT_MIN_COMMITTEE_MEMBER_COUNT;
   uint16_t min_witness_count = GRAPHENE_DEFAULT_MIN_WITNESS_COUNT;
   uint32_t num_special_accounts = 0;
   uint32_t num_special_assets = 0;

   feeless_accounts_type feeless_accounts;
   eternal_committee_accounts_type eternal_committee_members;
   sticky_lifetime_referrers_type sticky_lifetime_referrers;
   optional<string> enforced_lifetime_referrer;
};

} } // graphene::chain

FC_REFLECT( graphene::chain::immutable_chain_parameters,
   (min_committee_member_count)
   (min_witness_count)
   (num_special_accounts)
   (num_special_assets)
   (feeless_accounts)
   (eternal_committee_members)
   (sticky_lifetime_referrers)
   (enforced_lifetime_referrer)
)
