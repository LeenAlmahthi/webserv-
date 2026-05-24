#include "server.hpp"
#include "CGIHandler.hpp"

#include <sys/types.h>
#include <sys/wait.h>

extern bool g_running;

Server::Server(std::vector<server_rule> q)
{
	servers = q;
	// print_server_rule(servers);
	read_server = 0;
	while (read_server < servers.size())
	{
		setup_socket();
		rebuild_poll_fds();
		read_server++;		
	}
	
}
Server::~Server()
{
	for (long unsigned int i =0;i<fd_.size();i++)
	{
		if (fd_[i] >= 0)
			close(fd_[i]);

	}
	std::map<int, Client>::iterator q = clients_.begin();
	while (q != clients_.end())
	{
		close(q->first);
		++q;
	}
}

int Server::set_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);  // add to O_NONBLOCK to the flags in the fds   
}

void Server::setup_socket()
{
	int fd;
	fd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET : i will give you a ip SOCK_STREAM: tcp
	if (fd < 0)
		throw std::runtime_error("socket failed");

	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
		throw std::runtime_error("setsockopt failed");
	/*  SOL_SOCKET-> general socket [IPPROTO_TCP -> TCP options, IPPROTO_IP -> IP options]
		SO_REUSEADDR -> i can reuse same IP and port 
		1 : Allow me to bind to this port even if it is still in TIME_WAIT state
	*/
	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(servers[read_server].listen_port);
	if (servers[read_server].listen_ip == "0.0.0.0")
    	addr.sin_addr.s_addr = INADDR_ANY;
	else if (inet_pton(AF_INET, servers[read_server].listen_ip.c_str(), &addr.sin_addr) <= 0)
	{

    	std::cerr << servers[read_server].listen_ip <<"Invalid IP\n";
    	exit(1);
	}
	// addr.sin_family = atoi(servers[read_server].listen_ip); // i will give you a IP
	// std::memset(&addr, 0, sizeof(addr));
	// addr.sin_addr.s_addr = INADDR_ANY; // allow any request from any network
	// addr.sin_port = htons(servers[read_server].listen_port); // switch from a host and networks 

	// std::cout <<  servers[read_server].listen_ip<< " create socket to this port  : "  << servers[read_server].listen_port<<  " fd of socket is : "<<fd<< "\n";
	if (bind(fd,(struct sockaddr *)&addr, sizeof(addr)) < 0) // give the socket a IP and port 
		throw std::runtime_error("bind failed");
	if (listen(fd, 128) < 0) // 128 : # of client i will the socket accept  
		throw std::runtime_error("listen failed");

	if (set_nonblocking(fd) < 0) // changes in the system tell if they cant read dont wait return it 
		throw std::runtime_error("nonblocking listen socket failed");
	fd_.push_back(fd);
    // port_fd[fd] = port_[num_socket];


}

void Server::rebuild_poll_fds()
{
	poll_fds_.clear();
	fd_to_client.clear();

	// server sockets
	for (size_t i = 0; i < fd_.size(); i++)
	{
		pollfd pfd;
		pfd.fd = fd_[i];
		pfd.events = POLLIN;
		pfd.revents = 0;
		poll_fds_.push_back(pfd);
	}

	// clients + CGI
	std::map<int, Client>::iterator it = clients_.begin();
	while (it != clients_.end())
	{
		Client& c = it->second;

		// client socket
		pollfd cfd;
		cfd.fd = c.socket_fd;
		cfd.events = POLLIN;
		if (!c.response_buffer.empty())
			cfd.events |= POLLOUT;
		cfd.revents = 0;

		poll_fds_.push_back(cfd);
		fd_to_client[c.socket_fd] = &c;

		// CGI stdin (WRITE to CGI)
		if (c.is_cgi && c.cgi_stdin_fd != -1)
		{
			pollfd p;
			p.fd = c.cgi_stdin_fd;
			p.events = POLLOUT;
			p.revents = 0;

			poll_fds_.push_back(p);
			fd_to_client[c.cgi_stdin_fd] = &c;
		}

		// CGI stdout (READ from CGI)
		if (c.is_cgi && c.cgi_stdout_fd != -1)
		{
			pollfd p;
			p.fd = c.cgi_stdout_fd;
			p.events = POLLIN | POLLHUP;
			p.revents = 0;

			poll_fds_.push_back(p);
			fd_to_client[c.cgi_stdout_fd] = &c;
		}

		++it;
	}
}

