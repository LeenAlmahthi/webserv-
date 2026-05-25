#!/bin/bash

URL="http://127.0.0.1:8080/cgi/test.py"

for i in $(seq 1 100); do
	curl -s -o /dev/null -w "%{http_code}\n" "$URL" &
done | sort | uniq -c

wait
