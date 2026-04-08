#include "server.hpp"


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
	poll_fds_.clear(); //Clear old list
	for (long unsigned int i=0; i < fd_.size();i++)
	{
		pollfd pfd;
		pfd.fd = fd_[i];
		pfd.events = POLLIN; // a client is waiting in accept queue 
		pfd.revents = 0; // it will fill with os 
		poll_fds_.push_back(pfd); // this is the server
		// std::cout << "poll is :" << fd_[i] << " __ " ;
	}
	// std::cout << "#_ " << fd_.size() << "\n";
	std::map<int, Client>::iterator it = clients_.begin();
	while (it != clients_.end())
	{
			pollfd cfd;
			cfd.fd = it->first;
			cfd.events = POLLIN ;
			cfd.revents = 0;
			if (!it->second.response_buffer.empty())
				cfd.events |= POLLOUT;
		/*POLLIN → client sent request → I should read
		POLLOUT → socket ready → I should send response*/
			poll_fds_.push_back(cfd);
			++it;
		}
	/*\
	vector -> array -> vector <int> leen(98);
	leen[0] = 9; // 
	leen.size(); 
	leen.push_back(9); {9}
	leen.push_back(8); {9,8,}
	map <int ,int >nar key -> data ;
	nar[8] = 7;
	nar[0] = -7;
	nar[8] = 76;
	nar[0] = -76;
	0 -> -7;
	8 -> 7;
	map <int ,int>::iterator q = nar.begin(); [map.begin() map.end() map.find(0)];
	map <int ,int>::iterator q = map.find(0) ; [0,-7]
	q->first ; // 0; [first,second]
	q->second; // -7;
	while (it != end ())
	map<int,client> client;
	map[5];
	clinet nar;
	nar.fd = 5;
	nar.in = "leen";
	nar.out  = "nar";
	nar.port = 8080;
	map[5] = nar;
	map<int,client>::iterator q = nar.find(5);
	q->second.port; // 8080
	struct vector map unordermap 



	clients_
   |
   |----[5]----> Client
   |               fd = 5
   |               in = "GET /"
   |               out = ""
   |               close = false
   |
   |----[8]----> Client
   |               fd = 8
   |               in = ""
   |               out = "HTTP/1.1 200 OK..."
   |               close = false
   |
   |----[12]---> Client
                   fd = 12
                   in = ""
                   out = ""
                   close = true*/
	/*First I add the server socket to poll so I know when a new connection arrives.
	Then I add every client socket so poll notifies me when a client is ready
	to read a request or ready to send a response.*/
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
	if (it->second.response_buffer.empty())
		return;

	int n = send(fd, it->second.response_buffer.c_str(), it->second.response_buffer.size(), 0);
	if (n <= 0)
	{
		close_client(fd);
		return;
	}

	it->second.response_buffer.erase(0, static_cast<size_t>(n));
	if (it->second.response_buffer.empty())
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
	while (true)
	{
		rebuild_poll_fds();
		if (poll(&poll_fds_[0], poll_fds_.size(), -1) < 0) // there are error in the sever 
			throw std::runtime_error("poll failed");
		for (size_t i = 0; i < poll_fds_.size(); ++i)
		{
			int fd = poll_fds_[i].fd;
			short revents = poll_fds_[i].revents;
			if (revents == 0)
    			continue;
			if (is_port(poll_fds_[i].fd))
			{
				if ( poll_fds_[i].revents & POLLIN) // this is use a bitwise 
					{
						accept_new_clients(poll_fds_[i].fd);
						continue;
					}
			}
			if (poll_fds_[i].revents & (POLLERR | POLLHUP | POLLNVAL)) // If fd has error, hang-up, or invalid → close it
			{
				close_client(fd);
				continue;
			}
			if (poll_fds_[i].revents & POLLIN)  // 0x005 & 0x001 = 0x001 → triggers
			{
				std::cout << "read request\n";
				handle_client_read(fd);
			}
				
			if (poll_fds_[i].revents & POLLOUT) // // 0x005 & 0x004 = 0x004 → triggers 
			{
				std::cout << "write client request\n";
				handle_client_write(fd);
			}	
		}
	}

}
