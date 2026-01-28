#!/usr/bin/env python3
import os

print("Content-Type: text/plain")
REQUEST_METHOD = os.environ.get("REQUEST_METHOD","")
if REQUEST_METHOD == "POST":
    content_length = int(os.environ.get("CONTENT_LENGTH","0"))
    post_data = os.sys.stdin.read(content_length)
    print()
    print("POST_DATA=" + post_data)
else:
    print("STATUS: 400 Bad Request")
    print()
    print("No POST data received.")
