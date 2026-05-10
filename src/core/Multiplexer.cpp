/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Multiplexer.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mebo <mebo@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/19 20:14:26 by darwin            #+#    #+#             */
/*   Updated: 2026/01/21 15:40:59 by mebo             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/core/Multiplexer.hpp"
#include "../../include/http/Method.hpp"
# include "../../include/http/Request.hpp"
# include "../../include/utils/types.hpp"
# include "../../include/cgi/CgiHandler.hpp"


int Multiplexer::sig_number = 0;

Multiplexer::Multiplexer()
{
    this->poll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (this->poll_fd == -1)
        throw(std::runtime_error("Erro: " + std::string(std::strerror(errno))));
}

Multiplexer::~Multiplexer()
{
    close(this->poll_fd);
}

void    Multiplexer::addHttpClient(int sckt_fd, const std::vector<Socket *>& servers, std::vector<t_http>& client, struct epoll_event& event)
{
    t_http                http;
    int                   server_pos;

    // Identificar a requisição e aceitar a conexão do cliente (o accept abre um fd para comunicar com o cliente)
    http.client_fd = accept(sckt_fd, NULL, NULL);
    if (http.client_fd == -1)
    {
        std::cerr << "[ADD] Accept failed on listening_fd=" << sckt_fd << std::endl;
        return ;
    }

    // Transformar o fd do cliente num socket não bloqueante
    if(fcntl(http.client_fd, F_SETFL, O_NONBLOCK) == -1)
    {
        std::cerr << "[ADD] fcntl failed for fd=" << http.client_fd << std::endl;
        close(http.client_fd);
        return ;
    }

    if(fcntl(http.client_fd, F_SETFD, O_CLOEXEC) == -1)
    {
        std::cerr << "[ADD] fcntl failed for fd=" << http.client_fd << std::endl;
        close(http.client_fd);
        return ;
    }
    
    // Validar que encontramos o servidor antes de acessar o vetor
    server_pos = find_server_pos(servers, sckt_fd);
    if (server_pos < 0)
    {
        std::cerr << "[ADD] Server not found for listening_fd=" << sckt_fd << std::endl;
        close(http.client_fd);
        return ;
    }

    // Colocar o socket do cliente em modo de leitura e adicionar ele na lista de monitoramento da epoll (para ler as request)
    event.events = EPOLLIN;
    event.data.fd = http.client_fd;
    if (epoll_ctl(this->poll_fd, EPOLL_CTL_ADD, http.client_fd, &event) == -1)
    {
        std::cerr << "[ADD] epoll_ctl ADD failed for fd=" << http.client_fd << std::endl;
        close(http.client_fd);
        return ;
    }
    
    // Apenas após todas as validações, preencher e adicionar o cliente
    http.server = servers[server_pos];
    http.req = new Requests();
    http.res = new Response();
    http.cgi = NULL;
    http.res->clear();
    http.req->clear();
    http.state = STATE_READING_REQUEST;
    http.last_activity = std::time(NULL);
    client.push_back(http);
}

void    Multiplexer::epollinHandler(int action_fd, std::vector<t_http>& client, struct epoll_event& event)
{
    int pos;

    pos = find_client_pos(client, action_fd);
    if (pos < 0)
    {
        std::cerr << "[IN] Client not found for fd=" << action_fd << std::endl;
        return ;
    }
    
    client[pos].last_activity = std::time(NULL);
    client[pos].req->readRequest(action_fd);
    if (client[pos].req->flag)
    {
        epoll_ctl(this->poll_fd, EPOLL_CTL_DEL, action_fd, NULL);
        close(action_fd);
        delete client[pos].req;
        delete client[pos].res;
        if (client[pos].cgi != NULL)
            delete client[pos].cgi;
        client[pos].state = STATE_CLOSED;
        client.erase(client.begin() + pos);
        return ;
    }
    
    if (client[pos].req->requestComplete())
    {
        client[pos].req->parseRequest();
        event.events = EPOLLOUT;
        event.data.fd = action_fd;
        if (epoll_ctl(this->poll_fd, EPOLL_CTL_MOD, action_fd, &event) == -1)
        {
            std::cerr << "[IN] epoll_ctl MOD to EPOLLOUT failed for fd=" << action_fd << std::endl;
        }
    }

}

void    Multiplexer::epolloutHandler(int action_fd, std::vector<t_http>& client, struct epoll_event& event, std::map<std::string, Session> &sessions)
{
    int pos;

    pos = find_client_pos(client, action_fd);
    if (pos < 0)
    {
        std::cerr << "[OUT] Client not found for fd=" << action_fd << std::endl;
        return;
    }
    
    // CORREÇÃO: Verificar estado antes de executar método
    // Se estiver executando CGI, não processar novamente (evita loop infinito)
    
    if (client[pos].state == STATE_EXECUTING_CGI)
    {
        // CGI ainda em execução - ignorar evento EPOLLOUT
        // A resposta será enviada quando checkCgiTimeouts() detectar que o CGI terminou
        return;
    }
    if (client[pos].cgi == NULL && client[pos].state != STATE_WRITING_RESPONSE)
    {
        Method method(client[pos].req, client[pos].res,
                    client[pos].server->server_config->root, sessions, poll_fd);
        method.executeMethod(client[pos].server->server_config);

        client[pos].cgi = method.cgi;
        if (client[pos].cgi)
        {
            // CGI iniciado - mudar estado e adicionar à lista de CGIs ativos
            client[pos].state = STATE_EXECUTING_CGI;
            active_cgi.push_back(client[pos].cgi);
            
            // IMPORTANTE: Não modificar o cliente para EPOLLOUT aqui
            // O cliente permanecerá aguardando até o CGI terminar
            // checkCgiTimeouts() fará a transição para EPOLLOUT quando pronto
            return;
        }
        
        // Não é CGI - preparar para enviar resposta
        client[pos].state = STATE_WRITING_RESPONSE;
    }
    // Enviar resposta ao cliente
    client[pos].res->sendResponse(action_fd);
    if (client[pos].res->flag)
    {
        epoll_ctl(this->poll_fd, EPOLL_CTL_DEL, action_fd, NULL);
        close(action_fd);
        delete client[pos].req;
        delete client[pos].res;
        if (client[pos].cgi != NULL)
            delete client[pos].cgi;
        client[pos].state = STATE_CLOSED;
        client.erase(client.begin() + pos);
        return ;
    }
    
    
    if (!client[pos].res->getKeepAlive())
    {
        // Fechar conexão
        epoll_ctl(this->poll_fd, EPOLL_CTL_DEL, action_fd, NULL);
        close(action_fd);
        delete client[pos].req;
        delete client[pos].res;
        if (client[pos].cgi != NULL)
            delete client[pos].cgi;
        client[pos].state = STATE_CLOSED;
        client.erase(client.begin() + pos);
    }
    else
    {
        // ADICIONAR: Voltar ao estado de leitura
        event.events = EPOLLIN;
        event.data.fd = action_fd;
        if (epoll_ctl(this->poll_fd, EPOLL_CTL_MOD, action_fd, &event) == -1)
        {
            std::cerr << "[OUT] epoll_ctl MOD to EPOLLIN failed for fd=" << action_fd << std::endl;
        }
        
        // Limpar request/response mas manter conexão
        client[pos].req->clear();
        client[pos].res->clear();
        if (client[pos].cgi != NULL)
        	delete client[pos].cgi;
        client[pos].cgi = NULL;
        client[pos].last_activity = std::time(NULL);
        client[pos].state = STATE_READING_REQUEST;  // Voltar ao estado inicial
    }
}


void    Multiplexer::checkTimeouts(std::vector<t_http>& client)
{   
    for (size_t i = 0; i < client.size(); )
    {
        if (now - client[i].last_activity > CONNECTION_TIMEOUT)
        {
            // Timeout: fechar conexão
            epoll_ctl(this->poll_fd, EPOLL_CTL_DEL, client[i].client_fd, NULL);
            close(client[i].client_fd);
            delete client[i].req;
            delete client[i].res;
            if (client[i].cgi)
            {
                client[i].cgi->killProcess();
                //delete client[i].cgi;
                client[i].cgi = NULL;
            }
            client.erase(client.begin() + i);
        }
        else
            ++i;
    }
}

void Multiplexer::set_signal(int sign)
{
    sig_number = sign;
}

int Multiplexer::get_signal()
{
    return (sig_number);
}

void  handle_signal(int sig)
{
    if (sig == SIGTERM || sig == SIGQUIT || sig == SIGINT)
        Multiplexer::set_signal(sig);
}

void    Multiplexer::multiplexerLoop(const std::vector<Socket *>& servers, std::map<std::string, Session> &sessions)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, handle_signal);
    signal(SIGQUIT, handle_signal);
    signal(SIGTERM, handle_signal);
    struct epoll_event event, events[MAX_EVENTS];
    int fds_waiting, action_fd;
    std::vector<t_http> client;
    std::vector<int>    fds;
    
    now = std::time(NULL);

    // Colocar os sockets dos servers na lista de monitoramento da epoll
    for (size_t i = 0; i < servers.size(); i++)
    {
        event.events = EPOLLIN;
        event.data.fd = servers[i]->getSocketFd();
        if (epoll_ctl(this->poll_fd, EPOLL_CTL_ADD, servers[i]->getSocketFd(), &event) == -1)
            throw(std::runtime_error("Erro: " + std::string(std::strerror(errno))));
        fds.push_back(servers[i]->getSocketFd());
    }
    
    
    // Loop Infinto do server
    while (1)
    {
        if (Multiplexer::get_signal() != 0)
        {
            for (size_t i = 0; i < client.size(); i++)
            {
                if (client[i].req != NULL)
                    delete client[i].req;
                if (client[i].res != NULL)
                    delete client[i].res;
                if (client[i].cgi != NULL)
                    delete client[i].cgi;
            }
            break ;
        }
        // Pegar a quantidade de fd's/sockets que possuem algum evento (leitura/escrita/pedido de conexão) a espera na lista de monitoramento da epoll
        // Timeout de 1000ms para verificar conexões expiradas periodicamente
        fds_waiting = epoll_wait(this->poll_fd, events, MAX_EVENTS, 1000);
        if (fds_waiting == -1)
            continue ;
        
        // Se timeout sem eventos, apenas verificar timeouts
        if (fds_waiting == 0)
        {
            checkTimeouts(client);
            checkCgiTimeouts(client);
            //continue;
        }
        
        // Loop pelos fd's/sockets que possuem eventos para aplicar o tratamento
        for (int i = 0; i < fds_waiting; i++)
        {
            action_fd = events[i].data.fd;
            
            // Verificar se é um listening socket
            if ((std::find(fds.begin(), fds.end(), action_fd)) != fds.end())
            {
                // Evento: Pedido de conexão
                // Adicionar um cliente a lista de monitoramente
                addHttpClient(action_fd, servers, client, event);
                continue;
            }
            
            // Verificar se é um cliente
            int client_pos = find_client_pos(client, action_fd);
            if (client_pos >= 0) {
                if (events[i].events & EPOLLIN)
                    epollinHandler(action_fd, client, event);
                else if (events[i].events & EPOLLOUT)
                    epolloutHandler(action_fd, client, event, sessions);
                    
                continue;
            }

            // Verificar se é um CGI fd
            for (size_t j = 0; j < active_cgi.size(); j++)
            {
                CgiHandler* cgi = active_cgi[j];

                // stdout pronto para leitura
                if (action_fd == cgi->stdout_fd && (events[i].events & EPOLLIN))
                {
                    cgi->readStdout();
                }

                // stdin pronto para escrita
                if (action_fd == cgi->stdin_fd && (events[i].events & EPOLLOUT))
                {
                    cgi->writeStdin();
                }
            }
        }
        
        now = std::time(NULL);
        checkTimeouts(client);
        checkCgiTimeouts(client);
    }
}

void Multiplexer::checkCgiTimeouts(std::vector<t_http>& client)
{
    for (size_t j = 0; j < active_cgi.size(); )
    {
        CgiHandler* cgi = active_cgi[j];
        cgi->checkProcess();  // Verifica timeout e processo filho

        if (cgi->finished)
        {
            // CORREÇÃO: Remover FDs do epoll ANTES de fechar
            // Isso evita erros "Bad file descriptor" no epoll
            if (cgi->stdout_fd != -1)
            {
                epoll_ctl(poll_fd, EPOLL_CTL_DEL, cgi->stdout_fd, NULL);
                close(cgi->stdout_fd);
                cgi->stdout_fd = -1;
            }
            if (cgi->stdin_fd != -1)
            {
                epoll_ctl(poll_fd, EPOLL_CTL_DEL, cgi->stdin_fd, NULL);
                close(cgi->stdin_fd);
                cgi->stdin_fd = -1;
            }

            // Encontrar cliente associado ao CGI
            int client_idx = -1;
            for (size_t i = 0; i < client.size(); ++i)
            {
                if (client[i].cgi == cgi)
                {
                    client_idx = i;
                    break;
                }
            }
            
            if (client_idx >= 0)
            {
                // Preparar resposta baseada no resultado do CGI
                if (cgi->exit_code == 0)
                {
                    std::cout << cgi->output << std::endl;
                    client[client_idx].res->setBody(cgi->output);
                    client[client_idx].res->prepareCGIResponse(cgi->output);
                }
                else if (access(cgi->getInterpreter_path().c_str(), F_OK) == -1)
                {
                    std::map<int, std::string> erros = client[client_idx].server->server_config->error_pages;
                    std::map<int, std::string>::iterator it = erros.find(500);
                    if (it != erros.end() && !it->second.empty())
                    {
                        std::string error_path = client[client_idx].server->server_config->root + it->second;
                        struct  stat    error_info;
                        if (stat(error_path.c_str(), &error_info) == 0 && S_ISREG(error_info.st_mode))
                            client[client_idx].res->generateErrorPage(error_path, error_info, 500);
                    }
                    else
                        client[client_idx].res->generateErrorPage(500);
                }
                else if (cgi->finished && cgi->exit_code == -3)
                {
                    std::map<int, std::string> erros = client[client_idx].server->server_config->error_pages;
                    std::map<int, std::string>::iterator it = erros.find(502);
                    if (it != erros.end() && !it->second.empty())
                    {
                        std::string error_path = client[client_idx].server->server_config->root + it->second;
                        struct  stat    error_info;
                        if (stat(error_path.c_str(), &error_info) == 0 && S_ISREG(error_info.st_mode))
                            client[client_idx].res->generateErrorPage(error_path, error_info, 502);
                    }
                    else
                        client[client_idx].res->generateErrorPage(502);
                }
                else
                {
                    // CGI falhou ou teve timeout - retornar erro 502
                    //se falhou
                    std::map<int, std::string> erros = client[client_idx].server->server_config->error_pages;
                    std::map<int, std::string>::iterator it = erros.find(504);
                    if (it != erros.end() && !it->second.empty())
                    {
                        std::string error_path = client[client_idx].server->server_config->root + it->second;
                        struct  stat    error_info;
                        if (stat(error_path.c_str(), &error_info) == 0 && S_ISREG(error_info.st_mode))
                            client[client_idx].res->generateErrorPage(error_path, error_info, 504);
                    }
                    else
                        client[client_idx].res->generateErrorPage(504);
                }

                client[client_idx].res->buildResponse();

                // Mudar cliente para estado de escrita e ativar EPOLLOUT
                client[client_idx].state = STATE_WRITING_RESPONSE;
                struct epoll_event ev;
                ev.events = EPOLLOUT;
                ev.data.fd = client[client_idx].client_fd;
                epoll_ctl(poll_fd, EPOLL_CTL_MOD, client[client_idx].client_fd, &ev);
            }
            
            // Remover CGI da lista de ativos
            active_cgi.erase(active_cgi.begin() + j);
        }
        else
            ++j;
    }
}