void Server::accept_new_clients(int fd)
{
	size_t server_idx = 0;
	while (server_idx < fd_.size() && fd_[server_idx] != fd)
		++server_idx;

	while (true)
	{
		sockaddr_in client_addr;
		socklen_t len = sizeof(client_addr);
		int client_fd = accept(fd, reinterpret_cast<sockaddr *>(&client_addr), &len);
		if (client_fd < 0)
			break;
		if (set_nonblocking(client_fd) < 0)
		{
			close(client_fd);
			continue;
		}
		Client client;
		client.socket_fd = client_fd;
		client.is_connected = true;
		if (server_idx < servers.size())
			client.server_conf = &servers[server_idx];
		clients_[client_fd] = client; // this is the map fd -> key && data -> client 
	}
}

void Server::close_client(int fd)
{
	std::map<int, Client>::iterator it = clients_.find(fd);
	if (it == clients_.end())
		return;
	if (it->second.is_cgi || it->second.cgi_pid > 0)
		CGIHandler::cleanupCGI(it->second, true);
	close(fd);
	clients_.erase(it);
}

std::string Server::build_basic_response() const
{
	const std::string body = "<html><body><h1>webserv:8081</h1></body></html>";
	std::ostringstream length;
	length << body.size();
	std::string resp;
	resp += "HTTP/1.1 200 OK\r\n";
	resp += "Content-Type: text/html\r\n";
	resp += "Content-Length: ";
	resp += length.str();
	resp += "\r\n";
	resp += "Connection: close\r\n";
	resp += "\r\n";
	resp += body;
	return resp;
}

void Server::handle_client_read(int fd)
{
	std::map<int, Client>::iterator it = clients_.find(fd);
	if (it == clients_.end())
		return;

	client_readable(it->second);
	if (!it->second.is_connected && it->second.response_buffer.empty())
		close_client(fd);
}

void Server::handle_client_write(int fd)
{
	std::map<int, Client>::iterator it = clients_.find(fd);
	if (it == clients_.end())
		return;

	Client& client = it->second;

	if (client.response_buffer.empty())
		return;

	ssize_t n = send(fd, client.response_buffer.c_str(), client.response_buffer.size(), 0);

	if (n < 0)
	{
		close_client(fd);
		return;
	}

	if (n == 0)
	{
		close_client(fd);
		return;
	}

	client.response_buffer.erase(0, static_cast<size_t>(n));

	if (client.response_buffer.empty())
		close_client(fd);
}

bool Server::is_port(int fd)
{
	for (long unsigned int j=0;j < fd_.size();j++)
		{
			if (fd == fd_[j])
				return (1);
		}
	return (0);	
}
void Server::print_ip_instdin()
{
	for (long unsigned int i =0;i < servers.size();i++)
	{
		std::cout << "Listening on "<< servers[i].listen_ip << ":" << servers[i].listen_port << "\n";
	}
	std::cout <<"Server running with " << servers.size() <<" listening sockets\n";
};

void Server::run()
{
	print_ip_instdin();

	while (g_running)
	{
		rebuild_poll_fds();

		if (poll(&poll_fds_[0], poll_fds_.size(), 1000) < 0)
		{
			if (errno == EINTR)
				continue;

			throw std::runtime_error("poll failed");
		}

		for (size_t i = 0; i < poll_fds_.size(); ++i)
		{
			int fd = poll_fds_[i].fd;
			short revents = poll_fds_[i].revents;

			if (revents == 0)
				continue;

			if (is_port(fd))
			{
				if (revents & POLLIN)
					accept_new_clients(fd);
				continue;
			}

			Client* client = NULL;
			
			if (fd_to_client.count(fd))
				client = fd_to_client[fd];

			if (!client)
				continue;

			if (fd == client->cgi_stdout_fd && (revents & (POLLIN | POLLHUP)))
			{
				CGIHandler::handleCGIRead(*client);
				continue;
			}

			if (fd == client->cgi_stdin_fd && (revents & POLLOUT))
			{
				CGIHandler::handleCGIWrite(*client);
				continue;
			}

			if (fd == client->socket_fd && (revents & (POLLERR | POLLHUP | POLLNVAL)))
			{
				close_client(client->socket_fd);
				continue;
			}

			if (fd == client->socket_fd && (revents & POLLIN) && !client->is_cgi)
			{
				std::cout << "read request\n";
				handle_client_read(fd);
				continue;
			}

			if (fd == client->socket_fd && (revents & POLLOUT))
			{
				std::cout << "write client request\n";
				handle_client_write(fd);
				continue;
			}

			if (client->is_cgi && time(NULL) - client->cgi_start_time > 5)
			{
				CGIHandler::cleanupCGI(*client, true);
				send_error_response(*client, 504);
			}
		}
	}
}
