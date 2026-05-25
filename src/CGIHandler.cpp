#include "CGIHandler.hpp"
#include "HttpException.hpp"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <vector>
#include <map>
#include <sstream>

static std::string toString(size_t n)
{
	std::stringstream ss;
	ss << n;
	return (ss.str());
}

static int set_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static std::string getDirectory(const std::string& path)
{
	size_t pos = path.find_last_of('/');

	if (pos == std::string::npos)
		return ".";

	return path.substr(0, pos);
}

static std::string getFilename(const std::string& path)
{
	size_t pos = path.find_last_of('/');

	if (pos == std::string::npos)
		return path;

	return path.substr(pos + 1);
}

void	CGIHandler::free_env(char** env)
{
	size_t i = 0;

	while (env[i])
	{
		free(env[i]);
		i++;
	}

	delete[] env;
}

static void close_extra_fds()
{
	for (int fd = 3; fd < 1024; fd++)
		close(fd);
}

void CGIHandler::cleanupCGI(Client& client, bool kill_child)
{
	if (client.cgi_stdin_fd != -1)
	{
		close(client.cgi_stdin_fd);
		client.cgi_stdin_fd = -1;
	}

	if (client.cgi_stdout_fd != -1)
	{
		close(client.cgi_stdout_fd);
		client.cgi_stdout_fd = -1;
	}

	if (client.cgi_pid > 0)
	{
		pid_t pid = client.cgi_pid;
		client.cgi_pid = -1;

		if (kill_child)
			kill(pid, SIGKILL);

		int status;
		waitpid(pid, &status, 0);
	}

	client.is_cgi = false;
	client.cgi_done = true;
}

void	CGIHandler::run(const HttpRequest& req, Client& client, std::string scriptPath, std::string interpreter)
{
	char**		env = NULL;

	try
	{
		env = buildEnv(req, scriptPath);
		startCGI(req, client, scriptPath, interpreter, env);
	}
	catch(const HttpException& e)
	{
		if (env)
			free_env(env);
		send_error_response(client, e.getStatusCode());
	}
}

char** CGIHandler::buildEnv(const HttpRequest& req,
						const std::string& scriptPath)
{
	std::vector<std::string> env_strings;

	env_strings.push_back("REQUEST_METHOD=" + req.method);
	env_strings.push_back("QUERY_STRING=" + req.query_string);
	env_strings.push_back("SCRIPT_FILENAME=" + scriptPath);
	env_strings.push_back("SCRIPT_NAME=" + req.path);

	std::string uri = req.path;
	if (!req.query_string.empty())
		uri += "?" + req.query_string;
	env_strings.push_back("REQUEST_URI=" + uri);
	env_strings.push_back("SERVER_PROTOCOL=" + req.http_version);
	env_strings.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env_strings.push_back("PATH_INFO=");
	env_strings.push_back("PATH_TRANSLATED=");
	env_strings.push_back("SERVER_SOFTWARE=webserv/1.0");
	env_strings.push_back("REDIRECT_STATUS=200");

	// Host
	if (req.headers.count("host"))
	{
		env_strings.push_back("HTTP_HOST=" + req.headers.at("host"));
		env_strings.push_back("SERVER_NAME=" + req.headers.at("host"));
	}

	// Content headers
	if (req.headers.count("content-type"))
		env_strings.push_back("CONTENT_TYPE=" + req.headers.at("content-type"));

	if (req.headers.count("content-length"))
		env_strings.push_back("CONTENT_LENGTH=" + req.headers.at("content-length"));

	char** env = new char*[env_strings.size() + 1];

	for (size_t i = 0; i < env_strings.size(); i++)
		env[i] = strdup(env_strings[i].c_str());

	env[env_strings.size()] = NULL;

	return env;
}

void	CGIHandler::startCGI(const HttpRequest& req,  Client& client, const std::string& scriptPath, std::string interpreter, char** env)
{
	int	pipe_in[2];
	int	pipe_out[2];

	if (pipe(pipe_in) == -1)
		throw HttpException(500, "pipe_in creation failed");

	if (pipe(pipe_out) == -1)
	{
		close(pipe_in[0]);
		close(pipe_in[1]);
		throw HttpException(500, "pipe_out creation failed");
	}
	int	pid = fork();

	if (pid < 0)
	{
		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);
		throw HttpException(500, "fork failed");
	}
	if (pid == 0)
	{
		try
		{
			close(pipe_in[1]);
			close(pipe_out[0]);

			if (dup2(pipe_in[0], STDIN_FILENO) < 0)
				throw ExitChild();

			if (dup2(pipe_out[1], STDOUT_FILENO) < 0)
				throw ExitChild();

			int devnull = open("/dev/null", O_WRONLY);
			if (devnull >= 0)
			{
				if (dup2(devnull, STDERR_FILENO) < 0)
				{
					close(devnull);
					throw ExitChild();
				}
				close(devnull);
			}

			close_extra_fds();

			std::string scriptDir = getDirectory(scriptPath);
			std::string scriptName = getFilename(scriptPath);

			if (chdir(scriptDir.c_str()) < 0)
				throw ExitChild();

			char* const	argv[] = {const_cast<char*>(interpreter.c_str()), const_cast<char*>(scriptName.c_str()), NULL};
			execve(interpreter.c_str(), argv, const_cast<char**>(env));
			
			throw ExitChild();
		}
		catch(...)
		{
			if (env)
				free_env(env);

			close_extra_fds();

			throw ExitChild();
		}
	}
	else
	{
		close(pipe_in[0]);
		close(pipe_out[1]);

		if (set_nonblocking(pipe_in[1]) < 0 || set_nonblocking(pipe_out[0]) < 0)
		{
			close(pipe_in[1]);
			close(pipe_out[0]);
			throw HttpException(500, "set non-blocking failed");
		}

		client.is_cgi = true;
		client.cgi_pid = pid;
		client.cgi_stdin_fd = pipe_in[1];
		client.cgi_stdout_fd = pipe_out[0];
		client.cgi_input = req.body;
		client.cgi_written = 0;
		client.cgi_output.clear();
		client.cgi_start_time = time(NULL);
		client.cgi_done = false;

		free_env(env);
	}

	return ;
}

