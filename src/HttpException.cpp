#include "HttpException.hpp"

HttpException::HttpException(int status_code, const std::string& msg) : _status_code(status_code), _msg(msg)
{}

HttpException::~HttpException() throw()
{}

const char*	HttpException::what() const throw()
{
	return (_msg.c_str());
}

int	HttpException::getStatusCode() const
{
	return (_status_code);
}

const std::string&	HttpException::getMsg() const
{
	return (_msg);
}
