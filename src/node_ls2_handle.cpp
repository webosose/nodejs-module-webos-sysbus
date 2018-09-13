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

#include "node_ls2.h"
#include "node_ls2_error_wrapper.h"
#include "node_ls2_handle.h"
#include "node_ls2_message.h"
#include "node_ls2_call.h"
#include "node_ls2_utils.h"

#include <syslog.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <unistd.h>

using namespace std;
using namespace v8;
using namespace node;

static Persistent<String> cancel_symbol;
static Persistent<String> request_symbol;

LS2Handle::ServiceContainer LS2Handle::fRegisteredServices;

static std::set<std::string> trustedScripts = {
#include "trusted_scripts.inc"
};


// Called during add-on initialization to add the "Handle" template function
// to the target object.
void LS2Handle::Initialize(Handle<Object> target)
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    HandleScope scope(isolate);

    Local<FunctionTemplate> t = FunctionTemplate::New(isolate, New);

    t->SetClassName(v8::String::NewFromUtf8(isolate, "palmbus/Handle"));

    t->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(t, "call", CallWrapper);
    NODE_SET_PROTOTYPE_METHOD(t, "watch", WatchWrapper);
    NODE_SET_PROTOTYPE_METHOD(t, "subscribe", SubscribeWrapper);
    NODE_SET_PROTOTYPE_METHOD(t, "registerMethod", RegisterMethodWrapper);
    NODE_SET_PROTOTYPE_METHOD(t, "subscriptionAdd", SubscriptionAddWrapper);
    NODE_SET_PROTOTYPE_METHOD(t, "cancel", CancelWrapper);
    NODE_SET_PROTOTYPE_METHOD(t, "pushRole", PushRoleWrapper);
    NODE_SET_PROTOTYPE_METHOD(t, "unregister", UnregisterWrapper);

    cancel_symbol.Reset(isolate, String::NewFromUtf8(isolate, "cancel"));
    request_symbol.Reset(isolate, String::NewFromUtf8(isolate, "request"));

    target->Set(String::NewFromUtf8(isolate, "Handle"), t->GetFunction());
    NODE_SET_METHOD(target, "setAppId", LS2Handle::SetAppId);
}

void LS2Handle::CallCreated(LS2Call*)
{
    Ref();
}

void LS2Handle::CallCompleted(LS2Call*)
{
    Unref();
}

LSHandle* LS2Handle::Get()
{
    RequireHandle();
    return fHandle;
}

// Called by V8 when the "Handle" function is used with new.
void LS2Handle::New(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    try {
        HandleScope scope(isolate);
        if (args.Length() < 1) {
            throw std::runtime_error("Too few arguments");
        }
        ConvertFromJS<const char*> serviceName(args[0]);
        LSHandle* ls_handle = nullptr;

        LSErrorWrapper err;
        if (args.Length() >= 2 && args[1]->IsBoolean()) {
            // deprecated initialization. Arguments: "service name", "public/private bus"
            if (!LSRegisterPubPriv(serviceName.value(), &ls_handle, args[1]->BooleanValue(), err)) {
                err.ThrowError();
            }
        }
        else { // correct initialization. Arguments: "service name"
            if (!LSRegisterApplicationService(serviceName.value(), findMyAppId(isolate).c_str(),
                &ls_handle, err))
            {
                err.ThrowError();
            }
        }

        LS2Handle *handle = new LS2Handle(ls_handle);
        handle->Wrap(args.This());

        args.GetReturnValue().Set(args.This());
    } catch(const std::exception& ex) {
        isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(
            isolate, ex.what())));
    } catch(...) {
        isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(
            isolate, "Native function threw an unknown exception.")));
    }
}

LS2Handle::LS2Handle(LSHandle* handle)
    : fHandle(handle)
{
    LSErrorWrapper err;

    Attach(GetMainLoop());

    if (!LSSubscriptionSetCancelFunction(fHandle, LS2Handle::CancelCallback, static_cast<void*>(this), err)) {
        err.ThrowError();
    }
}

LS2Handle::~LS2Handle()
{
#if TRACE_DESTRUCTORS
    cerr << "LS2Handle::~LS2Handle()" << endl;
#endif
	// This should never happen, since the destructor won't get called when we still have registered methods
	if (fRegisteredMethods.size() > 0) {
		cerr << "LS2Handle::~LS2Handle() called with registered methods active" << endl;
		this->Unregister();
	}
}

void LS2Handle::CallWrapper(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    MemberFunctionWrapper<LS2Handle, Handle<Value>, const char*, const char*>(&LS2Handle::Call, args);
}

