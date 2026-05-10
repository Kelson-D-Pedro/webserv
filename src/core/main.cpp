/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: orisaebo <orisaebo@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/15 10:02:45 by jmiguel           #+#    #+#             */
/*   Updated: 2026/01/29 22:33:48 by orisaebo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/config/ConfigParser.hpp"
# include "../../include/core/Socket.hpp"
# include "../../include/core/Multiplexer.hpp"

void print_server_data(const std::vector<t_server> &servers)
{
    for (size_t i = 0; i < servers.size(); ++i)
    {
        const t_server &srv = servers[i];
        std::cout << "==================== SERVER " << i + 1 << " ====================\n";
        
        // Listen (IPs e portas)
        std::cout << "Listen:\n";
        for (size_t k = 0; k < srv.ports.size(); ++k)
        {
            std::cout << "  Address: " 
                      << (k < srv.full_addresses.size() ? srv.full_addresses[k] : "0.0.0.0")
                      << ":" << srv.ports[k] << "\n";
        }

        // Outras diretivas do bloco server
        std::cout << "Server Name: " << srv.server_name << "\n";
        std::cout << "Root: " << srv.root << "\n";
        std::cout << "Index files:\n";
        for (size_t k = 0; k < srv.indexes.size(); ++k)
            std::cout << "  - " << srv.indexes[k] << "\n";
        if (srv.client_max_body_size != 0)
            std::cout << "Client Max Body Size: " << srv.client_max_body_size << "\n";
        
        std::cout << "Error Pages:\n";
        for (std::map<int, std::string>::const_iterator it = srv.error_pages.begin(); it != srv.error_pages.end(); ++it)
            std::cout << "  " << it->first << " => " << it->second << "\n";

        if (srv.return_directive.code != 0)
        {
            std::cout << "Return: " << srv.return_directive.code;
            if (!srv.return_directive.url.empty())
                std::cout << " " << srv.return_directive.url;
            std::cout << "\n";
        }

        // LOCATION BLOCKS
        std::cout << "\n--- Locations ---\n";
        for (size_t j = 0; j < srv.locations.size(); ++j)
        {
            std::cout << "==================== LOCATION " << j + 1 << " ====================\n";

            const t_location &loc = srv.locations[j];
            std::cout << "\n  Path: " << loc.path << "\n";
            std::cout << "  Root: " << loc.root << "\n";
            std::cout << "  Autoindex: " << (loc.autoindex ? "on" : "off") << "\n";
            if (loc.client_max_body_size != 0)
                std::cout << "  Client Max Body Size: " << loc.client_max_body_size << "\n";
            std::cout << "  Indexes:\n";
            for (size_t k = 0; k < loc.indexes.size(); ++k)
                std::cout << "    - " << loc.indexes[k] << "\n";

            std::cout << "  Allow Methods:\n";
            for (size_t k = 0; k < loc.allow_methods.size(); ++k)
                std::cout << "    - " << loc.allow_methods[k] << "\n";

            if (!loc.upload_path.empty())
                std::cout << "  Upload Path: " << loc.upload_path << "\n";

            if (loc.return_directive.code != 0)
            {
                std::cout << "  Return: " << loc.return_directive.code;
                if (!loc.return_directive.url.empty())
                    std::cout << " " << loc.return_directive.url;
                std::cout << "\n";
            }

            // CGI configurations
            if (!loc.cgis.empty())
            {
                std::cout << "  CGI configurations:\n";
                for (std::map<std::string, t_cgi>::const_iterator it = loc.cgis.begin(); it != loc.cgis.end(); ++it)
                {
                    const t_cgi &cgi = it->second;
                    std::cout << "    Extension: " << cgi.extension << "\n";
                    std::cout << "    Path: " << cgi.path << "\n";
                    std::cout << "    Pass: " << cgi.pass << "\n";
                    if (!cgi.params.empty())
                    {
                        std::cout << "    Params:\n";
                        for (std::map<std::string, std::string>::const_iterator pit = cgi.params.begin(); pit != cgi.params.end(); ++pit)
                            std::cout << "      " << pit->first << " = " << pit->second << "\n";
                    }
                }
            }
        }
        std::cout << "==============================================\n\n";
    }
}

int main(int argc, char **argv)
{
    ConfigParser parse;
    std::vector<Socket *> servers;
    std::map<std::string, Session> sessions;
    Multiplexer multi;
    std::string config_path;

    if (argc > 2)
    {
        std::cerr << "Invalid configuration" << std::endl << "Use: ./webserv or ./webserv config/[configuration_file]" << std::endl;
        return (1);
    }
    if (argc == 2)
        config_path = argv[1];
    else if (argc == 1)
        config_path = "config/default.conf";
    std::fstream conf_file(config_path.c_str(), std::ios::in | std::ios::out);
    if (!conf_file.is_open())
    {
        std::cerr << "Could not open the file" << std::endl;
        conf_file.close();
        return (1);
    }
    conf_file.seekg(0, std::ios::end);
    if (conf_file.tellg() == 0)
    {
        std::cerr << "File empty" << std::endl;
        conf_file.close();
        return (1);
    }
    conf_file.seekg(0, std::ios::beg);

    try
    {
        parse.parsing(conf_file);
        const std::vector<t_server> &servers_config = parse.getServerBlocks();
        std::string host, port;
        print_server_data(servers_config);

        for (size_t i = 0; i < servers_config.size(); i++)
        {
            for (size_t j = 0; j < servers_config[i].ports.size(); j++)
            {
                std::vector<std::string> aux1;
                host = servers_config[i].full_addresses[j];
                port = servers_config[i].ports[j];
                aux1 = split(host, '.');
                if (aux1.size() != 4 || (aux1.size() == 4 && aux1[3].empty()))
                    host = "0.0.0.0";
                Socket *aux = new Socket(host, port, &servers_config[i]);
                aux->listenMode();
                servers.push_back(aux);
            }
        }
        std::cout << "Server is running..." << std::endl;
        multi.multiplexerLoop(servers, sessions);
    }
    catch (const std::length_error& e)
    {
        std::cerr << "length_error: " << e.what() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    for (size_t i = 0; i < servers.size(); i++)
    {
        if (servers[i])
            delete servers[i];
    }
    return 0;
}
