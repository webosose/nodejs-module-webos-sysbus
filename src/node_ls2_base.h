// Copyright (c) 2010-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef NODE_LS2_BASE_H
#define NODE_LS2_BASE_H

#include <luna-service2/lunaservice.h>
#include <node.h>
#include <node_object_wrap.h>

class LS2Handle;

class LS2Base : public node::ObjectWrap {
protected:
	// Common routine called whenever a message arrives from the bus. Different symbols
	// are used to differentiate requests, responses and cancelled subscriptions
	void EmitMessage(const v8::Handle<v8::String>& symbol, LSMessage *message);
};

#endif
