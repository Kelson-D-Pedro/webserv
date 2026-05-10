#include "../../include/http/Response.hpp"

Response::Response()
    : http_version(""),
      reason_phrase(""),
      headers(),
      body(""),
      full_response(""),
      status_code(200),
      keep_alive(false),
      file_path(""),
      is_cgi(false),
      content_length(0),
      use_chunked(false),
      flag(false)
{
}

Response::~Response() {}

void Response::setStatus(int code)
{
    status_code = code;
}

void Response::setBody(std::string bod)
{
    body = bod;
}

void Response::setHeader(const std::string &key, const std::string &value)
{
    headers[key] = value;
}

void Response::setContentType(const std::string &type)
{
    setHeader("Content-Type", type);
}

void Response::setHttpVersion(const std::string &version)
{
    http_version = version;
}

void Response::setKeepAlive(bool condition)
{
    keep_alive = condition;
}

void Response::setUseChunked(bool chunked)
{
    use_chunked = chunked;
}

bool Response::getKeepAlive() const
{
    return keep_alive;
}

int Response::getStatus() const
{
    return status_code;
}

std::string Response::getHeader(std::string key)
{
    std::map<std::string, std::string>::iterator it = headers.find(key);
    if (it != headers.end())
        return it->second;
    return "";
}

std::string Response::getBody() const
{
    return body;
}

std::string Response::getFullResponse() const
{
    return full_response;
}

bool Response::getUseChunked() const
{
    return use_chunked;
}


void Response::buildResponse()
{
    reason_phrase = HttpStatus::getMessage(status_code);
    std::string response = "HTTP/1.1 " + intToString((size_t)status_code) + " " + reason_phrase + "\r\n";

    // Se usar chunked, adicionar header Transfer-Encoding e remover Content-Length
    if (use_chunked)
    {
        headers.erase("Content-Length");
        setHeader("Transfer-Encoding", "chunked");
    }
    else
    {
        // Para respostas normais (não chunked), garantir Content-Length
        if (headers.find("Content-Length") == headers.end())
            setHeader("Content-Length", intToString(body.length()));
    }
    for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it)
    {
        response += it->first + ": " + it->second + "\r\n";
    }
    response += "\r\n";
    response += body;
    
    full_response = response;
}

void Response::sendResponse(int client_fd)
{
    const char *data = full_response.c_str();
    size_t total_bytes = full_response.size();
    size_t bytes_sent = 0;

    while (bytes_sent < total_bytes)
    {
        ssize_t n = send(client_fd, data + bytes_sent, total_bytes - bytes_sent, 0);
        if (n == -1)
        {
            this->flag = true;
            break;
        }
        else if (n == 0)
            break;
        bytes_sent += n;
    }
}

void Response::generateErrorPage(int code)
{
    setStatus(code);
    reason_phrase = HttpStatus::getMessage(code);
    body = "<html><body><h1>" + intToString(code) + " - " + reason_phrase + "</h1></body></html>";
    setHeader("Content-Length", intToString(body.length()));
    setHeader("Content-Type", "text/html");
}

void Response::generateErrorPage(const std::string &path, const struct stat file_info, int code)
{
    int fd;
    size_t file_size = static_cast<size_t>(file_info.st_size);
    
    fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
    {
        generateErrorPage(code);
        return;
    }
    
    // Use dynamic allocation instead of VLA for C++ standard compliance
    this->body.resize(file_size);
    ssize_t bytes_read = read(fd, &this->body[0], file_size);
    close(fd);
    
    if (bytes_read > 0)
        this->body.resize(static_cast<size_t>(bytes_read));
    else if (bytes_read == 0)
        this->body.clear();
    else if (bytes_read < 0)
        this->body.clear();
    
    setStatus(code);
    this->reason_phrase = HttpStatus::getMessage(code);
    setHeader("Content-Type", MimeTypes::getContentType(path));
    setHeader("Content-Length", intToString(this->body.length()));
}

