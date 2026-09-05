#!/usr/bin/env python3
import os
import sys

# O CGI *DEVE* imprimir os cabeçalhos primeiro!
print("Content-Type: text/html\r\n\r\n", end="")

print("<!DOCTYPE html><html><body style='background-color: #282a36; color: #f8f8f2; font-family: monospace;'>")
print("<h1 style='color: #ff79c6;'>CGI Executado via fork() e execve()! 🚀</h1>")

# Mostrando as variáveis que vieram do Server
print("<h2>Variaveis de Ambiente recebidas:</h2><ul>")
for key, value in os.environ.items():
    if key in ["REQUEST_METHOD", "QUERY_STRING", "SCRIPT_FILENAME"]:
        print(f"<li><strong style='color: #8be9fd;'>{key}:</strong> {value}</li>")
print("</ul>")

# Lendo o corpo (POST) do pipeIn (STDIN)
body = sys.stdin.read()
if body:
    print("<hr><h2 style='color: #50fa7b;'>Corpo recebido via POST:</h2>")
    print(f"<p style='border: 1px solid #6272a4; padding: 10px;'>{body}</p>")

print("</body></html>")