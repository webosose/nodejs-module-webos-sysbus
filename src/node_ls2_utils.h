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

#ifndef NODE_LS2_UTILS_H
#define NODE_LS2_UTILS_H

/*
	Inspired and informed by v8-juice <http://code.google.com/p/v8-juice/>.
*/

#include <node.h>
#include <v8.h>
#include <node_object_wrap.h>
#include <iostream>
#include <stdexcept>

// This is a templated function declaration. It says for any type "T" there exists a function
// that will take a T and return it as a handle to a V8 object. It doesn't actually provide that
// function, though, so there need to be specializations for each of the types that are used to
// instantiate ConvertToJS. The more generic specializations are found in node_ls2_utils.cpp, others
// are found in the source files where the type in question is used.
template <typename T> v8::Handle<v8::Value> ConvertToJS(T v);


// This is a templated structure declaration for a structure that automatically converts a V8
// handle into type "T".
// ConvertFromJS is used in the generated template functions in node_ls2_member_function_wrappers.h
// to convert parameters passed in from JavaScript into their C++ equivelents.
// Conversion is done with a structure template, rather than a function template, so that we can take advantage of V8's
// String::Utf8Value() structure to convert v8 strings to const char* reference. String::Utf8Value
// cannot be copied, so we can't return it from a function.
template <typename T> struct ConvertFromJS {
	explicit ConvertFromJS(const v8::Handle<v8::Value>&);
	T value() const;
};

template <> struct ConvertFromJS<const char*> {
	explicit ConvertFromJS(const v8::Handle<v8::Value>& value) : isNull(value->IsNull() || value->IsUndefined()), fString(value) {}
	const char* value() const {
		return isNull ? nullptr : *fString;
	}

private:
	bool isNull;
	v8::String::Utf8Value fString;
};

template <> struct ConvertFromJS<std::string> {
	explicit ConvertFromJS(const v8::Handle<v8::Value>& value) : isNull(value->IsNull() || value->IsUndefined()), fString(value) {}
	std::string value() const {
		if (isNull) {
			throw std::runtime_error("Null value but string expected");
		}
		return *fString;
	}
private:
	bool isNull;
	v8::String::Utf8Value fString;
};

template <> struct ConvertFromJS<unsigned long> {
	explicit ConvertFromJS(const v8::Handle<v8::Value>& value) : fValue(value->Uint32Value()) {}
	unsigned long value() const {
		return fValue;
	}
	
	unsigned long fValue;
};

template <> struct ConvertFromJS<int> {
	explicit ConvertFromJS(const v8::Handle<v8::Value>& value) : fValue(value->Int32Value()) {}
	int value() const {
		return fValue;
	}

	int fValue;
};

// Include the generated templates. If we had C++0x we could use variadic templates instead.
#include "node_ls2_member_function_wrappers.h"

#endif
