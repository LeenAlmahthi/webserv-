#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <map>
#include <string>
#include <vector>
#include <poll.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

// ======================= STRUCTS =======================

struct location
{
    std::string root;
    std::string index;
    std::vector<std::string> method;
    std::vector<std::string> cgi;
    std::string upload_path;
    bool autoindex;
    int return_code;
    std::string return_url;
    bool has_return;

    location() : autoindex(false), return_code(0) {}
};

struct server_rule
{
    std::string listen_ip;
    int listen_port;
    int port;
    std::string server_name;
    std::string max_body_size;
    std::vector<std::string> error_page;
    std::map<std::string, location> location_map;

    server_rule() : listen_port(0), port(0) {}
};

// ======================= MAIN API =======================

// Parsing entry
std::vector<server_rule> configuration(std::string file_name);
std::vector<server_rule> fill_configuration(std::string file);

// Validation
bool validate_servers(std::vector<server_rule> &servers);

// Utils
std::string remove_spaces(std::string file);
void print_server_rule(std::vector<server_rule> servers);

// ======================= SERVER PARSING =======================

bool find_listen(std::string line, server_rule &server_1);
bool server_name(std::string line, server_rule &server_1);
bool find_max_body(std::string line, server_rule &server_1);
bool find_error_page(std::string line, server_rule &server_1);
bool find_location(std::string line, server_rule &server_1);

// ======================= LOCATION PARSING =======================

bool find_root(std::string line, location &loc);
bool find_upload_path(std::string line, location &loc);
bool find_index(std::string line, location &loc);
bool find_method(std::string line, location &loc);
bool find_cgi(std::string line, location &loc);
bool find_autoindex(std::string line, location &loc);
bool find_return(std::string line, location &loc);

// ======================= INTERNAL HELPERS =======================

bool fill_rule_location(std::string line, location &loc);
bool ft_split_servers(std::string line, std::vector<std::string> &out);

#endif