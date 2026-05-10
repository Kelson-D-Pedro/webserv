
# include "../../include/utils/utils.hpp"
# include "../../include/utils/types.hpp"

std::vector<std::string> split(const std::string& str, char c)
{
    size_t  begin;
    size_t  end;
    std::vector<std::string> matrix;

    begin = 0;
    while ((end = str.find(c, begin)) != std::string::npos)
    {
        matrix.push_back(str.substr(begin, end - begin));
        begin = end + 1;
    }
    matrix.push_back(str.substr(begin));
    return (matrix);
}

int find_client_pos(const std::vector<t_http>& client, int look_for)
{
    for (size_t i = 0; i < client.size(); i++)
        if (client[i].client_fd == look_for)
            return (i);
    return (-1);   
}

int find_server_pos(const std::vector<Socket *>& servers, int look_for)
{
    for (size_t i = 0; i < servers.size(); i++)
    {
        if (servers[i]->getSocketFd() == look_for)
            return (i);
    }
    return (-1);
}

std::string intToString(size_t number)
{
    std::ostringstream oss;
    oss << number;
    return oss.str();
}

std::string normalize_path(const std::string &base,
                           const std::string &path)
{
    std::vector<std::string> stack;
    std::string full = base;

    if (!full.empty() && full[full.size() - 1] != '/')
        full += '/';

    full += path;

    std::stringstream ss(full);
    std::string part;

    while (std::getline(ss, part, '/'))
    {
        if (part.empty() || part == ".")
            continue;

        if (part == "..")
        {
            if (!stack.empty())
                stack.pop_back();
            continue;
        }

        stack.push_back(part);
    }

    std::string result = "/";
    for (size_t i = 0; i < stack.size(); i++)
    {
        result += stack[i];
        if (i + 1 < stack.size())
            result += "/";
    }

    return result;
}

/*bool is_path_within_root(const std::string &root, const std::string &candidate)
{
    char real_root_buf[MAX];
    char real_parent_buf[MAX];
    struct stat st;

    if (root.empty())
        return false;

    // Canonicalizar root
    if (realpath(root.c_str(), real_root_buf) == NULL)
        return false;

    std::string real_root(real_root_buf);

    // Remover trailing slash manualmente (sem .back / sem .pop_back)
    if (!real_root.empty() && real_root[real_root.size() - 1] == '/')
        real_root = real_root.substr(0, real_root.size() - 1);

    // Construir o caminho alvo
    std::string combined;

    if (!candidate.empty() && candidate[0] == '/')
        combined = real_root + candidate;
    else
        combined = real_root + "/" + candidate;

    // Verificar nome do arquivo (não pode começar por '.')
    size_t last = candidate.find_last_of('/');
    std::string filename =
        (last == std::string::npos) ? candidate : candidate.substr(last + 1);

    if (!filename.empty() && filename[0] == '.')
        return false;

    // Calcular diretório pai
    std::string parent = combined;
    size_t pos = parent.find_last_of('/');
    if (pos == std::string::npos)
        parent = ".";
    else if (pos == 0)
        parent = "/";
    else
        parent = parent.substr(0, pos);

    // Canonicalizar parent
    if (realpath(parent.c_str(), real_parent_buf) == NULL)
        return false;

    std::string real_parent(real_parent_buf);

    // Verificar se parent está dentro de root
    if (real_parent.size() < real_root.size())
        return false;

    if (real_parent.compare(0, real_root.size(), real_root) != 0)
        return false;

    if (real_parent.size() > real_root.size() &&
        real_parent[real_root.size()] != '/')
        return false;

    // Se o arquivo existir, verificar caminho final real
    if (stat(combined.c_str(), &st) == 0)
    {
        char real_final_buf[MAX];
        if (realpath(combined.c_str(), real_final_buf) == NULL)
            return false;

        std::string real_final(real_final_buf);

        if (real_final.size() < real_root.size())
            return false;

        if (real_final.compare(0, real_root.size(), real_root) != 0)
            return false;

        if (real_final.size() > real_root.size() &&
            real_final[real_root.size()] != '/')
            return false;
    }

    return true;
}

bool is_path_inside_upload_path(const std::string &root, const std::string &candidate)
{
    char real_root_buf[MAX];
    char real_candidate_buf[MAX];

    if (root.empty() || candidate.empty())
        return false;

    if (realpath(root.c_str(), real_root_buf) == NULL)
        return false;

    if (realpath(candidate.c_str(), real_candidate_buf) == NULL)
        return false;

    std::string real_root(real_root_buf);
    std::string real_candidate(real_candidate_buf);

    // garantir separador
    if (real_root[real_root.size() - 1] != '/')
        real_root += '/';

    if (real_candidate.compare(0, real_root.size(), real_root) != 0)
        return false;

    return true;
}*/

bool is_path_within_root(const std::string &root,
                         const std::string &uri)
{
    if (root.empty())
        return false;

    std::string norm_root = normalize_path("/", root);
    std::string norm_full = normalize_path(norm_root, uri);

    if (norm_full.size() < norm_root.size())
        return false;

    if (norm_full.compare(0, norm_root.size(), norm_root) != 0)
        return false;

    if (norm_full.size() > norm_root.size() &&
        norm_full[norm_root.size()] != '/')
        return false;

    return true;
}

bool is_path_inside_upload_path(const std::string &upload_root, const std::string &target)
{
    std::string norm_root = normalize_path("/", upload_root);
    std::string norm_target = normalize_path(norm_root, target);

    if (norm_target.compare(0, norm_root.size(), norm_root) != 0)
        return false;

    return true;
}

std::string sanitize_filename(const std::string &name)
{
    std::string out;
    for (size_t i = 0; i < name.size(); ++i)
    {
        char c = name[i];
        // allow alnum and a few safe punctuation
        if (std::isalnum((unsigned char)c) || c == '.' || c == '-' || c == '_')
            out.push_back(c);
        // replace slashes/backslashes with underscore
        else if (c == '/' || c == '\\')
            out.push_back('_');
        else
            out.push_back('_');
        if (out.size() >= 255) // filename length limit
            break;
    }
    // remove sequences of dots that could be dangerous as standalone names
    while (out.find("..") != std::string::npos)
        out.replace(out.find(".."), 2, "__");
    if (out.empty())
        out = "upload";
    return out;
}

bool ensure_dir_exists(const std::string &path)
{
    if (path.empty())
        return false;

    std::string cur;
    if (path[0] == '/')
        cur = "/";

    std::vector<std::string> parts = split(path, '/');

    for (size_t i = 0; i < parts.size(); ++i)
    {
        const std::string &part = parts[i];

        if (part.empty() || part == ".")
            continue;

        if (part == "..")
        {
            // subir um nível lógico
            if (cur.size() > 1)
            {
                size_t pos = cur.find_last_of('/');
                if (pos != std::string::npos && pos != 0)
                    cur = cur.substr(0, pos);
                else
                    cur = "/";
            }
            continue;
        }

        if (cur.size() > 1 && cur[cur.size() - 1] != '/')
            cur += "/";

        cur += part;

        struct stat st;
        if (stat(cur.c_str(), &st) != 0)
            return false;

        if (!S_ISDIR(st.st_mode))
            return false;

        // garantir permissão de travessia
        if (access(cur.c_str(), X_OK) != 0)
            return false;
    }

    return true;
}
