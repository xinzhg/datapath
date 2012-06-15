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
m4_divert(-1)
m4_define(</TYPE/>,</m4_second($1)/>)dnl
m4_define(</VAR/>,</m4_first($1)/>)dnl

dnl # macro to glue two argument lists into one safely
dnl # $1: first list
dnl # $2: second list
m4_define(</GLUE_LISTS/>,</dnl
m4_ifdef(</__TEMP__/>, </m4_undefine(</__TEMP__/>)/>)dnl
m4_ifval(</$1/>,</dnl
m4_foreach(</_A_/>,</$1/>,</dnl
m4_append(</__TEMP__/>, _A_<//>, </</, />/>)dnl
/>)dnl
/>)dnl
m4_ifval(</$2/>,</dnl
m4_foreach(</_A_/>,</$2/>,</dnl
m4_append(</__TEMP__/>, _A_<//>, </</, />/>)dnl
/>)dnl
/>)dnl
m4_ifval(</$3/>,</dnl
m4_foreach(</_A_/>,</$3/>,</dnl
m4_append(</__TEMP__/>, _A_<//>, </</, />/>)dnl
/>)dnl
/>)dnl
__TEMP__<//>dnl
/>)dnl

dnl # macro to list arguments with types
dnl # $1=list of the form (var, type)
dnl # result is type var, ...
m4_define(</TYPED_ARGS/>,</dnl
m4_ifdef(</__TEMP__/>, </m4_undefine(</__TEMP__/>)/>)dnl
m4_foreach(</_A_/>,</$*/>,</dnl
m4_append(</__TEMP__/>, TYPE(_A_) VAR(_A_),</</, />/>)dnl
/>)dnl
__TEMP__<//>dnl
/>)dnl


dnl # macro to list arguments with types
dnl # $1=list of the form (var, type)
dnl # result is type& var, ...
m4_define(</TYPED_REF_ARGS/>,</dnl
m4_ifdef(</__TEMP__/>, </m4_undefine(</__TEMP__/>)/>)dnl
m4_foreach(</_A_/>,</$*/>,</dnl
m4_append(</__TEMP__/>, TYPE(_A_)& VAR(_A_),</</, />/>)dnl
/>)dnl
__TEMP__<//>dnl
/>)dnl


dnl # macro to list arguments with types
dnl # $1=list of the form (var, type)
dnl # result is var, ...
m4_define(</ARGS/>,</dnl
m4_ifdef(</__TEMP__/>, </m4_undefine(</__TEMP__/>)/>)dnl
m4_foreach(</_A_/>,</$*/>,</dnl
m4_append(</__TEMP__/>, VAR(_A_),</</, />/>)dnl
/>)dnl
__TEMP__<//>dnl
/>)dnl


dnl # macro to scan a file and extract info
dnl # $1=name of file without extension
m4_define(</SCAN_GLA_FILE/>, </dnl
m4_ifval($1,</dnl
m4_divert(-1)
m4_include(</$1.h/>)dnl
m4_divert(0)dnl
/>)dnl
/>)dnl

dnl # quoted definition macro
dnl # outputs a quoted m4_define statement defining the second argument as the
dnl # definition of the first argument
dnl # Note: the arguments will be evaluated
m4_define(</m4_qdefine/>,</dnl
<//>m4_quote(</m4_define(/>m4_dquote(m4_quote($1))</,/>m4_dquote(m4_quote($2))</)/>)dnl
<//>m4_define(m4_quote($1),m4_quote($2))dnl
/>)

m4_define(</m4_ifdef_undef/>, </dnl
<//>m4_ifdef(</$1/>, </m4_undefine(</$1/>)/>)dnl
/>)dnl

m4_define(</m4_redefine/>,</dnl
<//>m4_ifdef_undef(</$1/>)dnl
<//>m4_define(</$1/>, </$2/>)dnl
/>)

m4_define(</m4_autoincr/>, </dnl
<//>m4_define(</$1/>, m4_incr(m4_defn(</$1/>)))dnl
/>)
m4_divert(0)dnl
