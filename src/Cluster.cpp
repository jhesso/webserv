#include "../includes/Cluster.hpp"

bool global_shouldRun;

void	signalHandler(int sig)
{
	std::cout << "\ncaught signal: " << sig << ". Shutting down" << std::endl;
	global_shouldRun = false;
}

void	sigpipe_handler(int sig)
{
	(void)sig;
}

void printMap(const std::map<std::string, std::string>& mapToPrint)
{
  if (mapToPrint.empty())
  {
	std::cout << "Map is empty." << std::endl;
	return;
  }
  for (const auto& [key, value] : mapToPrint)
  {
	std::cout << "Key: " << key << ", Value: " << value << std::endl;
  }
}

std::vector<std::string> addHosts(std::vector<Server> &servers, unsigned long myIndex)
{
	std::vector<std::string> ret;
	for (unsigned long i = 0; i < servers.size(); i++)
	{
		if (i == myIndex)
			continue;
		ret.push_back(servers[i]._host);
	}
	return ret;
}

Cluster::Cluster(std::vector<Config> configs)
{
	global_shouldRun = true;
	signal(SIGINT, signalHandler);
	signal(SIGPIPE, sigpipe_handler);
	try
	{
		this->serverCount = configs.size();
		FileSystem *ptr;
		std::vector<std::pair<int,int>> portsAndSocks;
		for (unsigned long i = 0; i < configs.size(); i++)
		{
			ptr = new FileSystem(configs[i]);
			this->filesystems.push_back(ptr);
			this->servers.push_back(Server(configs[i], ptr, portsAndSocks));
			portsAndSocks.push_back(std::make_pair(this->servers[i].getPort(), this->servers[i].getSocket()));
		}
		/*add vectors of other hosts to all servers*/
		for (unsigned long i = 0; i < this->servers.size(); i++)
		{
			this->servers[i]._otherHosts = addHosts(this->servers, i);
		}
	}
	catch (std::exception &e)
	{
		std::cout << "exception caught: " << std::endl;
		std::cout << e.what() << std::endl;
		throw std::exception();
	}
}

Cluster::~Cluster(void)
{
	std::cout << "cluster destructor called" << std::endl;
	for (unsigned long i = 0; i < this->filesystems.size(); i++)
	{
		delete this->filesystems[i];
	}
}

/******************************************************************************/
/*							PRIVATE FUNCTIONS								  */
/******************************************************************************/

pollfd	Cluster::newSocketNode(int socket)
{
	pollfd pfd;
	fcntl(socket, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
	pfd.fd = socket;
	pfd.events = POLLIN;
	return pfd;
}

void Cluster::addServers(std::vector<pollfd> &pollfds)
{
	std::cout << "\n###Available servers###" << std::endl;
	for (unsigned long i = 0; i < this->serverCount; i++)
	{
		pollfds.push_back(newSocketNode(this->servers[i].getSocket()));
		std::cout << "Server " << i + 1 << " listening on port: " << this->servers[i].getPort()<< std::endl;
	}
	std::cout << std::endl;
}

/******************************************************************************/
/*							PUBLIC FUNCTIONS								  */
/******************************************************************************/

/*need to hange this to have each server to have own connection manage and the right one needs to be called*/
void Cluster::MainLoop()
{
	int clientSocket;
	std::map<int, Server*> clientServerMap;
	std::vector<pollfd> pollfds;
	Server* curServer;
	Server* tmpServer;
	Server* associatedServer;
	bool performedAction = false;

	addServers(pollfds);

	bool loadBalancer = false;
	std::cout << "###READY###" << std::endl;
	while (global_shouldRun)
	{
		int numEvents = poll(pollfds.data(), pollfds.size(), 0);

		if (numEvents == -1 && global_shouldRun)
		{
			std::cerr << "Error in poll(): " << std::endl;
			continue ;
		}
		loadBalancer = !loadBalancer;
		if (!loadBalancer)
		{
			performedAction = false;
			for (unsigned long i = 0; i < serverCount; i++)
			{
				curServer = &this->servers[i];
				if (curServer->_connectionManager->_hasMovableRequest)
				{
					HttpRequest toMove = curServer->_connectionManager->getMovable();
					std::string hst = toMove.getHost();
					for (unsigned long i = 0; i < serverCount; i++)
					{
						tmpServer = &this->servers[i];
						if (tmpServer->_host == hst)
						{
							tmpServer->_connectionManager->recvMovable(toMove);
							break ;
						}
					}
				}
				if (curServer->_connectionManager->hasReadyResponses())
				{
					int rmS = curServer->_connectionManager->handleResponse();
					if (rmS != -1)
					{
						close(rmS);
						for(unsigned long i = 0; i < pollfds.size(); i++)
						{
							if (pollfds[i].fd == rmS)
							{
								pollfds.erase(pollfds.begin() + i);
								break ;
							}
						}
						clientServerMap.erase(rmS);
					}
					performedAction = true;
					break ;
				}
				else if (curServer->_connectionManager->hasRunningProcesses())
				{
					performedAction = curServer->_connectionManager->completeProcess();
					break ;
				}
			}
			if (performedAction)
				continue;
			for (unsigned long i = 0; i < this->filesystems.size(); i++)
			{
				if (this->filesystems[i]->_numPendingEntries || this->filesystems[i]->_numPendingDeletes)
				{
					this->filesystems[i]->handlePending();
					performedAction = true;
					break ;
				}
			}
			if (performedAction)
				continue;
		}

		for (unsigned long i = 0; i < pollfds.size(); ++i)
		{
			if (pollfds[i].revents & POLLIN)
			{
				if (i < this->serverCount)
				{
					// Accept connection on the correct server socket
					clientSocket = accept(this->servers[i].getSocket(), nullptr, nullptr);
					if (clientSocket != -1)
					{
						pollfds.push_back(newSocketNode(clientSocket));
						clientServerMap[clientSocket] = &this->servers[i];
					}
				}
				else /*check from the map which servers connection manager to call*/
				{
					clientSocket = pollfds[i].fd;
					associatedServer = clientServerMap[clientSocket];
					// Handle data on client sockets here
					if (!associatedServer->_connectionManager->handleConnection(pollfds[i].fd))
					{
						close(pollfds[i].fd);
						pollfds.erase(pollfds.begin() + i);
						clientServerMap.erase(clientSocket);
					}
					break ;
				}
			}
		}
	}
}
