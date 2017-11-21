#!/bin/bash

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <timeout>"
    exit 1
fi

i=0
while read line; do
    echo "Printing output $i" > /dev/stderr
    echo "${line}"
    i=$((i+1))
    sleep $1
done < /dev/stdin
