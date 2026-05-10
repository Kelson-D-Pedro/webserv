/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   InitHelpers.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mebo <mebo@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/04 11:03:52 by mebo              #+#    #+#             */
/*   Updated: 2025/12/17 12:09:46 by mebo             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/config/ConfigParser.hpp"

void    ConfigParser::init_return(t_return &ret)
{
    ret.code = 0;
    ret.url.clear();
}

void    ConfigParser::init_cgi(t_cgi &cgis)
{
    cgis.extension.clear();
    cgis.params.clear();
    cgis.path.clear();
    cgis.pass.clear();
}

void    ConfigParser::init_location(t_location &loc)
{
    loc.allow_methods.clear();
    loc.client_max_body_size = 0;
    loc.autoindex = false;
    loc.upload_enabled = false;
    loc.cgis.clear();
    loc.indexes.clear();
    loc.path.clear();
    init_return(loc.return_directive);
    loc.root.clear();
    loc.upload_path.clear();
}

void    ConfigParser::init_server(t_server &server)
{
    server.cgis.clear();
    server.client_max_body_size = 0;
    server.error_pages.clear();
    server.full_addresses.clear();
    server.indexes.clear();
    server.locations.clear();
    server.ports.clear();
    init_return(server.return_directive);
    server.root.clear();
    server.server_name.clear();
}

void    ConfigParser::init_global(t_global &glob)
{
    glob.client_max_body_size = 0;
    glob.root.clear();
    glob.default_type.clear();
    glob.indexes.clear();
    glob.error_pages.clear();
}
