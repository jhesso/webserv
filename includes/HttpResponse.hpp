#ifndef HTTPRESPONSE_HPP
# define HTTPRESPONSE_HPP

#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "HttpRequest.hpp"
#include "Server.hpp"
#include "Folder.hpp"
#include <unistd.h>
#include <algorithm>

#define INTERPRETER_PYTHON "/usr/bin/python"

class Server;

class HttpResponse
{
	private:
		void	executeCmd();
		void	parseResponse();

		int _cs;

		std::string	findPhrase(int code);

		bool			isCgi();

		std::string getContentType();
		std::string getConnection();
		std::string getContentLength();

		void doGet();
		void doPost();
		void doDelete();

		std::string	findErrorPage(int code);

		bool 	imReady;
		bool	isSent;

		std::string _status;
		std::string _headers;
		std::string _body;

		std::string	_response;


		std::map<std::string, std::string> _reqHeaders;
		std::string _reqMethod;
		std::string	_reqPath;
		std::string _reqBody;

		Server *_parent;

		void	handleRooted();
		std::string findFreeName(Folder &folder, std::string path);
		int			infoCode;

		int	_readFd;
		int _childPid;
		bool _runningProcess;

		std::chrono::time_point<std::chrono::steady_clock> _time;

		char **createArgv();
		char **createEnvs();
		void doCgi();

	public:
		HttpResponse(HttpRequest &src, Server *parent);
		~HttpResponse(void);

		void sendResponse();


		bool	isReady();
		bool	hasBeenSent();
		bool	hasRunningProcess(){return (_runningProcess);};
		std::chrono::time_point<std::chrono::steady_clock> getTime(){return (_time);};
		int		getPid(){return (_childPid);};
		bool   completeMe(int status);
		int getCs(){return (_cs);};
};

#endif
