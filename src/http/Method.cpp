#include "../../include/http/Method.hpp"
#include "../../include/http/Autoindex.hpp"
# include "../../include/utils/utils.hpp"
# include "../../include/cgi/CgiHandler.hpp"

//std::map<std::string, Session> Method::sessions;

Method::Method(Requests* req, Response* res, const std::string& root_dir, std::map<std::string, Session> &session_f, int poll_fd)
    : request(req), response(res), root(root_dir), sessions(session_f), poll_fd(poll_fd), cgi(NULL)
{}
Method::~Method(){}

static std::string get_parent_dir(const std::string &full_path)
{
    size_t pos = full_path.find_last_of('/');
    if (pos == std::string::npos || pos == 0)
        return ("/");
    return (full_path.substr(0, pos));
}

void    Method::getMethod(const t_server *client)
{
    const t_location *aux = match_location(request->getPath(), client);
    // 1. Verificar return directive nos níveis de location e server PRIMEIRO
    if (handle_redirections_if_needed(client, aux))
        return ;

    if (aux != NULL && aux->path == "/login")
    {
        const std::string &id = request->getCookieID();
        if (!id.empty() && sessions.count(id))
        {
            handle_session_with_cookies("/dashboard");
            return ;
        }
    }

    if (aux != NULL && aux->path == "/dashboard")
    {
        const std::string &id = request->getCookieID();
        if (id.empty() || sessions.find(id) == sessions.end())
        {
            handle_session_with_cookies("/login");
            return ;
        }
    }
    std::string root_aux = aux ? aux->root : root;
    size_t prefix_len = aux ? aux->path.length() : 0;
    if (prefix_len > request->getPath().length())
        prefix_len = request->getPath().length();
    std::string rel = request->getPath().substr(prefix_len);
    if (rel.empty())
        rel = "/";
    else if (!rel.empty() && rel[rel.length() - 1] != '/')
    {
        std::string aux;
        if (!rel.empty() && rel[0] != '/' && root_aux[root_aux.length() - 1] != '/')
        {
            aux = "/" + rel;
            rel = aux;
        }
    }
    //std::map<std::string, t_cgi>::const_iterator it = aux->cgis.begin();
    
    // 3. Verificar se método é permitido
    if (!method_allowed(client, aux))
        return ;
    
    
    std::string full_path = root_aux + rel;
    struct  stat    file_info;
    if (stat(full_path.c_str(), &file_info) == -1)
    {
        handle_errors_status_pages(404, client);
        response->buildResponse();
        return ;
    }
    if (S_ISREG(file_info.st_mode))
    {
        // Verificar se é um script CGI
        std::string extension;
        size_t dot_pos = full_path.find_last_of('.');
        if (dot_pos != std::string::npos)
            extension = full_path.substr(dot_pos);
        
        if (aux && !extension.empty() && aux->cgis.count(extension))
        {
            handle_cgi_if_needed_for_get(client, aux, full_path, extension, root_aux);
        }
        else
        {
            // Ficheiro estático normal
            this->response->setFileResponse(full_path, file_info);
        }
    }
    else if (S_ISDIR(file_info.st_mode))
    {
        // Normalizar path do diretório
        std::string dir_path = full_path;
        if (dir_path[dir_path.length() - 1] != '/')
            dir_path += "/";
        
        // Tentar encontrar um index file válido
        bool index_found = false;
        struct stat index_info;
        
        // 1. Tentar indexes da location
        if (aux && !aux->indexes.empty())
        {
            for (size_t i = 0; i < aux->indexes.size(); ++i)
            {
                std::string index_path = dir_path + aux->indexes[i];
                if (stat(index_path.c_str(), &index_info) == 0 && S_ISREG(index_info.st_mode))
                {
                    this->response->setFileResponse(index_path, index_info);
                    index_found = true;
                    break;
                }
            }
        }
        // 2. Tentar indexes do servidor se location não tem
        else if (!client->indexes.empty())
        {
            for (size_t i = 0; i < client->indexes.size(); ++i)
            {
                std::string index_path = dir_path + client->indexes[i];
                if (stat(index_path.c_str(), &index_info) == 0 && S_ISREG(index_info.st_mode))
                {
                    this->response->setFileResponse(index_path, index_info);
                    index_found = true;
                    break;
                }
            }
        }
        
        // 3. Se nenhum index encontrado, usar autoindex (se habilitado)
        if (!index_found)
            building_autoindex(aux, full_path, client);
    }
    else
    {
        handle_errors_status_pages(403, client);
    }
    support_keepAlive();
}

