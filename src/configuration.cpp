#include "server.hpp"

void print_server_rule(std::vector<server_rule> servers)
{
    for (size_t i = 0; i < servers.size(); i++)
    {
        std::cout << "\n==============================\n";
        std::cout << "Server [" << i + 1 << "]\n";
        std::cout << "------------------------------\n";

        std::cout << "Listen           : "
                  << servers[i].listen_ip << '\t'
                  << servers[i].listen_port << "\n";

        if (!servers[i].max_body_size.empty())
            std::cout << "Max body size    : "
                      << servers[i].max_body_size << "\n";

        if (!servers[i].error_page.empty())
        {
            std::cout << "Error pages      :\n";
            for (size_t j = 0; j < servers[i].error_page.size(); j++)
                std::cout << "   - " << servers[i].error_page[j] << "\n";
        }

        if (!servers[i].server_name.empty())
        {
            std::cout << "Server names : ";
                std::cout << servers[i].server_name;
                            std::cout << "\n";
        }

        std::cout << "\nLocations\n";
        std::cout << "------------------------------\n";

        std::map<std::string, location>::iterator it =
            servers[i].location_map.begin();

        while (it != servers[i].location_map.end())
        {
            std::cout << "\nLocation: " << it->first << "\n";

            if (!it->second.root.empty())
                std::cout << "   root           : "
                          << it->second.root << "\n";

            if (!it->second.index.empty())
                std::cout << "   index          : "
                          << it->second.index << "\n";

            std::cout << "   autoindex      : "
                      << (it->second.autoindex ? "on" : "off") << "\n";

            if (!it->second.upload_path.empty())
                std::cout << "   upload_path    : "
                          << it->second.upload_path << "\n";

            if (it->second.has_return)
            {
                std::cout << "   return_code    : " << it->second.return_code << "\n";
                if (!it->second.return_url.empty())
                    std::cout << "   return_url     : " << it->second.return_url << "\n";
            }

            if (!it->second.method.empty())
            {
                std::cout << "   methods        : ";
                for (size_t k = 0; k < it->second.method.size(); k++)
                    std::cout << it->second.method[k] << " ";
                std::cout << "\n";
            }

            if (!it->second.cgi.empty())
            {
                std::cout << "   cgi            :\n";
                for (size_t k = 0; k < it->second.cgi.size(); k++)
                    std::cout << "      - " << it->second.cgi[k] << "\n";
            }

            ++it;
        }
    }
}

std::string remove_spaces(std::string file)
{
    std::string result;
    size_t i = 0;

    while (i < file.size())
    {
        while (i < file.size() && std::isspace(file[i]))
            i++;

        if (i >= file.size())
            break;

        size_t start = i;
        while (i < file.size() && (std::isalpha(file[i]) || file[i] == '_'))
            i++;

        std::string key = file.substr(start, i - start);
        result += key;

        while (i < file.size() && std::isspace(file[i]))
            i++;

        if (i < file.size() && file[i] != '{' && file[i] != ';')
            result += " ";

        while (i < file.size() && file[i] != ';' && file[i] != '{' && file[i] != '}')
        {
            if (!std::isspace(file[i]))
                result += file[i];
            i++;
        }

        if (i < file.size())
        {
            result += file[i];
            i++;
        }
    }

    return result;
}

bool find_method(std::string line, location &loc)
{
    size_t pos = line.find("methods");
    if (pos == std::string::npos)
        return 1;

    size_t end = line.find(';', pos);
    if (end == std::string::npos)
        return 0;

    std::string raw = line.substr(pos + 8 , end - (pos + 8));
    if (raw.empty())
        return false;

    loc.method.clear();
    size_t i = 0;
    while (i < raw.size())
    {
        if (raw.compare(i, 3, "GET") == 0)
        {
            loc.method.push_back("GET");
            i += 3;
        }
        else if (raw.compare(i, 4, "POST") == 0)
        {
            loc.method.push_back("POST");
            i += 4;
        }
        else if (raw.compare(i, 6, "DELETE") == 0)
        {
            loc.method.push_back("DELETE");
            i += 6;
        }
        else
            return false;
    }
    return true;
}

