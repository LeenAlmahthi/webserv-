#ifndef SERVER_HPP
#define SERVER_HPP

#include <map>
#include <string>
#include <vector>
#include <poll.h>
#include <cstdlib>
// #include "configuration.hpp"
// #include "configuration.hpp"
struct Client
    {
        int fd; // socket 
        std::string in; // request  (recv)  {segment segment segment segment}  
        std::string out; // response (send) {segment segment segment segment} // nar  -> client? 
        std::string status; // 
        // int port; // port  8080 9090 
        bool closing; // yes  
    };
    struct location{
        std::string root;
        std::string index;
        std::vector<std::string>method;
        std::vector<std::string>cgi;
        std::string upload_path;
        bool autoindex;
        int ft_return;
    };
    struct server_rule{
        std::string listen_ip;
        int listen_port;
        int port;
        std::string max_body_size;
        std::vector<std::string> error_page;
        std::map<std::string,location> location_map;
    };
    class Server
    {
        public:
            // explicit Server();
            Server(std::vector<server_rule> tmp);  
            ~Server();
            void run();
            // void add_port(int port);
        private:
            std::vector<server_rule> servers;
            long unsigned int read_server;
            std::vector<int> fd_;
            // std::vector<int> port_;
            // int num_socket = 0;
            std::vector<pollfd> poll_fds_;
            std::map<int, Client> clients_;
            // std::map<int, int> port_fd;
            void setup_socket();
            void print_ip_instdin();
            bool is_port(int fd);
            void rebuild_poll_fds();
            void accept_new_clients(int fd);
            void handle_client_read(int fd);
            void handle_client_write(int fd);
            void close_client(int fd);
            std::string build_basic_response() const;
            static int set_nonblocking(int fd);
};
std::vector<server_rule> read_configuration(std::string file_name);
bool fill_configuration(std::string file_name);
std::string remove_spaces(std::string file);
bool fill_rule_server(std::vector<std::string> &spilt_server);
void print_spilt_server(std::vector<std::string> spilt_server);
bool find_location(std::string line, server_rule &server_1);
bool find_root(std::string line, location &server_1,std::string target);
void print_server_rule(std::vector<server_rule> servers);
#endif