void Method::postMethod(const t_server *client)
{
    const t_location *aux = match_location(request->getPath(), client);
    
    if (!aux)
    {
        handle_errors_status_pages(404, client);
        response->buildResponse();
        return;
    }
    // Verificar return directive no nível de location
    if (handle_redirections_if_needed(client, aux))
        return ;
    
    // Verificar se método é permitido
    if (!method_allowed(client, aux))
        return ;
    // Verificar se é um script CGI antes de processar upload
    std::string root_aux = aux ? aux->root : root;
    size_t prefix_len = aux ? aux->path.length() : 0;
    if (prefix_len > request->getPath().length())
        prefix_len = request->getPath().length();
    std::string rel = request->getPath().substr(prefix_len);
    if (rel.empty())
        rel = "/";
    else if (!rel.empty() && rel[rel.length() - 1] != '/')
    {
        std::string aux;
        if (!rel.empty() && rel[0] != '/' && root_aux[root_aux.length() - 1] != '/')
        {
            aux = "/" + rel;
            rel = aux;
        }
    }
    std::string full_path = root_aux + rel;
    struct stat st;
    if (stat(full_path.c_str(), &st) != 0)
    {
        handle_errors_status_pages(404, client);
        response->buildResponse();
        return ;
    }
    
    if (handle_cgi_if_needed_for_post(client, aux, full_path, root_aux))
        return;
    
    // Se não é CGI, processar como upload
    if (validate_upload_headers(client, aux))
        return ;
    
    std::string uploads_dir = aux ? aux->upload_path : "";
    // Validar boundary existence
    std::string contentType = request->getHeader("Content-Type");
    std::vector<char> body = request->getBody();
    std::vector<char> final_data;
    size_t max_body_size = aux && aux->client_max_body_size ? aux->client_max_body_size : MAX_UPLOAD_SIZE;
    std::string filename;
    std::string target;
    size_t body_size;
    if (contentType.find("multipart/form-data") != std::string::npos)
    {
        std::string boundary;
        size_t pos = contentType.find("boundary=");
        if (pos == std::string::npos)
        {
            handle_errors_status_pages(400, client);
            response->buildResponse();
            return;
        }
        boundary = "--" + contentType.substr(pos + 9);
        
        // Validar Content-Length ANTES de ler body
        std::string contentLength = request->getHeader("Content-Length");
        if (!contentLength.empty())
        {
            size_t declared_size = std::atol(contentLength.c_str());
            std::cout << "content length: " << declared_size << std::endl;
            std::cout << "client max body size: " << max_body_size << std::endl;
            if (declared_size > max_body_size)
            {
                handle_errors_status_pages(413, client);
                response->buildResponse();
                return;
            }
        }
        if (aux->path == "/login" && sessions.find(request->getCookieID()) == sessions.end())
        {
            handle_cookies(boundary);
            return ;
        }
        
        extractFilenameFromMultipart(body, boundary, filename);
        if (aux->upload_enabled == false && !filename.empty())
        {
            handle_errors_status_pages(403, client);
            response->buildResponse();
            return;
        }
        if (aux->upload_path.empty() && aux->upload_enabled)
        {
            handle_errors_status_pages(403, client);
            response->buildResponse();
            return;
        }
        if (filename.empty())
        {
            handle_errors_status_pages(400, client);
            response->buildResponse();
            return ;
        }
        std::string safe_name = sanitize_filename(filename);
        target = uploads_dir;
        if (target[target.length() - 1] != '/') 
            target += '/';
        target += safe_name;
        if (target.compare(0, root.size(), root) != 0)
        {
            handle_errors_status_pages(403, client);
            response->buildResponse();
            return;
        }
        if (!is_path_within_root(root, target.substr(root.size())))
        {
            handle_errors_status_pages(403, client);
            response->buildResponse();
            return;
        }
        std::vector<char> fileData;
        if (!extractFileFromMultipart(body, boundary, fileData))
        {
            handle_errors_status_pages(500, client);
            response->buildResponse();
            return ;
        }
        body_size = fileData.size();
        final_data = fileData;
    }
    else
    {
        filename = get_final_filename();
        std::string safe_name = sanitize_filename(filename);
        std::string contentLength = request->getHeader("Content-Length");
        if (!contentLength.empty())
        {
            size_t declared_size = std::atol(contentLength.c_str());
            std::cout << "content length: " << declared_size << std::endl;
            std::cout << "client max body size: " << max_body_size << std::endl;
            if (declared_size > max_body_size)
            {
                handle_errors_status_pages(413, client);
                response->buildResponse();
                return;
            }
        }
        target = uploads_dir;
        if (target.empty())
            target = aux->root;
        if (target[target.length() - 1] != '/') 
            target += '/';
        target += safe_name;
        if (!is_path_within_root(root, target.substr(root.size())))
        {
            handle_errors_status_pages(403, client);
            response->buildResponse();
            return;
        }
        body_size = body.size();
        final_data = body;
    }
    std::cout << "body size: " << body_size << std::endl;
    if (validate_body_length(client, body_size, max_body_size))
        return ;
    time_t t = std::time(NULL);
    int rnd = std::rand();
    std::ostringstream tmpname;

    tmpname << target << ".tmp." << t << "." << rnd;
    std::string tmp = tmpname.str();

    int fd = open(tmp.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);
    if (fd < 0)
    {
        handle_errors_status_pages(500, client);
        response->buildResponse();
        return ;
    }
    size_t to_write = body_size;
    size_t written_total = 0;

    while (to_write > 0)
    {
        ssize_t w = write(fd, final_data.data() + written_total, to_write);
        if (w < 0 || w == 0)
        {
            close(fd);
            std::remove(tmp.c_str());
            handle_errors_status_pages(500, client);
            response->buildResponse();
            return ;
        }
        written_total += w;
        to_write -= w;
    }
    //fsync(fd);
    close(fd);

    if (std::rename(tmp.c_str(), target.c_str()) != 0)
    {
        std::remove(tmp.c_str());
        handle_errors_status_pages(500, client);
        response->buildResponse();
        return ;
    }
    response->setStatus(201);
    response->setHeader("Location", request->getPath());
    response->setContentType("text/html");
    
    // Adicionar suporte a Connection keep-alive
    support_keepAlive();
}

