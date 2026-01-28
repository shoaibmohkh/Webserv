#!/usr/bin/env python3
import sys
import time

print("Content-Type: text/html")
print()
print("CGI WORKED")
sys.stdout.flush()          # ensure the response is sent immediately
time.sleep(10)          # long-running CGI for timeout testing
