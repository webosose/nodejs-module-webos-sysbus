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

var h;

sys.log("creating javascript service");

function testCallback (message) {
    sys.log("payload in testCallback: '" + message.payload() + "'");
    var r = {msg: "ahoy, matie " + message.payload()};
    message.respond(JSON.stringify(r));
}

function delayCallback (message) {
    sys.log("payload in testCallback: '" + message.payload() + "'");
    var params = JSON.parse(message.payload());
    var r = {
        msg: "ahoy, matie ",
        params: params
    };
    var f = function() {
        message.respond(JSON.stringify(r));
    };
    _.delay(f, 1000*params.delay)
    _.delay(f, 1000*(params.delay+2))
    _.delay(f, 1000*(params.delay+10))
}

function errorCallback (message) {
    message.respond("bang");
    var x = thingThatDoesntExist;
}

function dieCallback (message) {
    var r = {msg: "bye bye"};
    message.respond(JSON.stringify(r));
    message.respond("bye bye")
    _.delay(process.exit, 1);
}

function requestArrived(message) {
    sys.log("requestArrived");
    message.print();
    switch(message.method()) {
    case "test":
        testCallback(message);
        break;
    case "delay":
        delayCallback(message);
        break;
    case "die":
        dieCallback(message);
        break;
    case "error":
        errorCallback(message);
        break;
    }
}

h = new pb.Handle("com.webos.node_js_service", false);
h.registerMethod("", "test")
h.registerMethod("", "delay")
h.registerMethod("", "die")
h.registerMethod("", "error")

h.addListener('request', requestArrived);
