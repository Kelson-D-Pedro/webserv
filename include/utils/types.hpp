# ifndef TYPES__HPP
# define TYPES__HPP

# include "../http/Request.hpp"
# include "../http/Response.hpp"
# include "../http/Method.hpp"
# include "../core/Socket.hpp"
# include "../cgi/CgiHandler.hpp"

class   Requests;
class   Response;
class   Socket;
class   Method;

enum ClientState {
    STATE_READING_REQUEST,    // Lendo requisição HTTP
    STATE_EXECUTING_CGI,      // CGI em execução (não enviar resposta ainda)
    STATE_WRITING_RESPONSE,   // Enviando resposta ao cliente
    STATE_CLOSED              // Conexão encerrada
};

typedef struct s_http
{
    int         client_fd;
    Socket      *server;
    CgiHandler  *cgi;
    Requests    *req;
    Response    *res;
    time_t      last_activity;
    ClientState state;        // Estado explícito do cliente
} t_http;


# endif