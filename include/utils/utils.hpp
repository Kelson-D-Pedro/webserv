# ifndef UTILS_HPP
# define UTILS_HPP

#define MAX 4096
# include <iostream>
# include <vector>
# include <sys/epoll.h>
# include <sys/socket.h>
# include <netdb.h>
# include "./types.hpp"
# include <sstream>
# include <ctime>
# include "../core/Socket.hpp"

std::vector<std::string> split(const std::string& str, char c);

int find_client_pos(const std::vector<t_http>& client, int look_for);
int find_server_pos(const std::vector<Socket *>& servers, int look_for);
std::string intToString(size_t number);

// Segurança e uploads
// Tamanho máximo por upload (10 MB)
# define MAX_UPLOAD_SIZE (10 * 1024 * 1024)

// Verifica se o caminho candidato (relativo a root ou absoluto) fica dentro de root.
bool is_path_within_root(const std::string &root, const std::string &candidate);

// Verifica se o caminho está dentro do upload path (necessário para DELETE)
bool is_path_inside_upload_path(const std::string &root, const std::string &candidate);


// Sanitiza um nome de ficheiro (remove barras, sequences .. e caracteres perigosos).
std::string sanitize_filename(const std::string &name);

// Garante que o diretório existe, criando os diretórios pai se necessário.
bool ensure_dir_exists(const std::string &path);

bool extractFileFromMultipart(const std::vector<char> &body, const std::string &boundary, std::vector<char> &fileData);
bool extractFilenameFromMultipart(const std::vector<char> &body, const std::string &boundary, std::string &filename);


# endif