/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Socket.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mebo <mebo@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/17 12:59:54 by kpedro            #+#    #+#             */
/*   Updated: 2026/01/21 15:39:56 by mebo             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/core/Socket.hpp"

void    Socket::socketCreation()
{
    int             opt;
    struct addrinfo *aux;

    // Loop pelas possiveis "identidades do socket do server, para tentar fazer o bind numa delas"
    for (aux = this->sckt_config; aux != NULL; aux = aux->ai_next)
    {
        // Cria o descritor do socket (uma possivel conexão) usando as configurações retornadas por getaddrinfo()
        // (família de endereços, tipo de socket e protocolo).
        // Se a criação do socket falhar, tenta o próximo endereço da lista.
        // Configura o socket para permitir reutilizar o mesmo endereço (IP + porta)
        // imediatamente após o encerramento do servidor, evitando o erro "Address already in use".
        // 'SOL_SOCKET' indica que a opção pertence ao nível do socket,
        // e 'SO_REUSEADDR' é a flag que habilita a reutilização do endereço.
        this->socket_fd = socket(aux->ai_family, aux->ai_socktype | SOCK_CLOEXEC | SOCK_NONBLOCK, aux->ai_protocol);
        if (this->socket_fd == -1)
            continue ;
        opt = 1;
        setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

        // F_GETFL pede ao fcntl() que retorne as flags atuais de status do arquivo/socket.
        // Se a obtenção das flags falhar (-1) OU se a tentativa de definir o socket
        // como não bloqueante (O_NONBLOCK) falhar, o socket é fechado e o loop continua.
        // O_NONBLOCK faz com que chamadas como accept(), recv() ou send()
        // não bloqueiem o programa caso não haja dados disponíveis.
        if(fcntl(this->socket_fd, F_SETFL, O_NONBLOCK) == -1)
        {
            close(this->socket_fd);
            this->socket_fd = -1;
            continue ;
        }

        // Tenta associar o socket ao endereço retornado por getaddrinfo().
        // Se o bind() for bem-sucedido (retorna 0), sai do loop (break).
        // Caso o bind() falhe, marca o socket como inválido e continua tentando
        // com o próximo endereço da lista (aux aponta para o próximo nó).
        if (bind(this->socket_fd, aux->ai_addr, aux->ai_addrlen) == 0)
            break ;
        close(this->socket_fd);
        this->socket_fd = -1;
    }
}

Socket::Socket(const std::string ip, const std::string port, const t_server *config) 
{
    int info;
    struct addrinfo hints;

    this->ip_addr = ip;
    this->port = port;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    info = getaddrinfo(ip.c_str(), port.c_str(),
        &hints, &this->sckt_config);

    if (info != 0)
    {
        throw(std::runtime_error(std::string("Erro: ") + std::string(gai_strerror(info))));
    }
    socketCreation();

    if (this->socket_fd == -1)
    {
        freeaddrinfo(this->sckt_config);
        throw(std::runtime_error(std::string("Erro: ") + std::string(std::strerror(errno))));
    }
    this->server_config = config;
}

Socket::~Socket()
{
    freeaddrinfo(this->sckt_config);
    close(this->socket_fd);
}

void    Socket::listenMode(void) const
{
    if (listen(this->socket_fd, SOMAXCONN) == -1)
        throw(std::runtime_error(std::string("Erro: ") + std::strerror(errno)));
    std::cout << "Servidor ouvindo em: " << this->ip_addr << ":" << this->port << std::endl;
}

int Socket::getSocketFd() const
{
    return (this->socket_fd);
}
