# ifndef METHOD_HPP
# define METHOD_HPP

# include "./Request.hpp"
# include "./Response.hpp"
# include <ctime>
# include "../config/ConfigParser.hpp"
# include "../cgi/CgiHandler.hpp"

class   Requests;
class   Response;
class   CgiHandler;

class Method
{
    private:
        Requests *request;
        Response *response;
        std::string root;
        std::map<std::string, Session> &sessions;
    
    public:
    
        Method(Requests* req, Response* res, const std::string& root_dir, std::map<std::string, Session> &session_f, int poll_fd);
        ~Method();
        void    getMethod(const t_server *client);
        void    postMethod(const t_server *client);
        void    deleteMethod(const t_server *client);
        void    executeMethod(const t_server *client);

        //Auxiliar functions para ajudar na melhoria dos métodos (GET e POST até agora)
        const t_location *match_location(const std::string &path, const t_server *client);
        void    handle_errors_status_pages(int code, const t_server *client);
        void    handle_errors_status_pages_closing(int code, const t_server *client);
        void    building_autoindex(const t_location *loc, std::string &path, const t_server *client);
        void    build_file_content(std::string &bd);
        void    handle_redirection(const t_location *aux);
        void    handle_redirection(const t_server *client);
        std::string    extract_file_name();
        std::string get_final_filename();
        void    support_keepAlive();
        
        //Bonus
        void    handle_session_with_cookies(std::string new_location);
        void    handle_cookies(std::string boundary);
        //Funções de refatoração
        bool    handle_redirections_if_needed(const t_server *client, const t_location *aux);
        bool    method_allowed(const t_server *client, const t_location *aux);
        bool    handle_cgi_if_needed_for_post(const t_server *client, const t_location *aux, std::string full_path, std::string root);
        void    handle_cgi_if_needed_for_get(const t_server *client, const t_location *aux, std::string full_path, std::string extension, std::string root);
        bool    validate_upload_headers(const t_server *client, const t_location *aux);
        bool    validate_body_length(const t_server *client, size_t body, size_t max_size);
        
        int     poll_fd;
        CgiHandler  *cgi;
};


# endif