/*
 * Copyright (c) 2018 01 People, s.r.o. (01 CryptoHouse).
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

#include <graphene/app/plugin.hpp>
#include <graphene/chain/database.hpp>

#include <graphene/chain/operation_archive_object.hpp>
#include <graphene/account_archive/operation_database.hpp>

#include <fc/thread/future.hpp>

namespace graphene { namespace account_archive {
   using namespace chain;

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various template automagic depends on them being known at compile
// time.
//
#ifndef ACCOUNT_ARCHIVE_SPACE_ID
#define ACCOUNT_ARCHIVE_SPACE_ID 7 
#endif

namespace detail
{
   class account_archive_plugin_impl;
}

   class account_archive_plugin : public graphene::app::plugin
   {
      public:
         account_archive_plugin();
         ~account_archive_plugin();

         std::string plugin_name() const override;
         void plugin_initialize(const boost::program_options::variables_map& options) override;
         void plugin_startup() override;
         void plugin_set_program_options(
            boost::program_options::options_description& cli,
            boost::program_options::options_description& cfg) override;

         operation_history_object load(uint32_t index) const;

         friend class detail::account_archive_plugin_impl;
         std::unique_ptr<detail::account_archive_plugin_impl> impl;
   };

} } // graphene::account_archive
