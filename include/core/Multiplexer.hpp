/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Multiplexer.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mebo <mebo@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/19 20:15:33 by darwin            #+#    #+#             */
/*   Updated: 2026/01/16 12:15:34 by mebo             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# ifndef MULTIPLEXER_HPP
# define MULTIPLEXER_HPP

# ifndef MAX_EVENTS
# define MAX_EVENTS 200
# endif

# ifndef CONNECTION_TIMEOUT
# define CONNECTION_TIMEOUT 360
# endif

# include <iostream>
# include <algorithm>
# include "../utils/utils.hpp"
# include "../config/ConfigParser.hpp"
# include "Socket.hpp"

class Socket;

class Multiplexer
{
    private:

        int poll_fd;
        static int sig_number;
    
    public:

        Multiplexer();
        ~Multiplexer();
        
        static void set_signal(int sign);
        static int  get_signal();
        void    multiplexerLoop(const std::vector<Socket *>& severs, std::map<std::string, Session> &sessions);
        void    addHttpClient(int sckt_fd, const std::vector<Socket *>& servers, std::vector<t_http>& client, struct epoll_event& event);
        void    epollinHandler(int action_fd, std::vector<t_http>& client, struct epoll_event& event);
        void    epolloutHandler(int action_fd, std::vector<t_http>& client, struct epoll_event& event, std::map<std::string, Session> &sessions);
        void    checkTimeouts(std::vector<t_http>& client);
        void    checkCgiTimeouts(std::vector<t_http>& client);
        
        std::vector<CgiHandler*> active_cgi;
        time_t now;
};

# endif