# 📊 RELATÓRIO TÉCNICO COMPLETO - WEBSERVER C++ HTTP/1.1

## 🎯 RESUMO EXECUTIVO

Este webserver C++ implementa um servidor HTTP/1.1 com arquitetura baseada em **epoll** para I/O multiplexado, **parser de configuração robusto**, e suporte parcial para funcionalidades HTTP essenciais. O projeto está **70-75% completo**, com lacunas críticas em CGI, chunked encoding, timeouts e algumas features HTTP/1.1.

---

## 📦 1. ESTADO ATUAL DOS MÓDULOS

### 🔷 1.1 CORE (Socket + Multiplexer)

**Ficheiros:** `Socket.hpp/cpp`, `Multiplexer.hpp/cpp`

#### ✅ IMPLEMENTADO:
- ✓ Socket não bloqueante com `SO_REUSEADDR` e `O_NONBLOCK`
- ✓ `getaddrinfo()` para resolução de endereços IPv4
- ✓ Epoll com `EPOLLIN`/`EPOLLOUT` para monitoramento de eventos
- ✓ Loop principal com `epoll_wait()` (timeout infinito)
- ✓ Accept de clientes e adição à epoll
- ✓ Separação clara entre sockets de servidor e cliente
- ✓ `SIGPIPE` ignorado corretamente

#### ❌ LACUNAS:
1. **Sem timeout para conexões**: Clientes lentos podem bloquear indefinidamente
2. **Sem gestão de memória em caso de erro**: Memory leaks em `t_http` quando há erro
3. **Sem controle de conexões simultâneas**: Nenhum limite de clientes
4. **Sem flag `EPOLLRDHUP`/`EPOLLHUP`**: Detecção de desconexão não otimizada
5. **Sem timestamp das conexões**: Impossível implementar timeouts de leitura/escrita
6. **Keep-alive implementado mas não gerido pelo epoll**: Conexão não retorna ao estado `EPOLLIN` após enviar resposta

---

### 🔷 1.2 CONFIG PARSER

**Ficheiros:** `ConfigParser.hpp/cpp`, `Tokenizer.cpp`, `ParseConfigData.cpp`, `VerifingParseData.cpp`

#### ✅ IMPLEMENTADO:
- ✓ Tokenizador robusto
- ✓ Verificação sintática completa (brackets, semicolons, hierarquia)
- ✓ Parsing de `server`, `location`, `listen`, `root`, `index`, `error_page`
- ✓ Support para `client_max_body_size`, `autoindex`, `allow_methods`, `upload_path`
- ✓ Support para `return` (redirect)
- ✓ Support para CGI (`cgi_ext`, `cgi_path`, `cgi_pass`, `cgi_param`)
- ✓ Herança de diretivas (global → server → location)
- ✓ Validação semântica de paths e portas

#### ❌ LACUNAS:
1. **Return directive parseada mas NÃO executada no Method**: A diretiva `return` existe nas estruturas mas não é verificada/aplicada em `Method::executeMethod()`
2. **CGI configurado mas não integrado**: CGI parseado mas `CgiHandler.cpp/hpp` estão vazios
3. **Sem validação de executabilidade de CGI**: Não verifica se o binário CGI existe ou tem permissões
4. **Sem support para múltiplos `server_name`**: Parser aceita apenas uma string

---

### 🔷 1.3 HTTP REQUEST

**Ficheiros:** `Request.hpp/cpp`

#### ✅ IMPLEMENTADO:
- ✓ Parse completo de request line (method, path, HTTP version)
- ✓ Parse de headers em `std::map` (O(log n) lookup)
- ✓ Parse de body até `Content-Length`
- ✓ Normalização de path (remove `.`, `..`, barras duplas)
- ✓ Leitura não bloqueante com `EAGAIN`/`EWOULDBLOCK`
- ✓ Detecção de request completo via `Content-Length`

#### ❌ LACUNAS:
1. **Sem suporte a `Transfer-Encoding: chunked`**: Apenas `Content-Length` suportado
2. **Sem limite de tamanho de header**: Pode consumir memória indefinidamente
3. **Sem timeout de leitura**: Request pode demorar infinito
4. **Sem validação de método**: Aceita qualquer string como método
5. **Sem parsing de query string**: `?param=value` não é extraído
6. **Sem parsing detalhado de `multipart/form-data`**: Implementado em `helping_methods.cpp` mas não robusto
7. **Sem detecção de HTTP/1.0 vs HTTP/1.1**: Assume sempre HTTP/1.1

