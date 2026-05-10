/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   AuxiliarVerifing.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: orisaebo <orisaebo@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/15 09:31:31 by mebo              #+#    #+#             */
/*   Updated: 2025/11/05 22:34:16 by orisaebo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/config/ConfigParser.hpp"

void    fill_block(std::map<std::string, std::vector<std::string> > &blocks)
{
    blocks["server"].push_back("global");
    blocks["server"].push_back("http");
    blocks["location"].push_back("server");
    blocks["http"].push_back("global");
}

void    fill_directive(std::map<std::string, std::vector<std::string> > &scopes)
{
    scopes["listen"].push_back("server");
    scopes["server_name"].push_back("server");
    scopes["root"].push_back("server");
    scopes["root"].push_back("location");
    scopes["root"].push_back("global");
    scopes["allow_methods"].push_back("location");
    scopes["return"].push_back("server");
    scopes["return"].push_back("location");
    scopes["upload_path"].push_back("location");
    scopes["index"].push_back("server");
    scopes["index"].push_back("location");
    scopes["index"].push_back("global");
    scopes["autoindex"].push_back("location");
    scopes["error_page"].push_back("server");
    scopes["error_page"].push_back("global");
    scopes["default_type"].push_back("global");
    scopes["client_max_body_size"].push_back("global");
    scopes["client_max_body_size"].push_back("server");
    scopes["client_max_body_size"].push_back("location");
}

bool is_valid_listen_arg(const std::string &str)
{
    int colon_count = 0;
    for (size_t i = 0; i < str.size(); i++)
    {
        if (!std::isdigit(str[i]) && str[i] != '.' && str[i] != ':')
            return false;
        if (str[i] == ':')
            colon_count++;
    }
    return (colon_count <= 1 && !str.empty());
}

bool is_valid_body_size_arg(const std::string &str)
{
    if (str.empty())
        return false;

    for (size_t i = 0; i < str.size(); i++)
    {
        if (!std::isdigit(str[i]))
        {
            // permite o último caractere ser K, k, M ou m
            if (i == str.size() - 1 && (str[i] == 'K' || str[i] == 'k' || str[i] == 'M' || str[i] == 'm' || str[i] == 'G' || str[i] == 'g'))
                return true;
            return false;
        }
    }
    return true;
}

void ConfigParser::verify_semicolons_and_brackets()
{
    int curr_line;
    size_t j;
    bool has_bracket = false;
    bool has_semicolon = false;
    bool is_block_header = false;

    curr_line = tkns[0].line;
    for (size_t i = 0; i < tkns.size(); i++)
    {
        has_bracket = false;
        has_semicolon = false;
        is_block_header = false;

        j = i;
        while (j < tkns.size() && tkns[j].line == curr_line)
        {
            const std::string &val = tkns[j].value;

            if (val == "{")
                has_bracket = true;
            else if (val == "}")
                has_bracket = true;
            else if (val == ";")
                has_semicolon = true;

            if (val == "server" || val == "location" || val == "http")
                is_block_header = true;

            j++;
        }
        if (has_bracket && has_semicolon)
        {
            std::cerr << "Error (line " << tkns[i].line << "): ";
            throw (std::runtime_error("Unexpected symbol ';' at line"));
        }
        if (!has_bracket && !has_semicolon && !is_block_header)
        {
            if (is_block_header && j < tkns.size() && tkns[j].value == "{")
            {
                //Tudo okay
            }
            else
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw (std::runtime_error("Missing ';' at end of directive"));
            }
        }
        if (has_semicolon && !has_bracket && is_block_header)
        {
            std::cerr << "Error (line " << tkns[i].line << "): ";
            throw (std::runtime_error("Unexpected symbol ';' at line"));
        }
        if (j < tkns.size())
            curr_line = tkns[j].line;
        i = j - 1;
    }
}

void ConfigParser::verify_unclosed_brackets()
{
    int brackets;

    brackets = 0;
    for (size_t i = 0; i < tkns.size(); i++)
    {
        if (tkns[i].value == "{")
            brackets++;
        else if (tkns[i].value == "}")
        {
            brackets--;
            if (brackets < 0)
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw (std::runtime_error("Unexpected '}' without matching '{'"));
            }
        }
    }
    if (brackets != 0)
    {
        std::cerr << "Error: ";
        throw (std::runtime_error("Unclosed bracket"));
    }
}

void ConfigParser::verify_block_openings()
{
    size_t j;
    std::stack<std::string> st;
    for (size_t i = 0; i < tkns.size(); i++)
    {
        const std::string &curr = tkns[i].value;
        if (curr == "{")
        {
            std::string opener;
            j = i;
            if (j > 0 && tkns[j - 1].line == tkns[i].line)
            {
                while (j > 0 && tkns[j].line == tkns[i].line)
                {
                    j--;
                    if (tkns[j].value == "server" || tkns[j].value == "location" || tkns[j].value == "http")
                    {
                        opener = tkns[j].value;
                        break ;
                    }
                }
            }
            else
            {
                while (j > 0)
                {
                    j--;
                    if (tkns[j].value == "server" || tkns[j].value == "location" || tkns[j].value == "http")
                    {
                        opener = tkns[j].value;
                        break ;
                    }
                }
            }
            if (opener != "server" && opener != "location" && opener != "http")
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw (std::runtime_error("Unexpected '{' - invalid block declaration")); 
            }
            st.push(opener);
        }
        else if (curr == "}")
        {
            if (st.empty())
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw (std::runtime_error("Unexpected '}' without matching '{'"));
            }
            st.pop();
        }
    }
    if (!st.empty())
    {
        std::cerr << "Error: ";
        throw std::runtime_error("Unclosed '{' — some blocks not closed properly");
    }
}

void ConfigParser::verify_hierarchy()
{
    std::map<std::string, std::vector<std::string> > valid_blocks;
    std::map<std::string, std::vector<std::string> > valid_scopes;
    fill_block(valid_blocks);
    fill_directive(valid_scopes);
    std::stack<std::string> blocks;
    blocks.push("global");

    for (size_t i = 0; i < tkns.size(); i++)
    {
        const std::string &val = tkns[i].value;

        if (val == "server" || val == "location" || val == "http")
        {
            const std::string current_scope = blocks.top();
            const std::vector<std::string> &allowed = valid_blocks[val];

            if (std::find(allowed.begin(), allowed.end(), current_scope) == allowed.end())
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw (std::runtime_error("Block \'" + val + "\' not allowed inside \'" + current_scope + "\'"));
            }
            size_t j = i + 1;
            /*while (j < tkns.size() && tkns[j].type == "space")
                j++;*/
            if (j >= tkns.size())
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw std::runtime_error("Expected '{' after block declaration '" + val + "'");
            }
            blocks.push(val);
        }
        else if (val == "}")
        {
            if (blocks.empty())
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw std::runtime_error("Unexpected '}'");
            }
            blocks.pop();
            if (blocks.empty())
                blocks.push("global");
        }
        else if (valid_scopes.find(val) != valid_scopes.end())
        {
            const std::string current_scope = blocks.top();
            std::vector<std::string> &allowed = valid_scopes[val];
            if (std::find(allowed.begin(), allowed.end(), current_scope) == allowed.end())
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw (std::runtime_error("Directive \'" + val + "\' not allowed in \'" + current_scope + "\' context"));
            }
        }
    }
}

