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
            std::map<int, Client*> fd_to_client;
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
#endif
