/*
 * Copyright (c) 2019 01 People, s.r.o. (01 CryptoHouse).
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

#include <fstream>
#include <fc/filesystem.hpp>

#include <graphene/chain/protocol/operations.hpp>

namespace graphene { namespace account_archive {
    using namespace chain;

    class operation_database
    {
        public:

        operation_database();
        ~operation_database();

        void open(const fc::path& dir);
        void close();
        void flush();
        void wipe(const fc::path& dir);

        operation_history_object load(uint32_t index) const;
        uint32_t                 store(const operation_history_object& oho);
        void                     truncate(uint32_t block_num);
        uint32_t                 get_stored_operation_count() const { return _next_index; }

        private:

        mutable std::fstream _indices;
        mutable std::fstream _operations;

        uint32_t _next_index;
        uint32_t _last_block_num;

        bool     is_open() const;
        void     load_index();
        uint32_t get_block_num(uint32_t index);
    };

} } // graphene::account_archive
