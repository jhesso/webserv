#ifndef FILESYSTEM_HPP
# define FILESYSTEM_HPP

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
#include "Configuration.hpp"
#include "Folder.hpp"

class Folder;
class DirOrFile;

#define IS_ACCESSIBLE_DIRECTORY 11
#define IS_DENIED_DIRECTORY 12
#define IS_FILE 13
#define NOT_FOUND 14

class FileSystem
{
	private:
		std::map<std::string, Folder> _fileSystem;

		std::string	_root;
		std::string	_index;
		std::map<int, std::string>	_error_pages;

		bool	_hasIndex;
		bool	_hasErrorPage;

		std::map<std::string, DirOrFile>  readDirectory(std::string path);
		std::pair<std::string, std::string> createPaths(std::string serverRoot, std::string fromProjectRoot);
		bool rootExists();
	public:
		bool hasIndex(){return _hasIndex;};
		bool hasErrorPage(){return _hasErrorPage;};
		std::string getIndex(){return _index;};
		std::string getErrorPage(int errorNum);
		int	_numPendingEntries;
		int _numPendingDeletes;
		std::map<std::string, std::string> _pendingEntries;
		std::map<std::string, std::string> _pendingDeletes;

		std::pair<std::string, int>	findWithPath(std::string &path);
		Folder &getFolder(std::string &path);
		Folder &getFileParentFolder(std::string &path);

		FileSystem(Configuration &configuration);
		FileSystem(FileSystem &other);
		~FileSystem(void);
		void printFileSystem();
		void handlePending();
};

#endif
