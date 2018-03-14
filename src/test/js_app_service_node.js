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
// Definition of service provided groups:
// /usr/share/luna-service2/api-permissions.d/com.webos.nodeservice.example.json
/*
{
  "js.example": ["com.webos.nodeservice.example/*"]
}
*/
// Service permissions file
// /usr/share/luna-service2/client-permissions.d/com.webos.nodeservice.example.json
/*
{
  "com.webos.nodeservice.example": ["public","private"]
}
*/
// Service role file
// /usr/share/luna-service2/roles.d/com.webos.nodeservice.example.json
/*
{
  "allowedNames": ["com.webos.nodeservice.example"],
  "permissions":
    [
      {
        "service": "com.webos.nodeservice.example",
        "outbound": ["*"]
      }
    ],
  "type": "regular",
  "appId": "com.webos.nodeservice.example"
}
*/
// Service file
// /usr/share/dbus-1/system-services/com.webos.nodeservice.example.service
/*
[D-BUS Service]
Name=com.webos.nodeservice.example
Exec=/usr/bin/node
Type=static
*/

var palmbus = require('palmbus');
palmbus.setAppId("com.webos.nodeservice.example");

console.log("creating example javascript service");

function testCallback (message) {
    console.log("payload in test call: '" + message.payload() + "'");
    var r = {msg: "echo request: " + message.payload()};
    message.respond(JSON.stringify(r));
}

function requestArrived(message) {
    console.log("requestArrived");
    switch(message.method()) {
    case "test":
        testCallback(message);
        break;
    }
}

var h = new palmbus.Handle("com.webos.nodeservice.example");
h.registerMethod("/", "test")
h.addListener('request', requestArrived);