---

### 🔷 1.4 HTTP RESPONSE

**Ficheiros:** `Response.hpp/cpp`

#### ✅ IMPLEMENTADO:
- ✓ Construção de headers HTTP/1.1
- ✓ Support para status codes (via `HttpStatus`)
- ✓ Support para MIME types (via `MimeTypes`)
- ✓ Envio de ficheiros via `setFileResponse()`
- ✓ Geração de error pages customizadas
- ✓ Método `sendChunked()` (mas **NÃO utilizado**)
- ✓ Header `Connection: keep-alive` / `close`

#### ❌ LACUNAS:
1. **`sendChunked()` existe mas NUNCA é chamado**: Resposta sempre enviada inteira
2. **Sem `Transfer-Encoding: chunked` em headers**: Header nunca é setado
3. **Sem suporte a HEAD**: Deve enviar apenas headers, sem body
4. **Sem header `Allow` em 405**: RFC 7231 exige listar métodos permitidos
5. **Sem timeout de escrita**: `send()` pode bloquear indefinidamente
6. **Sem retry em `EAGAIN`/`EWOULDBLOCK`**: Envio pode falhar parcialmente
7. **Ficheiros grandes são carregados inteiros na memória**: Sem streaming

---

### 🔷 1.5 METHOD (Roteamento e Lógica HTTP)

**Ficheiros:** `Method.hpp/cpp`, `helping_methods.cpp`

#### ✅ IMPLEMENTADO:
- ✓ Dispatch para GET, POST, DELETE
- ✓ Location matching (prefixo mais longo)
- ✓ Verificação de `allow_methods` (405)
- ✓ Servir ficheiros estáticos via `stat()` + `open()`
- ✓ Servir diretórios com index
- ✓ Autoindex com HTML gerado
- ✓ Upload de ficheiros via `multipart/form-data`
- ✓ Validação de `client_max_body_size` (413)
- ✓ Sanitização de filenames e paths
- ✓ Support para error pages customizadas

#### ❌ LACUNAS:
1. **Return directive NÃO verificada**: `t_location::return_directive` e `t_server::return_directive` existem mas não são testados
2. **CGI não executado**: Apenas detectado mas sem execução (ficheiros CGI vazios)
3. **Sem HEAD**: Não implementado
4. **Sem OPTIONS**: Não implementado
5. **Sem header `Last-Modified` / `ETag`**: Sem suporte a caching
6. **Sem header `Content-Range`**: Sem suporte a partial requests (206)
7. **Autoindex não verifica permissões de leitura**: Pode gerar erro 500
8. **DELETE não verifica se é diretório**: Pode falhar silenciosamente

---

### 🔷 1.6 CGI

**Ficheiros:** `CgiHandler.hpp/cpp`

#### ✅ IMPLEMENTADO:
- (Nada - ficheiros vazios)

#### ❌ LACUNAS CRÍTICAS:
1. **Ficheiros vazios**: Sem nenhuma implementação
2. **Sem `fork()` + `execve()`**: Não executa binários CGI
3. **Sem pipes para stdin/stdout/stderr**: Não comunica com CGI
4. **Sem environment variables**: `PATH_INFO`, `QUERY_STRING`, `REQUEST_METHOD`, etc.
5. **Sem timeout para CGI**: Processo pode rodar indefinidamente
6. **Sem captura de status code de saída**: Não detecta erros do CGI
7. **Sem parsing de CGI headers**: Deve extrair `Status:`, `Content-Type:`, etc.
8. **Sem suporte a CGI chunked output**: CGI pode enviar `Transfer-Encoding: chunked`

---

### 🔷 1.7 AUTOINDEX

**Ficheiros:** `Autoindex.hpp/cpp`

#### ✅ IMPLEMENTADO:
- ✓ Geração de HTML básico
- ✓ Listagem de ficheiros com `readdir()`
- ✓ Links para parent directory (`..`)
- ✓ Integração com error pages customizadas (403)

