#include "server.hpp"

static size_t find_directive_pos(const std::string &line, const std::string &key, size_t from)
{
    size_t pos = from;
    while ((pos = line.find(key, pos)) != std::string::npos)
    {
        if (pos == 0 || line[pos - 1] == ';' || line[pos - 1] == '{')
            return pos;
        ++pos;
    }
    return std::string::npos;
}

static size_t find_directive_pos(const std::string &line, const std::string &key)
{
    return find_directive_pos(line, key, 0);
}

static bool find_method(std::string line, std::string target, location &loc)
{
    (void)target;
    const std::string key = "methods";
    const size_t pos = find_directive_pos(line, key);
    if (pos == std::string::npos)
        return true;

    const size_t end = line.find(';', pos);
    if (end == std::string::npos)
        return false;

    const std::string raw = line.substr(pos + key.size(), end - (pos + key.size()));
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

static bool find_cgi(std::string line, std::string target, location &loc)
{
    (void)target;
    const std::string key = "cgi_extensions";
    const size_t pos = find_directive_pos(line, key);
    if (pos == std::string::npos)
        return true;

    const size_t end = line.find(';', pos);
    if (end == std::string::npos)
        return false;

    std::string raw = line.substr(pos + key.size(), end - (pos + key.size()));
    if (raw.empty())
        return false;

    loc.cgi.clear();
    size_t i = 0;
    while (i < raw.size())
    {
        if (raw[i] != '.')
            return false;
        const size_t slash = raw.find('/', i);
        if (slash == std::string::npos)
            return false;

        const std::string ext = raw.substr(i, slash - i);
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

static bool find_autoindex(std::string line, std::string target, location &loc)
{
    (void)target;
    const std::string key = "autoindex";
    const size_t pos = find_directive_pos(line, key);
    if (pos == std::string::npos)
        return true;

    const size_t end = line.find(';', pos);
    if (end == std::string::npos)
        return false;

    const std::string raw = line.substr(pos + key.size(), end - (pos + key.size()));
    if (raw == "on")
    {
        loc.autoindex = true;
        return true;
    }
    if (raw == "off")
    {
        loc.autoindex = false;
        return true;
    }
    return false;
}

static bool find_return(std::string line, std::string target, location &loc)
{
    (void)target;
    const std::string key = "return";
    const size_t pos = find_directive_pos(line, key);
    if (pos == std::string::npos)
    {
        loc.ft_return = -1;
        return true;
    }

    const size_t end = line.find(';', pos);
    if (end == std::string::npos)
        return false;

    const std::string raw = line.substr(pos + key.size(), end - (pos + key.size()));
    if (raw.empty())
        return false;

    loc.ft_return = atoi(raw.c_str());
    return true;
}

bool find_listen(std::string line, server_rule &server_1)
{
    int end = 0;
    if (line.find("listen") != std::string::npos)
    {
        for (long unsigned int i =0; i < line.length();i++)
        {
            if (line[i] == ';')
            {
                end = i;
                break;
            }
        }
        end  -= line.find("listen") +5; 
        std::string tmp = line.substr(line.find("listen")+6,end- line.find("listen")+6 );
        if (tmp.find(':') == std::string::npos)
        {
            server_1.listen_ip = "127.0.0.1";
            server_1.listen_port = atoi(tmp.c_str());
        }     
        else 
        {
            server_1.listen_ip = tmp.substr(0,tmp.find(':'));
            std::string q = tmp.substr(tmp.find(':')+1,tmp.length());
            server_1.listen_port = atoi(q.c_str());
        }
        line =  line.substr(tmp.length()+6,line.length());
        if (line.find("listen") != std::string::npos)
        {
            std::cout <<"Error: Duplicate 'listen' directive found in server block\n";
            return(0);
        }    
    }
    else 
        return (0);
    return (1);
 };
bool find_max_body(std::string line, server_rule &server_1)
{

    int index = line.find("client_max_body_size");
    if (line.find("client_max_body_size") != std::string::npos)
    {
        line = line.substr(index+20,line.length());
        index = 0;
        for (long unsigned int i =0; i < line.length();i++)
        {
            if (line[i] == ';')
            {
                index = i;
                break;
            }
        }
        std::string tmp = line.substr(0,index);
        line = line.substr(index+1,line.length());
        if (line.find("client_max_body_size") != std::string::npos)
        {
            std::cout <<"Error: Duplicate 'client_max_body_size' directive found in server block\n";
            return(0);
        }    
        server_1.max_body_size = tmp;
        return (1);
    }
    return(0);
};
bool find_error_page(std::string line, server_rule &server_1)
{
    while (line.find("error_page") != std::string::npos)
    {
        size_t index = line.find("error_page");
        size_t start = index + 10;
        size_t end = line.find(';', start);

        if (end == std::string::npos || start + 3 > end)
            return false;

        std::string code_str = line.substr(start, 3);
        int code = atoi(code_str.c_str());

        if (code < 400 || code > 599)
            return false;

        std::string file = line.substr(start + 3, end - (start + 3));
        server_1.error_page.push_back(code_str + " " + file);

        line = line.substr(end + 1);
    }
    return true;
}
bool find_root(std::string line, location &server_1,std::string target)
{
    int end = 0;
    if (line.find("root") != std::string::npos)
    {
        for (long unsigned int i =0; i < line.length();i++)
        {
            if (line[i] == ';')
            {
                end = i;
                break;
            }
        }
        end  -= line.find("root") +4; 
        std::string tmp = line.substr(line.find("root")+4,end - line.find("root"));
        if (!tmp.empty())
            server_1.root = tmp;   
    }
    else if (target == "/upload")
    {
        if (line.find("upload_path") != std::string::npos)
        {
            for (long unsigned int i =0; i < line.length();i++)
            {
                if (line[i] == ';')
                {
                    end = i;
                    break;
                }
            }   
            end  -= line.find("upload_path") +12; 
            line = line.substr(line.find("upload_path")+12,end - line.find("upload_path"));
            if (!line.empty())
                server_1.upload_path = line;
            if (line.find("upload_path") != std::string::npos)
            {
                std::cout <<"Error: Duplicate 'upload_path' directive found in server block\n";
                return(0);
            }    
        }
    }
    return (1);
 };
bool find_index(std::string line, location &server_1,std::string target)
{
    if (target == "/upload")
        return true;

    const std::string key = "index";
    const size_t pos = find_directive_pos(line, key);
    if (pos == std::string::npos)
        return true;

    const size_t end = line.find(';', pos);
    if (end == std::string::npos)
        return false;

    const std::string value = line.substr(pos + key.size(), end - (pos + key.size()));
    if (value.empty())
        return false;

    server_1.index = value;

    if (find_directive_pos(line, key, end + 1) != std::string::npos)
    {
        std::cout <<"Error: Duplicate 'index' directive found in server block\n";
        return false;
    }
    return true;
 };
 bool fill_rule_location(std::string line,location &loc,std::string target){
 
    int q = -1;
    int j = 0;
    for (long unsigned int i = 0; i < line.length(); i++)
    {
        if (line[i] == '{')
            q = i;
        else if (line[i] == '}' && q >= 0)
        {
            j = i;
            break;
        }
    }
    line = line.substr(q+1,j-q-1);
    if (!find_root(line,loc,target))
        return(0);
    if (!find_index(line,loc,target))
        return(0);
    if (!find_method(line,target,loc))
        return (0);
    if (!find_cgi(line,target,loc))
        return (0);
    if (!find_autoindex(line,target,loc))
        return (0);
    if (!find_return(line,target,loc))
        return (0);    
    return (1);
};
bool find_location(std::string line, server_rule &server_1)
{
        while (line.find("location") != std::string::npos)
        {
            std::string target;
            location loc;
            int q = 0;
            int index = line.find("location");
            if (line.find("location") != std::string::npos)
            {
                line = line.substr(index+8,line.length());
                for (long unsigned int i =0; i < line.length();i++)
                {
                    if (line[i] == '{')
                    {
                        q = i;
                        break;
                    }
                }
                target = line.substr(0,q);
                if (!fill_rule_location(line, loc, target))
                    return false;
                server_1.location_map[target] = loc;
        }
        else 
            return(0);    
    }
    return (1);
}
void ft_split_servers(std::string line,std::vector<std::string> &spilt_server)
{
    size_t pos = 0;
    while ((pos = line.find("server", pos)) != std::string::npos)
    {
        size_t start = pos;
        size_t open = line.find('{', pos + 6);
        if (open == std::string::npos)
            break;
        int braces = 1;
        size_t i = open + 1;
        while (i < line.length() && braces > 0)
        {
            if (line[i] == '{')
                braces++;
            else if (line[i] == '}')
                braces--;
            i++;
        }
        if (braces != 0) // if braces != 0, the block is not closed correctly
            break;
        spilt_server.push_back(line.substr(start, i - start));
        pos = i;
    }
}
void print_server_rule(std::vector<server_rule> servers)
{
    for (size_t i = 0; i < servers.size(); i++)
    {
        std::cout << "\n==============================\n";
        std::cout << "Server [" << i + 1 << "]\n";
        std::cout << "------------------------------\n";

        std::cout << "Listen           : "
                  << servers[i].listen_ip << ":"
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

            if (it->second.ft_return != 0)
                std::cout << "   return_code    : "
                          << it->second.ft_return << "\n";

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

    std::cout << "\n==============================\n";
}
bool fill_rule_server(std::vector<std::string> &spilt_server, std::vector<server_rule> &servers)
{
    for (long unsigned int i = 0; i < spilt_server.size(); i++)
    {
        server_rule server_1;
        if (!find_listen(spilt_server[i],server_1))
            return(0);
        if (!find_max_body(spilt_server[i],server_1))
            return(0);
        if (!find_error_page(spilt_server[i],server_1))
            return 0;
        if (!find_location(spilt_server[i],server_1))
             return 0;
        servers.push_back(server_1);
    }
    if (servers.size() != 0)
        return (1);
    return (0);
}
std::string remove_spaces(std::string file)
{
    std::string compact;

    for (size_t i = 0; i < file.length(); ++i)
    {
        if (!std::isspace(static_cast<unsigned char>(file[i])))
            compact += file[i];
    }

    // std::string normalized;
    // normalized.reserve(compact.length() * 2);
    // for (size_t i = 0; i < compact.length(); ++i)
    // {
    //     normalized += compact[i];
    //     if(file.find("error_page") != std::string::npos)
    //     {
    //          compact += file[i];
    //     }
    //     if (compact[i] == ';' || compact[i] == '{' || compact[i] == '}')
    //         normalized += ' ';
    // }
    return compact;
};
void print_spilt_server(std::vector<std::string> spilt_server)
{
    for (size_t i = 0; i < spilt_server.size(); ++i)
    {
        std::cout << spilt_server[i] << std::endl; 
    }
}
std::vector<server_rule> fill_configuration(std::string file){
    file = remove_spaces(file);
    std::vector<std::string> spilt_server;
    std::vector<server_rule> servers;
    ft_split_servers(file, spilt_server);
    fill_rule_server(spilt_server,servers);
    return(servers);
}
std::vector<server_rule>  read_configuration(std::string file_name)
{
    std::ifstream file;
    std::vector<server_rule> leen;
    file.open(file_name.c_str());
    if (!file.is_open())
        return (leen);
    std::string buf;
    std::string line;
    while (getline(file,buf))
    {
        line += buf;
        line += "\n";
    }
    std::vector<server_rule> servers = fill_configuration(line);
    return (servers);
}
bool validate_servers(std::vector<server_rule> &servers)
{
    for (size_t i = 0; i < servers.size(); i++)
    {
        if (servers[i].listen_port <= 0 || servers[i].listen_port > 65535)
        {
            std::cout << "Invalid port\n";
            return false;
        }

        if (servers[i].location_map.empty())
        {
            std::cout << "Server must contain at least one location\n";
            return false;
        }

        std::map<std::string, location>::iterator it =
            servers[i].location_map.begin();

        while (it != servers[i].location_map.end())
        {
            if (it->second.root.empty() && it->second.upload_path.empty())
            {
                std::cout << "Location must have root or upload_path\n";
                return false;
            }

            ++it;
        }
    }

    return true;
}