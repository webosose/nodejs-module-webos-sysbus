# Copyright (c) 2010-2018 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 2.8.7)

project(webos-sysbus CXX)

include(webOS/webOS)
webos_modules_init(1 0 0 QUALIFIER RC2)
webos_component(2 0 1)

include(FindPkgConfig)

pkg_check_modules(NODEJS REQUIRED nodejs)
include_directories(${NODEJS_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${NODEJS_CFLAGS_OTHER})

pkg_check_modules(LS2 REQUIRED luna-service2)
include_directories(${LS2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LS2_CFLAGS_OTHER})

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LSGLIB2_CFLAGS_OTHER})

webos_add_compiler_flags(ALL -g -Wall -Wno-error=strict-aliasing -DEV_MULTIPLICITY=0 CXX -std=c++14)

# Can't specify --no-undefined because the plugin is allowed to link with all of
# the routines from the embedded v8 when it is loaded at runtime, but there's no
# libv8.so to link against now. Doesn't work to attempt to link with node.
# XXX There's got to be some way to let the linker know what node makes available
#     for its plugins so that --no-undefined CAN be specified.
# webos_add_linker_options(ALL --no-undefined)

webos_build_system_bus_files()

add_executable(${CMAKE_PROJECT_NAME}.node
                src/node_ls2.cpp
                src/node_ls2_base.cpp
                src/node_ls2_call.cpp
                src/node_ls2_error_wrapper.cpp
                src/node_ls2_handle.cpp
                src/node_ls2_message.cpp
                src/node_ls2_utils.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME}.node ${NODEJS_LDFLAGS} ${LS2_LDFLAGS} ${GLIB2_LDFLAGS})

webos_build_nodejs_module()

# Must have a symlink to the old name until all its users are changed
webos_install_symlink(${CMAKE_PROJECT_NAME}.node ${WEBOS_INSTALL_LIBDIR}/nodejs/palmbus.node)

