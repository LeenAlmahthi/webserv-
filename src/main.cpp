// #include <sys/socket.h> // For socket functions
// #include <netinet/in.h> // For sockaddr_in
// #include <cstdlib> // For exit() and EXIT_FAILURE
// #include <iostream> // For cout
// #include <unistd.h> // For read
// using namespace std;
// // struct  sockaddr {
// //     sa_family_t    sin_family; /* address family: AF_INET */
// //     in_port_t      sin_port;   /* port in network byte order */
// //     struct in_addr sin_addr;   /* internet address */
// // };

// int main ()
// {
//     int fd = socket(AF_INET,SOCK_STREAM,0); //AF_INET : ip version 4  || SOCK_STREAM: cpu || 0: only one protocol 
//     sockaddr_in addr;
//   //  sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

//     addr.sin_family = AF_INET; // type : i will use a ip version 4 to connect with you
//     addr.sin_port = htons(8080);// i will use this port to connect with you [htons()  → host → network it will تعكس between the host and networks  ||  ntohs()  → network → host] 
//     addr.sin_addr.s_addr = INADDR_ANY; // any networks can talk with you  [you want to specific ip // addr.sin_addr.s_addr = inet_addr("127.0.0.1");]
//     int  retuen_bind = bind(fd,(struct sockaddr *)&addr, sizeof(sockaddr_in)); // tell it this prot will toke with you 
//     int re_listen = listen(fd, 50000); // wait to request  
//     socklen_t addrlen = sizeof(sockaddr);
    
//     // poll()
//     int connection = accept(fd, (struct sockaddr *)&addr, &addrlen); // accept the request  it will return to you fd with a request 
//     char buffer[100];
//     auto bytesRead = read(connection, buffer, 100); // read with a read buff 
//     std::cout << "The message was: " << buffer;
//     // Send a message to the connection
//     std::string response = "Good talking to you\n"; 
//     send(connection, response.c_str(), response.size(), 0); // send a response 
//     cout << fd <<" | " << retuen_bind <<" |" << re_listen << "|" << connection <<   "\n";
//     close(fd);
// }
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iostream>
#include "server.hpp"

// bool read_configuration(std::string filename){
//    std::ifstream file;
//     file.open(filename);
//     if (!file.is_open())
//         return (0);
//     std::string buf;
//     std::string line;
//     while (getline(file,buf))
//     {
//         line += buf;
//         line += "\n";
//     }
//     std::cout << line;
//     return (0);
// };

int main(int argc, char **argv)
{
  if (argc != 2 )
      return (1);
  try
  {
    // full_data();
    std::vector<server_rule> servers = read_configuration(argv[1]); 
    if (servers.empty())
    {
      // std::cout << "Error in the configuration file\n";
      std::cout << "\n";
      return (1);
      
    }
    // print_server_rule(servers);

    Server server(servers);
    // server.add_port(8080);
    // server.add_port(8081);
    server.run();
  }
  catch (const std::exception &ex)
  {
    std::cerr << "Fatal error: " << ex.what() << std::endl;
    return 1;
  }
  return 0;
}