#### ❌ LACUNAS:
1. **Sem ordenação alfabética**: Ficheiros não ordenados
2. **Sem display de tamanho/data**: Apenas nome
3. **Sem distinção visual entre ficheiros e diretórios**: Pode confundir
4. **Sem encoding de URLs**: Ficheiros com espaços geram links inválidos
5. **Sem verificação de permissões de leitura**: Pode gerar 500

---

### 🔷 1.8 UTILS

**Ficheiros:** `utils.hpp/cpp`, `helping_methods.cpp`, `types.hpp`

#### ✅ IMPLEMENTADO:
- ✓ `split()` para strings
- ✓ `intToString()` para conversão
- ✓ `is_path_within_root()` com `realpath()` para segurança
- ✓ `sanitize_filename()` contra path traversal
- ✓ `ensure_dir_exists()` com criação recursiva
- ✓ `extractFileFromMultipart()` para uploads
- ✓ `extractFilenameFromMultipart()`

#### ❌ LACUNAS:
1. **Sem URL encoding/decoding**: Paths com `%20` não são tratados
2. **Sem HTML escaping**: XSS possível em autoindex
3. **Sem função para ler ficheiros em chunks**: Sempre carrega tudo na memória
4. **Sem função para timestamp**: Necessário para timeouts

---

## 📋 2. MAPEAMENTO DE FUNCIONALIDADES

| Funcionalidade | Estado | Ficheiro Principal | Observações |
|----------------|--------|-------------------|-------------|
| **Epoll multiplexing** | ✅ 100% | `Multiplexer.cpp` | Funcional |
| **Socket não bloqueante** | ✅ 100% | `Socket.cpp` | Funcional |
| **Config parser** | ✅ 95% | `ConfigParser.cpp` | Falta validação de CGI executables |
| **Request parsing** | ⚠️ 80% | `Request.cpp` | Falta chunked requests |
| **Response building** | ⚠️ 75% | `Response.cpp` | Falta chunked responses |
| **GET method** | ⚠️ 85% | `Method.cpp` | Falta HEAD e CGI |
| **POST method** | ⚠️ 80% | `Method.cpp` | Falta CGI e chunked |
| **DELETE method** | ✅ 90% | `Method.cpp` | Funcional |
| **Static files** | ✅ 90% | `Method.cpp` | Funcional mas sem streaming |
| **Error pages** | ✅ 95% | `Response.cpp` | Funcional |
| **Autoindex** | ⚠️ 70% | `Autoindex.cpp` | Básico, falta polimento |
| **Upload** | ⚠️ 75% | `Method.cpp` | Funcional mas limitado |
| **CGI** | ❌ 0% | `CgiHandler.cpp` | **NÃO IMPLEMENTADO** |
| **Return (redirect)** | ❌ 30% | `Method.cpp` | Parseado mas **NÃO EXECUTADO** |
| **405 Allow header** | ❌ 0% | `Response.cpp` | **NÃO IMPLEMENTADO** |
| **HEAD method** | ❌ 0% | `Method.cpp` | **NÃO IMPLEMENTADO** |
| **Chunked requests** | ❌ 0% | `Request.cpp` | **NÃO IMPLEMENTADO** |
| **Chunked responses** | ❌ 10% | `Response.cpp` | Função existe mas **NÃO USADA** |
| **Keep-alive** | ⚠️ 60% | `Multiplexer.cpp` | Header setado mas **sem loop** |
| **Timeouts** | ❌ 0% | `Multiplexer.cpp` | **NÃO IMPLEMENTADO** |
| **Body size limits** | ✅ 95% | `Method.cpp` | Funcional |
| **Path security** | ✅ 95% | `utils.cpp` | Funcional |

**Legenda:**
- ✅ = Completo e funcional
- ⚠️ = Parcialmente implementado
- ❌ = Não implementado ou crítico

---

## 🚨 3. LACUNAS CRÍTICAS PARA HTTP/1.1

### 🔴 PRIORIDADE MÁXIMA (Obrigatórias)

#### 3.1 **CGI HANDLER COMPLETO**
**Estado:** ❌ 0% (ficheiros vazios)

**Impacto:** Crítico - CGI é requisito obrigatório dos projetos webserv da 42

**Ficheiros a criar/editar:**
- `src/cgi/CgiHandler.cpp`
- `include/cgi/CgiHandler.hpp`
- `src/http/Method.cpp` (integração)