Handle<Value> LS2Handle::Call(const char* busName, const char* payload)
{
    return CallInternal(busName, payload, 1);
}

void LS2Handle::WatchWrapper(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    MemberFunctionWrapper<LS2Handle, Handle<Value>, const char*, const char*>(&LS2Handle::Watch, args);
}

Handle<Value> LS2Handle::Watch(const char* busName, const char* payload)
{
    return CallInternal(busName, payload, 2);
}

void LS2Handle::SubscribeWrapper(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    MemberFunctionWrapper<LS2Handle, Handle<Value>, const char*, const char*>(&LS2Handle::Subscribe, args);
}

Handle<Value> LS2Handle::Subscribe(const char* busName, const char* payload)
{
    return CallInternal(busName, payload, LS2Call::kUnlimitedResponses);
}

void LS2Handle::CancelWrapper(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    MemberFunctionWrapper<LS2Handle, bool, LSMessageToken>(&LS2Handle::Cancel, args);
}

bool LS2Handle::Cancel(LSMessageToken t)
{
    RequireHandle();
    LSErrorWrapper err;
    if(!LSCallCancel(fHandle, t, err)) {
        err.ThrowError();
    }
    return true;
}

void LS2Handle::PushRoleWrapper(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    VoidMemberFunctionWrapper<LS2Handle, const char*>(&LS2Handle::PushRole, args);
}

void LS2Handle::PushRole(const char* pathToRoleFile)
{
    RequireHandle();
    LSErrorWrapper err;
    if(!LSPushRole(fHandle, pathToRoleFile, err)) {
        err.ThrowError();
    }
}

void LS2Handle::RegisterMethodWrapper(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    VoidMemberFunctionWrapper<LS2Handle, const char*, const char*>(&LS2Handle::RegisterMethod, args);
}

void LS2Handle::RegisterMethod(const char* category, const char* methodName)
{
    RequireHandle();
    if (fRegisteredMethods.size() == 0) {
        // Establish a self-reference on the first method registration so that this object
        // won't get collected.
        Ref();
    }
    RegisteredMethod* m = new RegisteredMethod(methodName);
    fRegisteredMethods.push_back(m);
    RegisterCategory(category, m->GetMethods());
}

void LS2Handle::UnregisterWrapper(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    VoidMemberFunctionWrapper<LS2Handle>(&LS2Handle::Unregister, args);
}

void LS2Handle::Unregister()
{
    //cerr << "LS2Handle::Unregister()" << endl;
    if (fHandle) {
        LSErrorWrapper err;
        if(!LSUnregister(fHandle, err)) {
            // Cannot call err.ThrowError() here, since throwing from a destructor is a no-no
            std::cerr << "LSUnregister failed during Unregister()." << std::endl;
            err.Print();
        }
	fHandle = 0;
    }
    if (fRegisteredMethods.size() > 0) {
        // Undo the Ref() operation from the first registerMethod() - this object is now eligible for collection
        Unref();
    }
    // I wish I had boost and could use shared_ptr here.
    for(MethodVector::const_iterator i = fRegisteredMethods.begin(); i != fRegisteredMethods.end(); ++i) {
        RegisteredMethod* m = *i;
        delete m;
    }

    // Since we've deleted all of the items above, make sure we
    // clean up the vector as well (make size == 0) so that if this gets
    // called again for some reason we won't attempt to double-free
    fRegisteredMethods.clear();
}

void LS2Handle::SubscriptionAddWrapper(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    VoidMemberFunctionWrapper<LS2Handle, const char*, LS2Message*>(&LS2Handle::SubscriptionAdd, args);
}

void LS2Handle::SubscriptionAdd(const char* key, LS2Message* msg)
{
    RequireHandle();
    LSErrorWrapper err;
    if(!LSSubscriptionAdd(fHandle, key, msg->Get(), err)) {
        err.ThrowError();
    }
}

Handle<Value> LS2Handle::CallInternal(const char* busName, const char* payload, int responseLimit)
{
    RequireHandle();
    Local<Object> callObject = LS2Call::NewForCall();
    LS2Call *call = node::ObjectWrap::Unwrap<LS2Call>(callObject);
    if (!call) {
        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        return isolate->ThrowException(
                v8::String::NewFromUtf8(isolate, "Unable to unwrap native object."));
    }
    call->SetHandle(this);
    call->Call(busName, payload, responseLimit);
    return callObject;
}

