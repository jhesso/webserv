#include "../includes/ConnectionManager.hpp"

ConnectionManager::ConnectionManager(Server *parent) : _hasMovableRequest(false), _parent(parent), _responsesReady(0)
{}

ConnectionManager::~ConnectionManager(void)
{}

HttpRequest	ConnectionManager::getMovable()
{
	_hasMovableRequest = false;
	HttpRequest ret(this->_requests[_movableIndex]);
	_requests.erase(_requests.begin() + _movableIndex);
	return ret;
}

void ConnectionManager::recvMovable(HttpRequest objct)
{
	_responses.push_back(HttpResponse(objct, this->_parent));
	if (_responses.back().isReady())
		_responsesReady++;
}

int ConnectionManager::findObjectIndex(int cs)
{
	for (size_t i = 0; i < _requests.size(); ++i)
	{
	if (_requests[i].compareCs(cs))
	{
		return static_cast<int>(i);
	}
	}
	return -1;
}

int	ConnectionManager::handleResponse()
{
	for (unsigned long i = 0; i < _responses.size(); i++)
	{
		if (_responses[i].isReady())
		{
			try {
				_responses[i].sendResponse();
				if (_responses[i].hasBeenSent())
				{
					_responsesReady--;
					_responses.erase(_responses.begin() + i);
				}
				return -1;
			}
			catch (std::exception &e) {
				std::cerr << "Error in sendResponse(): " << e.what() << std::endl;
				_responsesReady--;
				int sock = _responses[i].getCs();
				_responses.erase(_responses.begin() + i);
				return sock;
			}
		}
	}
	return -1;
}

bool	ConnectionManager::hasRunningProcesses()
{
	for (unsigned long i = 0; i < _responses.size(); i++)
	{
		if (_responses[i].hasRunningProcess())
			return true;
	}
	return false;
}

bool	ConnectionManager::completeProcess()
{
	for (unsigned long i = 0; i < _responses.size(); i++)
	{
		if (_responses[i].hasRunningProcess())
		{
			std::chrono::time_point<std::chrono::steady_clock> _cur = std::chrono::steady_clock::now();
			auto duration = _cur - _responses[i].getTime();
			int stat;
			if (waitpid(_responses[i].getPid(), &stat, WNOHANG) > 0)
			{
				_responses[i].completeMe(stat);
				_responsesReady++;
				return true;
			}
			else if (duration > std::chrono::seconds(5))
			{
				kill(_responses[i].getPid(), SIGKILL);
				_responses[i].completeMe(-1);
				_responsesReady++;
				return false;
			}
			return false;
		}
	}
	return false;
}

bool	ConnectionManager::hasReadyResponses()
{
	if (_responsesReady > 0)
		return true;
	return false;
}

bool	ConnectionManager::handleConnection(int cs)
{
	const int bufferSize = 1024;
	char buffer[bufferSize];

	ssize_t bytesRead = recv(cs, buffer, bufferSize - 1, 0);
	if (bytesRead == -1 || bytesRead == 0)
	{
		return false;
	}
	else
	{
		buffer[bytesRead] = '\0';

		int indexOfRequest = findObjectIndex(cs);
		if (indexOfRequest == -1)
		{
			try {
				_requests.push_back(HttpRequest(buffer, cs, this->_parent->getBodySize()));
			}
			catch (std::exception &e) {
				HttpRequest error = HttpRequest(std::stoul(e.what()), cs);
				_responses.push_back(HttpResponse(error, this->_parent));
				_responsesReady++;
				return true;
			}
			if (_requests.back().seeIfComplete())
			{
				if (_requests.back().isForThisServer(this->_parent->_host, this->_parent->_otherHosts))
				{
					_responses.push_back(HttpResponse(_requests.back(), this->_parent));
					_requests.pop_back();
					if (_responses.back().isReady())
						_responsesReady++;
				}
				else
				{
					_hasMovableRequest = true;
					_movableIndex = _requests.size() - 1;
				}
			}
		}
		else
		{
			try {
				_requests[indexOfRequest].parseCurrentBuffer(buffer);
			}
			catch (std::exception &e) {
				_requests.erase(_requests.begin() + indexOfRequest);
				HttpRequest error = HttpRequest(std::stoul(e.what()), cs);
				_responses.push_back(HttpResponse(error, this->_parent));
				_responsesReady++;
				return true;
			}
			if (_requests[indexOfRequest].seeIfComplete())
			{
				if (_requests.back().isForThisServer(this->_parent->_host, this->_parent->_otherHosts))
				{
					_responses.push_back(HttpResponse(_requests[indexOfRequest], this->_parent));
					_requests.erase(_requests.begin() + indexOfRequest);
					if (_responses.back().isReady())
						_responsesReady++;
				}
				else
				{
					_hasMovableRequest = true;
					_movableIndex = _requests.size() - 1;
				}
			}
		}
		return true;
	}
}
