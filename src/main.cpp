#include "server.hpp"

int main(int argc, char **argv)
{
  if (argc != 2 )
      return (1);
  try
  {
    std::vector<server_rule> servers = read_configuration(argv[1]); 
    if (!validate_servers(servers))
    {
      std::cout << "Configuration validation failed\n";
      return 1;
  }
  if (servers.empty())
    {
      // std::cout << "Error in the configuration file\n";
      std::cout << "\n";
      return (1);
      
    }
    Server server(servers);
    server.run();
  }
  catch (const std::exception &ex)
  {
    std::cerr << "Fatal error: " << ex.what() << std::endl;
    return 1;
  }
  return 0;
}