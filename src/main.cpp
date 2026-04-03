#include "server.hpp"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cout << "Error: Enter file configuration\n";
        return 1;
    }

    try
    {
        std::vector<server_rule> servers = configuration(argv[1]);
        print_server_rule(servers);
        if (servers.empty())
        {
            std::cout << "Configuration validation failed\n";
            return 1;
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