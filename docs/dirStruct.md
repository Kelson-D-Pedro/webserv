# 📂 Estrutura de Diretórios do Projeto *Webserv*

Este projeto segue o paradigma de **Programação Orientada a Objetos (OOP)**.  
A organização está dividida em **implementação** (`.cpp`) e **interfaces** (`.hpp`), agrupadas por módulos.  

---

## 🔹 Implementações (`src/`)

Dentro da pasta `src/`, cada subdiretório contém os **arquivos `.cpp`** de implementação das classes relacionadas ao módulo que o nome da pasta indica.  

### **src/core/**
Módulo central, responsável pela infraestrutura do servidor.  

- `Server.cpp` → classe que gerencia o ciclo de vida do servidor.  
- `Socket.cpp` → abstração de socket.  
- `Poller.cpp` → encapsula `poll()` / `select()` para multiplexação de conexões.  

### **src/http/**
Módulo responsável pela lógica do protocolo HTTP.  

- `Request.cpp` → parser de requisições HTTP.  
- `Response.cpp` → constrói respostas HTTP.  
- `Router.cpp` → decide qual recurso/ficheiro deve ser servido.  

### *(outros módulos seguem a mesma lógica)*  
- `src/config/` → classes para parsing de arquivos de configuração.  
- `src/cgi/` → classes para execução de scripts CGI.  
- `src/utils/` → classes auxiliares (logger, manipuladores de strings, etc.).  

---

## 🔹 Interfaces (`include/`)

Na pasta `include/`, cada subdiretório contém os **arquivos `.hpp`** que definem as **interfaces (headers)** das classes do módulo correspondente.  

### **include/core/**
- `Server.hpp`  
- `Socket.hpp`  
- `Poller.hpp`  

### **include/http/**
- `Request.hpp`  
- `Response.hpp`  
- `Router.hpp`  

### *(outros módulos seguem a mesma lógica)*  
- `include/config/ConfigParser.hpp`  
- `include/cgi/CgiHandler.hpp`  
- `include/utils/Logger.hpp`, `StringUtils.hpp`  

---

## ✅ Resumo da Convenção
- **Implementações** (`.cpp`) → ficam em `src/<módulo>/`.  
- **Interfaces** (`.hpp`) → ficam em `include/<módulo>/`.  
- Cada subdiretório agrupa apenas classes do seu módulo, mantendo **coerência e separação de responsabilidades**.  

---

👉 Desta forma, o projeto mantém uma estrutura **modular, limpa e fácil de manter**, alinhada com boas práticas de **OOP em C++98**.  