bool find_cgi(std::string line, location &loc)
{
    size_t pos = line.find("cgi_extensions");
    if (pos == std::string::npos)
        return true;

    size_t end = line.find(';', pos);
    if (end == std::string::npos)
        return false;

    std::string raw = line.substr(pos + 15, end - (pos + 15));
    if (raw.empty())
        return false;

    loc.cgi.clear();
    size_t i = 0;
    while (i < raw.size())
    {
        if (raw[i] != '.')
            return false;

        size_t slash = raw.find('/', i);
        if (slash == std::string::npos)
            return false;

        std::string ext = raw.substr(i, slash - i);
        size_t next_ext = raw.find('.', slash + 1);
        std::string path;
        if (next_ext == std::string::npos)
        {
            path = raw.substr(slash);
            i = raw.size();
        }
        else
        {
            path = raw.substr(slash, next_ext - slash);
            i = next_ext;
        }

        if (ext.empty() || path.empty())
            return false;

        loc.cgi.push_back(ext + ":" + path);
    }
    return true;
}

bool find_autoindex(std::string line, location &loc)
{
    size_t pos = line.find("autoindex");
    if (pos == std::string::npos)
        return true;

    size_t end = line.find(';', pos);
    if (end == std::string::npos)
        return false;

    std::string raw = line.substr(pos + 10, end - (pos + 10));

    if (raw == "on")
        loc.autoindex = true;
    else if (raw == "off")
        loc.autoindex = false;
    else 
        return false;
    if (line.find("autoindex",end) != std::string::npos)
        return false;
    return true;
}

bool find_return(std::string line, location &loc)
{
    size_t pos = line.find("return");
    if (pos == std::string::npos)
    {
        loc.has_return = false;
        return true;
    }

    size_t end = line.find(';', pos);
    if (end == std::string::npos)
        return false;

    std::string raw = line.substr(pos + 7, end - (pos + 7));
    if (raw.empty())
        return false;

    if (line.find("return", end) != std::string::npos)
        return false;

    std::stringstream ss(raw);

    int code;
    std::string url;

    ss >> code;
    if (code < 100 || code > 599)
        return false;

    loc.return_code = code;

    if (ss >> url)
        loc.return_url = url;
    else
        loc.return_url = "";

    loc.has_return = true;

    return true;
}


bool server_name(std::string line, server_rule &srv)
{
     size_t pos = line.find("server_name");
    if (pos == std::string::npos)
        return true;

    size_t end = line.find(';', pos + 1);
    if (end == std::string::npos)
        return false;

    std::string raw = line.substr(pos + 12, end - (pos + 12));
    if (raw.empty())
        return false;

    if (line.find("server_name", end) != std::string::npos)
        return false;
        
    srv.server_name = raw;
    return true;
}

bool find_listen(std::string line, server_rule &server_1)
{
    if (line.find("listen") == std::string::npos)
        return 0;

    size_t start = line.find("listen ") + 7;
    size_t end = line.find(';', start);
    if (end == std::string::npos)
        return 0; // check if the line end with a ;
    
     if (line.find("error_page", start) < end ||
        line.find("client_max_body_size", start) < end ||
        line.find("location", start) < end)
    {
        std::cout << "Error: invalid listen syntax\n";
        return false;
    }
    std::string tmp = line.substr(start, end - start);

    if (tmp.find(':') == std::string::npos)
    {
        server_1.listen_ip = "127.0.0.1";
        server_1.listen_port = atoi(tmp.c_str());
    }
    else
    {
        server_1.listen_ip = tmp.substr(0, tmp.find(':'));
        server_1.listen_port = atoi(tmp.substr(tmp.find(':') + 1).c_str());
    }

    if (line.find("listen", end) != std::string::npos)
    {
        std::cout << "Error: Duplicate listen\n";
        return 0;
    }

    return 1;
}

bool find_max_body(std::string line, server_rule &server_1)
{
    size_t pos = line.find("client_max_body_size");
    if (pos == std::string::npos)
        return 1;
    size_t start = pos + 21;
    size_t end = line.find(';', start);
    if (end == std::string::npos)
        return 0; // the line not end the ;
    if (line.find("error_page", start) < end ||
        line.find("listen", start) < end ||
        line.find("location", start) < end)
        return false;
    if (line.find("client_max_body_size",end) != std::string::npos)
    {
          std::cout << "Error: Duplicate client_max_body_size\n";
          return 0;
    }
    server_1.max_body_size = line.substr(start, end - start);
    return 1;
}

