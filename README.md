nodejs-module-webos-sysbus
==========================

Summary
-------
A module for nodejs that allows Javascript access to the Open webOS system bus

How to Build on Linux
=====================

## Dependencies

Below are the tools and libraries (and their minimum versions) required to build
_nodejs-module-webos-sysbus_:

* cmake (version required by webosose/cmake-modules-webos)
* g++ 4.6.3
* glib-2.0 2.32.1
* make (any version)
* webosose/cmake-modules-webos 1.0.0 RC3
* webosose/luna-service2 3.0.0
* webosose/nodejs 0.4.12-0webos3
* pkg-config 0.26

## Building

Once you have downloaded the source, enter the following to build it (after
changing into the directory under which it was downloaded):

    $ mkdir BUILD
    $ cd BUILD
    $ cmake ..
    $ make
    $ sudo make install

The directory under which the files are installed defaults to `/usr/local/webos`.
You can install them elsewhere by supplying a value for `WEBOS_INSTALL_ROOT`
when invoking `cmake`. For example:

    $ cmake -D WEBOS_INSTALL_ROOT:PATH=$HOME/projects/webosose ..
    $ make
    $ make install

will install the files in subdirectories of `$HOME/projects/webosose`.

Specifying `WEBOS_INSTALL_ROOT` also causes `pkg-config` to look in that tree
first before searching the standard locations. You can specify additional
directories to be searched prior to this one by setting the `PKG_CONFIG_PATH`
environment variable.

If not specified, `WEBOS_INSTALL_ROOT` defaults to `/usr/local/webos`.

To configure for a debug build, enter:

    $ cmake -D CMAKE_BUILD_TYPE:STRING=Debug ..

To see a list of the make targets that `cmake` has generated, enter:

    $ make help

## Uninstalling

From the directory where you originally ran `make install`, enter:

    $ [sudo] make uninstall

You will need to use `sudo` if you did not specify `WEBOS_INSTALL_ROOT`.

Usage Notes
===========

*Example client (see ./src/test/ls2_app_test.js for details)*
```
    var palmbus = require('palmbus');
    palmbus.setAppId("com.webos.nodeclient.example");

    function responseArrived(message) {
        console.log("responseArrived[" + message.responseToken() + "]:" + message.payload());
    }

    console.log("creating ls2 handle object");

    var h = new palmbus.Handle("com.webos.nodeclient.example");
    var params = {msg: "Hi there"};
    var request = h.call("palm://com.webos.nodeservice.example/test", JSON.stringify(params));
    request.addListener('response', responseArrived);
```

*Example service (see ./src/test/js_app_service_node.js for details)*
```
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
```

## Function, Event, Object, and Method Reference

The following section lists the functions and objects that _nodejs-module-webos-sysbus_ adds, what methods
you can call on those objects and what events those objects emit.

### Functions

#### setAppId(appId)

This function sets application ID for Luna services registered in this JS application.
Application ID allows to enforce application-based LS2 security restrictions for executed
Node.js service. This ID should be set in service bootstrap code.

### Handle object

#### Handle(serviceName, [publicBus])

Constructor function used to create a new LS2 service bus object.

Parameters:

- **serviceName** - name of a service to register
- **publicBus [deprecated][optional]** - deprecated argument. LS2 public/private buses separation
is obsolete and use of separate buses is discouraged. If this argument is not passed -
registered service will use security restrictions associated with application ID set
using _**setAppId**_ function.
When specified this argument should be true for services registered on both the public and
private buses, or false for private bus only services.

An object created with palmbus.Handle has the following methods:

#### call(serviceNameAndMethod, methodParameters)
#### watch(serviceNameAndMethod, methodParameters)
#### subscribe(serviceNameAndMethod, methodParameters)

Calls the named service and method, passing the methodParameters string as the
message payload. Usually this is a JSON encoded object, but this method does not
do the encoding for you. Returns a `Call` object that is the source of events and
object to make method calls related to this call.

See the section on garbage collection for an explanation
of the difference between call, watch and subscribe. When in doubt, use call.

#### registerMethod(category, method)

Registers a category and method with the bus. Note that _nodejs-module-webos-sysbus_ does
not provide any mechanism for method-specific dispatch. The event listener
attached to the handle is responsible for using the category() and method()
methods of the message object passed as the first parameter to dispatch the
request to an appropriate handler.

#### subscriptionAdd(key, message)

Enable a message to be used as a subscription. See the Luna Service Library
documentation for a more detailed discussion of subscriptions.

### Handle events

#### 'cancel' event

The 'cancel' event is emitted from a Handle object when a message sent or received
via that Handle is canceled. The canceled message is passed as the single parameter
to any event listener.

#### 'request' event

The 'request' event is emitted from a Handle object when a method registered on
that handle is invoked by the bus. The message that invoked the method is passed
as the single parameter to any event listener. The category() and method() methods
of the message object can be used to dispatch the request to an appropriate handler.

#### 'response' event

The 'response' event is emitted by a Call object when a response to that call is
received. The message that was received is passed as the single parameter to any
event listener.

### Call object

#### cancel()

Use to cancel a call to prevent any further responses and release the resources
needed for the call.

#### setResponseTimeout(timeout_ms)

Sets timeout for a method call. The call will be canceled if no reply
is received after the timeout_ms milliseconds.

### Message object

This object cannot be constructed from JavaScript, but is passed to various events.
A Message object has the following methods:

#### respond(responseString)

Responds to a message. The response string is expected to be a JSON object, but
this method does not do the encoding.

#### payload()

Returns a string containing the payload for this message. This is usually a JSON
encoded object, but this method does not decode it for you.

#### method()

Returns a string describing the method used to send this message.

#### category()

Returns the category for this message.

#### applicationID()

Returns a string containing the applicationID, if any, of the sender of this
message.

#### senderServiceName()

Returns a string containing the service bus name, if any, of the sender of this
message.

#### sender()

Returns the sender of this message.

#### uniqueToken()

Returns the unique token for this message.

#### token()

Returns the token for this message.

#### responseToken()

Returns the response token for this message.

#### kind()

Returns the kind for this message.

#### isSubscription()

Returns true if this message is a subscription.

#### print()

Prints the contents of a message to the terminal.

## Garbage Collection

As long as they are in use for registered methods or calls, Handle objects keep
a reference to themselves and will not be garbage collected. Any event listeners
attached to them will also be referenced and not collected.

Since there is no way to unregister a method, any handle used to register a
service on the bus with registerMethod will never be collected.

In the call case, as long as a call has been made and the expected number of
responses have not yet arrived the call and handle are protected. Such a system
could lead to serious resource use if over time a user of _nodejs-module-webos-sysbus_
creates a lot of calls with pending responses.

In order to minimize such resource use there are three different call producing
methods on Handle; call, watch and subscribe.

Call expects only one response. As soon as it arrives both the call self-reference
and the call's reference to the handle are released. Call also uses the LS2 API
function LSCallOneReply on the hope that it is more efficient for this case.

Watch expects two responses. After the second response arrives the call
self-reference and the call's reference to the handle are released.

Subscribe allows for any number of responses. Unless canceled, the call
self-reference and the call's reference to the handle will never be released.

The only exception to the above cases is when an error is returned from the bus.
In that case the call self-reference and the call's reference to the handle are
released immediately.

# Copyright and License Information

Copyright (c) 2010-2018 LG Electronics, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

SPDX-License-Identifier: Apache-2.0
