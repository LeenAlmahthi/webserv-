#include "UploadHandler.hpp"
#include "HttpException.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sstream>
#include <errno.h>
#include <cctype>

static std::string toString(size_t n)
{
	std::stringstream ss;
	ss << n;
	return (ss.str());
}

void	UploadHandler::handle(const HttpRequest& req, Client& client, const location& loc, const std::string& location_prefix)
{
	std::string					boundary;
	std::vector<MultipartPart>	parts;
	std::string					fileName;
	std::string					response;

	try
	{
		validateUploadRequest(req, loc);

		if (isMultipart(req))
			handleMultipart(req, client, loc);
		else
			handleRaw(req, client, loc, location_prefix);
	}
	catch(const HttpException& e)
	{
		response = buildResponse(e.getStatusCode(), e.getMsg());
		client.response_buffer.append(response);
	}
}

void	UploadHandler::validateUploadRequest(const HttpRequest& req, const location& loc)
{
	if (req.method != "POST")
		throw HttpException(405, "Method Not Allowed");

	if (loc.upload_path.empty())
		throw HttpException(403, "Upload not allowed");

	if (req.body.empty())
		throw HttpException(400, "Empty body");
}

bool UploadHandler::isMultipart(const HttpRequest& req)
{
	if (!req.headers.count("content-type"))
		return false;

	return req.headers.at("content-type").find("multipart/form-data") != std::string::npos;
}

void UploadHandler::handleMultipart(const HttpRequest& req, Client& client, const location& loc)
{
	std::string boundary = extractBoundary(req.headers.at("content-type"));
	std::vector<MultipartPart> parts = parseMultipart(req.body, boundary);

	bool saved = false;

	for (size_t i = 0; i < parts.size(); i++)
	{
		std::string fileName = extractFilename(parts[i].headers["content-disposition"]);

		if (fileName.empty())
			continue;

		fileName = sanitizeFilename(fileName);
		saveFile(loc.upload_path, fileName, parts[i].body);
		saved = true;
	}

	if (!saved)
		throw HttpException(400, "No file part found");

	client.response_buffer.append(buildResponse(201, "Upload successful"));
}

void UploadHandler::handleRaw(const HttpRequest& req, Client& client, const location& loc, const std::string& location_prefix)
{
	std::string fileName = extractRawFilename(req, location_prefix);
	fileName = sanitizeFilename(fileName);

	saveFile(loc.upload_path, fileName, req.body);

	client.response_buffer.append(buildResponse(201, "Upload successful"));
}

std::string UploadHandler::extractRawFilename(const HttpRequest& req, const std::string& location_prefix)
{
	std::string name = req.path;

	if (location_prefix != "/" &&
		req.path.compare(0, location_prefix.size(), location_prefix) == 0)
		name = req.path.substr(location_prefix.size());

	while (!name.empty() && name[0] == '/')
		name.erase(0, 1);

	if (name.empty())
		throw HttpException(400, "Missing upload filename");

	return name;
}

std::string	UploadHandler::extractBoundary(const std::string& contentType)
{
	size_t pos = contentType.find("boundary=");

	if (pos == std::string::npos)
		throw HttpException(400, "boundary not found");

	pos += 9; // length of "boundary="

	size_t end = contentType.find(';', pos);

	std::string boundary;

	if (end == std::string::npos)
		boundary = contentType.substr(pos);
	else
		boundary = contentType.substr(pos, end - pos);

	if (boundary.empty())
		throw HttpException(400, "empty boundary");

	return (boundary);
}

