#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "HttpRequest.hpp"
#include "server.hpp"

#include <string>
#include <cctype>
#include <exception>

#define BUFFER_SIZE 4096
#define MAX_CGI_OUTPUT_SIZE (1024 * 1024) // 1 MB

class ExitChild : public std::exception
{
	public:
		virtual const char* what() const throw()
		{
			return "CGI child must exit";
		}
};

class CGIHandler
{
	public:
		void run(const HttpRequest& req, Client& client, std::string scriptPath, std::string interpreter);
		static void handleCGIWrite(Client& client);
		static void handleCGIRead(Client& client);
		static void cleanupCGI(Client& client, bool kill_child);
	private:
		char** buildEnv(const HttpRequest& req, const std::string& scriptPath);
		void startCGI(const HttpRequest& req,  Client& client, const std::string& scriptPath, std::string interpreter, char** env);
		std::string parseOutput(const std::string& output);
		void free_env(char** env);
};

#endif
