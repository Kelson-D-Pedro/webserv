/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mebo <mebo@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/17 17:46:16 by mebo              #+#    #+#             */
/*   Updated: 2026/01/16 12:15:15 by mebo             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <vector>
#include <deque>
#include <stack>
#include <map>
#include <set>
#include <algorithm>

typedef struct s_return
{
    int code;
    std::string url;
} t_return;


typedef struct s_token
{
    std::string type;
    std::string value;
    int line;
} t_token;

typedef struct s_global
{
    size_t client_max_body_size;
    std::string root;
    std::string default_type;
    std::vector <std::string> indexes;
    std::map<int, std::string> error_pages;
} t_global;

typedef struct s_cgi
{
    std::string extension;
    std::string path;
    std::string pass;
    std::map<std::string, std::string> params;
} t_cgi;

typedef struct s_location
{
    bool autoindex;
    bool upload_enabled;
    std::string path;
    std::string root;
    std::string upload_path;
    t_return return_directive;
    size_t client_max_body_size;
    std::vector<std::string> indexes;
    std::vector<std::string> allow_methods;
    std::map<std::string, t_cgi> cgis;
} t_location;

typedef struct s_session
{
    std::string username;
    std::string role;
    time_t last_access;
} Session;

typedef struct s_server
{
    size_t client_max_body_size;
    std::string root;
    std::string server_name;
    t_return return_directive;
    std::vector<std::string> ports;
    std::vector<t_location>locations;
    std::vector<std::string> indexes;
    std::vector<std::string> full_addresses;
    std::map<int, std::string> error_pages;
    std::map<std::string, t_cgi> cgis;
} t_server;

class ConfigParser
{
    private:
        t_global global;
        std::vector<t_token> tkns;
        std::vector<t_server> serverblock;
    public:
        //Função geral
        void parsing(std::fstream &config_file);
        
        //Tokenizador para verifcação
        void tokenizer(std::string str);
        
        //Validação sintática
        void verify_semicolons_and_brackets();
        void verify_unclosed_brackets();
        void verify_block_openings();
        void verify_unique_directives();
        void verify_numeric_directives();
        void verify_hierarchy();
        void verify_empty_line();
        void verify_duplicates();
        void verify_require_directives();
        void verify_http_methods();
        void verify_path_format();
        void verify_return_codes();
        void verify_tokens();

        // Helpers
        void init_return(t_return &ret);
        void init_cgi(t_cgi &cgi);
        void init_location(t_location &loc);
        void init_server(t_server &serve);
        void init_global(t_global &glob);

        //Getter
        const std::vector<t_server> &getServerBlocks() const;

        //Validação semântica
        void validate_server_directives(t_server servering);
        void validate_location_directives(t_server servering);
        void validate_listening_conditions();
        void validate_file_paths(t_server servering);
        void validate_cgi_executables(t_server servering);
        void validate_upload_paths(t_server servering);
        void validate_bidy_size_limits(t_server servering);
        void validate_parse_data();

        //Preenchimento das estruturas
        t_location parse_location_block(size_t &j);
        void parse_config_data();
        void fill_global_scope(size_t &i);

        //Auxiliares
        void apply_inheritance();

        //Forma Canônica Ortodoxa
        ConfigParser();
        ConfigParser(const ConfigParser &obj);
        ConfigParser &operator=(const ConfigParser &obj);
        ~ConfigParser();
};

#endif