void CGIHandler::handleCGIWrite(Client& client)
{
	if (client.cgi_stdin_fd < 0)
		return;

	if (client.cgi_input.empty() || client.cgi_written >= client.cgi_input.size())
	{
		close(client.cgi_stdin_fd);
		client.cgi_stdin_fd = -1;
		return;
	}

	size_t remaining = client.cgi_input.size() - client.cgi_written;

	ssize_t ret = write(
		client.cgi_stdin_fd,
		client.cgi_input.c_str() + client.cgi_written,
		remaining
	);

	if (ret > 0)
	{
		client.cgi_written += ret;

		if (client.cgi_written >= client.cgi_input.size())
		{
			close(client.cgi_stdin_fd);
			client.cgi_stdin_fd = -1;
		}
		return;
	}

	if (ret <= 0)
	{
		close(client.cgi_stdin_fd);
		client.cgi_stdin_fd = -1;

		if (client.cgi_stdout_fd != -1)
		{
			close(client.cgi_stdout_fd);
			client.cgi_stdout_fd = -1;
		}

		CGIHandler::cleanupCGI(client, true);
		send_error_response(client, 500);
	}
}

void CGIHandler::handleCGIRead(Client& client)
{
	if (client.cgi_stdout_fd < 0)
		return;

	char buffer[BUFFER_SIZE];

	ssize_t ret = read(client.cgi_stdout_fd, buffer, BUFFER_SIZE);

	if (ret > 0)
	{
		if (client.cgi_output.size() + ret > MAX_CGI_OUTPUT_SIZE)
		{
			CGIHandler::cleanupCGI(client, true);
			send_error_response(client, 413);
			return;
		}

		client.cgi_output.append(buffer, ret);
		return;
	}

	if (ret == 0)
	{
		close(client.cgi_stdout_fd);
		client.cgi_stdout_fd = -1;

		int status;
		pid_t result = waitpid(client.cgi_pid, &status, WNOHANG);

		if (result == -1)
		{
			client.cgi_pid = -1;
			client.is_cgi = false;
			send_error_response(client, 500);
			return;
		}

		if (result == client.cgi_pid)
		{
			client.cgi_pid = -1;

			if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
			{
				send_error_response(client, 500);
				client.is_cgi = false;
				return;
			}
		}

		client.cgi_done = true;
		client.is_cgi = false;

		try
		{
			std::string response = CGIHandler().parseOutput(client.cgi_output);
			client.response_buffer.append(response);
		}
		catch (...)
		{
			send_error_response(client, 500);
		}

		return;
	}

	close(client.cgi_stdout_fd);
	client.cgi_stdout_fd = -1;

	if (client.cgi_stdin_fd != -1)
	{
		close(client.cgi_stdin_fd);
		client.cgi_stdin_fd = -1;
	}

	CGIHandler::cleanupCGI(client, true);
	send_error_response(client, 500);
}

std::string CGIHandler::parseOutput(const std::string& output)
{
	// Find header/body separator
	size_t pos = output.find("\r\n\r\n");
	size_t separator_len = 4;

	if (pos == std::string::npos)
	{
		pos = output.find("\n\n");
		separator_len = 2;
	}

	if (pos == std::string::npos)
		throw HttpException(500, "Invalid CGI output");

	std::string headers_part = output.substr(0, pos);
	std::string body = output.substr(pos + separator_len);

	// Default values
	std::string status = "200 OK";
	std::string content_type = "text/html";

	// Parse headers line by line
	std::istringstream stream(headers_part);
	std::string line;

	while (std::getline(stream, line))
	{
		// remove trailing '\r'
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue;

		std::string key = line.substr(0, colon);
		std::string value = line.substr(colon + 1);

		// trim spaces
		while (!value.empty() && value[0] == ' ')
			value.erase(0, 1);

		// normalize key
		for (size_t i = 0; i < key.size(); i++)
			key[i] = std::tolower(static_cast<unsigned char>(key[i]));

		if (key == "content-type")
			content_type = value;
		else if (key == "status")
			status = value;
	}

	std::string response;

	response += "HTTP/1.1 " + status + "\r\n";
	response += "Content-Type: " + content_type + "\r\n";
	response += "Content-Length: " + toString(body.size()) + "\r\n";
	response += "Connection: close\r\n";
	response += "\r\n";
	response += body;
	response += "\r\n";

	return response;
}
