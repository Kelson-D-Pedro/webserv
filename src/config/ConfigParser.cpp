/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: orisaebo <orisaebo@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/17 17:46:19 by mebo              #+#    #+#             */
/*   Updated: 2026/02/02 21:29:39 by orisaebo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../../include/config/ConfigParser.hpp"

ConfigParser::ConfigParser()
{
}

ConfigParser::ConfigParser(const ConfigParser &obj)
{
    (void)obj;
}
/*
ConfigParser    &ConfigParser::operator=(const ConfigParser &obj)
{
    (void)obj;
}
*/

const std::vector<t_server> &ConfigParser::getServerBlocks() const
{
    return (serverblock);
}

ConfigParser::~ConfigParser()
{
}

static void read_config_str(std::fstream &file, std::string &str)
{
    std::string line;
    bool has_content = false;
    file.seekg(0, std::ios::beg);

    while (std::getline(file, line))
    {
        str += line + '\n';
        // Verifica se há algum carácter que não seja espaço ou tab ou newline
        for (size_t i = 0; i < line.size(); ++i)
        {
            if (line[i] != ' ' && line[i] != '\t')
            {
                has_content = true;
                break ;
            }
        }
    }
    if (!has_content)
    {
        std::cerr << "Error: ";
        throw (std::runtime_error("file with no content"));
    }
}

void    ConfigParser::parsing(std::fstream &config_file)
{
    std::string file_str;

    read_config_str(config_file, file_str);
    config_file.close();
    tokenizer(file_str);
    /*for (size_t i = 0; i < tkns.size(); i++)
    {
        std::cout << "Type: " << tkns[i].type << " --" << " Value: " << tkns[i].value << " --" << " Line: " << tkns[i].line << std::endl;
    }*/
   verify_tokens();
   parse_config_data();
   validate_parse_data();
}
