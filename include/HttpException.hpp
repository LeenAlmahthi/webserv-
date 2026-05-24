#ifndef HTTP_EXCEPTION_HPP
#define HTTP_EXCEPTION_HPP

#include <exception>
#include <string>

class HttpException : public std::exception
{
	private:
		int _status_code;
		std::string _msg;
	public:
		HttpException(int status_code, const std::string& msg);
		virtual ~HttpException() throw();
		virtual const char* what() const throw();
		int getStatusCode() const;
		const std::string& getMsg() const;
};

#endif