bool find_error_page(std::string line, server_rule &server_1)
{
    while (line.find("error_page") != std::string::npos)
    {
        size_t index = line.find("error_page");
        size_t start = index + 11;
        size_t end = line.find(';', start);

        if (end == std::string::npos || start + 3 > end)
            return 0;
        
           if (line.find("listen", start) < end ||
        line.find("client_max_body_size", start) < end ||
        line.find("location", start) < end)
        return false;

        std::string code = line.substr(start, 3); //error_page500500.html; in http the code is always 3 
        std::string file = line.substr(start + 3, end - (start + 3)); 

        server_1.error_page.push_back(code + " " + file);
        line = line.substr(end + 1);
    }
    return 1;
}
bool find_root(std::string line, location &server_1)
{
    size_t start = line.find("root");
    if (start == std::string::npos )
        return true;
    size_t end = line.find(";", start+5);
    if (end == std::string::npos)
        return false;

    std::string tmp = line.substr(start +5,end  - (start  + 5));

    if (!tmp.empty() && line.find("root",end) == std::string::npos)
            server_1.root = tmp;   
    else 
        return false;
    return (1);
 };
bool find_upload_path(std::string line, location &server_1)
{
    size_t start = line.find("upload_path");
    if (start == std::string::npos)
        return true;
    size_t end = line.find(";",start + 12);
    if (end == std::string::npos)
        return false;
    std::string tmp = line.substr(start +12,end - (start +12));
    if (!tmp.empty() && line.find("upload_path",end) == std::string::npos)
            server_1.upload_path = tmp;   
    else
    {
        std::cout <<"Error: Duplicate 'upload_path' directive found in server block\n";
        return false;
    }
    return true; 
}

bool find_index(std::string line, location &server_1)
{

    size_t pos = line.find(";index");
    if (pos == std::string::npos)
            return  true;
    size_t end = line.find(';', pos + 7);
    if (end == std::string::npos)
        return false;
    std::string value = line.substr(pos + 7 , end - (pos + 7));
    if (value.empty() )
        return false;
    server_1.index = value;
    return true;
 }
bool fill_rule_location(std::string line, location &loc)
{
    int open = line.find('{');
    int close = line.find('}', open);

    if (open < 0 || close < 0)
        return false;

    line = line.substr(open + 1, close - open - 1);
    if (!find_root(line,loc))
        return(false);
    if (!find_upload_path(line,loc))
        return(false);
    if (!find_index(line,loc))
        return(0);
    if (!find_method(line, loc))
        return false;
    if (!find_cgi(line, loc))
        return false;
    if (!find_autoindex(line, loc))
        return false;
    if (!find_return(line, loc))
        return false;
    return true;
}

bool find_location(std::string line, server_rule &server_1)
{
    while (true)
    {
        size_t index = line.find("location");
        if (index == std::string::npos)
            break;

        line = line.substr(index + 8);

        size_t open = line.find('{');
        if (open == std::string::npos)
            return false;

        std::string target = line.substr(1, open);
        if (target.empty() || target[0] != '/')
        {
            std::cout << "Error: location must start with '/'" << std::endl;
            return false;
        }
        location loc;
        if (!fill_rule_location(line, loc))
            return false;
        
        server_1.location_map[target] = loc;  // ← CRITICAL FIX

        size_t close = line.find('}');
        if (close == std::string::npos)
            return false;

        line = line.substr(close + 1);
    }
    return true;
}

