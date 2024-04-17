#ifndef FOLDER_HPP
# define FOLDER_HPP

#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/types.h>
#include <utility>
#include <filesystem>
#include "DirOrFile.hpp"

class DirOrFile;

class Folder
{
	private:
		std::map<std::string, DirOrFile> _folderContent;

	public:
		bool _isRedirected;
		std::string _redirectPath;
		bool _hasDefaultPage;
		std::string	_defaultPage;
		bool _postAllowed;
		bool _getAllowed;
		bool _deleteAllowed;
		bool _cgiAllowed;
		bool _autoIndexOn;
		bool _isRooted;
		std::string _rootedTo;
		bool _uploadPass;
		std::string _uploadPath;

		Folder(std::map<std::string, DirOrFile> folder, std::vector<std::pair<std::string, std::map<std::string, std::string>>> locations, std::string path);
		~Folder(void);
		Folder(const Folder& other);
		Folder& operator=(const Folder& other);

		void	printFolder();
		DirOrFile &findEntry(const std::string &path);
		std::string	autoIndexGenerator(std::string &path);
		void	createEntry(const std::string &path, std::string content);
		void	deleteEntry(const std::string &path);
};

#endif