void LS2Handle::Attach(GMainLoop *mainLoop)
{
    LSErrorWrapper err;
    if(!LSGmainAttach(fHandle, mainLoop, err)) {
        err.ThrowError();
    }
}

bool LS2Handle::RegisterCategory(const char* categoryName, LSMethod *methods)
{
    LSErrorWrapper err;
    const char* catName = "/";
    if (strlen(categoryName) > 0) {
        catName = categoryName;
    }

    if(!LSRegisterCategoryAppend(fHandle, catName, methods, 0, err)) {
        err.ThrowError();
    }

    if(!LSCategorySetData(fHandle, catName, static_cast<void*>(this), err)) {
        err.ThrowError();
    }

    return true;
}

bool LS2Handle::CancelCallback(LSHandle *sh, LSMessage *message, void *ctx)
{
    LS2Handle* h = static_cast<LS2Handle*>(ctx);
    return h->CancelArrived(message);
}

bool LS2Handle::CancelArrived(LSMessage *message)
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    HandleScope scope(isolate);
    //UnrefIfPending(LSMessageGetResponseToken(message));
    EmitMessage(Local<String>::New(isolate, cancel_symbol), message);
    return true;
}

bool LS2Handle::RequestCallback(LSHandle *sh, LSMessage *message, void *ctx)
{
    LS2Handle* h = static_cast<LS2Handle*>(ctx);
    return h->RequestArrived(message);
}

bool LS2Handle::RequestArrived(LSMessage *message)
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    HandleScope scope(isolate);
    EmitMessage(Local<String>::New(isolate, request_symbol), message);
    return true;
}

void LS2Handle::SetAppId(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    HandleScope scope(isolate);

    try {
        checkCallerScriptPermissions(isolate);

        if (args.Length() < 2) {
            throw std::runtime_error("Invalid arguments");
        }

        ConvertFromJS<std::string> appId(args[0]);
        if (appId.value().empty()) {
            throw std::runtime_error("Empty application Id is not allowed.");
        }

        ConvertFromJS<std::string> servicePath(args[1]);
        if (servicePath.value().empty()) {
            throw std::runtime_error("Empty service path is not allowed.");
        }

        auto serviceInfo = fRegisteredServices.find(servicePath.value());
        if (serviceInfo != fRegisteredServices.end()) {
            // generate exception only if servicePath is different
            if (serviceInfo->second != appId.value()) {
                throw std::runtime_error("Application Id already has been set.");
            }
            return;
        }

        fRegisteredServices[servicePath.value()] = appId.value();
    }
    catch(const std::exception& e) {
        std::stringstream err_message;
        err_message << "SetAppId: " << e.what();
        isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(
            isolate, err_message.str().c_str())));
    }
    catch(...) {
        isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(
            isolate, "SetAppId: unknown exception")));
    }
}

void LS2Handle::RequireHandle()
{
    if (fHandle == 0) {
        throw runtime_error("Handle object is missing native handle.");
    }
}

void LS2Handle::checkCallerScriptPermissions(v8::Isolate* isolate)
{
    Local<StackTrace> trace = StackTrace::CurrentStackTrace(isolate, 1, StackTrace::kScriptName);
    ConvertFromJS<std::string> scriptName(trace->GetFrame(0)->GetScriptName());
    if (!trustedScripts.count(scriptName.value())) {
        throw std::runtime_error("Incorrect execution context");
    }
}

const std::string& LS2Handle::findMyAppId(v8::Isolate* isolate)
{
    v8::Local<v8::StackTrace> trace = v8::StackTrace::CurrentStackTrace(isolate, 50, v8::StackTrace::kScriptName);
    for(int i = 0; i < trace->GetFrameCount(); i++) {
        std::string scriptName = ConvertFromJS<std::string>(trace->GetFrame(i)->GetScriptName()).value();
        std::string scriptDirectory = scriptName.substr(0, scriptName.rfind('/') + 1);
        auto serviceInfo = fRegisteredServices.find(scriptDirectory);
        if (serviceInfo != fRegisteredServices.end()) {
            return serviceInfo->second;
        }
    }
    throw std::runtime_error("The service is not registered");
}

LS2Handle::RegisteredMethod::RegisteredMethod(const char* name)
{
    std::memset(&fMethods, 0, sizeof(fMethods));
    copy(name, name + strlen(name), back_inserter(fName));
    fName.push_back(0);
    fMethods[0].name = &fName[0];
    fMethods[0].function = &LS2Handle::RequestCallback;
}

LSMethod* LS2Handle::RegisteredMethod::GetMethods()
{
    return fMethods;
}
