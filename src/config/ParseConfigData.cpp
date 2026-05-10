/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseConfigData.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mebo <mebo@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/18 17:42:50 by mebo              #+#    #+#             */
/*   Updated: 2026/01/16 12:41:01 by mebo             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/config/ConfigParser.hpp"

void    ConfigParser::apply_inheritance()
{
    for (size_t i = 0; i < serverblock.size(); i++)
    {
        t_server &serve = serverblock[i];
        if (serve.root.empty())
            serve.root = global.root;
        if (serve.indexes.empty())
            serve.indexes = global.indexes;
        if (serve.client_max_body_size == 0)
            serve.client_max_body_size = global.client_max_body_size;
        if (serve.error_pages.empty())
            serve.error_pages = global.error_pages;
        else
        {
            std::map<int, std::string>::iterator it;
            for (it = global.error_pages.begin(); it != global.error_pages.end(); ++it)
            {
                int code = it->first;
                if (serve.error_pages.find(code) == serve.error_pages.end())
                    serve.error_pages[code] = it->second;
            }
        }
        for (size_t j = 0; j < serve.locations.size(); j++)
        {
            t_location &locs = serve.locations[j];
            if (locs.root.empty())
                locs.root = serve.root;
            if (locs.indexes.empty())
                locs.indexes = serve.indexes;
            if (locs.client_max_body_size == 0)
                locs.client_max_body_size = serve.client_max_body_size;
            if (locs.allow_methods.empty())
            {
                locs.allow_methods.push_back("GET");
                locs.allow_methods.push_back("POST");
                locs.allow_methods.push_back("DELETE");
            }
        }
        if (serve.server_name.empty())
            serverblock[i].server_name = "_";
    }
}

t_location    ConfigParser::parse_location_block(size_t &j)
{
    t_location localizacao;
    init_location(localizacao);
    std::string ext;

    if (tkns[j + 1].value == "~")
    {
        std::cerr << "Error (line " << tkns[j].line << "): ";
        throw std::runtime_error("regex not required");   
    }
    localizacao.path = tkns[++j].value;
    j++;
    if (tkns[j].value != "{")
        throw (std::runtime_error("Expected \'{\' after 'server'"));
    j++;
    while (j < tkns.size() && tkns[j].value != "}")
    {
        std::string curr = tkns[j].value;
        if (curr == "root")
        {
            j++;
            localizacao.root = tkns[j].value;
        }
        else if (curr == "index")
        {
            j++;
            while (j < tkns.size() && tkns[j].value != ";")
                localizacao.indexes.push_back(tkns[j++].value);
            continue ;
        }
        else if (curr == "client_max_body_size")
        {
            j++;
            std::string client = tkns[j].value;
            std::string number_part;
            size_t len = client.length();
            char unit = std::toupper(client[len - 1]);
            size_t size;

            if (std::isdigit(unit))
                number_part = client;
            else
                number_part = client.substr(0, len - 1);
            size = std::atol(number_part.c_str());
            if (unit == 'K')
                size *= 1024;
            else if (unit == 'M')
                size *= 1024 * 1024;
            else if (unit == 'G')
                size *= 1024 * 1024 * 1024;
            localizacao.client_max_body_size = size;
        }
        else if (curr == "allow_methods")
        {
            j++;
            while (j < tkns.size() && tkns[j].value != ";")
                localizacao.allow_methods.push_back(tkns[j++].value);
            continue ;
        }
        else if (curr == "autoindex")
        {
            if (j + 1 < tkns.size() && tkns[j + 1].value == "on")
                localizacao.autoindex = true;
            else if (j + 1 < tkns.size() && tkns[j + 1].value == "off")
                localizacao.autoindex = false;
        }
        else if (curr == "upload_enabled")
        {
            if (j + 1 < tkns.size() && tkns[j + 1].value == "on")
                localizacao.upload_enabled = true;
            else if (j + 1 < tkns.size() && tkns[j + 1].value == "off")
                localizacao.upload_enabled = false;
        }
        else if (curr == "upload_path")
        {
            j++;
            localizacao.upload_path = tkns[j].value;
        }
        else if (curr == "return")
        {
            t_return ret;
            init_return(ret);
            
            if (j + 1 >= tkns.size() || tkns[j + 1].type != "number")
                throw (std::runtime_error("Invalid 'return' directive: missing status code"));
            
            ret.code = std::atoi(tkns[j + 1].value.c_str());
            if (j + 2 < tkns.size() && tkns[j + 2].type == "word")
                ret.url = tkns[j + 2].value;
            localizacao.return_directive = ret;
        }
        // CGIs parser
        else if (curr == "cgi_ext")
        {
            if (!ext.empty())
            {
                std::cerr << "Error (line " << tkns[j].line << "): ";
                throw std::runtime_error("cgi extension already defined");
            }
            ext = tkns[j + 1].value;
            t_cgi cgi;
            init_cgi(cgi);
            cgi.extension = ext;
            localizacao.cgis[ext] = cgi;
        }
        else if (curr == "cgi_path")
        {
            if (ext.empty())
            {
                std::cerr << "Error (line " << tkns[j].line << "): ";
                throw std::runtime_error("cgi_path declared before cgi_extension");
            }
            localizacao.cgis[ext].pass = tkns[j + 1].value;
        }
        else if (curr == "cgi_pass")
        {
            if (ext.empty())
            {
                std::cerr << "Error (line " << tkns[j].line << "): ";
                throw std::runtime_error("cgi_pass declared before cgi_extension");
            }
            localizacao.cgis[ext].pass = tkns[j + 1].value;
        } 
        else if (curr == "cgi_param")
        {
            if (ext.empty())
            {
                std::cerr << "Error: line(" << tkns[j].line << "): ";
                throw std::runtime_error("cgi_pass declared before cgi_extension");
            }
            std::string key = tkns[j + 1].value;
            std::string value = tkns[j + 2].value;
            localizacao.cgis[ext].params[key] = value;
        }
        else if (curr != "}" && curr != ";")
        {
            std::cerr << "Error (line " << tkns[j].line << "): ";
            throw (std::runtime_error("Unknown \'" + tkns[j].value + "\' directive in location block"));
        }
        while (j < tkns.size() && tkns[j].value != "}" && tkns[j].value != ";")    
            j++;
        if (tkns[j].value == ";")
            j++;
    }
    if (tkns[j].value == "}")
        j++;
    return (localizacao);
}

void    ConfigParser::fill_global_scope(size_t &i)
{
    while (i < tkns.size() && tkns[i].value != "server")
    {
        std::string value = tkns[i].value;

        if (value == "root")
        {
            if (global.root.size())
            {
                std::cerr << "Error (line " <<  tkns[i].line << "): ";
                throw (std::runtime_error("Duplicate directive root in global scope"));
            }
            i++;
            global.root = tkns[i].value;
        }
        else if (value == "index")
        {
            if (!global.indexes.empty())
            {
                std::cerr << "Error (line " <<  tkns[i].line << "): ";
                throw (std::runtime_error("Duplicate directive index in global scope"));
            }
            i++;
            while (i < tkns.size() && tkns[i].value != ";")
                global.indexes.push_back(tkns[i++].value);
            continue ;
        }
        else if (value == "client_max_body_size")
        {
            i++;
            std::string client = tkns[i].value;
            std::string number_part;
            size_t len = client.length();
            char unit = std::toupper(client[len - 1]);
            size_t size;

            if (std::isdigit(unit))
                number_part = client;
            else
                number_part = client.substr(0, len - 1);
            size = std::atol(number_part.c_str());
            if (unit == 'K')
                size *= 1024;
            else if (unit == 'M')
                size *= 1024 * 1024;
            else if (unit == 'G')
                size *= 1024 * 1024 * 1024;
            global.client_max_body_size = size;
        }
        else if (value == "default_type")
        {
            i++;
            global.default_type = tkns[i].value;
        }
        else if (value == "error_page")
        {
            i++;
            int code = std::atoi(tkns[i].value.c_str());
            if (global.error_pages.count(code))
            {
                std::cerr << "Error (line " <<  tkns[i].line << "): ";
                throw std::runtime_error("Duplicate error_page for code " + tkns[i].value);
            }
            i++;
            global.error_pages[code] = tkns[i].value;
        }
        else if (value == ";" || value == "}")
        {
        }
        else
        {
            std::cerr << "Error (line " <<  tkns[i].line << "): ";
            throw std::runtime_error("Duplicate error_page for code " + tkns[i].value);
        }
        while (i < tkns.size() && tkns[i].value != ";" && tkns[i].value != "server")
            i++;
        if (i < tkns.size() && tkns[i].value == ";")
            i++;
    }
}

void    ConfigParser::parse_config_data()
{
    size_t k;

    k = 0;
    init_global(global);
    fill_global_scope(k);

    for (size_t i = k; i < tkns.size(); i++)
    {
        if (tkns[i].value == "server")
        {
            t_server serve;
            init_server(serve);
            size_t j = i + 1;

            if (tkns[j].value != "{")
                throw (std::runtime_error("Expected \'{\' after 'server'"));
            j++;
            while (j < tkns.size() && tkns[j].value != "}")
            {
                std::string value = tkns[j].value;
                if (value == "listen")
                {
                    if (tkns[j + 1].type == "word")
                    {
                        std::string address = tkns[j + 1].value;
                        std::string addr;
                        size_t aux = 0;
                        while (aux < address.size() && address[aux] != ':')
                        {
                            addr.push_back(address[aux]);
                            aux++;
                        }
                        serve.full_addresses.push_back(addr);
                        aux++;
                        addr.clear();
                        while (aux < address.size())
                        {
                            addr.push_back(address[aux]);
                            aux++;
                        }
                        if (!addr.empty())
                        serve.ports.push_back(addr);
                    }
                    else
                    {
                        j++;
                        serve.ports.push_back(tkns[j].value);
                        serve.full_addresses.push_back("0.0.0.0");
                    }
                }
                else if (value == "server_name")
                {
                    j++;
                    serve.server_name = tkns[j].value;
                }
                else if (value == "root")
                {
                    j++;
                    serve.root = tkns[j].value;
                }
                else if (value == "index")
                {
                    j++;
                    while (j < tkns.size() && tkns[j].value != ";")
                        serve.indexes.push_back(tkns[j++].value);
                    continue ;
                }
                else if (value == "location")
                {
                    t_location loc = {};
                    loc = parse_location_block(j);
                    serve.locations.push_back(loc);
                    continue ;
                }
                else if (value == "client_max_body_size")
                {
                    j++;
                    std::string client = tkns[j].value;
                    std::string number_part;
                    size_t len = client.length();
                    char unit = std::toupper(client[len - 1]);
                    size_t size;

                    if (std::isdigit(unit))
                        number_part = client;
                    else
                        number_part = client.substr(0, len - 1);
                    size = std::atol(number_part.c_str());
                    if (unit == 'K')
                        size *= 1024;
                    else if (unit == 'M')
                        size *= 1024 * 1024;
                    else if (unit == 'G')
                        size *= 1024 * 1024 * 1024;
                    serve.client_max_body_size = size;
                }
                else if (value == "error_page")
                {
                    j++;
                    int code = std::atoi(tkns[j].value.c_str());
                    j++;
                    if (serve.error_pages[code].empty())
                    {
                        std::string path = tkns[j].value;
                        serve.error_pages[code] = path;
                    }
                    else
                    {
                        std::cerr << "Error (line " << tkns[j].line << "): ";
                        throw (std::runtime_error("Error page in this code already defined"));
                    }
                }
                else if (value == "return")
                {
                    t_return ret;
                    init_return(ret);

                    if (j + 1 >= tkns.size() || tkns[j + 1].type != "number")
                        throw (std::runtime_error("Invalid 'return' directive: missing status code"));
                    
                    ret.code = std::atoi(tkns[j + 1].value.c_str());
                    if (j + 2 < tkns.size() && tkns[j + 2].type == "word")
                        ret.url = tkns[j + 2].value;
                    serve.return_directive = ret;
                }
                else if (value != "}" && value != ";")
                {
                    std::cerr << "Error (line " << tkns[j].line << "): ";
                    throw (std::runtime_error("Unknown \'" + tkns[j].value + "\' directive in server block"));
                }
                while (j < tkns.size() && tkns[j].value != "}" && tkns[j].value != ";")
                    j++;
                if (tkns[j].value == ";")
                    j++;
            }
            serverblock.push_back(serve);
            i = j;
        }
    }
    apply_inheritance();
}
