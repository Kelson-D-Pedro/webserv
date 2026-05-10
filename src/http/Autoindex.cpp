/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Autoindex.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mebo <mebo@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/15 16:54:47 by mebo              #+#    #+#             */
/*   Updated: 2026/01/21 15:28:59 by mebo             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/http/Autoindex.hpp"
#include "../../include/http/Method.hpp"

// Função para URL encoding
std::string Autoindex::urlEncode(const std::string &str)
{
    std::string encoded;
    for (size_t i = 0; i < str.size(); ++i)
    {
        unsigned char c = str[i];
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/')
            encoded += c;
        else
        {
            char hex[4];
            encoded += hex;
        }
    }
    return encoded;
}

// Função para HTML escaping
std::string Autoindex::htmlEscape(const std::string &str)
{
    std::string escaped;
    for (size_t i = 0; i < str.size(); ++i)
    {
        unsigned char c = str[i];
        switch (c)
        {
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            case '&':
                escaped += "&amp;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            case '\'':
                escaped += "&#39;";
                break;
            default:
                escaped += c;
                break;
        }
    }
    return escaped;
}

// Função para formatar tamanho
std::string Autoindex::formatSize(off_t size)
{
    std::ostringstream oss;

    if (size < 1024)
        oss << size << "B";
    else if (size < 1024 * 1024)
        oss << std::fixed << std::setprecision(1) << (size / 1024.0) << "K";
    else if (size < 1024 * 1024 * 1024)
        oss << std::fixed << std::setprecision(1) << (size / (1024.0 * 1024.0)) << "M";
    else
        oss << std::fixed << std::setprecision(1) << (size / (1024.0 * 1024.0)) << "G";
    return oss.str();
}

// Função para formatar data
std::string Autoindex::formatTime(time_t time)
{
    char buffer[32];
    struct tm *tm_info = std::localtime(&time);
    std::strftime(buffer, sizeof(buffer), "%d-%b-%Y %H:%M", tm_info);
    return std::string(buffer);
}

// Comparador para ordenação alfabética
bool compareEntries(const DirEntry &a, const DirEntry &b)
{
    // Diretórios primeiro, depois ficheiros
    if (a.is_dir != b.is_dir)
        return a.is_dir;
    // Alfabeticamente
    return a.name < b.name;
}

std::string Autoindex::generate(std::string &dir_path, std::string &req_path, std::map<int, std::string> error_pages, Response *response)
{
    DIR *dir = opendir(dir_path.c_str());
    std::stringstream html;

    // Verificar permissões de leitura
    if (!dir)
    {
        std::map<int, std::string>::iterator it = error_pages.find(403);
        if (it != error_pages.end() && !it->second.empty())
        {
            html << it->second;
            response->setStatus(403);
            return html.str();
        }
        else
        {
            html << "<html><body><h1>403 Forbidden</h1></body></html>";
            response->setStatus(403);
            return (html.str());
        }
    }

    // Ler e armazenar entradas
    std::vector<DirEntry> entries;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == ".")
            continue;
            
        DirEntry dir_entry;
        dir_entry.name = name;
        
        // Obter informações do ficheiro
        std::string full_path = dir_path;
        if (full_path[full_path.length() - 1] != '/')
            full_path += '/';
        full_path += name;
        
        struct stat file_stat;
        if (stat(full_path.c_str(), &file_stat) == 0)
        {
            dir_entry.is_dir = S_ISDIR(file_stat.st_mode);
            dir_entry.size = file_stat.st_size;
            dir_entry.mtime = file_stat.st_mtime;
        }
        else
        {
            dir_entry.is_dir = false;
            dir_entry.size = 0;
            dir_entry.mtime = 0;
        }
        
        entries.push_back(dir_entry);
    }
    closedir(dir);

    // Ordenar entradas alfabeticamente (diretórios primeiro)
    std::sort(entries.begin(), entries.end(), compareEntries);

    // Gerar HTML com estilo melhorado
    std::string safe_req_path = htmlEscape(req_path);
    html << "<html><head><title>Index of " << safe_req_path << "</title>";
    html << "<style>";
    html << "body { font-family: Arial, sans-serif; margin: 20px; }";
    html << "h1 { color: #333; }";
    html << "table { border-collapse: collapse; width: 100%; }";
    html << "th { background-color: #f0f0f0; padding: 10px; text-align: left; border-bottom: 2px solid #ddd; }";
    html << "td { padding: 8px; border-bottom: 1px solid #eee; }";
    html << "tr:hover { background-color: #f9f9f9; }";
    html << "a { text-decoration: none; color: #0066cc; }";
    html << "a:hover { text-decoration: underline; }";
    html << ".dir { font-weight: bold; }";
    html << ".size { text-align: right; }";
    html << "</style></head>";
    html << "<body><h1>Index of " << safe_req_path << "</h1>";
    html << "<table><thead><tr>";
    html << "<th>Name</th><th>Last Modified</th><th class=\"size\">Size</th>";
    html << "</tr></thead><tbody>";

    // Adicionar entrada para diretório pai
    if (req_path != "/")
    {
        std::string parent = req_path;
        if (parent.size() > 1 && parent[parent.size() - 1] == '/')
            parent.erase(parent.size() - 1);
        size_t pos = parent.find_last_of('/');
        if (pos == 0)
            parent = "/";
        else
            parent = parent.substr(0, pos);
        
        html << "<tr><td class=\"dir\"><a href=\"" << parent << "\">../</a></td>";
        html << "<td>-</td><td class=\"size\">-</td></tr>";
    }

    // Adicionar entradas
    for (size_t i = 0; i < entries.size(); ++i)
    {
        std::string link = req_path;
        if (link[link.length() - 1] != '/')
            link += '/';
        link += urlEncode(entries[i].name);
        
        if (entries[i].is_dir)
            link += '/';
        
        std::string display_name = htmlEscape(entries[i].name);
        if (entries[i].is_dir)
            display_name += '/';
        
        html << "<tr>";
        html << "<td" << (entries[i].is_dir ? " class=\"dir\"" : "") << ">";
        html << "<a href=\"" << link << "\">" << display_name << "</a></td>";
        html << "<td>" << formatTime(entries[i].mtime) << "</td>";
        html << "<td class=\"size\">";
        if (entries[i].is_dir)
            html << "-";
        else
            html << formatSize(entries[i].size);
        html << "</td></tr>";
    }

    html << "</tbody></table></body></html>";
    response->setStatus(200);
    return (html.str());
}