bool ft_split_servers(std::string line, std::vector<std::string> &out)
{
    size_t pos = 0;

    while ((pos = line.find("server", pos)) != std::string::npos)
    {
        size_t open = line.find('{', pos);
        if (open == std::string::npos)
            return 0;
        int braces = 1;
        size_t i = open + 1;

        while (i < line.size() && braces > 0) // can't use find [}] cuz the location have braces
        {
            if (line[i] == '{')
                braces++;
            if (line[i] == '}')
                braces--;
            i++;
        }
        out.push_back(line.substr(pos, i - pos));
        pos = i;
    }
    return 1;
}
bool is_allowed(const std::string &key)
{
    const std::string allowed[] = {
        "server",
        "listen",
        "server_name",
        "client_max_body_size",
        "error_page",
        "location",
        "root",
        "index",
        "methods",
        "autoindex",
        "upload_path",
        "cgi_extensions",
        "return"
    };

    for (size_t i = 0; i < sizeof(allowed) / sizeof(allowed[0]); i++)
    {
        if (key == allowed[i])
            return true;
    }
    return false;
}bool has_forbidden(const std::string &file)
{
    size_t i = 0;

    while (i < file.size())
    {
        while (i < file.size() && (file[i] == ' ' || file[i] == '\n' || file[i] == '\t' || file[i] == ';' || file[i] == '{' || file[i] == '}'))
            i++;

        if (i >= file.size())
            break;

        size_t start = i;

        while (i < file.size() && (isalpha(file[i]) || file[i] == '_'))
            i++;

        std::string key = file.substr(start, i - start);

        if (!is_allowed(key))
        {
            std::cout << "Forbidden directive: " << key << "\n";
            return true;
        }

        while (i < file.size() && file[i] != ';' && file[i] != '{' && file[i] != '}')
            i++;
    }

    return false;
}
std::vector<server_rule> fill_configuration(std::string file)
{
    std::vector<std::string> blocks;
    std::vector<server_rule> servers;

    if (!ft_split_servers(file, blocks))
        return std::vector<server_rule>();

    if (blocks.empty())
    {
        std::cout << "Error: no servers\n";
        return servers;
    }

    for (size_t i = 0; i < blocks.size(); i++)
    {
        server_rule srv;
        std::string normalized_block = remove_spaces(blocks[i]);
        
        if (!server_name(blocks[i], srv))
            return std::vector<server_rule>();
        if (has_forbidden(normalized_block))
            return std::vector<server_rule>();
        if (!find_listen(normalized_block, srv))
            return std::vector<server_rule>();
        else if (!find_max_body(normalized_block, srv))
            return std::vector<server_rule>();
        else if (!find_error_page(normalized_block, srv))
            return std::vector<server_rule>();
        else if (!find_location(normalized_block, srv))
            return std::vector<server_rule>();
        servers.push_back(srv);
    }
    return servers;
}std::string read_and_validate_config(const std::string &path)
{
    std::ifstream file(path.c_str());
    if (!file.is_open())
        return std::string();

    std::string line;
    std::string result;
    int braces = 0;

    while (std::getline(file, line))
    {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos)
        {
            result += line + "\n";
            continue;
        }

        size_t end = line.find_last_not_of(" \t");
        std::string tmp = line.substr(start, end - start + 1);

        int open_c = 0;
        int close_c = 0;

        for (size_t i = 0; i < tmp.size(); i++)
        {
            if (tmp[i] == '{')
            {
                open_c++;
                braces++;
            }
            else if (tmp[i] == '}')
            {
                close_c++;
                if (braces == 0)
                    return std::string();
                braces--;
            }
        }

        if (open_c > 1 || close_c > 1)
            return std::string();

        if (open_c == 1 && tmp.find('{') != tmp.size() - 1)
            return std::string();

        if (close_c == 1 && tmp.find('}') != tmp.size() - 1)
            return std::string();

        char last = tmp[tmp.size() - 1];

        if (last == '{' || last == '}')
        {
            result += line + "\n";
            continue;
        }

        if (braces > 0 && last != ';')
        {
            if (tmp.rfind("location", 0) == 0 && braces == 1)
            {
                result += line + "\n";
                continue;
            }
            return std::string();
        }

        result += line + "\n";
    }

    if (braces != 0)
        return std::string();

    return result;
}
std::vector<server_rule> configuration(std::string file_name)
{
    std::string file_data;
    file_data = read_and_validate_config(file_name);
    if (file_data.empty())
    {
        std::cout << "Error Missing ';' or braces \n";
        return std::vector<server_rule>();
    }
    std::vector<server_rule> servers = fill_configuration(file_data);
    return servers;
}