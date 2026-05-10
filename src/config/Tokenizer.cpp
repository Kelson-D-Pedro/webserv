/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Tokenizer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mebo <mebo@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/18 08:43:15 by mebo              #+#    #+#             */
/*   Updated: 2026/01/21 15:31:32 by mebo             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/config/ConfigParser.hpp"

static bool is_word_char(char c)
{
    return (std::isalpha((unsigned char)c) || std::isdigit((unsigned char)c) ||
    c == '_' || c == '.' || c == '/' || c == '%' || c == '+' ||
    c == '-' || c == '~' || c == ':' || c == '@' || c == '^' || c == '$' || c == '\\');
}

static void give_an_advance(t_token &token, size_t &j, int i, std::string &curr, std::string str, char type)
{
    if (type == 'w')
    {
        while (j < str.size() && is_word_char(str[j]))
            curr.push_back(str[j++]);
        if (!curr.empty())
        {
            token.line = i;
            token.type = "word";
            token.value = curr;
            curr.clear();
        }
    }
    else if (type == 's')
    {
        char quote = str[j - 1];
    
        while (j < str.size() && str[j] != quote && str[j] != '\n')
            curr.push_back(str[j++]);
        if (j == str.size() || str[j] == '\n')
        {
            std::cerr << "Error (line " << i << ") : ";
            throw(std::runtime_error("unterminated string at line "));
        }
        j++;
        if (!curr.empty())
        {
            token.line = i;
            token.type = "string";
            token.value = curr;
            curr.clear();
        }        
    }
    else if (type == 'n')
    {
        while (j < str.size() && std::isdigit((unsigned char)str[j]))
            curr.push_back(str[j++]);
        if (!curr.empty())
        {
            token.line = i;
            token.type = "number";
            token.value = curr;
            curr.clear();
        }
    }
}

static int is_symbol(char c)
{
    return (c == '{' || c == '}' || c == ';' || c == ',' || c == '#' || c == '=');
}

void    ConfigParser::tokenizer(std::string str)
{
    std::string curr;
    size_t j = 0;
    static int i = 1;

    while (j < str.size())
    {
        if (std::isspace((unsigned char)str[j]))
        {
            if (str[j] == '\n')
                i++;
            j++;
            continue ;
        }
        else if (str[j] == '\"' || str[j] == '\'')
        {
            j++;
            t_token tkn_aux;
            give_an_advance(tkn_aux, j, i, curr, str, 's');
            tkns.push_back(tkn_aux);
            continue ;
        }
        else if (is_symbol(str[j]))
        {
            if (str[j] == '#')
            {
                while (j < str.size() && str[j] != '\n')
                    j++;
                continue ;
            }
            t_token tkn_sym;

            tkn_sym.line = i;
            tkn_sym.type = "symbol";
            tkn_sym.value = str[j];
            tkns.push_back(tkn_sym);
            j++;
            continue ;
        }
        else if (std::isdigit(str[j]))
        {
            size_t k = j;
            int is_olny_digits = 1;
            while (k < str.size() && !std::isspace((unsigned char)str[k]) && !is_symbol(str[k]))
            {
                if (!std::isdigit((unsigned char)str[k]))
                {
                    is_olny_digits = 0;
                    break ;
                }
                k++;
            }
            t_token tkn_aux;
            if (is_olny_digits)
                give_an_advance(tkn_aux, j, i, curr, str, 'n');
            else
                give_an_advance(tkn_aux, j, i, curr, str, 'w');
            tkns.push_back(tkn_aux);
            continue ;
        }
        else if (std::isalpha(str[j]) || is_word_char(str[j]))
        {
            t_token tkn_aux;
            give_an_advance(tkn_aux, j, i, curr, str, 'w');
            tkns.push_back(tkn_aux);
            continue ;
        }
        j++;
    }
}
