#!/bin/bash

URL="http://127.0.0.1:8080/"

for i in $(seq 1 500); do
	curl -s -o /dev/null -w "%{http_code}\n" "$URL" &
done | sort | uniq -c

wait
