# ifndef CGIHANDLER_HPP
# define CGIHANDLER_HPP

# include <string>
# include <vector>
# include <map>
# include <sys/wait.h>
# include <fcntl.h>
# include <ctime>
# include <cerrno>
# include <cstring>
# include <iostream>
# include <cstdlib>

class CgiHandler
{
    
    private:

        std::string _script_path;
        std::string _root;
        std::string _interpreter_path;
        std::map<std::string, std::string> _env;
        int _timeout_seconds;
        
        char** buildEnvArray() const;
        void freeEnvArray(char** env) const;
        
        public:
        
        CgiHandler(const std::string& script, const std::string& interpreter, std::string root);
        ~CgiHandler();
        
        void setEnv(const std::string& key, const std::string& value);
        void setStdin(const std::vector<char>& data);
        void setTimeout(int seconds);
        
        // Pipes e PID públicos para integração com epoll
        int stdout_fd;
        int stdin_fd;
        pid_t pid;
        size_t _stdin_offset;
        std::vector<char> stdin_data;
        
        std::string output;
        time_t start_time;

        std:: string    getInterpreter_path(void);
        
        bool finished;      // sinaliza que o CGI terminou
        int exit_code;
        bool output_done;   // stdout foi completamente lido (EOF recebido)
        bool input_done;    // stdin foi completamente escrito
    
        // Inicializa e cria o processo CGI (não-bloqueante)
        bool execute();
    
        // Leitura parcial do stdout
        void readStdout();
        
        // Escrita parcial do stdin
        void writeStdin();
    
        // Checa se o processo terminou
        void checkProcess();

        void killProcess();

};

# endif