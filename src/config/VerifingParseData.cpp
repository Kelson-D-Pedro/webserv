/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   VerifingParseData.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mebo <mebo@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/23 16:57:20 by mebo              #+#    #+#             */
/*   Updated: 2026/01/19 09:24:19 by mebo             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/config/ConfigParser.hpp"

void    ConfigParser::validate_server_directives(t_server servering)
{
    if (servering.root.empty())
    {
        std::cerr << "Semantic Error: ";
        throw (std::runtime_error("Missing root path in server"));
    }
    if (servering.ports.empty())
    {
        std::cerr << "Semantic Error: ";
        throw (std::runtime_error("Server is not listening to any port"));        
    }
    std::vector<std::string> copy;
    for (size_t i = 0; i < servering.ports.size(); i++)
    {
        std::string curr_port = servering.ports[i];
        for (size_t j = i + 1; j < servering.ports.size(); j++)
        {
            std::string aux = servering.ports[j];
            if (aux != curr_port)
                continue ;
            if (servering.full_addresses[j] == servering.full_addresses[i] || servering.full_addresses[i] == "0.0.0.0" || servering.full_addresses[j] == "0.0.0.0")
            {
                std::cerr << "Semantic Error: ";
                throw (std::domain_error("Server listening in the same ports"));
            }
        }
        if (std::atoi(curr_port.c_str()) <= 0 ||std::atoi(curr_port.c_str()) > 65535)
        {
            std::cerr << "Semantic Error: ";
            throw (std::domain_error("Server listening in an invalid port"));
        }
        copy.push_back(curr_port);
    }
    if (access(servering.root.c_str(), F_OK) == -1)
    {
        std::cerr << "Semantic Error: ";
        throw (std::runtime_error("Server root path doesn't exist: " + servering.root));
    }
    struct stat st;
    if (stat(servering.root.c_str(), &st) == -1 || !S_ISDIR(st.st_mode))
    {
        std::cerr << "Semantic Error: ";
        throw (std::runtime_error("Invalid root: not a directory or doesn't exist -> " + servering.root));
    }
    std::map<int, std::string>::iterator it;
    for (it = servering.error_pages.begin(); it != servering.error_pages.end(); ++it)
    {
        int code = it->first;
        if (code < 100 || code > 599)
        {
            std::cerr << "Semantic Error: ";
            throw (std::invalid_argument("Invalid status code defined"));
        }
        std::string path = servering.error_pages[code];
        if (path.empty() || path[0] != '/')
        {
            std::cerr << "Semantic Error: ";
            throw (std::invalid_argument("Invalid error_page path: " + path));
        }
    }
    if (servering.return_directive.code != 0)
    {
        if (servering.return_directive.code < 100 || servering.return_directive.code > 599)
        {
            std::cerr << "Semantic Error: ";
            throw (std::invalid_argument("Invalid status code in return directive"));
        }
        if (servering.return_directive.url.empty())
        {
            std::cerr << "Semantic Error: ";
            throw (std::invalid_argument("Missing rdirect URL in return directive"));
        }
    }
}

void    ConfigParser::validate_location_directives(t_server servering)
{
    for (size_t i = 0; i < servering.locations.size(); i++)
    {
        t_location &locs = servering.locations[i];

        if (locs.return_directive.code != 0)
        {
            if (locs.return_directive.code < 100 || locs.return_directive.code > 599)
            {
                std::cerr << "Semantic Error: ";
                throw (std::invalid_argument("Invalid status code in return directive"));
            }
            if (locs.return_directive.url.empty())
            {
                std::cerr << "Semantic Error: ";
                throw (std::invalid_argument("Missing rdirect URL in return directive"));
            }
        }
        for (std::map<std::string, t_cgi>::iterator it = locs.cgis.begin(); it != locs.cgis.end(); ++it)
        {
            //const std::string &ext = it->first;
            for (size_t j = i + 1; j < servering.locations.size(); j++)
            {
                if (servering.locations[j].path == locs.path)
                {
                    std::cerr << "Semantic Error: ";
                    throw (std::invalid_argument("Duplicate location path: " + locs.path));
                }
                /*if (servering.locations[j].cgis.count(ext))
                {
                    std::cerr << "Semantic Error: ";
                    throw (std::invalid_argument("CGI extension already defined in another location: " + ext + " (conflict between '" + locs.path + "' and '" + servering.locations[j].path + "')"));
                }*/
            }
        }
    }
}

void    ConfigParser::validate_listening_conditions()
{
    for (size_t i = 0; i < serverblock.size(); i++)
    {
        const t_server &s1 = serverblock[i];

        for (size_t j = i + 1; j < serverblock.size(); j++)
        {
            const t_server &s2 = serverblock[j];

            for (size_t p1 = 0; p1 < s1.ports.size(); p1++)
            {
                for (size_t p2 = 0; p2 < s2.ports.size(); p2++)
                {
                    if (s1.ports[p1] != s2.ports[p2])
                        continue ;
                    std::string ip1 = s1.full_addresses[p1];
                    std::string ip2 = s2.full_addresses[p2];
                    if (ip1 == ip2 || ip1 == "0.0.0.0" || ip2 == "0.0.0.0")
                    {
                        std::cerr << "Semantic Error: ";
                        throw (std::domain_error("Duplicate server listening on the same address:port: (" + ip1 + ":" + s1.ports[p1]+ ")"));
                    }
                }
            }
        }

    }
}

