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

#include <glib.h>
#include <iostream>
#include <node.h>
#include <stdlib.h>
#include <v8.h>
#include <uv.h>
#include <list>
#include <map>
#include <algorithm>

#include "node_ls2_call.h"
#include "node_ls2_handle.h"
#include "node_ls2_message.h"

GMainLoop* gMainLoop = 0;

using namespace v8;
using namespace node;
using namespace std;

typedef std::map<int, uv_poll_t *> WatcherMap;
typedef std::multimap<int, GPollFD *> PollfdMap;
typedef std::map<int, int> EventMap;

struct econtext {
    GPollFD* pfd; // GpollFD objects from Glib
    int nfd, afd; // number of GpollFD objects, allocated number of objects (rounded up to 2^n)
    gint maxpri;

    uv_prepare_t pw;
    uv_check_t cw;
    uv_timer_t tw;   // timeout to prevent libuv from running event loop "too soon"

    GMainContext* gc;
};

struct PollData {
    int fd;
};

// a map of fds to GPollFDs
static PollfdMap pfdMap;

// a map of fds to uv_poll_t
static WatcherMap pollwMap;

#undef VERBOSE_LOGGING

static uv_timer_t timeout_handle;
static bool query = false;

static void timeout_cb(uv_timer_t* w)
{
    /* nop */
}

static void uv_timeout_cb(uv_timer_t *handle)
{
    query = true;
    uv_timer_stop(&timeout_handle);
}

// Cleanup memory after poll handle is closed
static void close_cb(uv_handle_t* handle)
{
    uv_poll_t* pollw = (uv_poll_t*) handle;

    PollData *data = (PollData *)pollw->data;
    delete data;
    delete pollw;
}

static void poll_cb(uv_poll_t* handle, int status, int events)
{
    PollData *data = (PollData *) handle->data;
    int fd = data->fd;
    #ifdef VERBOSE_LOGGING
        std::cerr << "poll_cb, fd: " << fd << "events: " << events << std::endl;
    #endif
    // Iterate over *all* GPollFDs matching the watcher's fd
    std::pair <PollfdMap::iterator, PollfdMap::iterator> ret;
    ret = pfdMap.equal_range(fd);
    for (PollfdMap::iterator it=ret.first; it!=ret.second; ++it) {
        GPollFD *pfd = it->second;
        pfd->revents |= pfd->events & ((events & UV_READABLE ? G_IO_IN : 0) | (events & UV_WRITABLE ? G_IO_OUT : 0));
        #ifdef VERBOSE_LOGGING
            std::cerr << "    pfd->fd: " << pfd->fd << "pfd->events: " << pfd->events << std::endl;
            std::cerr << "    pfd->revents: " << pfd->revents << std::endl;
        #endif
    }

    pfdMap.erase(fd);
    uv_poll_stop(handle);
}