void Method::deleteMethod(const t_server *client)
{
    const t_location *aux = match_location(request->getPath(), client);
    struct stat st;
    
    // Verificar return directive no nível de location
    if (!aux)
    {
        std::cout << "NULL\n";
    }
    if (aux && aux->return_directive.code != 0)
    {
        handle_redirection(aux);
        return;
    }
    
    // Verificar return directive no nível de server
    if (client->return_directive.code != 0)
    {
        handle_redirection(client);
        return;
    }
    if (aux && std::find(aux->allow_methods.begin(), aux->allow_methods.end(), request->getMethod()) == aux->allow_methods.end())
    {
        // Adicionar header Allow com métodos permitidos (RFC 7231)
        std::string allow;
        for (size_t i = 0; i < aux->allow_methods.size(); ++i)
        {
            if (i > 0) allow += ", ";
            allow += aux->allow_methods[i];
        }
        response->setHeader("Allow", allow);
        handle_errors_status_pages(405, client);
        response->buildResponse();
        return  ;
    }
    
    std::string root_aux = aux ? aux->root : root;
    size_t prefix_len = aux ? aux->path.length() : 0;
    if (prefix_len > request->getPath().length())
        prefix_len = request->getPath().length();
    std::string rel = request->getPath().substr(prefix_len);
    if (rel.empty())
        rel = "/";
    else if (!rel.empty() && rel[rel.length() - 1] != '/')
    {
        std::string aux;
        if (!rel.empty() && rel[0] != '/' && root_aux[root_aux.length() - 1] != '/')
        {
            aux = "/" + rel;
            rel = aux;
        }
    }
    std::string full_path = root_aux + rel;
    std::string parent = get_parent_dir(full_path);
    if (stat(full_path.c_str(), &st) != 0)
    {
        handle_errors_status_pages(404, client);
        response->buildResponse();
        return ;       
    }
    
    if (rel == "/" || full_path == root_aux)
    {
        handle_errors_status_pages(403, client);
        response->buildResponse();
        return ;
    }
    if (aux->upload_path.empty() || !is_path_inside_upload_path(aux->upload_path, full_path))
    {
        handle_errors_status_pages(403, client);
        response->buildResponse();
        return ;
    }
    if (aux && !aux->indexes.empty())
    {
        std::string filename = full_path.substr(full_path.find_last_of('/') + 1);

        for (size_t i = 0; i < aux->indexes.size(); ++i)
        {
            if (filename == aux->indexes[i])
            {
                handle_errors_status_pages(403, client);
                response->buildResponse();
                return;
            }
        }
    }

    if (!S_ISREG(st.st_mode))
    {
        handle_errors_status_pages(403, client);
        response->buildResponse();
        return ;
    }
    if (access(full_path.c_str(), W_OK) != 0)
    {
        handle_errors_status_pages(403, client);
        response->buildResponse();
        return ;    
    }
    if (access(parent.c_str(), W_OK | X_OK) != 0)
    {
        handle_errors_status_pages(403, client);
        response->buildResponse();
        return ;
    }
    if (std::remove(full_path.c_str()) != 0)
    {
        response->setStatus(500);
        response->buildResponse();
        return ;
    }
    response->setStatus(204);
    response->setContentType("text/html");
    support_keepAlive();
    //response->buildResponse();
}