void ConfigParser::validate_file_paths(t_server servering)
{
    // Verificar error_pages existem
    std::map<int, std::string>::iterator it;
    for (it = servering.error_pages.begin(); it != servering.error_pages.end(); ++it)
    {
        std::string full_path = servering.root + it->second;
        if (access(full_path.c_str(), R_OK) == -1)
        {
            std::cerr << "Warning: error_page file not readable: " << full_path << std::endl;
            // Ou throw para ser estrito
        }
    }
    // Verificar index files existem
    bool found_index = false;
    std::string full_path;
    for (size_t i = 0; i < servering.indexes.size(); i++)
    {
        full_path = servering.root + "/" + servering.indexes[i];
        if (access(full_path.c_str(), R_OK) == 0)
        {
            found_index = true;
            break;
        }
    }
    if (!found_index && !servering.indexes.empty())
    {
        std::cerr << "Warning: No index file " + full_path + " found in root" << std::endl;
    }
}
void ConfigParser::validate_cgi_executables(t_server servering)
{
    for (size_t i = 0; i < servering.locations.size(); i++)
    {
        t_location &loc = servering.locations[i];
        
        std::map<std::string, t_cgi>::iterator it;
        for (it = loc.cgis.begin(); it != loc.cgis.end(); ++it)
        {
            t_cgi &cgi = it->second;
            
            if (cgi.pass.empty())
            {
                std::cerr << "Semantic Error: ";
                throw std::runtime_error("CGI extension '" + cgi.extension + "' missing cgi_path");
            }
            if (!cgi.path.empty())
            {
                if (access(cgi.path.c_str(), X_OK) == -1)
                {
                    std::cerr << "Warning: " << "CGI interpreter not executable: " + cgi.path;
                }
            }
            else if (!cgi.pass.empty())
            {
                if (access(cgi.pass.c_str(), X_OK) == -1)
                {
                    std::cerr << "Warning: " << "CGI interpreter not executable: " + cgi.pass;
                }                
            }
        }
    }
}

void ConfigParser::validate_upload_paths(t_server servering)
{
    for (size_t i = 0; i < servering.locations.size(); i++)
    {
        t_location &loc = servering.locations[i];
        
        if (access(loc.root.c_str(), F_OK) == -1)
        {
            std::cerr << "Semantic Error: ";
            throw (std::invalid_argument("Location root '" + loc.root + "' doesn't exist"));
        }
        struct stat st;
        if (stat(loc.root.c_str(), &st) == -1 || !S_ISDIR(st.st_mode))
        {
            std::cerr << "Semantic Error: ";
            throw (std::runtime_error("Invalid root: not a directory or doesn't exist -> " + loc.root));
        }
        if (!loc.upload_path.empty())
        {
            // Verificar se POST está nos métodos permitidos
            if (std::find(loc.allow_methods.begin(), loc.allow_methods.end(), "POST") 
                == loc.allow_methods.end())
            {
                std::cerr << "Warning (location " << loc.path << "): upload_path defined but POST not in allow_methods";
            }
            
            // Verificar se o diretório existe e é escrevível
            struct stat st;
            if (stat(loc.upload_path.c_str(), &st) == -1)
            {
                std::cerr << "Semantic Error: ";
                throw std::runtime_error("upload_path doesn't exist: " + loc.upload_path);
            }
            
            if (!S_ISDIR(st.st_mode))
            {
                std::cerr << "Semantic Error: ";
                throw std::runtime_error("upload_path is not a directory: " + loc.upload_path);
            }
            
            if (access(loc.upload_path.c_str(), W_OK) == -1)
            {
                std::cerr << "Semantic Error: ";
                throw std::runtime_error("upload_path not writable: " + loc.upload_path);
            }
        }
    }
}

void ConfigParser::validate_bidy_size_limits(t_server servering)
{
    const size_t max_size_webserv = 1024UL * 1024 * 1024;
    
    if (servering.client_max_body_size > max_size_webserv)
    {
        std::cerr << "Warning: client_max_body_size very large: " 
                  << servering.client_max_body_size << " bytes" << std::endl;
    }
    
    for (size_t i = 0; i < servering.locations.size(); i++)
    {
        if (servering.locations[i].client_max_body_size > max_size_webserv)
        {
            std::cerr << "Warning (location " << servering.locations[i].path << "): "
                      << "client_max_body_size very large" << std::endl;
        }
    }
}

void    ConfigParser::validate_parse_data()
{
    validate_listening_conditions();
    for (size_t i = 0; i < serverblock.size(); i++)
    {
        t_server &serve = serverblock[i];

        validate_server_directives(serve);
        validate_location_directives(serve);
        validate_file_paths(serve);
        validate_cgi_executables(serve);
        validate_upload_paths(serve);
        validate_bidy_size_limits(serve);
    }
}
