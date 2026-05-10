#include "../../include/http/Method.hpp"
#include "../../include/http/Autoindex.hpp"

void    Method::handle_errors_status_pages(int code, const t_server *client)
{
    std::map<int, std::string> erros = client->error_pages;
    std::map<int, std::string>::iterator it = erros.find(code);
    support_keepAlive();
    if (it != erros.end() && !it->second.empty())
    {
        std::string error_path = root + it->second;
        struct  stat    error_info;
        if (stat(error_path.c_str(), &error_info) == 0 && S_ISREG(error_info.st_mode) && access(error_path.c_str(), R_OK) == 0)
        {
            this->response->generateErrorPage(error_path, error_info, code);
            return ;
        }
        this->response->generateErrorPage(code);
        return ;
    }
    this->response->generateErrorPage(code);
}

void    Method::handle_errors_status_pages_closing(int code, const t_server *client)
{
    std::map<int, std::string> erros = client->error_pages;
    std::map<int, std::string>::iterator it = erros.find(code);
    if (it != erros.end() && !it->second.empty())
    {
        std::string error_path = root + it->second;
        struct  stat    error_info;
        if (stat(error_path.c_str(), &error_info) == 0 && S_ISREG(error_info.st_mode) && access(error_path.c_str(), R_OK) == 0)
        {
            this->response->generateErrorPage(error_path, error_info, code);
            return ;
        }
        this->response->generateErrorPage(code);
        return ;
    }
    this->response->generateErrorPage(code);
}

const t_location  *Method::match_location(const std::string &path, const t_server *client)
{
    const t_location *best = NULL;

    for (size_t i = 0; i < client->locations.size(); i++)
    {
        const std::string &loc_path = client->locations[i].path;
        if (loc_path == "/")
        {
            if (!best || best->path.size() < 1)
                best = &client->locations[i];
            continue ;
        }
        if (path.compare(0, loc_path.size(), loc_path) == 0)
        {
            if (path.size() == loc_path.size() || path[loc_path.size()] == '/')
            {
                if (!best || loc_path.length() > best->path.length())
                    best = &client->locations[i];
            }
        }
    }
    return (best);
}

void Method::handle_redirection(const t_location *aux)
{
    response->setStatus(aux->return_directive.code);
    response->setHeader("Location", aux->return_directive.url);
    if (this->request->getHeader("Connection").compare("keep-alive") == 0)
    {
        this->response->setHeader("Connection", "keep-alive");
        this->response->setKeepAlive(true);
    }
    else
    {
        this->response->setHeader("Connection", "close");
        this->response->setKeepAlive(false);
    }
    response->buildResponse();
}

void Method::handle_redirection(const t_server *client)
{
    response->setStatus(client->return_directive.code);
    if (!client->return_directive.url.empty())
        response->setHeader("Location", client->return_directive.url);
    response->setContentType("text/html");
    if (this->request->getHeader("Connection").compare("keep-alive") == 0)
    {
        this->response->setHeader("Connection", "keep-alive");
        this->response->setKeepAlive(true);
    }
    else
    {
        this->response->setHeader("Connection", "close");
        this->response->setKeepAlive(false);
    }
    this->response->setHttpVersion(this->request->getHttpVersion());
    response->buildResponse();
}

// Função para a construção do autoindex com melhor integração dos erros e permissões
void    Method::building_autoindex(const t_location *loc, std::string &full_path, const t_server *client)
{
    bool autoindex_enabled = true;

    if (loc)
        autoindex_enabled = loc->autoindex;
    if (autoindex_enabled)
    {
        std::string req_path = request->getPath();
        std::string html = Autoindex::generate(full_path, req_path, client->error_pages, response);
        response->setBody(html);
        response->setHeader("Content-type", "text/html");
    }
    else
    {
        handle_errors_status_pages(403, client);
    }
}

size_t findBytes(const std::vector<char> &data, const std::vector<char> &pattern, size_t start)
{
    if (pattern.size() == 0 || data.size() < pattern.size())
        return std::string::npos;

    for (size_t i = start; i <= data.size() - pattern.size(); ++i)
    {
        bool match = true;
        for (size_t j = 0; j < pattern.size(); ++j)
        {
            if (data[i + j] != pattern[j])
            {
                match = false;
                break;
            }
        }
        if (match)
            return i;
    }
    return std::string::npos;
}

