#include <iostream>
#include "../includes/FileSystem.hpp"

FileSystem::FileSystem(Config &Config):
	_root(Config.getValue("root")),
	_index((Config.getValue("index"))),
	_hasIndex(false),
	_hasErrorPage(false),
	_numPendingEntries(0),
	_numPendingDeletes(0)
{
	if (_root.empty())
		_root = "/";
	if (!rootExists())
		throw std::runtime_error("server root doesn't exist");
	try {
		std::map<std::string, std::string> &directives = Config.getDirectives();
		for (auto &directive : directives)
		{
			std::string  entryKey = directive.first.substr(0, directive.first.find_first_of(" \t"));
			if (entryKey == "error_page")
			{
				std::string errorNumStr = directive.second.substr(0, directive.second.find_first_of(" \t"));
				std::string errorPath = directive.second.substr(directive.second.find_first_of(" \t") + 1);
				while (errorPath[0] == ' ' || errorPath[0] == '\t')
					errorPath = errorPath.substr(1);
				int errorNum = std::stoi(errorNumStr);
				std::ifstream file("." + errorPath);
				if (!file.is_open())
				{
					std::cout << "Error opening file: " << errorPath << " error page set off" << std::endl;
					continue;
				}
				std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
				_error_pages[errorNum] = content;
				_hasErrorPage = true;
			}
		}
	} catch (std::exception &e) {
		_hasErrorPage = false;
	}
	if (!_index.empty())
	{
		std::ifstream file("." + _index);
		if (!file.is_open())
		{
			std::cout << "Error opening file: " << _index << " index set off" << std::endl;
			_index = "";
			_hasIndex = false;
		}
		else
		{
			_hasIndex = true;
			std::string contentia((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			_index = contentia;
		}
	}
	std::vector<std::pair<std::string, std::map<std::string, std::string>>> locations = Config.getLocations();
	for (auto& locationPair : locations)
	{
		std::string &dirPath = locationPair.first;
		std::pair<std::string, std::string> paths = createPaths(_root, dirPath);
		_fileSystem.emplace(dirPath, Folder(readDirectory(dirPath), locations, dirPath));
	}

}

FileSystem::FileSystem(FileSystem& src)
{
	*this = src;
}

FileSystem::~FileSystem(void)
{
	std::cout << "FileSystem destructor called" << std::endl;
}

std::string FileSystem::getErrorPage(int errorNum)
{
	try
	{
		return _error_pages.at(errorNum);
	}
	catch (std::exception)
	{
		return "";
	}
}

bool FileSystem::rootExists()
{
	std::string rootPath = _root;

	if (rootPath.empty())
	{
		return false;
	}
	std::string combinedPath = ".";
	combinedPath += rootPath;

	DIR* dirPtr = opendir(combinedPath.c_str());
	if (dirPtr != nullptr)
	{
		closedir(dirPtr);
		return true;
	}
	return false;
}

std::pair<std::string, std::string> FileSystem::createPaths(std::string serverRoot, std::string fromProjectRoot)
{
	size_t serverRootLength = serverRoot.length();
	if (fromProjectRoot.length() >= serverRootLength &&
		fromProjectRoot.substr(0, serverRootLength) == serverRoot)
	{
		std::string relativePath = fromProjectRoot.substr(serverRootLength);
		std::pair<std::string, std::string> ret = std::make_pair(relativePath, fromProjectRoot);
		return ret;
	}
	else
	{
		throw std::runtime_error("location directive invalid. Directory not found within servers root");
	}
	return std::make_pair("", "");
}

std::map<std::string, DirOrFile>  FileSystem::readDirectory(std::string path)
{
	std::map<std::string, DirOrFile> folder;

	std::string relativePath = "." + path;
	DIR *dir = opendir(relativePath.c_str());
	if (dir == nullptr)
	{
		std::cout << "Error opening directory: " << relativePath << std::endl;
		throw std::runtime_error("Error opening directory");
	}
	struct dirent *entry;
	while ((entry = readdir(dir)) != nullptr)
	{
		try
		{
			if (entry->d_type != DT_REG && entry->d_type != DT_DIR)
				continue;
			if (entry->d_type == DT_DIR)
			{
				folder.emplace(path + entry->d_name, DirOrFile(true, entry->d_name, path));
			}
			else
			{
				folder.emplace(path + entry->d_name, DirOrFile(false, entry->d_name, path));
			}
		}
		catch (std::exception &e)
		{
			(void)e;
			continue;
		}
	}
	closedir(dir);
	return folder;
}

void FileSystem::printFileSystem()
{
	for (auto& folder : _fileSystem)
	{
		std::cout << "####" << " Folders called #" << folder.first << "# Content is ###" << std::endl;
		folder.second.printFolder();
	}
}

std::pair<std::string, int> FileSystem::findWithPath(std::string &path)
{
	try
	{
		_fileSystem.at(path);
		return (std::make_pair("", IS_ACCESSIBLE_DIRECTORY));
	}
	catch (std::exception)
	{
		try
		{
			size_t lastSlashPos = path.find_last_of("/");
			std::string modifiedPath = "";
			if (lastSlashPos != std::string::npos && lastSlashPos != 0)
			{
				if (lastSlashPos == path.size() - 1)
				{
					modifiedPath = path.substr(0, lastSlashPos);
					lastSlashPos = modifiedPath.find_last_of("/");
				}
				if (lastSlashPos != std::string::npos)
					lastSlashPos += 1;
				if (lastSlashPos != std::string::npos && lastSlashPos != path.size())
				{
					modifiedPath = path.substr(0, lastSlashPos);
					_fileSystem.at(modifiedPath);
					auto it = _fileSystem.find(modifiedPath);
					Folder &folder = it->second;
					DirOrFile & entry = folder.findEntry(path);
					if (entry.amIDir())
					{
						return (std::make_pair("", IS_DENIED_DIRECTORY));
					}
					else
					{
						return (std::make_pair(entry.getFileContent(), IS_FILE));
					}
				}
			}
			else if (lastSlashPos == 0)
			{
				_fileSystem.at("/");
				auto it = _fileSystem.find("/");
				Folder &folder = it->second;
				DirOrFile & entry = folder.findEntry(path);
				if (entry.amIDir())
				{
					return (std::make_pair("", IS_DENIED_DIRECTORY));
				}
				else
				{
					return (std::make_pair(entry.getFileContent(), IS_FILE));
				}
			}
			else
			{
				throw;
			}
		}
		catch (std::exception)
		{
			return (std::make_pair("", NOT_FOUND));
		}
	}
	return (std::make_pair("", NOT_FOUND));
}

Folder &FileSystem::getFolder(std::string &path)
{
	auto it = _fileSystem.find(path);
	if (it == _fileSystem.end())
	{
		throw std::runtime_error("404");
	}
	return(it->second);
}

Folder &FileSystem::getFileParentFolder(std::string &path)
{
	size_t lastSlashPos = path.find_last_of("/") + 1;
	std::string subPath = path.substr(0, lastSlashPos);
	auto it = _fileSystem.find(subPath);
	return it->second;
}

void	FileSystem::handlePending()
{
	if (_numPendingEntries > 0)
	{
		auto it = _pendingEntries.begin();
		std::string path = it->first;
		std::string content = it->second;
		_numPendingEntries--;
		std::ofstream file("." + path);
		file << content;
		file.close();
		_pendingEntries.erase(it);
	}
	else if (_numPendingDeletes > 0)
	{
		auto it = _pendingDeletes.begin();
		std::string path = it->first;
		_numPendingDeletes--;
		std::remove(("./" + path).c_str());
		_pendingDeletes.erase(it);
	}
}
