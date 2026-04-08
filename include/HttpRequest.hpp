#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <iostream>
#include <map>
#include <string>
#define MAX_BODY_SIZE (10 * 1024 * 1024)

// struct HttpRequest 
// {
//     std::string method;
//     std::string path;
//     std::string http_version;
//     std::map<std::string, std::string> headers;
//     std::string body;
//     bool is_chunked;  
    
//     HttpRequest() : is_chunked(false) {}
// };
struct HttpRequest {
    std::string method;
    std::string path;
    std::string http_version;
    std::map<std::string, std::string> headers;
    std::string body;
    bool is_chunked;
    bool oversized_body;  // ← ADD THIS FLAG
    
    HttpRequest() : is_chunked(false), oversized_body(false) {}  // ← INITIALIZE
};
enum ParsingResult
{
	PARSE_COMPLETE,
	PARSE_INCOMPLETE,
	PARSE_ERROR
};
void trim(std::string &str);
ParsingResult parse_http_req(std::string &buffer,HttpRequest &request,size_t &bytes_consume);
#endif