void Response::setFileResponse(const std::string &path, const struct stat file_info)
{
    int fd;
    size_t file_size = static_cast<size_t>(file_info.st_size);

    fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
    {
        generateErrorPage(500);
        return;
    }
    
    // Se ficheiro for maior que 1MB, usar chunked
    if (file_size > 1024 * 1024)
    {
        use_chunked = true;
    }
    
    // Use dynamic allocation instead of VLA for C++ standard compliance
    this->body.resize(file_size);
    ssize_t bytes_read = read(fd, &this->body[0], file_size);
    close(fd);
    
    if (bytes_read > 0)
        this->body.resize(static_cast<size_t>(bytes_read));
    else if (bytes_read == 0)
        this->body.clear();
    else if (bytes_read < 0)
        this->body.clear();
    
    setStatus(200);
    this->reason_phrase = HttpStatus::getMessage(200);
    setHeader("Content-Type", MimeTypes::getContentType(path));
    
    if (!use_chunked)
        setHeader("Content-Length", intToString(this->body.length()));
}

void Response::prepareCGIResponse(const std::string &cgi_output)
{
    size_t sep = cgi_output.find("\r\n\r\n");
    size_t sep_len = 4;
    if (sep == std::string::npos)
    {
        sep = cgi_output.find("\n\n");
        sep_len = 2;
    }
    std::string raw_headers;
    std::string body_part;
    if (sep != std::string::npos)
    {
        raw_headers = cgi_output.substr(0, sep);
        body_part = cgi_output.substr(sep + sep_len);
    }
    else
        body_part = cgi_output;
    std::istringstream stream(raw_headers);
    std::string line;
    while (getline(stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue ;
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        while (!value.empty() && value[0] == ' ')
            value.erase(0, 1);
        if (key == "Status")
            setStatus(std::atoi(value.c_str()));
        else
            setHeader(key, value);
    }
    body = body_part;
    std::string buf;
    buf = intToString(body.size());
    setHeader("Content-Length", buf);
    if (getHeader("Content-Type").empty() && getHeader("content-type").empty())
        setHeader("Content-Type", "text/html");
    is_cgi = true;
}

void Response::clear()
{
    http_version = "HTTP/1.1";
    reason_phrase = "OK";
    status_code = 200;
    headers.clear();
    body.clear();
    full_response.clear();
    file_path.clear();
    keep_alive = false;
    is_cgi = false;
    content_length = 0;
    use_chunked = false;
}

void Response::buildHeaders()
{
    std::string response = http_version + " " + intToString((size_t)status_code) + " " + reason_phrase + "\r\n";

    for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it)
    {
        response += it->first + ": " + it->second + "\r\n";
    }
    response += "\r\n";
    
    full_response = response;
}

void Response::sendFile(int client_fd)
{
    // Se usar chunked, tratar diferente
    if (use_chunked)
    {
        // Construir apenas headers para chunked
        std::string response = http_version + " " + intToString((size_t)status_code) + " " + reason_phrase + "\r\n";
        
        setHeader("Transfer-Encoding", "chunked");
        headers.erase("Content-Length");
        
        for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it)
        {
            response += it->first + ": " + it->second + "\r\n";
        }
        response += "\r\n";
        
        // Enviar headers primeiro
        send(client_fd, response.c_str(), response.length(), 0);
        
        // Enviar body chunked
        sendChunked(client_fd);
    }
    else
    {
        // Resposta normal
        buildResponse();
        sendResponse(client_fd);
    }
}

void Response::sendChunked(int client_fd)
{
    size_t pos = 0;
    const size_t chunk_size = 8192;  // Aumentado para 8KB
    char size_buf[32];

    while (pos < body.size())
    {
        size_t size = (body.size() - pos) > chunk_size ? chunk_size : (body.size() - pos);
        
        // Enviar tamanho do chunk em hexadecimal
        //sprintf(size_buf, "%lx\r\n", (unsigned long)size);
        ssize_t n = send(client_fd, size_buf, strlen(size_buf), 0);
        if (n < 0)
            break;
        
        // Enviar dados do chunk
        n = send(client_fd, body.c_str() + pos, size, 0);
        if (n < 0)
            break;
        
        // Enviar CRLF após chunk
        n = send(client_fd, "\r\n", 2, 0);
        if (n < 0)
            break;
        
        pos += size;
    }
    
    // Enviar chunk final (tamanho 0)
    send(client_fd, "0\r\n\r\n", 5, 0);
}
