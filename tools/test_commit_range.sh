#!/bin/sh
###########################################################################
#
#  Copyright 2016, ARM Limited, All Rights Reserved
#  SPDX-License-Identifier: Apache-2.0
#
#  Licensed under the Apache License, Version 2.0 (the "License"); you may
#  not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
###########################################################################

# Check if all commits since `origin/master` build

set -e

ORIGIN=origin/master
BRANCH=$(git rev-parse --abbrev-ref HEAD)

usage() {
    echo "Usage: ./$(basename $0) [<origin-branch>]"
    echo "    <origin-branch> The branch to test back to (defaults to origin/master)"
}

# If there are too many arguments, show help and exit with an error.
if test $# -gt 1; then
    usage
    exit 1
fi

# Read the optional <origin-branch> argument. If present, replace the default
# value of ORIGIN.
PARAM_ORIGIN=$1

if [ ! -z $PARAM_ORIGIN ]; then
    ORIGIN=$PARAM_ORIGIN
fi

# Come up with the list of commits between the ORIGIN and HEAD, starting from
# the oldest commit.
COMMITS=$(git rev-list --reverse $ORIGIN..HEAD)

# Test each commit with make clean and make.
for COMMIT in $COMMITS; do
    git checkout $COMMIT
    make clean
    make

    if [ $? -eq 0 ]; then
        echo $COMMIT - passed
    else
        echo $COMMIT - failed
        echo Returning to $BRANCH
        break
    fi
done

# Go back to the original starting point, to avoid leaving the user in detached
# HEAD state.
git checkout $BRANCH
