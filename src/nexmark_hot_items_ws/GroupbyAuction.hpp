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
 * GroupbyAuction.hpp
 *
 *  Created on: April 17, 2020
 *      Author: damien.tassetti
 */

#pragma once
#include "../dataflow/Vertex.hpp"
#include "../nexmark_hot_items/GroupbyAuction.hpp"

#include <map>

namespace nexmark_hot_items_ws {
using OriginalGroupbyAuction = nexmark_hot_items::GroupbyAuction;

class GroupbyAuction: public OriginalGroupbyAuction {

private:

	void sortMessages(list<Message*>* tmpMessages);

	void initExpectedMID(Message*const inMessage, int & expected_msgid, bool & is_first_time);

	void initExpectedWID(Message*const inMessage, const int expected_msgid, int & expected_wid, bool & is_expected_msgid_being_processed);

	bool isMessageExpected(Message*const inMessage, const int expected_msgid, const int expected_wid);

	void updateExpectedValues(Message*const currentMessage, list<Message*>* & messageList, int & expected_msgid, int & expected_wid, bool & is_expected_msgid_being_processed);

public:

	GroupbyAuction(int tag, int rank, int worldSize);

	void streamProcess(int channel);

};
};