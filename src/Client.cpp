#include "Client.hpp"
#include "CGIHandler.hpp"
#include "UploadHandler.hpp"

static std::string toString(size_t n)
{
    std::stringstream ss;
    ss << n;
    return ss.str();
}

static std::string toStringInt(int n)
{
    std::stringstream ss;
    ss << n;
    return ss.str();
}

static size_t stringToSizeT(const std::string& s, int base)
{
    char* end;
    errno = 0;

    unsigned long value = std::strtoul(s.c_str(), &end, base);

    if (errno != 0 || end == s.c_str() || *end != '\0')
        throw std::runtime_error("invalid number");

    return static_cast<size_t>(value);
}

static std::string getStatusText(int status_code)
{
	if (status_code == 400) return "Bad Request";
	if (status_code == 403) return "Forbidden";
	if (status_code == 404) return "Not Found";
	if (status_code == 405) return "Method Not Allowed";
	if (status_code == 409) return "Conflict";
	if (status_code == 413) return "Payload Too Large";
	if (status_code == 500) return "Internal Server Error";
	if (status_code == 501) return "Not Implemented";
	if (status_code == 504) return "Gateway Timeout";
	return "Error";
}

static bool read_file_to_string(const std::string& path, std::string& out)
{
	std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);

	if (!file.is_open())
		return false;

	std::ostringstream ss;
	ss << file.rdbuf();
	out = ss.str();

	return true;
}

static size_t get_max_body_limit(const Client &client)
{
    if (client.server_conf && !client.server_conf->max_body_size.empty())
    {
        try
        {
            return stringToSizeT(client.server_conf->max_body_size, 10);
        }
        catch (...)
        {
            return MAX_BODY_SIZE;
        }
    }
    return MAX_BODY_SIZE;
}

static std::string getFileExtension(const std::string& path)
{
	size_t pos = path.find_last_of('.');

	if (pos == std::string::npos)
		return "";

	return path.substr(pos);
}

static std::string getCgiInterpreter(const location& loc, const std::string& path)
{
	std::string ext = getFileExtension(path);

	for (size_t i = 0; i < loc.cgi.size(); i++)
	{
		size_t sep = loc.cgi[i].find(':');

		if (sep == std::string::npos)
			continue;

		std::string configured_ext = loc.cgi[i].substr(0, sep);
		std::string interpreter = loc.cgi[i].substr(sep + 1);

		if (configured_ext == ext)
			return interpreter;
	}

	return "";
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

std::string normalize_path(const std::string& root, const std::string& request_path)
{
    char root_resolved[PATH_MAX];
    if (realpath(root.c_str(), root_resolved) == NULL) 
        return "";
    std::string resolved_root(root_resolved);
    std::string combined = root + request_path;
    char path_resolved[PATH_MAX];
    if (realpath(combined.c_str(), path_resolved) == NULL) 
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
            size_t body_size = stringToSizeT(request.headers["content-length"], 10);
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

    route_request(request, client, selected->second, selected->first);
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
        size_t chunk_size = stringToSizeT(size_line, 16);
        if (chunk_size == 0) break;
        
        pos = terminator + 2;
        if (pos + chunk_size > chunked.size()) break;
        
        dechunked.append(chunked.substr(pos, chunk_size));
        pos += chunk_size + 2;
    }
    
    return dechunked;
}
//  ROUTE REQUEST 

