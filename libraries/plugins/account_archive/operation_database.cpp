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

#include <graphene/chain/database.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/account_archive/operation_database.hpp>

#include <fc/io/raw.hpp>

namespace graphene { namespace account_archive {

struct index_entry
{
    uint64_t offset;
    uint32_t nbytes;
    uint32_t block_num;

    inline bool operator==(const index_entry& e)
    {
        return offset == e.offset
            && nbytes == e.nbytes
            && block_num == e.block_num;
    }
};

const struct index_entry stop_entry =
{
    0xffffffffffffffff,
    0xf0f0f0f0,
    0xffff0000,
};

operation_database::operation_database()
{
    _next_index = 0xffffffff;
    _last_block_num = 0xffffffff;
}

operation_database::~operation_database()
{
    if (is_open())
        close();
}

void operation_database::open(const fc::path& dir)
{ try {
    const auto dbdir = dir / "operation_database";
    const auto indices_path = dbdir / "indices";
    const auto operations_path = dbdir / "operations";
    const auto exception_flags = std::ios_base::failbit | std::ios_base::badbit;
    const auto filestream_flags = std::fstream::binary | std::fstream::in | std::fstream::out;

    fc::create_directories(dbdir);
    _indices.exceptions(exception_flags);
    _operations.exceptions(exception_flags);

    if (!fc::exists(indices_path)) {
        _indices.open(indices_path.generic_string().c_str(), filestream_flags | std::fstream::trunc);
        _operations.open(operations_path.generic_string().c_str(), filestream_flags | std::fstream::trunc);
    } else {
        _indices.open(indices_path.generic_string().c_str(), filestream_flags);
        _operations.open(operations_path.generic_string().c_str(), filestream_flags);
    }

    load_index();
} FC_CAPTURE_AND_RETHROW((dir)) }

void operation_database::close()
{
    _indices.close();
    _operations.close();
    operation_database();
}

void operation_database::flush()
{
    _indices.flush();
    _operations.flush();
}

void operation_database::wipe(const fc::path& dir)
{
    if (is_open())
        close();
    fc::remove_all(dir / "operation_database");
}

operation_history_object operation_database::load(uint32_t index) const
{
    _indices.seekg(0, _indices.end);
    const auto indices_toldg = static_cast<size_t>(_indices.tellg());
    const auto offset = index * sizeof(index_entry);
    if ((index >= _next_index) || (indices_toldg < offset + sizeof(index_entry))) {
        FC_THROW_EXCEPTION(fc::key_not_found_exception, "Virtual operations database does not hold index ${index}", ("index",index));
    }

    index_entry e;
    _indices.seekg(offset);
    _indices.read((char*)&e, sizeof(e));

    if (e == stop_entry || !e.nbytes) {
        FC_THROW_EXCEPTION(fc::key_not_found_exception, "Virtual operations database does not contain correct data at index ${index}", ("index",index));
    }

    _operations.seekg(0, _operations.end);
    const auto operations_toldg = static_cast<uint64_t>(_operations.tellg());
    if (operations_toldg < e.offset + e.nbytes) {
        FC_ASSERT(operations_toldg >= e.offset + e.nbytes);
        FC_THROW_EXCEPTION(fc::key_not_found_exception, "Virtual operations database does not hold the operation indexed ${index}", ("index", index));
    }

    vector<char> data(e.nbytes);
    _operations.seekg(e.offset);
    _operations.read(data.data(), data.size());
    return fc::raw::unpack<operation_history_object>(data);
}

uint32_t operation_database::store(const operation_history_object& oho)
{
    FC_ASSERT(oho.block_num >= _last_block_num);

    _indices.seekp(0, _indices.end);
    _operations.seekp(0, _operations.end);

    const auto packed = fc::raw::pack(oho);
    index_entry e = {};

    e.offset = _operations.tellp();
    e.nbytes = packed.size();
    e.block_num = oho.block_num;

    _indices.write((const char*)&e, sizeof(e));
    _operations.write(packed.data(), packed.size());

    _last_block_num = oho.block_num;
    return _next_index++;
}

void operation_database::truncate(uint32_t block_num)
{
    // The standard C++ libraries do not support truncating
    // a file before the C++17 standard. While the Boost does
    // provide such feature, the truncated data will be small
    // and the databases will grow further anyway. From both
    // the implementation and the performance perspectives,
    // it is a better solution to just invalidate the tail.

    FC_ASSERT(is_open());

    auto i = _next_index;
    while (i) {
        const auto b = get_block_num(--i);
        if (b < block_num) {
            _next_index = ++i;
            break;
        } else {
            _indices.seekp(i * sizeof(index_entry));
            _indices.write((const char*)&stop_entry, sizeof(stop_entry));
        }
    }
    load_index();
}

bool operation_database::is_open() const
{
    return _indices.is_open() && _operations.is_open();
}

void operation_database::load_index()
{
    // Find last valid index_entry stored in the indices file.

    FC_ASSERT(is_open());

    _next_index = 0;
    _last_block_num = 0;

    _indices.seekg(0, _indices.end);
    const uint32_t toldg = static_cast<uint32_t>(_indices.tellg());
    _next_index = toldg / sizeof(index_entry);
    FC_ASSERT(toldg % sizeof(index_entry) == 0);

    if (_next_index) {
        while (_next_index--) {
            index_entry e;
            _indices.seekg(_next_index * sizeof(index_entry));
            _indices.read((char*)&e, sizeof(e));
            if (!(e == stop_entry)) {
                _next_index++;
                break;
            }
        }
    }
}

uint32_t operation_database::get_block_num(uint32_t index)
{
    index_entry e;
    _indices.seekg(index * sizeof(index_entry));
    _indices.read((char*)&e, sizeof(e));
    return e.block_num;
}

} } // graphene::account_archive

FC_REFLECT( graphene::account_archive::index_entry, (offset)(nbytes)(block_num) );