**Implementação necessária:**
```cpp
// CgiHandler.hpp - estrutura sugerida
class CgiHandler {
private:
    std::string script_path;
    std::string interpreter_path;
    std::map<std::string, std::string> env;
    std::vector<char> stdin_data;
    std::string stdout_output;
    int timeout_seconds;
    
public:
    CgiHandler(const std::string& script, const std::string& interpreter);
    void setEnv(const std::string& key, const std::string& value);
    void setStdin(const std::vector<char>& data);
    void setTimeout(int seconds);
    bool execute(); // fork + execve + pipes + timeout
    std::string getOutput() const;
    int getExitCode() const;
};
```

**Passos de implementação:**
1. Criar `CgiHandler` class com env vars (REQUEST_METHOD, PATH_INFO, QUERY_STRING, CONTENT_TYPE, CONTENT_LENGTH, SERVER_PROTOCOL, etc.)
2. Implementar `fork()` + `execve()` com pipes para stdin/stdout/stderr
3. Implementar timeout com `alarm()` + `SIGCHLD` ou `select()` com timer
4. Capturar output e exit code
5. Parse CGI headers (`Status:`, `Content-Type:`) do output
6. Integrar em `Method::getMethod()` e `Method::postMethod()`

**Risco:** Alto - requer conhecimento de processos e signals

---

#### 3.2 **RETURN DIRECTIVE (REDIRECTS)**
**Estado:** ❌ Parseado mas não executado

**Impacto:** Crítico - 301/302 redirects são essenciais

**Ficheiros a editar:**
- `src/http/Method.cpp` (`executeMethod()`, `getMethod()`)

**Implementação:**
```cpp
// No início de Method::executeMethod() ou getMethod()
void Method::executeMethod(const t_server *client)
{
    // 1. Checar server-level return ANTES de location matching
    if (client->return_directive.code != 0) {
        response->setStatus(client->return_directive.code);
        if (!client->return_directive.url.empty())
            response->setHeader("Location", client->return_directive.url);
        response->setContentType("text/html");
        response->buildResponse();
        return;
    }
    
    // 2. Checar location-level return APÓS match_location
    const t_location *loc = match_location(request->getPath(), client);
    if (loc && loc->return_directive.code != 0) {
        response->setStatus(loc->return_directive.code);
        if (!loc->return_directive.url.empty())
            response->setHeader("Location", loc->return_directive.url);
        response->setContentType("text/html");
        response->buildResponse();
        return;
    }
    
    // 3. Continuar com lógica normal...
}
```

**Risco:** Baixo - simples de implementar

---

#### 3.3 **CHUNKED REQUESTS (Transfer-Encoding: chunked)**
**Estado:** ❌ 0%

**Impacto:** Alto - Clientes podem enviar POST chunked

**Ficheiros a editar:**
- `src/http/Request.cpp` (`parseRequest()`, `requestComplete()`)

**Implementação:**
```cpp
// Em Request::parseRequest()
bool Requests::requestComplete()
{
    size_t header_end = full_req.find("\r\n\r\n");
    if (header_end == std::string::npos)
        return false;

    // Verificar se é chunked
    std::string transfer_encoding = getHeader("Transfer-Encoding");
    if (transfer_encoding.find("chunked") != std::string::npos) {
        // Parse chunks: "SIZE\r\nDATA\r\n"
        // Último chunk é "0\r\n\r\n"
        return full_req_body.find("0\r\n\r\n", header_end + 4) != std::string::npos;
    }
    
    // Lógica existente para Content-Length...
}

// Método auxiliar para parse de chunks
void Requests::parseChunkedBody() {
    // Parse cada chunk e append a `body`
    // Formato: HEX_SIZE\r\nDATA\r\n
}
```

**Risco:** Médio - parsing complexo

---

#### 3.4 **CHUNKED RESPONSES (Transfer-Encoding: chunked)**
**Estado:** ⚠️ Função existe mas não é usada

**Impacto:** Médio - Necessário para ficheiros grandes

**Ficheiros a editar:**
- `src/http/Response.cpp` (`sendResponse()`)
- `src/http/Method.cpp` (decidir quando usar chunked)

