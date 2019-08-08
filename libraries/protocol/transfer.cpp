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
#include <graphene/protocol/transfer.hpp>

#include <fc/io/raw.hpp>

namespace graphene { namespace protocol {

share_type transfer_operation::calculate_fee( const fee_parameters_type& schedule )const
{
   share_type core_fee_required = calculate_flat_or_percentage_fee(schedule);
   if( memo )
      core_fee_required += calculate_data_fee( fc::raw::pack_size(memo), schedule.price_per_kbyte );
   return core_fee_required;
}

/**
 * Calculates fee from amount and fee_parameters_type
 * <p>
 * - if fee_parameters_type::percentage and/or amount = 0, returns fee_parameters_type::fee
 * <p>
 * - else returns max( min(core_fee_required, max_fee), min_fee ), where core_fee_required is percentage of amount
 * @param schedule
 * @return fee value
 */
share_type transfer_operation::calculate_flat_or_percentage_fee(const fee_parameters_type &schedule) const {
   if (schedule.percentage == 0) {
      return schedule.fee;
   }
   auto core_fee_amount = fc::uint128(amount.amount.value);
   core_fee_amount *= schedule.percentage;
   core_fee_amount /= GRAPHENE_100_PERCENT;
   auto core_fee_required = core_fee_amount.to_uint64();
   auto min_fee = schedule.fee;
   auto max_fee = schedule.percentage_max_fee;
   /// max( min(core_fee_required, max_fee), min_fee )
   if( core_fee_required > max_fee ) core_fee_required = max_fee;
   if( core_fee_required < min_fee ) core_fee_required = min_fee;
   return core_fee_required;
}


void transfer_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( from != to );
   FC_ASSERT( amount.amount > 0 );
}



share_type override_transfer_operation::calculate_fee( const fee_parameters_type& schedule )const
{
   share_type core_fee_required = schedule.fee;
   if( memo )
      core_fee_required += calculate_data_fee( fc::raw::pack_size(memo), schedule.price_per_kbyte );
   return core_fee_required;
}


void override_transfer_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( from != to );
   FC_ASSERT( amount.amount > 0 );
   FC_ASSERT( issuer != from );
}

} } // graphene::protocol

GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::transfer_operation::fee_parameters_type )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::override_transfer_operation::fee_parameters_type )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::transfer_operation )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::override_transfer_operation )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::transfer_operation::operation_permissions_type )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::override_transfer_operation::operation_permissions_type )