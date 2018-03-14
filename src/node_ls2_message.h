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

#ifndef NODE_LS2_MESSAGE_H
#define NODE_LS2_MESSAGE_H

#include "node_ls2_utils.h"

#include <luna-service2/lunaservice.h>
#include <node.h>
#include <node_object_wrap.h>
#include <string>

struct LSMessage;



class LS2Message : public node::ObjectWrap {
public:
	// Create the "Message" function template and add it to the target.
	static void Initialize (v8::Handle<v8::Object> target);

	// Create a "Message" JavaScript object and wrap it around the C++ LSMessage object.
	static v8::Local<v8::Value> NewFromMessage(LSMessage*);

	LSMessage* Get() const;

protected:
	// Called by V8 when the "Message" function is used with new. This has to be here, but the
	// resulting "Message" object is useless as it has no matching LSMessage structure from
	// the lunaservice library.
	static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

private:
	// This constructor is private as these objects are only created by the
	// static functions "New" and "NewFromMessage"
	explicit LS2Message(LSMessage* m = 0);
	virtual ~LS2Message();

	// Replace the message on an existing "Message" object. Used to avoid
	// Having to create two LS2Messages.
    void SetMessage(LSMessage* m);

	// Wrappers and accessors for use in the "Message" function template.
	static void ApplicationIDWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	const char* ApplicationID() const;

	static void CategoryWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	const char* Category() const;

	static void IsSubscriptionWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	bool IsSubscription() const;

	static void KindWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	const char* Kind() const;

	static void MethodWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	const char* Method() const;

	static void PayloadWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	const char* Payload() const;

	static void PrintWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	void Print() const;

	static void RespondWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	bool Respond(const char* payload) const;

	static void ResponseTokenWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	LSMessageToken ResponseToken() const;

	static void SenderWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	const char* Sender() const;

	static void SenderServiceNameWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	const char* SenderServiceName() const;

	static void TokenWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	LSMessageToken Token() const;

	static void UniqueTokenWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	const char* UniqueToken() const;

	// Wrapper around an LSMessage string getter function that will throw an
	// exception if fMessage is null and also return an empty string if the
	// lower level accessor returns 0.
	typedef const char* (*StringGetterFunction) (LSMessage *msg);
	const char* GetString(StringGetterFunction f) const;

	// Wrapper around an LSMessage token getter function that will throw an
	// exception if fMessage is null.
	typedef LSMessageToken (*TokenGetterFunction) (LSMessage *msg);
	LSMessageToken GetToken(TokenGetterFunction f) const;

	// Throws an exception if fMessage is 0.
	void RequireMessage() const;

	// prevent copying
    LS2Message( const LS2Message& );
    const LS2Message& operator=( const LS2Message& );

	LSMessage* fMessage;
	static v8::Persistent<v8::FunctionTemplate> gMessageTemplate;
};

// Converter for LS2Message objects that converts a wrapped object to its native
// object. See node_ls2_utils.h for a description of how ConvertFromJS works.

template <> struct ConvertFromJS<LS2Message*> {
    explicit ConvertFromJS(const v8::Handle<v8::Value>& value) : fMessage(0) {
        v8::Handle<v8::Object> o = v8::Handle<v8::Object>::Cast(value);
        fMessage = node::ObjectWrap::Unwrap<LS2Message>(o);
        if (!fMessage) {
            throw std::runtime_error("Unable to unwrap native object.");
        }
    }

    LS2Message* value() const {
        return fMessage;
    }
private:
    LS2Message* fMessage;
};

#endif
