/*
 * Copyright (c) 2018 jmjatlanta and contributors.
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
#include <graphene/chain/protocol/htlc.hpp>

#define SECONDS_PER_DAY (60 * 60 * 24)

namespace graphene { namespace chain {

   void htlc_create_operation::validate()const {
      FC_ASSERT( fee.amount >= 0 );
      FC_ASSERT( amount.amount > 0 );
      FC_ASSERT( hash_type == htlc_hash_algorithm::ripemd160
            || hash_type == htlc_hash_algorithm::sha1
            || hash_type == htlc_hash_algorithm::sha256, "Unknown Hash Algorithm");
   }

   share_type htlc_create_operation::calculate_fee(const fee_parameters_type& fee_params)const
   {
      uint64_t days = (claim_period_seconds + SECONDS_PER_DAY - 1) / SECONDS_PER_DAY;
      return fee_params.fee + (fee_params.fee_per_day * days);
   }

   void htlc_redeem_operation::validate()const {
      FC_ASSERT( fee.amount >= 0 );
   }

   share_type htlc_redeem_operation::calculate_fee(const fee_parameters_type& fee_params)const
   {
      return fee_params.fee
             + ( preimage.size() + 1023 ) / 1024 * fee_params.fee_per_kb;
   }

   void htlc_extend_operation::validate()const {
      FC_ASSERT( fee.amount >= 0 );
   }

   share_type htlc_extend_operation::calculate_fee(const fee_parameters_type& fee_params)const
   {
      uint32_t days = (seconds_to_add + SECONDS_PER_DAY - 1) / SECONDS_PER_DAY;
      return fee_params.fee + (fee_params.fee_per_day * days);
   }
} }
