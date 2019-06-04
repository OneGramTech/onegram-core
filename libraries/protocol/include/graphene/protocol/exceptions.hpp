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

#include <fc/exception/exception.hpp>

#define GRAPHENE_ASSERT( expr, exc_type, FORMAT, ... )                \
   FC_MULTILINE_MACRO_BEGIN                                           \
   if( !(expr) )                                                      \
      FC_THROW_EXCEPTION( exc_type, FORMAT, __VA_ARGS__ );            \
   FC_MULTILINE_MACRO_END

namespace graphene { namespace protocol {

   FC_DECLARE_EXCEPTION( protocol_exception, 4000000 )

   FC_DECLARE_DERIVED_EXCEPTION( transaction_exception,      protocol_exception, 4010000 )

   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_active_auth,     transaction_exception, 4010001 )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_owner_auth,      transaction_exception, 4010002 )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_other_auth,      transaction_exception, 4010003 )
   FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_sig,          transaction_exception, 4010004 )
   FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate_sig,           transaction_exception, 4010005 )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_committee_approval, transaction_exception, 4010006 )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_fee,           transaction_exception, 4010007 )

} } // graphene::protocol