void ConfigParser::verify_empty_line()
{
    for (size_t i = 0; i < tkns.size(); i++)
    {
        if (tkns[i].value == ";")
        {
            if (i == 0 || (tkns[i - 1].type != "word" && tkns[i - 1].type != "number" && tkns[i - 1].type != "string"))
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw (std::runtime_error("Directive ';' in empty line"));
            }
        }
        else if (tkns[i].value == "listen" || 
            tkns[i].value == "root" || 
            tkns[i].value == "index" || 
            tkns[i].value == "location" || 
            tkns[i].value == "server_name" || 
            tkns[i].value == "error_page" ||
            tkns[i].value == "allow_methods" ||
            tkns[i].value == "upload_path" ||
            tkns[i].value == "cgi_ext" || 
            tkns[i].value == "cgi_path" ||
            tkns[i].value == "cgi_path" ||
            tkns[i].value == "autoindex")
        {
            if (tkns[i].value == "location") 
            {
                if (i + 1 >= tkns.size() || tkns[i + 1].value == "{")
                {
                    std::cerr << "Error (line " << tkns[i].line << "): ";
                    throw std::runtime_error("Directive 'location' without path");
                }
            }
            else if (i + 1 >= tkns.size() || 
                     (tkns[i + 1].type != "word" && tkns[i + 1].type != "number" && tkns[i + 1].type != "string"))
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw std::runtime_error("Directive '" + tkns[i].value + "' without arguments");
            }
            int arg_counter = 0;
            size_t j = i + 1;
            while (j < tkns.size() && tkns[j].value != ";")
            {
                arg_counter++;
                j++;
            }
            if (arg_counter != 1 && tkns[i].value != "location" && tkns[i].value != "index" && tkns[i].value != "error_page" && tkns[i].value != "allow_methods")
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw (std::runtime_error("Directive \'" + tkns[i].value + "\' with more than one argument"));
            }
        }
        if (tkns[i].value == "location")
        {
            int arg_counter = 0;
            size_t j = i + 1;
            while (j < tkns.size() && tkns[j].value != "{")
            {
                arg_counter++;
                j++;
            }
            if (arg_counter == 2 && (tkns[i + 1].value != "~" && tkns[i + 1].value != "~*"))
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw (std::runtime_error("Directive \'location\' with more than one argument"));
            }
        }
    }
}