static void prepare_cb(uv_prepare_t* w)
{
    struct econtext* ctx = (struct econtext*)(((char*)w) - offsetof(struct econtext, pw));
    gint timeout;
    int i;

    // return if uv_timeout is active
    if (!query)
        return;

    g_main_context_prepare(ctx->gc, &ctx->maxpri);

    // Get all sources from glib main context
    while (ctx->afd < (ctx->nfd = g_main_context_query(
                                      ctx->gc,
                                      ctx->maxpri,
                                      &timeout,
                                      ctx->pfd,
                                      ctx->afd))
          ) {
        free(ctx->pfd);

        ctx->afd = 1;

        while (ctx->afd < ctx->nfd) {
            ctx->afd <<= 1;
        }

        ctx->pfd = (GPollFD*)malloc(ctx->afd * sizeof(GPollFD));
    }

    // store read/write flags for each FD
    EventMap events;

    // libuv does not support more than one watcher on same fd
    // iterate through GPollFD list, accumulating read/write flags for each FD, and creating a map from fd to GpollFD
    // for event dispatch in poll_cb()
    pfdMap.clear();
    for (i = 0; i < ctx->nfd; ++i) {
        GPollFD* pfd = ctx->pfd + i;
        int fd = pfd->fd;
        //translate events from Glib constants to the libuv values
        int uv_events = (pfd->events & G_IO_IN ? UV_READABLE: 0) | (pfd->events & G_IO_OUT ? UV_WRITABLE: 0);
        #ifdef VERBOSE_LOGGING
            std::cerr << "fd: " << fd << ", pfd->events: " << pfd->events << ", uv_events: " << uv_events << std::endl;
        #endif
        //reset received events for the GPollFD
        pfd->revents = 0;
        // Create a map of fds to GPollFD objects
        pfdMap.insert(std::pair<int, GPollFD *>(fd, pfd));
        EventMap::iterator it = events.find(fd);
        if (it != events.end()) {
            it->second |= uv_events;
        } else {
            events.insert(std::pair<int, int>(fd, uv_events));
        }
    }
    // Create poll handles where necessary, and start polling
    for (EventMap::iterator it = events.begin(); it != events.end(); it++) {
        uv_poll_t *pollw;
        std::pair<int, int> p = *it;
        int fd = p.first;
        int mask = p.second;
        #ifdef VERBOSE_LOGGING
            std::cerr << "fd: " << fd << ", mask: " << mask << std::endl;
        #endif
        WatcherMap::iterator pollFound = pollwMap.find(fd);
        if (pollFound == pollwMap.end()) {
            // not found - create a new uv_poll_t watcher, and initialize it
            #ifdef VERBOSE_LOGGING
                std::cerr << "creating new uv_poll_t for fd:" << fd <<std::endl;
            #endif
            pollw = new uv_poll_t;
            PollData *pd = new PollData;
            pd->fd = fd;
            pollw->data = pd;
            pollwMap.insert(std::pair<int, uv_poll_t*>(fd, pollw));
            uv_poll_init(uv_default_loop(), pollw, fd);
        } else {
            // reuse existing watcher
            std::pair<int, uv_poll_t *> p = *pollFound;
            pollw = p.second;
        }
        // Feed fd data to libuv and start polling
        // This will reset the mask if pollw is already registered
        uv_poll_start(pollw, mask, poll_cb);
    }

    // remove watchers that are no longer needed
    for (WatcherMap::iterator it = pollwMap.begin(), next; it != pollwMap.end(); it = next) {
        std::pair<int, uv_poll_t *> p = *it;
        next = it;
        next++;
        int fd = p.first;
        uv_poll_t *pollw = p.second;
        EventMap::iterator eventFound = events.find(fd);
        if (eventFound == events.end()) {
            #ifdef VERBOSE_LOGGING
            std::cerr << "removing uv_poll_t for fd:" << fd <<std::endl;
            #endif
            uv_poll_stop(pollw);
            uv_close((uv_handle_t *) pollw, close_cb);
            pollwMap.erase(it); // invalidates "it", don't use it after this
        }
    }

    if (timeout >= 0) {
        uv_timer_start(&ctx->tw, timeout_cb, timeout * 1e-3, 0);
    }
}

static void check_cb(uv_check_t* w)
{
    struct econtext* ctx = (struct econtext*)(((char*)w) - offsetof(struct econtext, cw));

    if (uv_is_active((uv_handle_t*) &ctx->tw)) {
        uv_timer_stop(&ctx->tw);
    }

    int ready = g_main_context_check(ctx->gc, ctx->maxpri, ctx->pfd, ctx->nfd);
    if(ready)
        g_main_context_dispatch(ctx->gc);  

    // libuv is too fast for glib, hold on for a while
    query = false;
    if (!uv_is_active((uv_handle_t*) &timeout_handle)) {
        uv_timer_start(&timeout_handle, uv_timeout_cb, 1, 0);   // 1ms
    }
}

static struct econtext default_context;

void init(Handle<Object> target)
{
    HandleScope scope(Isolate::GetCurrent());
    gMainLoop = g_main_loop_new(NULL, true);

    GMainContext *gc = g_main_context_default();
    struct econtext *ctx = &default_context;

    ctx->gc = g_main_context_ref (gc);
    ctx->nfd = 0;
    ctx->afd = 0;
    ctx->pfd = 0;

    query = true;

    // Prepare
    uv_prepare_init (uv_default_loop(), &ctx->pw);
    uv_prepare_start (&ctx->pw, prepare_cb);
    uv_unref((uv_handle_t*) &ctx->pw);

    uv_check_init(uv_default_loop(), &ctx->cw);
    uv_check_start (&ctx->cw, check_cb);
    uv_unref((uv_handle_t*) &ctx->cw);

    // Timer
    uv_timer_init(uv_default_loop(), &ctx->tw);
    uv_timer_init(uv_default_loop(), &timeout_handle);

    LS2Handle::Initialize(target);
    LS2Message::Initialize(target);
    LS2Call::Initialize(target);
}

GMainLoop* GetMainLoop()
{
    return gMainLoop;
}

NODE_MODULE(webos_sysbus, init)