MultipartPart UploadHandler::parseOnePart(const std::string& partString)
{
	MultipartPart part;

	size_t pos = partString.find("\r\n\r\n");
	size_t separator_len = 4;

	if (pos == std::string::npos)
	{
		pos = partString.find("\n\n");
		separator_len = 2;
	}

	if (pos == std::string::npos)
		throw HttpException(400, "Invalid multipart part");

	std::string headers_part = partString.substr(0, pos);
	std::string body = partString.substr(pos + separator_len);

	std::istringstream stream(headers_part);
	std::string line;

	while (std::getline(stream, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue;

		std::string key = line.substr(0, colon);
		std::string value = line.substr(colon + 1);

		while (!value.empty() && value[0] == ' ')
			value.erase(0, 1);

		for (size_t i = 0; i < key.size(); i++)
			key[i] = std::tolower(static_cast<unsigned char>(key[i]));

		part.headers[key] = value;
	}

	part.body = body;
	return (part);
}

std::vector<MultipartPart> UploadHandler::parseMultipart(const std::string& body, const std::string& boundary)
{
	std::vector<MultipartPart> parts;

	if (boundary.empty())
		throw HttpException(400, "Empty boundary");

	std::string delimiter = "--" + boundary;
	std::string finalDelimiter = delimiter + "--";

	size_t pos = body.find(delimiter);
	if (pos == std::string::npos)
		throw HttpException(400, "Multipart boundary not found");

	while (pos != std::string::npos)
	{
		pos += delimiter.size();

		// Final boundary: --boundary--
		if (body.compare(pos, 2, "--") == 0)
			break;

		// Skip CRLF or LF after boundary
		if (body.compare(pos, 2, "\r\n") == 0)
			pos += 2;
		else if (body.compare(pos, 1, "\n") == 0)
			pos += 1;
		else
			throw HttpException(400, "Invalid multipart boundary format");

		size_t next = body.find(delimiter, pos);
		if (next == std::string::npos)
			throw HttpException(400, "Missing final multipart boundary");

		std::string partString = body.substr(pos, next - pos);

		// Remove trailing CRLF before next boundary
		if (partString.size() >= 2 &&
			partString.substr(partString.size() - 2) == "\r\n")
			partString.erase(partString.size() - 2);
		else if (!partString.empty() && partString[partString.size() - 1] == '\n')
			partString.erase(partString.size() - 1);

		if (!partString.empty())
			parts.push_back(parseOnePart(partString));

		pos = next;
	}

	if (parts.empty())
		throw HttpException(400, "Empty multipart body");

	return (parts);
}

std::string UploadHandler::extractFilename(const std::string& contentDisposition)
{
	size_t pos = contentDisposition.find("filename=");

	// No filename → normal form field
	if (pos == std::string::npos)
		return "";

	pos += 9; // length of "filename="

	// Quoted filename
	if (pos < contentDisposition.size() &&
		contentDisposition[pos] == '"')
	{
		pos++;

		size_t end = contentDisposition.find('"', pos);

		if (end == std::string::npos)
			throw HttpException(400, "Invalid filename");

		return contentDisposition.substr(pos, end - pos);
	}

	// Unquoted filename
	size_t end = contentDisposition.find(';', pos);

	if (end == std::string::npos)
		end = contentDisposition.size();

	return contentDisposition.substr(pos, end - pos);
}

std::string	UploadHandler::sanitizeFilename(const std::string& name)
{
	if (name.empty())
		throw HttpException(400, "Empty filename");

	std::string clean = name;

	// Remove Windows paths
	size_t pos = clean.find_last_of("\\");
	if (pos != std::string::npos)
		clean = clean.substr(pos + 1);

	// Remove Unix paths
	pos = clean.find_last_of("/");
	if (pos != std::string::npos)
		clean = clean.substr(pos + 1);

	// Reject dangerous names
	if (clean.empty() || clean == "." || clean == "..")
		throw HttpException(400, "Invalid filename");

	// Reject hidden files
	if (clean[0] == '.')
		throw HttpException(400, "Hidden files not allowed");

	// OAllow only safe chars
	for (size_t i = 0; i < clean.size(); i++)
	{
		char c = clean[i];

		if (!(std::isalnum(c) ||
			  c == '.' ||
			  c == '_' ||
			  c == '-'))
		{
			throw HttpException(400, "Invalid filename characters");
		}
	}

	return (clean);
}

void	UploadHandler::saveFile(const std::string& uploadDirectory, const std::string& fileName, const std::string& fileBody)
{
	// Validate upload directory
	struct stat st;

	if (stat(uploadDirectory.c_str(), &st) < 0)
		throw HttpException(500, "Upload directory not found");

	if (!S_ISDIR(st.st_mode))
		throw HttpException(500, "Upload path is not a directory");

	// Build full path
	std::string path = uploadDirectory;

	if (!path.empty() && path[path.size() - 1] != '/')
		path += "/";

	path += fileName;

	// Open the file & write all bytes
	int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
	if (fd < 0)
	{
		if (errno == EEXIST)
			throw HttpException(409, "File already exists");

		throw HttpException(500, "Failed to create upload file");
	}
	
	size_t written = 0;

	while (written < fileBody.size())
	{
		ssize_t ret = write(fd, fileBody.c_str() + written, fileBody.size() - written);

		if (ret < 0)
		{
			close(fd);
			throw HttpException(500, "Failed to write uploaded file");
		}

		written += ret;
	}

	if (close(fd) < 0)
		throw HttpException(500, "Failed to close uploaded file");
}

std::string	UploadHandler::buildResponse(int statusCode, const std::string& msg)
{
	std::string status;

	if (statusCode == 200)
		status = "200 OK";
	else if (statusCode == 201)
		status = "201 Created";
	else if (statusCode == 400)
		status = "400 Bad Request";
	else if (statusCode == 403)
		status = "403 Forbidden";
	else if (statusCode == 405)
		status = "405 Method Not Allowed";
	else if (statusCode == 409)
		status = "409 Conflict";
	else if (statusCode == 500)
		status = "500 Internal Server Error";
	else
		status = toString(statusCode) + " Error";

	std::string body = "<html><body><h1>" + msg + "</h1></body></html>";

	std::string response;
	response += "HTTP/1.1 " + status + "\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + toString(body.size()) + "\r\n";
	response += "Connection: close\r\n";
	response += "\r\n";
	response += body;

	return (response);
}
