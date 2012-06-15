#!/bin/bash
#
#  Copyright 2012 Alin Dobra and Christopher Jermaine
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

# This script generates cc code and compiles it as a library
# The script assumes that the file Main.m4 exists

numParallelJobs=1

# the two arguments
dirPath=$1
objects=$2


echo "Parameters: $1 $2"

cd $dirPath

echo "Compiling the code"

# just in case the compiler is the intel compiler
# source /opt/intel/Compiler/11.1/056/bin/iccvars.sh intel64

export OBJECTS="$objects"
make -k -f ../MK-generated -j ${numParallelJobs}
