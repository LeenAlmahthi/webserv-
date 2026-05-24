#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <sys/socket.h>
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
	// CGI related
	bool is_cgi;
	int cgi_pid;
	int cgi_stdin_fd;
	int cgi_stdout_fd;
	std::string cgi_input;   // request.body
	size_t cgi_written; // how much written
	std::string cgi_output;
	time_t cgi_start_time;
	bool cgi_done;
	
	Client() : socket_fd(-1), closing(false), is_connected(true), server_conf(NULL),
				is_cgi(false), cgi_pid(-1), cgi_stdin_fd(-1), cgi_stdout_fd(-1), cgi_written(0), cgi_start_time(0), cgi_done(false) {}
	Client(int fd) : socket_fd(fd), closing(false), is_connected(true), server_conf(NULL),
				is_cgi(false), cgi_pid(-1), cgi_stdin_fd(-1), cgi_stdout_fd(-1), cgi_written(0), cgi_start_time(0), cgi_done(false) {}
};
void client_readable(Client &client);
void process_request(HttpRequest &request, Client &client);
void send_error_response(Client &client, int status_code);
void route_request(HttpRequest &request, Client &client, const location &loc, const std::string& location_prefix);
bool is_directory(const std::string& path);
void serve_static_file( Client &client, const std::string& path);
void handle_directory(Client& client, const std::string& path, const location& loc);
void handle_delete(Client& client, const std::string& path);
#endif
