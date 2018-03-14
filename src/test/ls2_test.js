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

//var pb = require('webos-sysbus');
var pb = require('palmbus');
var sys = require('sys');
var _ = require('underscore')._;

sys.log("creating javascript service client");

function responseArrived(message) {
	sys.log("responseArrived[" + message.responseToken() + "]:" + message.payload());
}

function delayWithTimeoutResponseArrived(message) {
	sys.log("delayWithTimeoutResponseArrived[" + message.responseToken() + "]:" + message.payload());
}

function delayResponseArrived(message) {
	sys.log("delayResponseArrived[" + message.responseToken() + "]:" + message.payload());
}

var h = new pb.Handle("com.sample.service", false);

var p = {msg: "Rob"};
var s = JSON.stringify(p);
var call = h.call("palm://com.webos.node_js_service/test", s);
call.addListener('response', responseArrived);

var d = 0
var p = {delay: d};
var s = JSON.stringify(p);
var call = h.subscribe("palm://com.webos.node_js_service/delay", s);
call.addListener('response', delayWithTimeoutResponseArrived);
call.setResponseTimeout(1000*(d+3));

var call = h.subscribe("palm://com.webos.node_js_service/delay", s);
call.addListener('response', delayResponseArrived);
