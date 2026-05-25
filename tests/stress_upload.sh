#!/bin/bash

mkdir -p tests/tmp_uploads

for i in $(seq 1 50); do
	echo "upload test $i" > tests/tmp_uploads/file_$i.txt

	curl -s -X POST \
		-F "file=@tests/tmp_uploads/file_$i.txt" \
		http://127.0.0.1:8080/upload \
		-o /dev/null \
		-w "%{http_code}\n" &
done | sort | uniq -c

wait