// extrai o arquivo do campo "file" do body multipart/form-data
bool extractFileFromMultipart(const std::vector<char> &body, const std::string &boundary, std::vector<char> &fileData)
{
    std::vector<char> boundaryBytes;
    for (size_t i = 0; i < boundary.size(); ++i)
        boundaryBytes.push_back(boundary[i]);

    // criar vector<char> do \r\n\r\n
    std::vector<char> crlfcrlf;
    crlfcrlf.push_back('\r');
    crlfcrlf.push_back('\n');
    crlfcrlf.push_back('\r');
    crlfcrlf.push_back('\n');

    size_t pos = findBytes(body, boundaryBytes, 0);
    if (pos == std::string::npos)
        return false; // boundary não encontrado

    pos += boundaryBytes.size(); // pular boundary

    // encontrar início do conteúdo real do arquivo
    size_t header_end = findBytes(body, crlfcrlf, pos);
    if (header_end == std::string::npos)
        return false; // headers da parte não encontrados

    size_t content_start = header_end + crlfcrlf.size();

    // encontrar próximo boundary → final do arquivo
    size_t next_boundary = findBytes(body, boundaryBytes, content_start);
    if (next_boundary == std::string::npos)
        return false; // boundary final não encontrado

    // copiar bytes do arquivo para vector
    fileData.clear();
    fileData.insert(fileData.end(), body.begin() + content_start, body.begin() + next_boundary - 2); 
    // -2 para remover o \r\n antes do boundary

    return true;
}


// função para extrair filename do campo "file"
bool extractFilenameFromMultipart(const std::vector<char> &body, const std::string &boundary, std::string &filename)
{
    std::vector<char> boundaryBytes;
    for (size_t i = 0; i < boundary.size(); ++i)
        boundaryBytes.push_back(boundary[i]);

    // encontrar início da primeira parte
    size_t pos = findBytes(body, boundaryBytes, 0);
    if (pos == std::string::npos)
        return false;
    pos += boundaryBytes.size();

    // pular todos os CRLFs após o boundary
    while (pos + 1 < body.size() && body[pos] == '\r' && body[pos+1] == '\n')
        pos += 2;

    // agora pos aponta para o começo da linha do Content-Disposition
    size_t line_end = pos;
    while (line_end + 1 < body.size() && !(body[line_end] == '\r' && body[line_end+1] == '\n'))
        ++line_end;

    if (line_end + 1 >= body.size())
        return false;

    // copiar para string
    std::string header_line;
    for (size_t i = pos; i < line_end; ++i)
        header_line += body[i];

    // procurar filename="..."
    size_t fpos = header_line.find("filename=\"");
    if (fpos == std::string::npos)
        return false;

    fpos += 10; // pular 'filename="'
    size_t fend = header_line.find("\"", fpos);
    if (fend == std::string::npos)
        return false;

    filename = header_line.substr(fpos, fend - fpos);
    return true;
}

std::string    generate_random_session_id()
{
    const char digits[] = "abcdef1234567890";
    std::string ret;

    for (int i = 0; i < 6; i++)
        ret += digits[std::rand() % 16];    
    return (ret);
}

void fill_session_structure(const std::vector<char> &body, Session &s, const std::string &boundary)
{
    std::string data(body.data(), body.size());

    size_t pos = 0;
    while (1)
    {
        size_t start = data.find(boundary, pos);
        if (start == std::string::npos)
            break;
        start += boundary.size();

        if (data.substr(start, 2) == "--")
            break;

        if (data.substr(start, 2) == "\r\n")
            start += 2;

        size_t cd_pos = data.find("Content-Disposition:", start);
        if (cd_pos == std::string::npos)
            break;

        size_t name_pos = data.find("name=\"", cd_pos);
        if (name_pos == std::string::npos)
            break;
        name_pos += 6;

        size_t name_end = data.find("\"", name_pos);
        std::string field_name = data.substr(name_pos, name_end - name_pos);

        size_t value_start = data.find("\r\n\r\n", name_end);
        if (value_start == std::string::npos)
            break;
        value_start += 4;

        size_t value_end = data.find(boundary, value_start);
        if (value_end == std::string::npos)
            break;

        std::string value = data.substr(value_start, value_end - value_start);

        if (!value.empty() && value[value.length() - 1] == '\n')
            value.erase(value.size() - 1);
        if (!value.empty() && value[value.length() - 1] == '\r')
            value.erase(value.size() - 1);

        if (field_name == "username")
            s.username = value;
        else if (field_name == "role")
            s.role = value;

        // continuar o loop
        pos = value_end;
    }
}

