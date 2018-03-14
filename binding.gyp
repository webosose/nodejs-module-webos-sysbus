# Copyright (c) 2013-2018 LG Electronics, Inc.
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

{
  'variables' : {
    'sysroot%': ''
  },
  "targets": [
    {
      'target_name': "webos-sysbus",
      'include_dirs': [
         '<!@(pkg-config --cflags-only-I glib-2.0 | sed s/-I//g)'
      ],
      'sources': [ 'src/node_ls2.cpp',
                   'src/node_ls2_base.cpp',
                   'src/node_ls2_call.cpp',
                   'src/node_ls2_error_wrapper.cpp',
                   'src/node_ls2_handle.cpp',
                   'src/node_ls2_message.cpp',
                   'src/node_ls2_utils.cpp' ],
      'link_settings': {
          'libraries': [
              '<!@(pkg-config --libs glib-2.0)',
              '-lluna-service2',
              '-lpthread'
          ]
      },
      'cflags!': [ '-fno-exceptions' ],
      'cflags': [ '-g' ],
      'cflags_cc': [ '-g', '--std=c++11' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'ldflags': [ '-pthread' ],
      'actions': [
         {
            'variables': {
                'trusted_scripts': [
                    "$(webos_servicesdir)/jsservicelauncher/bootstrap-node.js",
                    "$(webos_prefix)/nodejs/unified_service_server.js"
               ]
            },
            'action_name':'gen_trusted',
            'inputs': [
               'tools/gen_list.sh',
               'binding.gyp'
            ],
            'outputs': [
               'src/trusted_scripts.inc'
            ],
            'action': [
                '/bin/sh', 'tools/gen_list.sh', '<@(_outputs)', '<@(trusted_scripts)'
            ],
            'message':'Generating trusted scripts list'
         }
      ]
    }
  ]
}
