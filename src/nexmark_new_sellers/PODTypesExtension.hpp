/**
 * Copyright (c) 2020 University of Luxembourg. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be
 * used to endorse or promote products derived from this software without specific prior
 * written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY OF LUXEMBOURG AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE UNIVERSITY OF LUXEMBOURG OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

/*
 * PODTypesExtension.hpp
 *
 *  Created on: July 22, 2020
 *      Author: damien.tassetti
 */

#pragma once
#define NAME_SIZE 17 + 1 + 16 + 1

namespace nexmark_new_sellers {
/**
 * This struct is based on the Person struct
 * (see src/nexmark_gen/PODTypes.hpp) but has
 * less attributes. Juts enough to be used in
 * nexmark query 8.
 * */
typedef struct {
    int id;
    char name[NAME_SIZE];

} PersonSelection;

/**
 * This struct is based on the Auction struct
 * (see src/nexmark_gen/PODTypes.hpp) but has
 * less attributes. Juts enough to be used in
 * nexmark query 8.
 * */
typedef struct {
    int seller_id;
    int id;
    int reserve_price;

} AuctionSelection;

/**
 * This enum serves to identify the join input type
 * (can be either Auction type or Person type depending
 * on which thread it comes from) by adding this
 * information to the header of the input messages.
 * */
enum JoinInputType {AuctionSelectionType, PersonSelectionType};

/**
 * This struct serves to encode and decode
 * easily the output of the join operator.
 * */
typedef struct {
    int seller_id;
    int reserve_price;
    int auction_id;
    char seller_name[NAME_SIZE];

} JoinOutput;
};