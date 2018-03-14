// Copyright (c) 2015-2018 LG Electronics, Inc.
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

// Needed files:
// Client permissions file
// /usr/share/luna-service2/client-permissions.d/com.webos.nodeclient.example.json
/*
{
  "com.webos.nodeclient.example": ["js.example"]
}
*/
// Client role file
// /usr/share/luna-service2/roles.d/com.webos.nodeclient.example.json
/*
{
  "allowedNames": ["com.webos.nodeclient.example"],
  "permissions":
    [
      {
        "service": "com.webos.nodeclient.example",
        "outbound": ["*"]
      }
    ],
  "type": "regular",
  "appId": "com.webos.nodeclient.example"
}
*/

var palmbus = require('palmbus');
palmbus.setAppId("com.webos.nodeclient.example");

function responseArrived(message) {
    console.log("responseArrived[" + message.responseToken() + "]:" + message.payload());
    process.exit(0);
}

console.log("creating ls2 handle object");

var h = new palmbus.Handle("com.webos.nodeclient.example");
var params = {msg: "Hi there"};
var request = h.call("palm://com.webos.nodeservice.example/test", JSON.stringify(params));
request.addListener('response', responseArrived);