void    Method::handle_cookies(std::string boundary)
{
    Session aux;

    std::string session_id = generate_random_session_id();
    fill_session_structure(request->getBody(), aux, boundary);
    aux.last_access = std::time(NULL);
    sessions[session_id] = aux;
    response->setHeader("Set-Cookie", "session_id=" + session_id + "; Path=/; HttpOnly; Max-Age=60");
    response->setStatus(302);
    response->setHeader("Location", "/dashboard");
    response->setContentType("text/html");
    response->setBody("");
    
    // Adicionar suporte a Connection keep-alive
    if (this->request->getHeader("Connection").compare("keep-alive") == 0)
    {
        this->response->setHeader("Connection", "keep-alive");
        this->response->setKeepAlive(true);
    }
    else
    {
        this->response->setHeader("Connection", "close");
        this->response->setKeepAlive(false);
    }
    this->response->setHttpVersion(this->request->getHttpVersion());
    
    response->buildResponse();
}

void    Method::support_keepAlive()
{
    if (this->request->getHeader("Connection").compare("") == 0)
    {
        this->response->setHeader("Connection", "keep-alive");
        this->response->setKeepAlive(true);
    }
    else if (this->request->getHeader("Connection").compare("keep-alive") == 0)
    {
        this->response->setHeader("Connection", "keep-alive");
        this->response->setKeepAlive(true);
    }
    else
    {
        this->response->setHeader("Connection", "close");
        this->response->setKeepAlive(false);
    }
    this->response->setHttpVersion(this->request->getHttpVersion());
    
    response->buildResponse();
}

void    Method::handle_session_with_cookies(std::string new_location)
{
    response->setStatus(302);
    response->setHeader("Location", new_location);
    response->setContentType("text/html");
    if (this->request->getHeader("Connection").compare("keep-alive") == 0)
    {
        this->response->setHeader("Connection", "keep-alive");
        this->response->setKeepAlive(true);
    }
    else
    {
        this->response->setHeader("Connection", "close");
        this->response->setKeepAlive(false);
    }
    this->response->setHttpVersion(this->request->getHttpVersion());
    response->buildResponse();
}

static std::string  get_extension(std::string content)
{
    std::string ext;
    if (content.find("application/json") != std::string::npos)
        ext = ".json";
    else if (content.find("text/plain") != std::string::npos)
        ext = ".txt";
    else if (content.find("image/jpeg") != std::string::npos)
        ext = ".jpg";
    else if (content.find("image/png") != std::string::npos)
        ext = ".png";
    else if (content.find("text/html") != std::string::npos)
        ext = ".html";
    else if (content.find("application/xml") != std::string::npos)
        ext = ".xml";
    return (ext);
}

std::string Method::get_final_filename()
{
    std::vector<char> body = request->getBody();
    std::string path = request->getPath();
    std::string content_type = request->getHeader("Content-Type").empty() ? request->getHeader("content-type") : request->getHeader("Content-Type");
    std::string filename;
    
    if (filename.empty())
    {
        size_t slash = path.find_last_of('/');
        if (slash != std::string::npos && slash + 1 < path.length())
            filename += path.substr(slash + 1) + "_";
        else
            filename += "post_";
    }
    time_t t = std::time(NULL);
    std::string extension = get_extension(content_type);
    std::ostringstream tmpname;
    tmpname << filename << t << extension;
    filename = tmpname.str();
    return (filename);
}
