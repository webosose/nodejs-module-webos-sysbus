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

#include "node_ls2_base.h"
#include "node_ls2_message.h"

#include <syslog.h>
#include <stdlib.h>

using namespace node;
using namespace v8;

void LS2Base::EmitMessage(const Handle<String>& symbol, LSMessage *message)
{
    Local<Value> messageObject = LS2Message::NewFromMessage(message);
    
    // messageObject will be empty if a v8 exception is thrown in
    // LS2Message::NewFromMessage
    if (!messageObject.IsEmpty()) {

      Handle<Value> argv[2] =
      {
        symbol, // event name
        messageObject  // argument
      };

      MakeCallback(Isolate::GetCurrent(),
                   this->handle(),
                   static_cast<const char*>("emit"),
                   2,
                   static_cast<v8::Handle<v8::Value>*>(argv));
    } else {
        // We don't want to silently lose messages
        syslog(LOG_USER | LOG_CRIT, "%s: messageObject is empty", __PRETTY_FUNCTION__);
        abort();
    }
}