**Implementação:**
```cpp
// Em Response::sendResponse()
void Response::sendResponse(int client_fd)
{
    // Detectar se deve usar chunked (ficheiro grande ou CGI)
    bool use_chunked = (body.size() > 1024 * 1024) || is_cgi;
    
    if (use_chunked) {
        setHeader("Transfer-Encoding", "chunked");
        headers.erase("Content-Length"); // Remover Content-Length
        buildHeaders(); // Rebuild sem body
        // Enviar headers
        send(client_fd, full_response.c_str(), full_response.size(), 0);
        // Enviar body chunked
        sendChunked(client_fd);
    } else {
        // Lógica existente...
    }
}
```

**Risco:** Médio - integração com lógica existente

---

#### 3.5 **TIMEOUTS DE LEITURA/ESCRITA**
**Estado:** ❌ 0%

**Impacto:** Crítico - Sem timeouts, clientes lentos bloqueiam o servidor

**Ficheiros a editar:**
- `src/core/Multiplexer.cpp` (`multiplexerLoop()`, `epollinHandler()`)
- `include/utils/types.hpp` (adicionar timestamp a `t_http`)

**Implementação:**
```cpp
// Em types.hpp - adicionar timestamp
typedef struct s_http
{
    int         client_fd;
    Socket      *server;
    Requests    *req;
    Response    *res;
    time_t      last_activity; // ADICIONAR
    
} t_http;

// Em Multiplexer.cpp - verificar timeouts no loop
void Multiplexer::multiplexerLoop(...)
{
    const int TIMEOUT_SECONDS = 30;
    
    while (1)
    {
        // Usar timeout curto (1 segundo) em vez de -1
        fds_waiting = epoll_wait(this->poll_fd, events, MAX_EVENTS, 1000);
        
        // Verificar timeouts
        time_t now = time(NULL);
        for (size_t i = 0; i < client.size(); ++i) {
            if (now - client[i].last_activity > TIMEOUT_SECONDS) {
                // Enviar 408 Request Timeout
                client[i].res->generateErrorPage(408);
                client[i].res->setHttpVersion("HTTP/1.1");
                client[i].res->buildResponse();
                client[i].res->sendResponse(client[i].client_fd);
                // Fechar conexão
                epoll_ctl(this->poll_fd, EPOLL_CTL_DEL, client[i].client_fd, NULL);
                close(client[i].client_fd);
                delete client[i].req;
                delete client[i].res;
                client.erase(client.begin() + i);
                --i;
            }
        }
        
        // Atualizar last_activity em cada evento...
    }
}
```

**Risco:** Médio - requer reestruturação do loop

---

#### 3.6 **405 METHOD NOT ALLOWED + HEADER ALLOW**
**Estado:** ❌ Header `Allow` não implementado

**Impacto:** Médio - RFC 7231 exige

**Ficheiros a editar:**
- `src/http/Method.cpp` (`getMethod()`, `postMethod()`, `deleteMethod()`)

**Implementação:**
```cpp
// Em Method.cpp - quando retornar 405
if (std::find(aux->allow_methods.begin(), aux->allow_methods.end(), 
              request->getMethod()) == aux->allow_methods.end())
{
    // Construir header Allow
    std::string allow;
    for (size_t i = 0; i < aux->allow_methods.size(); ++i) {
        if (i > 0) allow += ", ";
        allow += aux->allow_methods[i];
    }
    response->setHeader("Allow", allow);
    handle_errors_status_pages(405, client);
    // ...
}
```

**Risco:** Baixo - simples

---

#### 3.7 **HEAD METHOD**
**Estado:** ❌ 0%

**Impacto:** Médio - HTTP/1.1 básico

**Ficheiros a editar:**
- `src/http/Method.cpp` (`executeMethod()`, criar `headMethod()`)

**Implementação:**
```cpp
void Method::headMethod(const t_server *client)
{
    // Executar GET normalmente
    getMethod(client);
    
    // Limpar body mas manter headers
    response->setBody("");
    response->buildResponse();
}

void Method::executeMethod(const t_server *client)
{
    std::string method_type = request->getMethod();
    for (size_t i = 0; i < method_type.size(); ++i)
        method_type[i] = std::toupper(method_type[i]);

    if (method_type == "GET")
        getMethod(client);
    else if (method_type == "HEAD") // ADICIONAR
        headMethod(client);
    else if (method_type == "POST")
        postMethod(client);
    // ...
}
```

