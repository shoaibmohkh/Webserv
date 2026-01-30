#!/usr/bin/env python3
import os
print("Content-Type: text/plain")
print()
print("QUERY_STRING=" + os.environ.get("QUERY_STRING",""))
print("SCRIPT_NAME=" + os.environ.get("SCRIPT_NAME",""))
print("REQUEST_METHOD=" + os.environ.get("REQUEST_METHOD",""))