void route_request(HttpRequest &request, Client &client, const location &loc, const std::string& location_prefix)
{
    if (!loc.method.empty())
    {
        bool allowed = false;
        for (size_t i = 0; i < loc.method.size(); i++)
        {
            if (loc.method[i] == request.method)
            {
                allowed = true;
                break;
            }
        }
        if (!allowed)
        {
            send_error_response(client, 405);
            return;
        }
    }
    // HANDLE REDIRECT FIRST
    if (loc.has_return)
    {
        if (loc.return_code >= 300 && loc.return_code < 400)
        {
            if (loc.return_url.empty())
            {
                send_error_response(client, 500);
                return;
            }

            std::string status;

            if (loc.return_code == 301)
                status = "301 Moved Permanently";
            else if (loc.return_code == 302)
                status = "302 Found";
            else
                status = toStringInt(loc.return_code) + " Redirect";

            std::string response =
                "HTTP/1.1 " + status + "\r\n"
                "Location: " + loc.return_url + "\r\n"
                "Content-Length: 0\r\n"
                "Connection: close\r\n"
                "\r\n";

            client.response_buffer.append(response);
            return;
        }

        send_error_response(client, loc.return_code);
        return;
    }
    if (request.headers.count("transfer-encoding") && request.headers["transfer-encoding"] == "chunked") 
    {
        try 
        {
            request.body = dechunk_body(request.body);
            request.headers.erase("transfer-encoding");
            request.headers["content-length"] = toString(request.body.size());

            size_t max_body_limit = get_max_body_limit(client);

            if (request.body.size() > max_body_limit)
            {
                send_error_response(client, 413);
                return;
            }
        }
        catch (...) 
        {
            send_error_response(client, 400); 
            return;
        }
    }

    std::string relative_path = request.path;

    if (location_prefix != "/" && request.path.compare(0, location_prefix.size(), location_prefix) == 0)
    {
        relative_path = request.path.substr(location_prefix.size());

        if (relative_path.empty())
            relative_path = "/";
    }

    std::string physical_path = normalize_path(loc.root, relative_path);

    if (request.method == "POST" && !(loc.upload_path.empty()))
    {
        UploadHandler   upload;
        upload.handle(request, client, loc, location_prefix);
        return;
    }
    if (physical_path.empty())
     {
        send_error_response(client, 404); 
        return;
    }

    std::string interpreter = getCgiInterpreter(loc, physical_path);
    if (!interpreter.empty())
    {
        CGIHandler cgi;
        cgi.run(request, client, physical_path, interpreter);
        return;
    }
    if (request.method == "DELETE")
    {
        handle_delete(client, physical_path);
        return;
    }
    if (is_directory(physical_path))
    {
        handle_directory(client, physical_path, loc);
        return;
    }
    serve_static_file(client, physical_path);
}

//  ERROR RESPONSES 
void send_error_response(Client& client, int status_code)
{
	std::string status_text = getStatusText(status_code);
	std::string body;

	bool custom_page_found = false;

	if (client.server_conf)
	{
		for (size_t i = 0; i < client.server_conf->error_page.size(); i++)
		{
			std::stringstream ss(client.server_conf->error_page[i]);
			int code;
			std::string path;

			ss >> code >> path;

			if (code == status_code && !path.empty())
			{
				if (read_file_to_string(path, body))
					custom_page_found = true;
				break;
			}
		}
	}

	if (!custom_page_found)
	{
		body = "<html><head><title>" + toStringInt(status_code) + " " + status_text +
			   "</title></head><body><h1>" + toStringInt(status_code) + " " +
			   status_text + "</h1></body></html>";
	}

	std::string response;

	response += "HTTP/1.1 " + toStringInt(status_code) + " " + status_text + "\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + toString(body.size()) + "\r\n";
	response += "Connection: close\r\n";
	response += "\r\n";
	response += body;

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
        "Content-Length: " + toString(st.st_size) + "\r\n"
        "Connection: close\r\n"  
        "\r\n";
    
    response.append(file_data, st.st_size);
    delete[] file_data;
    
    client.response_buffer.append(response);
}

void handle_directory(Client& client, const std::string& path, const location& loc)
{
   if (!loc.index.empty())
    {
        std::string index_path = path;

        if (!index_path.empty() && index_path[index_path.size() - 1] != '/')
            index_path += "/";

        index_path += loc.index;

        if (access(index_path.c_str(), F_OK) == 0)
        {
            serve_static_file(client, index_path);
            return;
        }
    }

    if (!loc.autoindex)
    {
        send_error_response(client, 403);
        return;
    }

    // directory listing
    std::string html = "<html><body><h1>Directory Listing</h1><ul>\r\n";
    
    DIR* dir = opendir(path.c_str());
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
        "Content-Length: " + toString(html.length()) + "\r\n"
        "Connection: close\r\n"
        "\r\n";
    response.append(html);
    
    client.response_buffer.append(response);
}

void handle_delete(Client& client, const std::string& path)
{
    struct stat st;

    if (stat(path.c_str(), &st) < 0)
    {
        send_error_response(client, 404);
        return;
    }
    if (S_ISDIR(st.st_mode))
    {
        send_error_response(client, 403);
        return;
    }
    if (unlink(path.c_str()) < 0)
    {
        send_error_response(client, 500);
        return;
    }

    // build response
    std::string response = 
        "HTTP/1.1 204 No Content\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n";
    
    client.response_buffer.append(response);
}