**Risco:** Baixo

---

### 🟡 PRIORIDADE ALTA (Recomendadas)

#### 3.8 **KEEP-ALIVE LOOP COMPLETO**
**Estado:** ⚠️ Header setado mas sem loop

**Impacto:** Alto - Performance

**Ficheiros a editar:**
- `src/core/Multiplexer.cpp` (`epolloutHandler()`)

**Implementação:**
```cpp
void Multiplexer::epolloutHandler(...)
{
    // ... enviar resposta ...
    
    if (!client[pos].res->getKeepAlive())
    {
        // Fechar conexão
        epoll_ctl(this->poll_fd, EPOLL_CTL_DEL, action_fd, NULL);
        close(action_fd);
        delete client[pos].req;
        delete client[pos].res;
        client.erase(client.begin() + pos);
    }
    else
    {
        // ADICIONAR: Voltar ao estado de leitura
        event.events = EPOLLIN;
        event.data.fd = action_fd;
        if (epoll_ctl(this->poll_fd, EPOLL_CTL_MOD, action_fd, &event) == -1)
            std::cerr << "Erro " << strerror(errno) << std::endl;
        
        // Limpar request/response mas manter conexão
        client[pos].req->clear();
        client[pos].res->clear();
        client[pos].last_activity = time(NULL); // atualizar timestamp
    }
}
```

**Risco:** Baixo

---

#### 3.9 **UPLOAD ROBUSTO COM LIMITES**
**Estado:** ⚠️ Básico implementado

**Ficheiros a editar:**
- `src/http/Method.cpp` (`postMethod()`)
- `src/utils/helping_methods.cpp` (`extractFileFromMultipart()`)

**Melhorias:**
- Validar `Content-Length` ANTES de ler body
- Validar boundary existence
- Validar MIME types permitidos
- Validar espaço em disco

**Risco:** Baixo

---

#### 3.10 **AUTOINDEX COM ORDENAÇÃO E TAMANHOS**
**Estado:** ⚠️ Básico

**Ficheiros a editar:**
- `src/http/Autoindex.cpp` (`generate()`)

**Melhorias:**
```cpp
// Adicionar struct para entries
struct DirEntry {
    std::string name;
    bool is_dir;
    off_t size;
    time_t mtime;
};

// Ordenar alfabeticamente
std::sort(entries.begin(), entries.end(), ...);

// Exibir tamanho e data no HTML
html << "<li><a href=\"" << link << "\">" << name << "</a> "
     << "(" << formatSize(entry.size) << ", " 
     << formatTime(entry.mtime) << ")</li>";
```

**Risco:** Baixo

---

### 🟢 PRIORIDADE MÉDIA (Polimento)

#### 3.11 **URL ENCODING/DECODING**
- `src/utils/utils.cpp` - adicionar `urlDecode()`
- Necessário para paths com espaços ou caracteres especiais

#### 3.12 **HTML ESCAPING EM AUTOINDEX**
- Prevenir XSS em nomes de ficheiros

#### 3.13 **STREAMING DE FICHEIROS GRANDES**
- Ler ficheiros em chunks em vez de carregar tudo na memória
- `src/http/Response.cpp` - `sendFile()` com loop `read()` + `send()`

#### 3.14 **EPOLLRDHUP PARA DETECÇÃO DE DESCONEXÃO**
- `src/core/Multiplexer.cpp` - adicionar flag `EPOLLRDHUP`

#### 3.15 **VALIDAÇÃO DE MÉTODOS HTTP**
- `src/http/Request.cpp` - validar se método é GET/POST/DELETE/HEAD/etc.

---

## 🎯 4. CHECKLIST FINAL PARA CONCLUSÃO

### ✅ FASE 1: CORREÇÕES CRÍTICAS (1-2 dias)
1. ☐ Implementar **return directive** em `Method::executeMethod()`
2. ☐ Implementar **405 Allow header**
3. ☐ Implementar **HEAD method**
4. ☐ Corrigir **keep-alive loop** (voltar a EPOLLIN)