void ConfigParser::verify_duplicates()
{
    size_t j;
    int current_line;
    
    for (size_t i = 0; i < tkns.size(); i++)
    {
        std::vector<std::string> aux_container;
        current_line = tkns[i].line;
        j = i;
        while (j < tkns.size() && tkns[j].line == current_line)
        {
            const t_token &tok = tkns[j];
            if (tok.type == "word" || tok.type == "number" || tok.value == ";")
            {
                if (std::find(aux_container.begin(), aux_container.end(), tok.value) != aux_container.end())
                {
                    std::cerr << "Error (line " << tok.line << "): ";
                    throw (std::runtime_error("Duplicate token \'" + tok.value + "\' on the same line"));
                }
                aux_container.push_back(tok.value);
            }
            j++;
        }
        aux_container.clear();
        i = j - 1;
    }
}

void ConfigParser::verify_require_directives()
{
    std::vector<std::string> required;
    std::vector<std::string> values;

    required.push_back("listen");
    required.push_back("root");
    
    for (size_t i = 0; i < tkns.size(); i++)
        values.push_back(tkns[i].value);
    for (size_t i = 0; i < required.size(); i++)
    {
        if (std::find(values.begin(), values.end(), required[i]) == values.end())
        {
            std::cerr << "Error: ";
            throw (std::runtime_error("Missing directive \'" + required[i] + "\'"));
        }
    }
}

void ConfigParser::verify_unique_directives()
{
    std::set<std::string> unique_directives;
    std::set<std::string> multi_allowed;
    
    multi_allowed.insert("listen");
    multi_allowed.insert("error_page");

    for (size_t i = 0; i < tkns.size(); i++)
    {
        std::string val = tkns[i].value;

        if (val == "server" || val == "location")
        {
            unique_directives.clear();
            continue ;
        }
        if (val == "listen" || val == "server_name" || val == "root" ||
            val == "index" || val == "client_max_body_size" || val == "error_page")
        {
            if (unique_directives.count(val) && multi_allowed.count(val) == 0)
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw (std::runtime_error("Duplicate directive '" + val + "' in the same block"));
            }
            unique_directives.insert(val);
        }
    }
}

