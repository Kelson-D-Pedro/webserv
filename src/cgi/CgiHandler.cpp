#include "../../include/cgi/CgiHandler.hpp"

CgiHandler::CgiHandler(const std::string& script, const std::string& interpreter, std::string root)
    : _script_path(script),
      _root(root),
      _interpreter_path(interpreter),
      _timeout_seconds(5),
      stdout_fd(-1),
      stdin_fd(-1),
      pid(-1),
      _stdin_offset(0),
      start_time(0),
      finished(false),
      exit_code(-1),
      output_done(false),
      input_done(false)
{}

std::string CgiHandler::getInterpreter_path(void)
{
    return (this->_interpreter_path);
}


CgiHandler::~CgiHandler()
{
    if (stdout_fd != -1) close(stdout_fd);
    if (stdin_fd != -1) close(stdin_fd);
}

void CgiHandler::setEnv(const std::string& key, const std::string& value)
{
    _env[key] = value;
}

void CgiHandler::setStdin(const std::vector<char>& data)
{
    stdin_data = data;
    _stdin_offset = 0;
}

void CgiHandler::setTimeout(int seconds)
{
    _timeout_seconds = seconds;
}

char** CgiHandler::buildEnvArray() const
{
    char** env = new char*[_env.size() + 1];
    size_t i = 0;
    for (std::map<std::string,std::string>::const_iterator it = _env.begin(); it != _env.end(); ++it) {
        std::string entry = it->first + "=" + it->second;
        env[i] = new char[entry.size() + 1];
        std::strcpy(env[i], entry.c_str());
        ++i;
    }
    env[i] = NULL;
    return env;
}

void CgiHandler::freeEnvArray(char** env) const
{
    for (size_t i = 0; env[i] != NULL; ++i) delete[] env[i];
    delete[] env;
}

bool CgiHandler::execute()
{
    int pipe_in[2];
    int pipe_out[2];
    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1)
        return false;
    size_t pos = _script_path.find_last_of('/');
    std::string script_relative;
    if (pos == std::string::npos)
        script_relative = _script_path;
    else
        script_relative = _script_path.substr(pos + 1);

    pid = fork();
    if (pid == -1) 
        return false;

    if (pid == 0) {
        chdir(_root.c_str());
        // Child
        close(pipe_in[1]);
        close(pipe_out[0]);
        dup2(pipe_in[0], STDIN_FILENO);
        dup2(pipe_out[1], STDOUT_FILENO);
        close(pipe_in[0]);
        close(pipe_out[1]);

        char** env = buildEnvArray();
        char* argv[3];
        argv[0] = const_cast<char*>(_interpreter_path.c_str());
        argv[1] = const_cast<char*>(script_relative.c_str());
        argv[2] = NULL;
        execve(argv[0], argv, env);
        std::cerr << "CGI execve failed\n";
        std::exit(1);
    }

    // Parent
    close(pipe_in[0]);
    close(pipe_out[1]);
    stdout_fd = pipe_out[0];
    stdin_fd = pipe_in[1];

    // Pipes não-bloqueantes
    fcntl(stdout_fd, F_SETFL, O_NONBLOCK);
    fcntl(stdin_fd, F_SETFL, O_NONBLOCK);

    start_time = std::time(NULL);
    finished = false;
    exit_code = -1;

    return (true);
}

void CgiHandler::readStdout()
{
    if (stdout_fd == -1)
        return;

    char buf[4096];
    ssize_t n;

    // Ler até não haver mais dados disponíveis (EAGAIN/EWOULDBLOCK)
    while ((n = read(stdout_fd, buf, sizeof(buf))) > 0)
    {
        output.append(buf, n);
    }

    // EOF: o CGI fechou stdout - marcar como concluído
    if (n == 0)
    {
        output_done = true;
        close(stdout_fd);
        stdout_fd = -1;
    }
    else if (n < 0)
        return ;
    // n < 0: EAGAIN/EWOULDBLOCK → aguardar próximo evento EPOLLIN
    // Não precisa verificar errno explicitamente - simplesmente retornar
}


void CgiHandler::writeStdin()
{
    if (stdin_fd == -1)
        return;

    // Escrever até que todo stdin seja enviado ou EAGAIN/EWOULDBLOCK ocorra
    while (_stdin_offset < stdin_data.size())
    {
        ssize_t n = write(stdin_fd,
                          &stdin_data[_stdin_offset],
                          stdin_data.size() - _stdin_offset);

        if (n > 0)
        {
            _stdin_offset += n;
        }
        else if (n < 0)
        {
            // Erro (incluindo EPIPE se CGI fechou stdin prematuramente)
            // Fechar stdin e marcar como concluído para evitar novas tentativas
            input_done = true;
            close(stdin_fd);
            stdin_fd = -1;
            return;
        }
        else
        {
            // n == 0 não deveria acontecer com write(), mas tratar como EAGAIN
            return;
        }
    }

    // Todo stdin foi escrito - fechar o pipe
    if (_stdin_offset == stdin_data.size())
    {
        input_done = true;
        close(stdin_fd);
        stdin_fd = -1;
    }
}

void CgiHandler::checkProcess()
{
    if (finished)
        return;

    // Verificar se o processo filho terminou (não-bloqueante)
    int status;
    pid_t ret = waitpid(pid, &status, WNOHANG);
    if (ret == pid)
    {
        finished = true;
        if (WIFEXITED(status))
            exit_code = WEXITSTATUS(status);
        else
            exit_code = -1;
        if (exit_code != 0)
            exit_code = -3;
    }

    // Timeout: garantir que o CGI seja encerrado mesmo sem eventos de I/O
    // Isso evita que CGIs travados bloqueiem o servidor indefinidamente
    if (!finished && std::difftime(std::time(NULL), start_time) > _timeout_seconds)
    {
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0); // Aguardar término para evitar zombies
        finished = true;
        exit_code = -2;
    }
}

void CgiHandler::killProcess()
{
    if (finished)
        return;

    // 1️⃣ Tentar encerramento educado
    if (pid > 0)
        kill(pid, SIGTERM);

    // 2️⃣ Esperar um pouco (não-bloqueante)
    for (int i = 0; i < 5; ++i)
    {
        int status;
        pid_t result = waitpid(pid, &status, WNOHANG);

        if (result == pid)
        {
            finished = true;
            exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
            return;
        }
        //usleep(100000); // 100ms
    }

    // 3️⃣ Se ainda está vivo, matar sem negociação
    kill(pid, SIGKILL);

    int status;
    waitpid(pid, &status, 0);

    finished = true;
    exit_code = 1;
}

