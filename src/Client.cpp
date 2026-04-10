#include "Client.hpp"

static size_t get_max_body_limit(const Client &client)
{
    if (client.server_conf && !client.server_conf->max_body_size.empty())
    {
        try
        {
            return std::stoul(client.server_conf->max_body_size);
        }
        catch (...)
        {
            return MAX_BODY_SIZE;
        }
    }
    return MAX_BODY_SIZE;
}

//  STARRRTTTT 
void client_readable(Client &client)
{
    char temp_buf[4096];
    ssize_t received_bytes = recv(client.socket_fd, temp_buf, sizeof(temp_buf), 0);
    
    if (received_bytes == 0) 
    {
        client.is_connected = false;
        return;
    }
    
    if (received_bytes < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
             return;
        client.is_connected = false;
        return;
    }
    
    if (client.request_buffer.size() + received_bytes > 8192) 
    {
        client.is_connected = false;
        return;
    }
    
    client.request_buffer.append(temp_buf, received_bytes);
    
        while (1) 
        {
            HttpRequest request;
            size_t bytes_to_remove = 0;
            
            ParsingResult result = parse_http_req(client.request_buffer,request,bytes_to_remove);
            
            if (result == PARSE_COMPLETE) 
                {
                    client.request_buffer.erase(0, bytes_to_remove);
                    process_request(request, client);
                    continue;
                }
            else if (result == PARSE_INCOMPLETE) 
                {
                    break;
                }
            // else 
            //     {
            //         client.is_connected = false;
            //         break;
            //     }
            else { // PARSE_ERROR
                if (request.oversized_body)
                    send_error_response(client, 413);
                else
                    send_error_response(client, 400);
                client.closing = true;
                break;
            }
        }
}

bool ends_with(const std::string& str, const std::string& suffix) 
{
    if (suffix.length() > str.length()) {
        return false;
    }
    int start_pos = str.length() - suffix.length();
    std::string ending = str.substr(start_pos);
    if(ending == suffix)
    return true ;
    else 
    return false;
}
std::string get_content_type(const std::string& path) 
{
// std::string path = "/images/cat.jpg";
// std::string content_type = get_content_type(path);
// returns: "image/jpeg"
    if (ends_with(path, ".html") || ends_with(path, ".htm"))
             return "text/html";
    if (ends_with(path, ".css")) 
        return "text/css";
    if (ends_with(path, ".js"))
         return "application/javascript";
    if (ends_with(path, ".jpg") || ends_with(path, ".jpeg"))
         return "image/jpeg";
    if (ends_with(path, ".png"))
         return "image/png";
    if (ends_with(path, ".gif")) 
        return "image/gif";
    if (ends_with(path, ".ico"))
         return "image/x-icon";
    if (ends_with(path, ".svg")) 
        return "image/svg+xml";
    if (ends_with(path, ".json")) 
        return "application/json";
    if (ends_with(path, ".xml")) 
        return "application/xml";
    return "application/octet-stream";
}


//  CGI DETECTION 

bool is_cgi_script(const std::string& path) 
{
    return (ends_with(path, ".php") || ends_with(path, ".cgi") || ends_with(path, ".py"));
}
// PATH NORMALIZATION  

std::string normalize_path(const std::string& root, const std::string& request_path)
{
    char root_resolved[PATH_MAX];
    if (realpath(root.c_str(), root_resolved) == nullptr) 
        return "";
    std::string resolved_root(root_resolved);
    std::string combined = root + request_path;
    char path_resolved[PATH_MAX];
    if (realpath(combined.c_str(), path_resolved) == nullptr) 
        return "";

    std::string resolved_path(path_resolved);

    if (resolved_path.find(resolved_root) != 0)
        return "";
 
    if (resolved_path.length() > resolved_root.length() && resolved_path[resolved_root.length()] != '/') 
        return "";
 
    return resolved_path;
}
//  DIRECTORY CHECK 

bool is_directory(const std::string& path) 
{
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}


//  PROCESS REQUEST 
void process_request(HttpRequest &request, Client &client)
{
    size_t max_body_limit = get_max_body_limit(client);

    if (request.method != "GET" && request.method != "POST" && request.method != "DELETE") 
    {
        send_error_response(client, 405);
        return;
    }
    if ((request.method == "POST" || request.method == "DELETE") && request.headers.count("content-length"))
     {
        try 
        {
            size_t body_size = std::stoul(request.headers["content-length"]);
            if (body_size > max_body_limit)
            {  
                send_error_response(client, 413);
                return;
            }
        } 
        catch (...)
        {
            send_error_response(client, 400);  // Bad Request
            return;
        }
    }
    if (request.path.find("..") != std::string::npos)
    {
        send_error_response(client, 403);
        return;
    }
    if (!client.server_conf || client.server_conf->location_map.empty())
    {
        send_error_response(client, 404);
        return;
    }

    std::map<std::string, location>::const_iterator selected = client.server_conf->location_map.end();
    size_t best_len = 0;

    for (std::map<std::string, location>::const_iterator it = client.server_conf->location_map.begin();
         it != client.server_conf->location_map.end(); ++it)
    {
        const std::string &prefix = it->first;
        if (request.path.compare(0, prefix.size(), prefix) == 0 && prefix.size() >= best_len)
        {
            best_len = prefix.size();
            selected = it;
        }
    }

    if (selected == client.server_conf->location_map.end())
    {
        send_error_response(client, 404);
        return;
    }

    route_request(request, client, selected->second);
}

