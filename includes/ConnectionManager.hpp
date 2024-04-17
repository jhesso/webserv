#ifndef CONNECTION_MANAGER_HPP
#define CONNECTION_MANAGER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Server.hpp"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

class Server;
class HttpRequest;
class HttpResponse;

class ConnectionManager
{
	private:
		Server *_parent;
		int _responsesReady;
		std::vector<HttpRequest> _requests;
		std::vector<HttpResponse> _responses;

		int findObjectIndex(int cs);

	public:
		ConnectionManager(Server *parent);
		~ConnectionManager();

		bool	handleConnection(int cs);
		int		handleResponse();
		bool	hasReadyResponses();
		bool	hasRunningProcesses();
		bool	completeProcess();
		bool	_hasMovableRequest;
		unsigned long		_movableIndex;
		HttpRequest	getMovable();
		void		recvMovable(HttpRequest objct);
};

#endif