### ✅ FASE 2: CGI (3-4 dias)
5. ☐ Criar `CgiHandler` class básica
6. ☐ Implementar `fork()` + `execve()` + pipes
7. ☐ Implementar environment variables CGI
8. ☐ Implementar timeout de CGI com `alarm()` ou `select()`
9. ☐ Parse CGI headers (`Status:`, `Content-Type:`)
10. ☐ Integrar CGI em `Method::getMethod()` e `postMethod()`
11. ☐ Testar com `python-cgi` e `php-cgi`

### ✅ FASE 3: CHUNKED ENCODING (2-3 dias)
12. ☐ Implementar parse de **chunked requests** em `Request.cpp`
13. ☐ Implementar envio de **chunked responses** em `Response.cpp`
14. ☐ Testar com `curl --data-binary @large_file.bin --header "Transfer-Encoding: chunked"`

### ✅ FASE 4: TIMEOUTS (1-2 dias)
15. ☐ Adicionar `last_activity` timestamp a `t_http`
16. ☐ Implementar verificação de timeout no loop principal
17. ☐ Enviar `408 Request Timeout` para clientes lentos
18. ☐ Testar com `telnet` + esperar 30+ segundos

### ✅ FASE 5: POLIMENTO (2-3 dias)
19. ☐ Implementar URL decoding para paths
20. ☐ Implementar HTML escaping em autoindex
21. ☐ Implementar streaming de ficheiros grandes
22. ☐ Adicionar ordenação e tamanhos ao autoindex
23. ☐ Validar métodos HTTP inválidos (501 Not Implemented)
24. ☐ Adicionar `EPOLLRDHUP` para detecção de desconexão

### ✅ FASE 6: TESTES E VALIDAÇÃO (2-3 dias)
25. ☐ Testar com `siege` / `ab` (Apache Bench)
26. ☐ Testar com `curl` (todos os métodos + chunked)
27. ☐ Testar com navegador (keep-alive, autoindex, uploads)
28. ☐ Testar CGI com PHP e Python
29. ☐ Testar com ficheiros grandes (>100MB)
30. ☐ Testar memory leaks com `valgrind`
31. ☐ Testar configs complexos (múltiplos servers, locations)

---

## 📊 5. ESTATÍSTICAS DO PROJETO

| Métrica | Valor |
|---------|-------|
| **Ficheiros .cpp** | 15 |
| **Ficheiros .hpp** | 11 |
| **Linhas de código (estimativa)** | ~3500 |
| **Classes principais** | 10 |
| **Funcionalidades completas** | 60% |
| **Funcionalidades parciais** | 25% |
| **Funcionalidades ausentes** | 15% |
| **Conformidade HTTP/1.1** | ~70% |
| **Dias estimados para conclusão** | 12-18 dias |

---

## ⚠️ 6. RISCOS TÉCNICOS E IMPACTO

| Risco | Impacto | Probabilidade | Mitigação |
|-------|---------|---------------|-----------|
| **CGI timeout não funcional** | Alto | Média | Testar com scripts longos, usar `select()` com timeout |
| **Memory leaks em CGI** | Alto | Média | Valgrind após cada teste |
| **Chunked parsing incorreto** | Médio | Alta | Testar com `curl` e ficheiros variados |
| **Keep-alive break requests** | Médio | Média | Testar com navegador e múltiplas requests |
| **Timeouts desconectam prematuramente** | Baixo | Baixa | Ajustar timeout para 60s |
| **Path traversal em CGI** | Alto | Baixa | Validar paths com `realpath()` |

---

## 🏁 7. CONCLUSÃO

O projeto está **substancialmente avançado** com fundações sólidas em **epoll**, **config parsing** e **HTTP básico**. As lacunas principais são:

1. **CGI** (crítico - 0% implementado)
2. **Return directives** (crítico - parseado mas não executado)
3. **Chunked encoding** (crítico para HTTP/1.1)
4. **Timeouts** (crítico para estabilidade)

Com **12-18 dias de desenvolvimento focado**, seguindo a checklist acima, o projeto pode atingir **95%+ de conformidade HTTP/1.1** e passar nos testes obrigatórios da 42.

**Priorize FASE 1 e FASE 2** - elas desbloqueiam 80% das funcionalidades faltantes.

Boa sorte! 🚀

---

**Documento gerado em:** 2 de Dezembro de 2025  
**Versão:** 1.0  
**Branch:** feat/kelson