std::string dechunk_body(const std::string& chunked)
{
    std::string dechunked;
    size_t pos = 0;
    while (pos < chunked.size())
     {
        size_t terminator = chunked.find("\r\n", pos);
        if (terminator == std::string::npos) break;
        
        std::string size_line = chunked.substr(pos, terminator - pos);
        size_t semi = size_line.find(';');
        if (semi != std::string::npos)
     size_line = size_line.substr(0, semi);
        size_t chunk_size = std::stoul(size_line, nullptr, 16);
        if (chunk_size == 0) break;
        
        pos = terminator + 2;
        if (pos + chunk_size > chunked.size()) break;
        
        dechunked.append(chunked.substr(pos, chunk_size));
        pos += chunk_size + 2;
    }
    
    return dechunked;
}
//  ROUTE REQUEST 

void route_request(HttpRequest &request, Client &client, const location &loc)
{
    std::string physical_path = normalize_path(loc.root, request.path);//////
    if (physical_path.empty())
     {
        send_error_response(client, 404); 
        return;
    }
    if (request.headers.count("transfer-encoding") && request.headers["transfer-encoding"] == "chunked") 
    {
        
        if (is_cgi_script(physical_path))
        {
            try 
            {
                request.body = dechunk_body(request.body);
                request.headers.erase("transfer-encoding");
            } 
            catch (...) 
            {
                send_error_response(client, 400); 
                return;
            }
        } 
        else 
        {
            send_error_response(client, 501);  
            return;
        }
    }
  
    if (is_cgi_script(physical_path))
    {
        // execute_cgi(request, client, physical_path); FOR SARRAAAAH AL3ASAL<3
        return;
    }
    if (is_directory(physical_path))
    {
        handle_directory(client, physical_path);
        return;
    }
    serve_static_file(client, physical_path);
}

//  ERROR RESPONSES 
void send_error_response(Client &client, int status_code) 
{
    std::string status_text;
    std::string body;

    if(status_code == 400)
        status_text = "Bad Request";
    else if(status_code == 403)
        status_text = "Forbidden";
    else if(status_code == 404)
        status_text = "Not Found";
    else if(status_code == 405)
        status_text = "Method Not Allowed";
    else if(status_code == 413)
        status_text = "Payload Too Large";
    else if(status_code == 500)
        status_text = "Internal Server Error";
    else
        status_text = "Error";

    std::string status_line = std::to_string(status_code) + " " + status_text;

    body = "<html><head><title>" + status_line + "</title></head><body><h1>" + status_line + "</h1></body></html>";

    std::string response =
        "HTTP/1.1 " + status_line + "\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" + body;

    client.response_buffer.append(response);
}

//  STATIC FILES 

void serve_static_file( Client &client, const std::string& path) 
{
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) 
    {
        send_error_response(client, 404);
        return;
    }
    
    struct stat st;
    if (fstat(fd, &st) < 0)
     {  
        close(fd);
        send_error_response(client, 500);
        return;
    }
    if (st.st_size == 0) 
    {  
        std::string response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: " + get_content_type(path) + "\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n" 
            "\r\n";
        client.response_buffer.append(response);
        close(fd);
        return;
    }
    
    char* file_data = new char[st.st_size];
    ssize_t bytes_read = read(fd, file_data, st.st_size);  
    close(fd);
    
    if (bytes_read != st.st_size)
     {
        delete[] file_data;
        send_error_response(client, 500);
        return;
    }
    
    std::string response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: " + get_content_type(path) + "\r\n"
        "Content-Length: " + std::to_string(st.st_size) + "\r\n"
        "Connection: close\r\n"  
        "\r\n";
    
    response.append(file_data, st.st_size);
    delete[] file_data;
    
    client.response_buffer.append(response);
}

void handle_directory(Client &client, const std::string& dir_path)
{
   
    std::string html = "<html><body><h1>Directory Listing</h1><ul>\r\n";
    
    DIR* dir = opendir(dir_path.c_str());
    if (!dir) 
    {
        send_error_response(client, 403);
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) 
    {
        if (entry->d_name[0] == '.') 
            continue;  
        html += "<li><a href=\"" + std::string(entry->d_name) + "\">" + std::string(entry->d_name) + "</a></li>\r\n";
    }
    closedir(dir);
    
    html += "</ul></body></html>";
    
    // build response
    std::string response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(html.length()) + "\r\n"
        "Connection: close\r\n"
        "\r\n";
    response.append(html);
    
    client.response_buffer.append(response);
}

void execute_cgi(Client &client)
{
//    FOR SARRAAAAH AL3ASAL<3
    send_error_response(client, 501); 
}