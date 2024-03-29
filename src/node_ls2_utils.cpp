// Copyright (c) 2010-2020 LG Electronics, Inc.
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

#include "node_ls2_utils.h"

template <> v8::Local<v8::Value> ConvertToJS<const char*>(const char* v)
{
    return v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), v).ToLocalChecked();
}

template <> v8::Local<v8::Value> ConvertToJS<uint32_t>(uint32_t v)
{
    return v8::Integer::NewFromUnsigned(v8::Isolate::GetCurrent(), v);
}

template <> v8::Local<v8::Value> ConvertToJS<bool>(bool v)
{
    return v8::Boolean::New(v8::Isolate::GetCurrent(), v);
}

template <> v8::Local<v8::Value> ConvertToJS< v8::Local<v8::Value> >(v8::Local<v8::Value>  v)
{
    return v;
}

