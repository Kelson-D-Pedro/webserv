#!/usr/bin/env python3
import time
import sys

# Cabeçalho CGI obrigatório
print("Content-Type: text/plain\r\n")
sys.stdout.flush()

print("CGI iniciado. Entrando em loop infinito...")
sys.stdout.flush()

i = 0

while True:
    i += 1

    # Mensagem periódica (opcional)
    print(f"Iteração {i}")
    sys.stdout.flush()

    # Dorme 1 segundo para NÃO consumir 100% da CPU
    # Remova ou diminua para testar CPU-bound
    time.sleep(1)
