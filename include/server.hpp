#ifndef SERVER_HPP
#define SERVER_HPP

#include "configuration.hpp"
#include "Client.hpp"

    class Server
    {
        public:
            Server(std::vector<server_rule> tmp);  
            ~Server();
            void run();
        private:
            std::vector<server_rule> servers;
            long unsigned int read_server;
            std::vector<int> fd_;
            std::vector<pollfd> poll_fds_;
            std::map<int, Client> clients_;
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
// bool    validate_servers(std::vector<server_rule> &servers);
// bool    valid_configuration(std::string file_name);
// std::vector<server_rule> read_configuration(std::string file_name);
//  std::vector<server_rule> fill_configuration(std::string file_name);
// std::string remove_spaces(std::string file);
// bool fill_rule_server(std::vector<std::string> &spilt_server,std::vector<server_rule> &servers);
// void print_spilt_server(std::vector<std::string> spilt_server);
// bool find_location(std::string line, server_rule &server_1);
// bool find_root(std::string line, location &server_1,std::string target);
// void print_server_rule(std::vector<server_rule> servers);
#endif
