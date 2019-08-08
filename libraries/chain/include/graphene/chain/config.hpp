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

#include <graphene/protocol/config.hpp>

#define GRAPHENE_MIN_UNDO_HISTORY 10
#define GRAPHENE_MAX_UNDO_HISTORY 10000

#define GRAPHENE_MAX_NESTED_OBJECTS (200)

#define GRAPHENE_CURRENT_DB_VERSION                          "OGC1.14"

#define GRAPHENE_RECENTLY_MISSED_COUNT_INCREMENT             4
#define GRAPHENE_RECENTLY_MISSED_COUNT_DECREMENT             3


/**
 *  User facing branding strings
 */
///@{
#define GRAPHENE_CHAIN_IDENTIFIER_TXT "OneGram Chain Identifier"
#define GRAPHENE_DELAYED_NODE_TXT "OneGram Delayed Node"
#define GRAPHENE_EMPTY_BLOCKS_TXT "OneGram empty blocks"
#define GRAPHENE_WITNESS_NODE_TXT "OneGram Witness Node"
#define GRAPHENE_NEW_CHAIN_BANNER_TXT \
"********************************\n"\
"*                              *\n"\
"*   ------- NEW CHAIN ------   *\n"\
"*   - Welcome to OneGram!  -   *\n"\
"*   ------------------------   *\n"\
"*                              *\n"\
"********************************\n"
#define GRAPHENE_NODE_NAME_TXT "OneGram"
#define GRAPHENE_P2P_NODE_NAME_TXT "OneGram Reference Implementation"
///@}
