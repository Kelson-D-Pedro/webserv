/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Socket.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mebo <mebo@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/17 11:44:58 by kpedro            #+#    #+#             */
/*   Updated: 2026/01/21 15:40:33 by mebo             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SOCKET_HPP
# define SOCKET_HPP

# include <cerrno>
# include <cstring>
# include <iostream>
# include "../utils/utils.hpp"
# include "../config/ConfigParser.hpp"

class ConfigParser;

class Socket
{
    private:

        std::string         port;
        std::string         ip_addr;
        struct addrinfo     *sckt_config;
        int                 socket_fd;
        
        public:
        
        Socket( const std::string ip, const std::string port, const t_server *config);
        ~Socket( void );

        void    socketCreation();
        void    listenMode(void) const;

        int     getSocketFd() const;

        const t_server            *server_config;

};

#endif