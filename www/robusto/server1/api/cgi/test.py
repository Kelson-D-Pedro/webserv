#!/usr/bin/env python3

import os
import sys

print("Content-Type: text/html\n")
print("<html>")
print("<head><title>CGI Test</title></head>")
print("<body>")
print("<h1>CGI Script Funcionando!</h1>")
print("<h2>Environment Variables:</h2>")
print("<ul>")
print(f"<li>REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'N/A')}</li>")
print(f"<li>SCRIPT_FILENAME: {os.environ.get('SCRIPT_FILENAME', 'N/A')}</li>")
print(f"<li>PATH_INFO: {os.environ.get('PATH_INFO', 'N/A')}</li>")
print(f"<li>QUERY_STRING: {os.environ.get('QUERY_STRING', 'N/A')}</li>")
print(f"<li>SERVER_PROTOCOL: {os.environ.get('SERVER_PROTOCOL', 'N/A')}</li>")
print(f"<li>GATEWAY_INTERFACE: {os.environ.get('GATEWAY_INTERFACE', 'N/A')}</li>")
print(f"<li>CONTENT_TYPE: {os.environ.get('CONTENT_TYPE', 'N/A')}</li>")
print(f"<li>CONTENT_LENGTH: {os.environ.get('CONTENT_LENGTH', 'N/A')}</li>")
print("</ul>")

if os.environ.get('REQUEST_METHOD') == 'POST':
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    if content_length > 0:
        post_data = sys.stdin.read(content_length)
        print("<h2>POST Data:</h2>")
        print(f"<pre>{post_data}</pre>")

print("</body>")
print("</html>")