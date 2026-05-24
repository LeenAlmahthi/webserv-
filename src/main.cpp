#include "server.hpp"
#include "CGIHandler.hpp"

#include <signal.h>

bool g_running = true;

void handle_sigint(int)
{
    g_running = false;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./webserv <config_file>\n";
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
        signal(SIGINT, handle_sigint);
        server.run();
    }
    catch (const ExitChild&)
    {
        return 1;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
