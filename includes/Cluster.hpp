#ifndef CLUSTER_HPP
# define CLUSTER_HPP

#include <vector>
#include <string>
#include <iostream>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include "Config.hpp"
#include "Server.hpp"
#include "ConnectionManager.hpp"
#include "FileSystem.hpp"

class Cluster
{
	private:
		Cluster(void) {};
		void addServers(std::vector<pollfd> &pollfds);
		pollfd	newSocketNode(int socket);

	public:
		~Cluster(void);
		Cluster(std::vector<Config> configs);

		void	MainLoop();

		std::vector<Server> servers;
		std::vector<FileSystem *> filesystems;
		unsigned long serverCount;
};

#endif
