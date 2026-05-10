/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Autoindex.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mebo <mebo@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/15 16:54:40 by mebo              #+#    #+#             */
/*   Updated: 2026/01/16 12:21:55 by mebo             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef AUTOINDEX_HPP
#define AUTOINDEX_HPP
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <sstream>
#include <string>
#include <iomanip>
#include <map>
#include <vector>
#include <algorithm>
#include "Response.hpp"

class Response;

struct DirEntry
{
    std::string name;
    bool is_dir;
    off_t size;
    time_t mtime;
};

class Autoindex
{
    public:
        static std::string generate(std::string &dir_path, std::string &req_path, std::map <int, std::string> error_pages, Response *response);
        static std::string urlEncode(const std::string &str);
        static std::string htmlEscape(const std::string &str);
        static std::string formatSize(off_t size);
        static std::string formatTime(time_t time);
};

#endif