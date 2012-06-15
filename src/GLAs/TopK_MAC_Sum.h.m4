dnl #
dnl #  Copyright 2012 Alin Dobra and Christopher Jermaine
dnl #
dnl #  Licensed under the Apache License, Version 2.0 (the "License");
dnl #  you may not use this file except in compliance with the License.
dnl #  You may obtain a copy of the License at
dnl #
dnl #      http://www.apache.org/licenses/LICENSE-2.0
dnl #
dnl #  Unless required by applicable law or agreed to in writing, software
dnl #  distributed under the License is distributed on an "AS IS" BASIS,
dnl #  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
dnl #  See the License for the specific language governing permissions and
dnl #  limitations under the License.
dnl #
include(GLA-templates.m4)

m4_define(</TOPK_NAME/>, </TopK_MAC_Sum/>)dnl
m4_define(</TOPK_TUPLE/>,</(mac, MACADDR), (sum, DOUBLE)/>)dnl

#ifndef _TopK_MAC_Sum_H_
#define _TopK_MAC_Sum_H_

m4_include(TopKGLA.h.m4)dnl

#endif 


