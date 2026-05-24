#ifndef UPLOAD_HANDLER_HPP
#define UPLOAD_HANDLER_HPP

#include "HttpRequest.hpp"
#include "server.hpp"

#include <string>
#include <vector>
#include <map>

struct MultipartPart
{
	std::map<std::string, std::string> headers;
	std::string body;
};

class UploadHandler
{
	public:
		void handle(const HttpRequest& req, Client& client, const location& loc,
			const std::string& location_prefix);
	private:
		void validateUploadRequest(const HttpRequest& req, const location& loc);
		bool isMultipart(const HttpRequest& req);
		void handleMultipart(const HttpRequest& req, Client& client, const location& loc);
		void handleRaw(const HttpRequest& req, Client& client, const location& loc, const std::string& location_prefix);
		std::string extractRawFilename(const HttpRequest& req, const std::string& location_prefix);
		std::string extractBoundary(const std::string& contentType);
		MultipartPart parseOnePart(const std::string& partString);
		std::vector<MultipartPart> parseMultipart(const std::string& body, const std::string& boundary);
		std::string extractFilename(const std::string& contentDisposition);
		std::string sanitizeFilename(const std::string& name);
		void saveFile(const std::string& uploadDirectory, const std::string& fileName, const std::string& fileBody);
		std::string buildResponse(int statusCode, const std::string& msg);
};

#endif