void ConfigParser::verify_numeric_directives()
{
    for (size_t i = 0; i < tkns.size(); i++)
    {
        if (tkns[i].value == "listen")
        {
            if (i + 1 >= tkns.size() || !is_valid_listen_arg(tkns[i + 1].value))
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw (std::runtime_error("Invalid argument for 'listen' directive: " + tkns[i + 1].value));
            }
        }
        else if (tkns[i].value == "client_max_body_size")
        {
            if (i + 1 >= tkns.size() || !is_valid_body_size_arg(tkns[i + 1].value))
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw (std::runtime_error("Invalid argument for 'client_max_body_size': " + tkns[i + 1].value));
            }
        }
    }
}

void ConfigParser::verify_path_format()
{
    for (size_t i = 0; i < tkns.size(); i++)
    {
        if (tkns[i].value == "root" || tkns[i].value == "upload_path")
        {
            if (i + 1 >= tkns.size())
                continue;
            
            std::string path = tkns[i + 1].value;
            if (path.empty() || (path[0] != '/' && path[0] != '.'))
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw std::runtime_error("Path must be absolute (start with '/'): '" + path + "'");
            }
        }
        else if (tkns[i].value == "location")
        {
            if (i + 1 >= tkns.size())
                continue;
            
            // Location pode ser ~ ou ~* seguido de regex, ou path absoluto
            if (tkns[i + 1].value != "~" && tkns[i + 1].value != "~*")
            {
                std::string path = tkns[i + 1].value;
                if (path.empty() || path[0] != '/')
                {
                    std::cerr << "Error (line " << tkns[i].line << "): ";
                    throw std::runtime_error("Location path must start with '/': '" + path + "'");
                }
            }
        }
    }
}

void ConfigParser::verify_return_codes()
{
    for (size_t i = 0; i < tkns.size(); i++)
    {
        if (tkns[i].value == "return")
        {
            if (i + 1 >= tkns.size() || tkns[i + 1].type != "number")
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw std::runtime_error("'return' requires status code");
            }
            
            int code = std::atoi(tkns[i + 1].value.c_str());
            if (code < 100 || code > 599)
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw std::runtime_error("Invalid HTTP status code: " + tkns[i + 1].value);
            }
        }
        else if (tkns[i].value == "error_page")
        {
            if (i + 1 >= tkns.size() || tkns[i + 1].type != "number")
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw std::runtime_error("'error_page' requires status code");
            }
            
            int code = std::atoi(tkns[i + 1].value.c_str());
            if (code < 400 || code > 599)
            {
                std::cerr << "Error (line " << tkns[i].line << "): ";
                throw std::runtime_error("error_page code must be 4xx or 5xx: " + tkns[i + 1].value);
            }
        }
    }
}

void ConfigParser::verify_http_methods()
{
    std::vector<std::string>alloweds;

    alloweds.push_back("POST");
    alloweds.push_back("PUT");
    alloweds.push_back("GET");
    alloweds.push_back("DELETE");
    alloweds.push_back("HEAD");
    for (size_t i = 0; i < tkns.size(); i++)
    {
        if (tkns[i].value == "allow_methods")
        {
            size_t j = i + 1;
            while (j < tkns.size() && tkns[j].value != ";")
            {
                if (std::find(alloweds.begin(), alloweds.end(), tkns[j].value) == alloweds.end())
                {
                    std::cerr << "Error (line " << tkns[j].line << "): ";
                    throw std::runtime_error("Invalid HTTP method: '" + tkns[j].value + "'");
                }
                j++;
            }
        }
    }
}

void ConfigParser::verify_tokens()
{
    if (tkns.empty())
        return ;
    verify_semicolons_and_brackets();
    verify_unclosed_brackets();
    verify_block_openings();
    verify_hierarchy();
    verify_duplicates();
    verify_empty_line();
    verify_require_directives();
    verify_unique_directives();
    verify_numeric_directives();
    verify_http_methods();
    verify_path_format();
    verify_return_codes();
}
