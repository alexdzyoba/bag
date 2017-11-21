#!/bin/bash

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <string length> <timeout> [<split by n chars>]"
    exit 1
fi

i=0
while true; do
    line=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $1 | head -n1)
    if [[ -n "$3" ]]; then
        l1=$(echo "${line}" | cut -c-$3)
        l2=$(echo "${line}" | cut -c$(($3+1))-)
        echo -ne "$l1\n$l2\n"
        echo "Written line $i" > /dev/stderr
        i=$((i+1))
        echo "Written line $i" > /dev/stderr
        i=$((i+1))
    else
        echo "${line}"
        echo "Written line $i" > /dev/stderr
        i=$((i+1))
    fi
    sleep $2
done
