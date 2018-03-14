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

#ifndef NODE_LS2_CALL_H
#define NODE_LS2_CALL_H

#include "node_ls2_base.h"

#include <string>

class LS2Handle;

class LS2Call : public LS2Base {
public:
    enum {kUnlimitedResponses = 0};

	// Create the "Call" function template and add it to the target.
	static void Initialize (v8::Handle<v8::Object> target);
	
	// Create a "Call" JavaScript object and with a handle and a token.
	static v8::Local<v8::Object> NewForCall();
	
	explicit LS2Call();
	
    void SetHandle(LS2Handle* handle);

    void Call(const char* busName, const char* payload, int responseLimit);

protected:
	// Called by V8 when the "Call" function is used with new. This has to be here, but the
	// resulting "Call" object is useless as it has no matching LSHandle structure.
    static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

    static void CancelWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void SetResponseTimeoutWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
    void Cancel();
	void SetResponseTimeout(int timeout_ms);

private:
	virtual ~LS2Call();
	static bool ResponseCallback(LSHandle *sh, LSMessage *message, void *ctx);
	bool ResponseArrived(LSMessage *message);
    void CancelInternal(LSMessageToken token, bool shouldThrow, bool cancelDueToError);

	// Throws an exception if fHandle or fToken are invalid.
	void RequireHandle();

	// prevent copying
    LS2Call( const LS2Call& );
    const LS2Call& operator=( const LS2Call& );

	LS2Handle* fHandle;
    LSMessageToken fToken;
    int fResponseLimit;
    int fResponseCount;
    
    static v8::Persistent<v8::FunctionTemplate> gCallTemplate;
};

#endif
