#!/usr/bin/env python3
import os
import sys
a = 1
# while (True):
#     a+1
#     print("Ola Mundo")
print("content-type: text/html")
print("Status: 200 OK")
print("""
<!DOCTYPE html>
<html lang="pt">
<head>
    <meta charset="UTF-8">
    <title>Variáveis CGI</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #F4F4F4;
            color: #333;
            padding: 20px;
        }
        h1 {
            text-align: center;
            color: #444;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 25px;
            background-color: white;
        }
        th, td {
            padding: 8px 14px;
            border: 1px solid #ccc;
        }
        th {
            background-color: #222;
            color: white;
        }
        tr:nth-child(even) {
            background-color: #F9F9F9;
        }
        .section {
            margin-top: 40px;
        }
        pre {
            background-color: #eee;
            padding: 10px;
            overflow-x: auto;
        }
    </style>
</head>
<body>
    <h1>Variáveis de Ambiente CGI</h1>
    <table>
        <tr>
            <th>Variável</th>
            <th>Valor</th>
        </tr>
""")
for key, value in os.environ.items():
    print(f"<tr><td>{key}</td><td>{value}</td></tr>")
print("""</table>""")
# Corpo da requisição (POST)
length = int(os.environ.get("CONTENT_LENGTH", 0))
if length > 0:
    body = sys.stdin.read(length)
    print("""
    <div class="section">
        <h2>Corpo da Requisição (POST)</h2>
        <pre>{}</pre>
    </div>
    """.format(body.replace("<", "&lt;").replace(">", "&gt;")))
print("""</body>
</html>""")