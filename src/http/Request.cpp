#include "../../include/http/Request.hpp"

Requests::Requests() : is_chunked(false), header_too_large(false), flag(false)
{
}
Requests::~Requests()
{
    
}

std::string Requests::getMethod() const 
{
    return method;
}

std::string Requests::getPath() const
{
    return path;
}

std::string Requests::getQueryString() const
{
    return query_string;
}

std::string Requests::getHttpVersion() const
{
    return http_version;
}

std::string Requests::getHeader(const std::string &key) const
{
    // Use map for O(log n) lookup instead of O(n) linear search
    std::map<std::string, std::string>::const_iterator it = headers_map.find(key);
    if (it != headers_map.end())
        return it->second;
    return "";
}

std::string Requests::getCookieID() const
{
    return cookies_session_id;
}

const std::vector<char>& Requests::getBody() const
{
    return body;
}

std::string Requests::getFull_req() const
{
    return full_req;
}

const std::vector<char>& Requests::getFull_req_body() const
{
    return full_req_body;
}

bool Requests::isChunked() const
{
    return is_chunked;
}

bool Requests::isHeaderTooLarge() const
{
    return header_too_large;
}

bool Requests::isValidMethod() const
{
    return (method == "GET" || method == "POST" || method == "DELETE");
}

void Requests::clear()
{
    cookies_session_id.clear();
    body.clear();
    full_req.clear();
    full_req_body.clear();
    headers_map.clear();
    http_version.clear();
    method.clear();
    path.clear();
    query_string.clear();
    is_chunked = false;
    header_too_large = false;
}

void Requests::normalizePath()
{
    std::vector<std::string> splited_path = split(path, '/');
    std::vector<std::string> final_path;

    for (size_t i = 0; i < splited_path.size(); ++i)
    {
        const std::string& part = splited_path[i];  // Use const reference to avoid copy
        if (part.empty() || part == ".")
            continue;
        else if (part == "..")
        {
            if (!final_path.empty())
                final_path.pop_back();
        }
        else
        {
            final_path.push_back(part);
        }
    }

    // Pre-calculate the size needed for the normalized path to avoid reallocations
    // Format is "/part1/part2/part3" - initial slash plus each part plus separating slashes
    size_t total_size = 1; // for initial '/'
    for (size_t i = 0; i < final_path.size(); ++i)
    {
        total_size += final_path[i].size();
        if (i < final_path.size() - 1)
            total_size += 1;  // for separator '/' between parts
    }
    
    std::string normalized;
    normalized.reserve(total_size);
    normalized = "/";

    for (size_t i = 0; i < final_path.size(); ++i)
    {
        normalized += final_path[i];
        if (i < final_path.size() - 1)
            normalized += "/";
    }

    if (normalized.empty())
        normalized = "/";

    path = normalized;
}

void Requests::parseQueryString()
{
    size_t query_pos = path.find('?');
    if (query_pos != std::string::npos)
    {
        query_string = path.substr(query_pos + 1);
        path = path.substr(0, query_pos);
    }
}

void Requests::parseChunkedBody()
{
    size_t header_end = full_req.find("\r\n\r\n");
    if (header_end == std::string::npos)
        return;

    size_t pos = header_end + 4;
    body.clear();

    while (pos < full_req_body.size())
    {
        // Find chunk size line ending
        size_t chunk_end = full_req.find("\r\n", pos);
        if (chunk_end == std::string::npos)
            break;

        // Extract chunk size (in hex)
        std::string chunk_size_str(full_req.begin() + pos, full_req.begin() + chunk_end);
        
        // Remove any chunk extensions (after ';')
        size_t semicolon = chunk_size_str.find(';');
        if (semicolon != std::string::npos)
            chunk_size_str = chunk_size_str.substr(0, semicolon);

        // Convert hex to decimal
        char* endptr;
        long chunk_size = std::strtol(chunk_size_str.c_str(), &endptr, 16);
        
        // Check if conversion was successful
        if (endptr == chunk_size_str.c_str() || chunk_size < 0)
            break;

        // If chunk size is 0, we've reached the end
        if (chunk_size == 0)
            break;

        // Move past the chunk size line
        pos = chunk_end + 2;

        // Check if we have enough data for this chunk
        if (pos + chunk_size > full_req_body.size())
            break;

        // Append chunk data to body
        body.insert(body.end(), 
                   full_req_body.begin() + pos, 
                   full_req_body.begin() + pos + chunk_size);

        // Move past the chunk data and trailing \r\n
        pos += chunk_size + 2;
    }
}

