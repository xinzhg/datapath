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
dnl # // M4 PREAMBLE CODE
include(MessagesFunctions.m4)
dnl # END OF M4 CODE

#ifndef _DUMPREQUESTMESSAGE_H_
#define _DUMPREQUESTMESSAGE_H_

///////////// DUMP REQUEST MESSAGE  ////////////

/** Message send by the Diagnose to SqliteDumper

    Arguments:
	msgContainer, killMe
    */

M4_CREATE_MESSAGE_TYPE(DumpRequestMessage,
  </(killMe, bool)/>,
  </(msgContainer, MessageContainer)/>)

#endif
