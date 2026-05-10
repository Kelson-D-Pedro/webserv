#include "../../include/http/Method.hpp"
#include "../../include/http/Autoindex.hpp"
# include "../../include/cgi/CgiHandler.hpp"

void    Method::handle_cgi_if_needed_for_get(const t_server *client, const t_location *aux, std::string full_path, std::string extension, std::string root)
{
    const t_cgi &cgi_config = aux->cgis.at(extension);
    this->cgi = new CgiHandler(full_path, cgi_config.pass, root);
    std::cout << cgi_config.pass << std::endl;
    struct epoll_event ev;
    
    // Configurar environment variables CGI
    cgi->setEnv("REQUEST_METHOD", "GET");
    cgi->setEnv("SCRIPT_FILENAME", full_path);
    cgi->setEnv("PATH_INFO", request->getPath());
    cgi->setEnv("QUERY_STRING", request->getQueryString());
    cgi->setEnv("SERVER_PROTOCOL", request->getHttpVersion());
    cgi->setEnv("GATEWAY_INTERFACE", "CGI/1.1");
    cgi->setEnv("CONTENT_LENGTH", "0");
    
    // Executar CGI
    if (!cgi->execute())
    {
        handle_errors_status_pages(500, client);
        response->buildResponse();
        return ;
    }

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = cgi->stdout_fd;
    epoll_ctl(poll_fd, EPOLL_CTL_ADD, cgi->stdout_fd, &ev);

    // stdin do CGI → queremos escrever (POST)
    if (!cgi->stdin_data.empty()) {
        ev.events = EPOLLOUT | EPOLLET;
        ev.data.fd = cgi->stdin_fd;
        epoll_ctl(poll_fd, EPOLL_CTL_ADD, cgi->stdin_fd, &ev);
}
}
