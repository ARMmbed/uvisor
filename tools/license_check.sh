#!/bin/bash
################################################################
#
#  This confidential and  proprietary  software may be used only
#  as authorised  by  a licensing  agreement  from  ARM  Limited
#
#             (C) COPYRIGHT 2015 ARM Limited
#                    ALL RIGHTS RESERVED
#
#   The entire notice above must be reproduced on all authorised
#   copies and copies  may only be made to the  extent permitted
#   by a licensing agreement from ARM Limited.
#
################################################################

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