void Method::executeMethod(const t_server *client)
{
    std::string method_type = request->getMethod();
    std::string http_version = request->getHttpVersion();
    std::string path = request->getPath();

    //std::cout << request->getFull_req() << std::endl;

    for (size_t i = 0; i < method_type.size(); ++i)
    {
        if (method_type[i] < 'A' || method_type[i] > 'Z')
        {
            handle_errors_status_pages_closing(400, client);
            this->response->setHeader("Connection", "close");
            response->buildResponse();
            return;
        }
    }
    if (path[0] != '/')
    {
        handle_errors_status_pages_closing(400, client);
        this->response->setHeader("Connection", "close");
        response->buildResponse();
        return;
    }

    if (http_version.size() != 8 ||
        http_version.compare(0, 5, "HTTP/") != 0 ||
        http_version[5] < '0' || http_version[5] > '9' ||
        http_version[6] != '.' ||
        http_version[7] < '0' || http_version[7] > '9')
    {
        handle_errors_status_pages_closing(400, client);
        this->response->setHeader("Connection", "close");
        response->buildResponse();
        return;
    }

    if (http_version.empty() || path.empty() || method_type.empty())
    {
        handle_errors_status_pages_closing(400, client);
        this->response->setHeader("Connection", "close");
        response->buildResponse();
        return;
    }
    if (http_version != "HTTP/1.1")
    {
        handle_errors_status_pages_closing(505, client);
        this->response->setHeader("Connection", "close");
        response->buildResponse();
        return;
    }
    // Validar se método HTTP é válido
    if (!request->isValidMethod())
    {
        handle_errors_status_pages_closing(501, client);  // 501 Not Implemented
        response->buildResponse();
        return;
    }

    if (request->getHeader("Host").empty())
    {
        handle_errors_status_pages_closing(400, client);
        this->response->setHeader("Connection", "close");
        response->buildResponse();
        return;
    }

    if (method_type == "GET")
        getMethod(client);
    else if (method_type == "POST")
        postMethod(client);
    else if (method_type == "DELETE")
        deleteMethod(client);
    else
    {
        handle_errors_status_pages(501, client);
        response->buildResponse();
    }
    //std::cout << response->getFullResponse() << std::endl;
}

// ============== CGI ASSÍNCRONO ==============
