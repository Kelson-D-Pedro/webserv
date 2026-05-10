/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mebo <mebo@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/17 17:41:44 by mebo              #+#    #+#             */
/*   Updated: 2026/01/16 12:21:47 by mebo             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# ifndef REQUEST_HPP
# define REQUEST_HPP

# ifndef BUFFER_SIZE
# define BUFFER_SIZE 4092
# endif

# ifndef MAX_HEADER_SIZE
# define MAX_HEADER_SIZE 8192
# endif

# include <iostream>
# include <cstdlib>
# include <cstring>
# include <cctype>
# include <sys/socket.h>
# include <vector>
# include <cerrno>
# include <list>
# include <string>
# include <map>
# include <algorithm>
# include "../utils/utils.hpp"

class Requests
{
    private:
        std::string method;
        std::string path;
        std::string query_string;
        std::string http_version;
        std::map<std::string, std::string> headers_map;  // Use map for O(log n) lookup
        std::string cookies_session_id;
        std::vector<char> body;
        std::string full_req;
        std::vector<char> full_req_body;
        bool is_chunked;
        bool header_too_large;
        
    public:

        Requests();
        ~Requests();

        std::string getMethod() const;
        std::string getPath() const;
        std::string getQueryString() const;
        std::string getHttpVersion() const;
        std::string getCookieID() const;
        const std::vector<char>& getBody() const;  // Return by const reference to avoid copy
        std::string getFull_req() const;
        const std::vector<char>& getFull_req_body() const;  // Return by const reference
        std::string getHeader(const std::string &key) const;
        bool isChunked() const;
        bool isHeaderTooLarge() const;

        bool    requestComplete();
        bool    isValidMethod() const;
        bool flag;

        void    readRequest(int fd);
        void    normalizePath();
        void    parseRequest();
        void    parseQueryString();
        void    parseCookies();
        void    parseChunkedBody();
        void    clear();

};

# endif

