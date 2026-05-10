#include "../../include/http/Method.hpp"
# include "../../include/cgi/CgiHandler.hpp"


bool    Method::handle_redirections_if_needed(const t_server *client, const t_location *aux)
{
    if (aux && aux->return_directive.code != 0)
    {
        handle_redirection(aux);
        return (true);
    }
    
    // Verificar return directive no nível de server
    if (client->return_directive.code != 0)
    {
        handle_redirection(client);
        return (true);
    }
    return (false);
}

bool    Method::method_allowed(const t_server *client, const t_location *aux)
{
    if (aux && std::find(aux->allow_methods.begin(), aux->allow_methods.end(), request->getMethod()) == aux->allow_methods.end())
    {
        // Adicionar header Allow com métodos permitidos (RFC 7231)
        std::string allow;
        for (size_t i = 0; i < aux->allow_methods.size(); ++i)
        {
            if (i > 0)
                allow += ", ";
            allow += aux->allow_methods[i];
        }
        response->setHeader("Allow", allow);
        handle_errors_status_pages(405, client);
        if (request->getMethod() == "GET")
            this->response->setHttpVersion(this->request->getHttpVersion());
        response->buildResponse();
        return (false);
    }
    return (true);
}

// NOTA: Esta função NÃO executa mais o CGI de forma bloqueante.
// Ela retorna false para indicar que o CGI NÃO foi executado aqui.
// O CGI deve ser iniciado de forma assíncrona pelo Multiplexer usando startCgiAsync.
bool    Method::handle_cgi_if_needed_for_post(const t_server *client, const t_location *aux, std::string full_path, std::string root)
{
    std::string extension;
    size_t dot_pos = full_path.find_last_of('.');
    if (dot_pos != std::string::npos)
        extension = full_path.substr(dot_pos);
    if (aux && !extension.empty() && aux->cgis.count(extension))
    {
        const t_cgi &cgi_config = aux->cgis.at(extension);
        this->cgi = new CgiHandler(full_path, cgi_config.pass, root);
        struct epoll_event ev;
        std::string content_length = request->getHeader("Content-Length").empty() ? request->getHeader("content-length") : request->getHeader("Content-Length");
        std::string content_type = request->getHeader("Content-Type").empty() ? request->getHeader("content-type") : request->getHeader("Content-Type");
        // Configurar environment variables CGI
        cgi->setEnv("REQUEST_METHOD", "POST");
        cgi->setEnv("SCRIPT_FILENAME", full_path);
        cgi->setEnv("PATH_INFO", request->getPath());
        cgi->setEnv("QUERY_STRING", request->getQueryString());
        cgi->setEnv("SERVER_PROTOCOL", request->getHttpVersion());
        cgi->setEnv("GATEWAY_INTERFACE", "CGI/1.1");
        cgi->setEnv("CONTENT_TYPE", content_type);
        cgi->setEnv("CONTENT_LENGTH", content_length);
        
        // Enviar body do request como stdin do CGI
        cgi->setStdin(request->getBody());
        
        // Executar CGI
        if (!cgi->execute())
        {
            delete cgi;
            cgi = NULL;
            handle_errors_status_pages(500, client);
            response->buildResponse();
            return (true);
        }
        
        // Registrar stdout do CGI no epoll para leitura assíncrona
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = cgi->stdout_fd;
        epoll_ctl(poll_fd, EPOLL_CTL_ADD, cgi->stdout_fd, &ev);
        
        // Registrar stdin do CGI no epoll para escrita assíncrona
        if (!cgi->stdin_data.empty()) {
            ev.events = EPOLLOUT | EPOLLET;
            ev.data.fd = cgi->stdin_fd;
            epoll_ctl(poll_fd, EPOLL_CTL_ADD, cgi->stdin_fd, &ev);
        }
        
        return (true);
    }
    return (false);
}

bool    Method::validate_upload_headers(const t_server *client, const t_location *aux)
{
    std::string uploads_dir = aux ? aux->upload_path : "";
    
    if (uploads_dir.empty() && aux->upload_enabled)
    {
        handle_errors_status_pages(403, client);
        response->buildResponse();
        return (true);
    }

    if (!ensure_dir_exists(uploads_dir) && aux->upload_enabled)
    {
        handle_errors_status_pages(500, client);
        response->buildResponse();
        return (true);
    }

    // Validar Content-Type
    std::string contentType = request->getHeader("Content-Type");
    if (contentType.empty())
    {
        handle_errors_status_pages(400, client);
        response->buildResponse();
        return (true);
    }
    return (false);
}

bool    Method::validate_body_length(const t_server *client, size_t body, size_t max_size)
{
    if (body > max_size)
    {
        handle_errors_status_pages(413, client);
        response->buildResponse();
        return(true);
    }
    return (false);
}
