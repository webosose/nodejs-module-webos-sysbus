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

#pragma once

#include <v8.h>
#include <functional>
#include <type_traits>
#include <utility>

template <typename Object, typename Result, typename... Args>
std::function<Result(Args...)> bindMethod(Object &obj, Result (Object::*fn)(Args...))
{
	return [=, &obj](Args &&... args) {
		return (obj.*fn)(std::forward<Args>(args)...);
	};
}

template <typename Object, typename Result, typename... Args>
std::function<Result(Args...)> bindMethod(const Object &obj, Result (Object::*fn)(Args...) const)
{
	return [=, &obj](Args &&... args) {
		return (obj.*fn)(std::forward<Args>(args)...);
	};
}

template <typename Result, typename... Args>
typename std::enable_if<!std::is_void<Result>::value>::type // restrict to non-void type
setCallResult(const std::function<Result(Args...)> &fn, const v8::FunctionCallbackInfo<v8::Value>& pack, Args &&... bound)
{
	pack.GetReturnValue().Set(ConvertToJS<Result>(fn(std::forward<Args>(bound)...)));
}

template <typename... Args>
void setCallResult(const std::function<void(Args...)> &fn, const v8::FunctionCallbackInfo<v8::Value>& pack, Args &&... bound)
{
    fn(std::forward<Args>(bound)...);
    pack.GetReturnValue().SetUndefined();
}

template <typename Result, typename... Args>
void call(const std::function<Result(Args...)> &fn, const v8::FunctionCallbackInfo<v8::Value>& pack, Args &&... bound)
{
	setCallResult(fn, pack, std::forward<Args>(bound)...);
}

template <typename Result>
void call(const std::function<Result()> &fn, const v8::FunctionCallbackInfo<v8::Value>& pack)
{
	setCallResult(fn, pack);
}

template <typename Result, typename... Args, typename... BoundArgs>
void call(const std::function<Result(Args...)> &fn, const v8::FunctionCallbackInfo<v8::Value>& pack, BoundArgs &&... bound)
{
	static_assert( sizeof...(Args) > sizeof...(BoundArgs), "There should be some args to bind" );
	using ArgsTuple = std::tuple<Args...>;
	constexpr std::size_t next_arg = sizeof...(bound);
	using next_type = typename std::tuple_element<next_arg, ArgsTuple>::type;
	call(fn, pack, std::forward<BoundArgs>(bound)..., std::forward<next_type>(ConvertFromJS<next_type>(pack[next_arg]).value()));
}

template <typename T,typename Result,typename... ArgsToMember>
void MemberFunctionWrapper(Result (T::*MemFunc)(ArgsToMember...), const v8::FunctionCallbackInfo<v8::Value>& info )
{
	v8::Isolate*  isolate = info.GetIsolate();

	try {
		if (info.Length() != sizeof...(ArgsToMember))
			throw std::runtime_error("Invalid number of parameters");

		T *o = node::ObjectWrap::Unwrap<T>(info.This());
		if (!o) throw std::runtime_error("Unable to unwrap native object.");

		call(bindMethod(*o, MemFunc), info);

	} catch( std::exception const & ex ) {
		isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(isolate,
			ex.what())));
	} catch( ... ) {
		isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(isolate,
			"Native function threw an unknown exception.")));
	}
}

template <typename T, typename... ArgsToMember>
void VoidMemberFunctionWrapper(void (T::*MemFunc)(ArgsToMember...), const v8::FunctionCallbackInfo<v8::Value>& info )
{
	MemberFunctionWrapper(MemFunc, info);
}

template <typename T,typename Result, typename... ArgsToMember>
void MemberFunctionWrapper(Result (T::*MemFunc)(ArgsToMember...) const, const v8::FunctionCallbackInfo<v8::Value>& info )
{
	v8::Isolate*  isolate = info.GetIsolate();

	try {
		if (info.Length() != sizeof...(ArgsToMember))
			throw std::runtime_error("Invalid number of parameters");

		T *o = node::ObjectWrap::Unwrap<T>(info.This());
		if (!o) throw std::runtime_error("Unable to unwrap native object.");

		call(bindMethod(*o, MemFunc), info);

	} catch( std::exception const & ex ) {
		isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(isolate,
			ex.what())));
	} catch( ... ) {
		isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(isolate,
			"Native function threw an unknown exception.")));
	}
}

template <typename T, typename... ArgsToMember>
void VoidMemberFunctionWrapper(void (T::*MemFunc)(ArgsToMember...) const, const v8::FunctionCallbackInfo<v8::Value>& info )
{
	MemberFunctionWrapper(MemFunc, info);
}
