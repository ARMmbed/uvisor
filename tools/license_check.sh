#!/bin/bash
###########################################################################
#
#  Copyright (c) 2013-2015, ARM Limited, All Rights Reserved
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

# regular expression to find an existing license header and extract the year
lic_regex="(?<=\(C\) COPYRIGHT [0-9]{4}\-)[0-9]{4}(?= ARM Limited)"

# git root folder
git_root=$(git rev-parse --show-toplevel)

# parse all files in the repository
echo "Parsing files license headers in $git_root..."
cnt=0
while read -r line
do
    full_name=$git_root/$line

    # get year of latest commit
    git_yr=$(git log -1 --format="%ad" -- $full_name | awk '{print $5}')

    # check if the file with a date mismatch actually has a license header
    lic_yr=$(grep -Pos "$lic_regex" "$full_name")
    if [ -n "$lic_yr" ]
    then
        # check if there is a date mismatch
        if [ "$git_yr" -gt "$lic_yr" ]
        then
            # replace the current year in the license header
            echo "  Changed year: $git_yr (was $lic_yr) for $line"
            perl -pi -e "s/$lic_regex/$git_yr/" $full_name

            # increment counter
            cnt=$((cnt + 1))
        fi
    fi
done < <(git ls-tree -r --name-only HEAD)

# final message
if [ "$cnt" -eq 1 ]
then
    echo "$cnt file changed"
else
    echo "$cnt files changed"
fi
echo "Goodbye!"
