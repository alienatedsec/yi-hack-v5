#!/bin/sh

# This file is part of yi-hack-v5 (https://github.com/alienatedsec/yi-hack-v5).
# Copyright (c) 2021 alienatedsec

set -e

git config -f .gitmodules --get-regexp '^submodule\..*\.path$' |
    while read path_key path
    do
        url_key=$(echo $path_key | sed 's/\.path/.url/')
        url=$(git config -f .gitmodules --get "$url_key")
        git submodule add --force $url $path
    done
