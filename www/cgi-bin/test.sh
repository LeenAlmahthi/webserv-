#!/bin/bash

echo "Content-Type: text/html"
echo ""

echo "<html>"
echo "<body>"
echo "<h1>Shell CGI Works 🎉</h1>"
echo "<p>Method: $REQUEST_METHOD</p>"
echo "<p>Query: $QUERY_STRING</p>"
echo "<p>Script: $SCRIPT_NAME</p>"
echo "<p>Server: $(uname -a)</p>"
echo "</body>"
echo "</html>"
