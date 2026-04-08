#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <sys/socket.h>
#include "HttpRequest.hpp"
#define MAX_BODY_SIZE (10 * 1024 * 1024)

#include "HttpRequest.hpp"
#include <cassert>
#include <cerrno>
#include <cstring>
#include <sys/stat.h> 
#include <unistd.h>     
#include <fcntl.h>      
#include <dirent.h>   
#include <limits.h>    
#include "configuration.hpp"

struct Client
{
	int socket_fd;//from leen
	std::string request_buffer;// when we append we sote them in this buffer (bytes read from socket)
    std::string response_buffer;//Response to send back
	std::string status;
	bool closing;
	bool is_connected;// Is this client still connected?
	const server_rule *server_conf;
	
	Client() : socket_fd(-1), closing(false), is_connected(true), server_conf(NULL) {}
	Client(int fd) : socket_fd(fd), closing(false), is_connected(true), server_conf(NULL) {}
};
void client_readable(Client &client);
void process_request(HttpRequest &request, Client &client);
void send_error_response(Client &client, int status_code);
void route_request(HttpRequest &request, Client &client, const location &loc);
bool is_directory(const std::string& path);
void serve_static_file( Client &client, const std::string& path);
void handle_directory(Client &client, const std::string& path);
void execute_cgi( Client &client); // Sarrraaaaah
#endif