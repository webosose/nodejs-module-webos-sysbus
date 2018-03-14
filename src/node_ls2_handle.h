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

#ifndef NODE_LS2_HANDLE_H
#define NODE_LS2_HANDLE_H

#include "node_ls2_base.h"

#include <set>
#include <glib.h>
#include <luna-service2/lunaservice.h>
#include <vector>
#include <string>
#include <unordered_map>

class LS2Message;
class LS2Call;

class LS2Handle : public LS2Base {
public:
	// Create the "Handle" function template and add it to the target.
	static void Initialize (v8::Handle<v8::Object> target);

    void CallCreated(LS2Call* call);
    void CallCompleted(LS2Call* call);

    LSHandle* Get();
    bool IsValid() { return fHandle != 0;}

protected:
	// Called by V8 when the "Handle" function is used with new.
	static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

private:
	// This constructor is private as these objects are only created by the
	// static function "New".
	LS2Handle(LSHandle* handle);
	virtual ~LS2Handle();

	static void CallWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	v8::Handle<v8::Value> Call(const char* busName, const char* payload);

	static void WatchWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	v8::Handle<v8::Value> Watch(const char* busName, const char* payload);

	static void SubscribeWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	v8::Handle<v8::Value> Subscribe(const char* busName, const char* payload);

	static void CancelWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	bool Cancel(LSMessageToken token);

	static void RegisterMethodWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	void RegisterMethod(const char* category, const char* methodName);

	static void UnregisterWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	void Unregister();

	static void PushRoleWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	void PushRole(const char* pathToRoleFile);

	static void SubscriptionAddWrapper(const v8::FunctionCallbackInfo<v8::Value>& args);
	void SubscriptionAdd(const char* key, LS2Message* msg);

	// Common implmentation for Call, Watch and Subscribe
   	v8::Handle<v8::Value> CallInternal(const char* busName, const char* payload, int responseLimit);

   	// Glib integration
	void Attach(GMainLoop *mainLoop);

	// Method registration implementation method.
	bool RegisterCategory(const char* categoryName, LSMethod *methods);

	static bool CancelCallback(LSHandle *sh, LSMessage *message, void *ctx);
	bool CancelArrived(LSMessage *message);

	static bool RequestCallback(LSHandle *sh, LSMessage *message, void *ctx);
	bool RequestArrived(LSMessage *message);

	static void SetAppId(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void checkCallerScriptPermissions(v8::Isolate* isolate);
	static const std::string& findMyAppId(v8::Isolate* isolate);
	// Common routine called whenever a message arrives from the bus. Different symbols
	// are used to differentiate requests, responses and cancelled subscriptions
	//void EmitMessage(const v8::Handle<v8::String>& symbol, LSMessage *message);

	// Throws an exception if fHandle is 0.
	void RequireHandle();

	// prevent copying
    LS2Handle( const LS2Handle& );
    const LS2Handle& operator=( const LS2Handle& );

    // Helper class to manage the data structures LS2 requires for method registration.
	class RegisteredMethod {
	public:
		RegisteredMethod(const char* name);
		LSMethod* GetMethods();
	private:
		std::vector<char> fName;
		LSMethod fMethods[2];
	};

	LSHandle* fHandle;

	typedef std::vector<RegisteredMethod*> MethodVector;
	MethodVector fRegisteredMethods;

    typedef std::unordered_map<std::string, std::string> ServiceContainer;
	static ServiceContainer fRegisteredServices;
};


#endif