void Requests::readRequest(int fd)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(fd, buffer, BUFFER_SIZE, 0);

    if (bytes_read > 0)
    {
        full_req.append(buffer, bytes_read);
        full_req_body.insert(full_req_body.end(), buffer, buffer + bytes_read);

        size_t header_end = full_req.find("\r\n\r\n");
        if (header_end == std::string::npos && full_req.size() > MAX_HEADER_SIZE)
            header_too_large = true;
    }
    else if (bytes_read == 0)
    {
        return ;
    }
    else // bytes_read == -1
    {
        this->flag = true;
        return;
    }
}

void    Requests::parseCookies()
{
    std::string header = getHeader("Cookie");
    size_t pos = header.find("session_id=", 0);
    if (pos == std::string::npos)
        return ;
    pos += 11;
    size_t final_pos = header.find(";", pos);
    if (final_pos == std::string::npos)
        final_pos = header.length();
    cookies_session_id = header.substr(pos, final_pos - pos);
}

void Requests::parseRequest()
{
    size_t header_end = full_req.find("\r\n\r\n");
    if (header_end == 0 || full_req_body.size() < header_end + 4)
        return;

    std::string header_part(full_req.begin(), full_req.begin() + header_end);
    body.clear();
    if (full_req_body.size() > header_end + 4)
        body.insert(body.end(), full_req_body.begin() + header_end + 4, full_req_body.end());

    std::vector<std::string> lines = split(header_part, '\n');
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (!lines[i].empty() && lines[i][lines[i].size() - 1] == '\r')
            lines[i].erase(lines[i].size() - 1, 1);
    }

    std::vector<std::string> first_line = split(lines[0], ' ');
    if (first_line.size() == 3)
    {
        method = first_line[0];
        path = first_line[1];
        http_version = first_line[2];
    }

    // Parse headers into map for O(log n) lookup instead of storing raw strings
    for (size_t i = 1; i < lines.size(); ++i)
    {
        size_t pos = lines[i].find(':');
        if (pos != std::string::npos)
        {
            std::string key = lines[i].substr(0, pos);
            std::string value = lines[i].substr(pos + 1);
            // Trim leading whitespace from value
            size_t start = 0;
            while (start < value.size() && value[start] == ' ')
                ++start;
            if (start > 0)
                value = value.substr(start);
            headers_map[key] = value;
        }
    }

    //Parse para Cookies
    std::string cookie_header = getHeader("Cookie");
    if (!cookie_header.empty())
        parseCookies();
    // Check for Transfer-Encoding: chunked
    std::string transfer_encoding = getHeader("Transfer-Encoding");
    if (transfer_encoding.find("chunked") != std::string::npos)
    {
        is_chunked = true;
        parseChunkedBody();
    }
    // Parse query string before normalizing path
    parseQueryString();
    if (!path.empty() && path[0] == '/')
        normalizePath();
}

bool Requests::requestComplete()
{
    size_t header_end = full_req.find("\r\n\r\n");
    if (header_end == std::string::npos)
        return false;

    // Check for Transfer-Encoding: chunked
    size_t te_pos = full_req.find("Transfer-Encoding:");
    if (te_pos != std::string::npos && te_pos < header_end)
    {
        size_t te_start = te_pos + 18;
        size_t te_end = full_req.find("\r\n", te_start);
        std::string te_value = full_req.substr(te_start, te_end - te_start);
        
        // Trim leading spaces
        size_t start = 0;
        while (start < te_value.size() && te_value[start] == ' ')
            ++start;
        if (start > 0)
            te_value = te_value.substr(start);
        
        if (te_value.find("chunked") != std::string::npos)
        {
            // For chunked encoding, look for final chunk "0\r\n\r\n"
            return full_req.find("0\r\n\r\n", header_end + 4) != std::string::npos;
        }
    }

    // Check for Content-Length
    size_t pos = full_req.find("Content-Length:");
    if (pos != std::string::npos && pos < header_end)
    {
        size_t start = pos + 15;
        size_t end = full_req.find("\r\n", start);
        int content_length = std::atoi(full_req.substr(start, end - start).c_str());
        size_t total_length = header_end + 4 + content_length;
        return (full_req.size() >= total_length);
    }

    return true;
}
