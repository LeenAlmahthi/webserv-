#include "HttpRequest.hpp"
#include "Client.hpp"
#include <sstream>
#include <vector>

void trim(std::string &str) 
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) 
    {
        str = "";
        return;
    }
    size_t end = str.find_last_not_of(" \t\r\n");
    str = str.substr(start, end - start + 1);
}

ParsingResult parse_http_req(std::string &reqbuffer, HttpRequest &request, size_t &bytes_to_remove) 
{
    // [1] Find header terminator
    size_t end_of_header = reqbuffer.find("\r\n\r\n");
    if (end_of_header == std::string::npos)
        return PARSE_INCOMPLETE;
    size_t header_size = end_of_header + 4;

    // [2] Parse request line
    size_t end_of_first_line = reqbuffer.find("\r\n");
    if (end_of_first_line == std::string::npos || end_of_first_line >= end_of_header)
        return PARSE_ERROR;

    std::string first_line = reqbuffer.substr(0, end_of_first_line);
    std::istringstream iss(first_line);
    std::string extra;

    iss >> request.method;
    iss >> request.path;
    iss >> request.http_version;

    if (request.method.empty() || request.path.empty() || request.http_version.empty())
        return PARSE_ERROR;
    if (request.http_version != "HTTP/1.1" && request.http_version != "HTTP/1.0")
        return PARSE_ERROR;
    if (iss >> extra)
        return PARSE_ERROR;

    // [3] Parse headers
    size_t headers_start = end_of_first_line + 2;
    size_t headers_len = end_of_header - headers_start;
    std::string headers_section = reqbuffer.substr(headers_start, headers_len);

    std::istringstream headers_stream(headers_section);
    std::string line;
    size_t header_count = 0;

    request.headers.clear();

    while (std::getline(headers_stream, line, '\n')) 
    {
        if (++header_count > 100)
            return PARSE_ERROR;

        if (!line.empty() && line[line.length() - 1] == '\r')
            line.erase(line.length() - 1);

        if (line.empty())
            continue;

        size_t colon = line.find(":");
        if (colon == std::string::npos)
            return PARSE_ERROR;

        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        for (size_t i = 0; i < key.length(); ++i)
            key[i] = std::tolower(static_cast<unsigned char>(key[i]));

        trim(key);
        trim(value);

        request.headers[key] = value;
    }

    // [4] Handle body
    if (request.headers.count("transfer-encoding"))
    {
        //Here just determining the begining and ending of the chunked section in the req
        if (request.headers["transfer-encoding"] == "chunked")
        {
            request.is_chunked = true;
            // Walk chunks properly to avoid false match on data containing "0\r\n\r\n"
            size_t pos = header_size;
            while (pos < reqbuffer.size()) 
            {
                size_t line_end = reqbuffer.find("\r\n", pos);
                if (line_end == std::string::npos)
                    return PARSE_INCOMPLETE;

                std::string size_str = reqbuffer.substr(pos, line_end - pos);

                // Strip chunk extensions (ex) "a; name=value" → "a")
                size_t semi = size_str.find(';');
                if (semi != std::string::npos)
                    size_str = size_str.substr(0, semi);

                trim(size_str);
                if (size_str.empty())
                    return PARSE_ERROR;

                size_t chunk_size;
                try 
                { 
                    chunk_size = std::stoul(size_str, nullptr, 16); 
                }
                catch (...) 
                { 
                    return PARSE_ERROR; 
                }

                if (chunk_size == 0) 
                {
                    // Terminating chunk found — total includes "0\r\n" + "\r\n"
                    size_t total = (line_end + 2 + 2) - header_size;
                    request.body = reqbuffer.substr(header_size, total);
                    bytes_to_remove = header_size + total;
                    return PARSE_COMPLETE;
                }

                // Skip: size_line\r\n + chunk_data + \r\n
                pos = line_end + 2 + chunk_size + 2;
            }
            return PARSE_INCOMPLETE;
        }
        else 
        {
            return PARSE_ERROR; // Unsupported transfer encoding
        }
    }
    // else if (request.headers.count("content-length")) 
    // {
    //     size_t bodysize = 0;
    //     try 
    //     {
    //         bodysize = std::stoul(request.headers["content-length"]);
    //     }
    //     catch (...) 
    //     {
    //         return PARSE_ERROR;
    //     }

    //     if (reqbuffer.size() < header_size + bodysize)
    //         return PARSE_INCOMPLETE;

    //     if (bodysize > 0)
    //         request.body = reqbuffer.substr(header_size, bodysize);
    //     else
    //         request.body.clear();

    //     bytes_to_remove = header_size + bodysize;
    //     return PARSE_COMPLETE;
    // }
    else if (request.headers.count("content-length")) 
{
    size_t bodysize = 0;
    try 
    {
        bodysize = std::stoul(request.headers["content-length"]);
    }
    catch (...) 
    {
        return PARSE_ERROR;
    }

    // ✅ FIX #1: Check size BEFORE completeness (critical for DoS prevention)
    const size_t MAX_BODY_SIZOE = 10 * 1024 * 1024;  // 10MB hardcoded
    if (bodysize > MAX_BODY_SIZOE) {
        request.oversized_body = true;
        return PARSE_ERROR;  // Reject immediately - no waiting for body
    }

    // ✅ FIX #2: Check body completeness AFTER size validation
    if (reqbuffer.size() < header_size + bodysize)
        return PARSE_INCOMPLETE;

    // Extract body
    if (bodysize > 0)
        request.body = reqbuffer.substr(header_size, bodysize);
    else
        request.body.clear();

    bytes_to_remove = header_size + bodysize;
    return PARSE_COMPLETE;
}
    else 
	{
        // No body (ex GET requests)
        request.body.clear();
        bytes_to_remove = header_size;
    }

    // [5] HTTP/1.1 requires Host header
    if (request.http_version == "HTTP/1.1" && !request.headers.count("host"))
        return PARSE_ERROR;

    if (!request.headers.count("content-length") && !request.headers.count("transfer-encoding"))
        return PARSE_COMPLETE;

    return PARSE_COMPLETE;
}