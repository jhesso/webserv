#include "../includes/Server.hpp"

long long Server::getBodySize()
{
	return _maxBodySizeInBytes;
}

Server::Server(const Server &other) : _connectionManager(new ConnectionManager(this))
{
	_host = other._host;
	_otherHosts = other._otherHosts;
	_fileSystem = other._fileSystem;
	_serverSocket = other._serverSocket;
	_serverName = other._serverName;
	_root = other._root;
	_maxBodySizeInBytes = other._maxBodySizeInBytes;
	_port = other._port;
}

Server& Server::operator=(Server& other)
{
	if (this != &other)
	{
		delete this->_connectionManager;
		_connectionManager = new ConnectionManager(this);
		_fileSystem = other._fileSystem;
		_serverSocket = other._serverSocket;
		_serverName = other._serverName;
		_host = other._host;
		_root = other._root;
		_maxBodySizeInBytes = other._maxBodySizeInBytes;
		_port = other._port;
		_otherHosts = other._otherHosts;
	}
	return *this;
}

/*port is left*/
Server::Server(Config &Config, FileSystem *fs, std::vector<std::pair<int,int>> &portsInUse):
	 _connectionManager(new ConnectionManager(this)),
	 _fileSystem(fs)
{
	bool portUnused = true;
	for (unsigned long i = 0; i < portsInUse.size(); i++)
	{
		std::pair<int,int> portAndSock = portsInUse[i];
		if (portAndSock.first == Config.getListenPort())
		{
			this->_serverSocket = portAndSock.second;
			portUnused = false;
			break;
		}
	}
	if (portUnused)
	{
		this->_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (_serverSocket == -1)
			throw std::runtime_error("Error creating socket");
		int reuse = 1;
		if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
		{
			close(_serverSocket);
			throw std::runtime_error("Error setting socket options");
		}
		sockaddr_in serverAddress;
		std::memset(&serverAddress, 0, sizeof(serverAddress));
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.s_addr = INADDR_ANY; // Accept connections on any interface
		serverAddress.sin_port = htons(Config.getListenPort()); // Set the port to right one
		if (bind(_serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1)
		{
			close(_serverSocket);
			throw std::runtime_error("Error binding socket");
		}
		if (listen(_serverSocket, 10) == -1)
		{
			close(_serverSocket);
			throw std::runtime_error("Error listening on socket");
		}
	}
	_port = Config.getListenPort();
	_serverName = Config.getValue("server_name");
	_host = Config.getValue("host");
	_root = Config.getValue("root");
	if (Config.getValue("client_max_body_size") != "")
		_maxBodySizeInBytes = std::stoull(Config.getValue("client_max_body_size"));
}

Server::~Server(void)
{
	delete this->_connectionManager;
}

int Server::getPort()
{
	return _port;
}

int Server::getSocket()
{
	return (this->_serverSocket);
}

std::string	Server::getValue(std::string what)
{
	if (what == "server_name")
		return _serverName;
	if (what == "host")
		return _host;
	if (what == "root")
		return _root;
	return "";
}

void Server::printMyVals()
{
	std::cout << "##server " << _serverName << " has these values##" << std::endl;
	std::cout << "serverSocket: " << _serverSocket << std::endl;
	std::cout << "serverName: " << _serverName << std::endl;
	std::cout << "hostAddress: " << _host << std::endl;
	std::cout << "rootDir: " << _root << std::endl;
	std::cout << "###########################" << std::endl;
}
