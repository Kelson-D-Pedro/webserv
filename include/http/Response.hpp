# ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <cerrno>
# include <cctype>
# include <cstdlib>
# include <cstring>
# include <cstdio>
# include <fstream>  
# include <iostream>
# include <map>
# include <sstream> 
# include "../utils/utils.hpp"
# include "./Request.hpp"
# include "../../include/http/MimeTypes.hpp"
# include "../../include/http/HttpStatus.hpp"


class Requests;

class Response
{
    private:

        std::string                 http_version;
        std::string                 reason_phrase;
        std::map<std::string, std::string> headers;
        std::string                 body;
        std::string                 full_response;
        int                         status_code;
        bool                        keep_alive;
        std::string                 file_path;
        bool                        is_cgi;
        size_t                      content_length;
        bool                        use_chunked;
    
    public:
        Response();
        ~Response();

        void setHttpVersion(const std::string &version);
        void setStatus(int code);
        void setBody(std::string bod);
        void setHeader(const std::string &key, const std::string &value);
        void setContentType(const std::string &type);
        void setKeepAlive(bool condition);
        void setUseChunked(bool chunked);
        
        int getStatus() const;
        std::string getHeader(std::string key);
        std::string getBody() const;
        std::string getFullResponse() const;
        bool getKeepAlive() const;
        bool getUseChunked() const;
        bool flag;

        void buildHeaders();
        void buildResponse();
        void generateErrorPage(const std::string &path, const struct stat file_info, int code);
        void generateErrorPage(int code);
        void setFileResponse(const std::string &path, const struct stat file_info);
        void prepareCGIResponse(const std::string &cgi_output);
        void clear();

        void sendResponse(int client_fd);
        void sendFile(int client_fd);
        void sendChunked(int client_fd);

};


# endif
