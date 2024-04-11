#include "../includes/Folder.hpp"

Folder::Folder(const Folder& other):
      _isRedirected(other._isRedirected),
      _redirectPath(other._redirectPath),
	  _hasDefaultPage(other._hasDefaultPage),
	  _defaultPage(other._defaultPage),
      _postAllowed(other._postAllowed),
      _getAllowed(other._getAllowed),
      _deleteAllowed(other._deleteAllowed),
      _cgiAllowed(other._cgiAllowed),
      _autoIndexOn(other._autoIndexOn),
	  _isRooted(other._isRooted),
	  _rootedTo(other._rootedTo),
	  _uploadPass(other._uploadPass),
	  _uploadPath(other._uploadPath)
  {
      for (const auto& [key, value] : other._folderContent)
	  {
          _folderContent.emplace(key, value);
      }
  }

Folder::Folder(std::map<std::string, DirOrFile> folder, std::vector<std::pair<std::string, std::map<std::string, std::string>>> locations, std::string path):
	_folderContent(folder),
	_isRedirected(false),
	_hasDefaultPage(false),
	_postAllowed(false),
	_getAllowed(false),
	_deleteAllowed(false),
	_cgiAllowed(false),
	_autoIndexOn(false),
	_isRooted(false),
	_uploadPass(false)
{
	for ( auto& locationPair : locations)
	{
		std::string dirName = locationPair.first;
		if (dirName == path)
		{
			std::map<std::string, std::string> &dirConfig = locationPair.second;
			auto it = dirConfig.find("POST");
			if (it != dirConfig.end())
			{
				_postAllowed = true;
			}
			auto it2 = dirConfig.find("GET");
			if (it2 != dirConfig.end())
			{
				_getAllowed = true;
			}
			auto it3 = dirConfig.find("DELETE");
			if (it3 != dirConfig.end())
			{
				_deleteAllowed = true;
			}


			auto it4 = dirConfig.find("cgi");
			if (it4 != dirConfig.end())
			{
				if (it4->second == "allowed")
					_cgiAllowed = true;
			}
			auto it5 = dirConfig.find("autoindex");
			if (it5 != dirConfig.end())
			{
				if (it5->second == "on")
					_autoIndexOn = true;
			}
			auto it6 = dirConfig.find("proxy_pass");
			if (it6 != dirConfig.end())
			{
				_isRedirected = true;
				_redirectPath = it6->second;
			}
			auto it8 = dirConfig.find("root");
			if (it8 != dirConfig.end())
			{
				_isRooted = true;
				_rootedTo = it8->second;
			}
			auto it9 = dirConfig.find("upload_pass");
			if (it9 != dirConfig.end())
			{
				_uploadPass = true;
				_uploadPath = it9->second;
			}
			auto it7 = dirConfig.find("index");
			if (it7 != dirConfig.end())
			{
				_hasDefaultPage = true;
				std::ifstream file("." + it7->second);
				if (!file.is_open())
				{
					std::cout << "Error opening file: " << it7->second << " index set off" << std::endl;
					_hasDefaultPage = false;
					break;
				}
				std::string content;
				_defaultPage = "";
				while (std::getline(file, content))
				{
					_defaultPage += content;
				}
				file.close();
			}
			break;
		}
	}
}

Folder& Folder::operator=(const Folder& other)
{
  if (this != &other)
  {
    _folderContent.clear();
    for (auto [key, value] : other._folderContent)
	{
      // Create a new DirOrFile object for each entry
      _folderContent.emplace(key, DirOrFile(value.amIDir(), value.getFileContent())); // This assumes a copy constructor exists for DirOrFile
    }

    // Copy other primitive members
    _isRedirected = other._isRedirected;
    _redirectPath = other._redirectPath;
    _hasDefaultPage = other._hasDefaultPage;
    _defaultPage = other._defaultPage;
    _postAllowed = other._postAllowed;
    _getAllowed = other._getAllowed;
    _deleteAllowed = other._deleteAllowed;
    _cgiAllowed = other._cgiAllowed;
    _autoIndexOn = other._autoIndexOn;
    _isRooted = other._isRooted;
    _rootedTo = other._rootedTo;
	_uploadPass = other._uploadPass;
	_uploadPath = other._uploadPath;
  }
  return *this;
}

Folder::~Folder()
{
	_folderContent.clear();
}

void Folder::printFolder()
{
	for (auto& entry : _folderContent)
	{
		std::cout << entry.first << std::endl;
	}
}

DirOrFile &Folder::findEntry(const std::string &path)
{
	auto it = _folderContent.find(path);
	if (it == _folderContent.end())
		throw std::exception();
	return (it->second);
}

std::string Folder::autoIndexGenerator(std::string &path)
{
	std::string html = "<!DOCTYPE html>\n";
	html += "<html lang=\"en\">\n";
	html += "<head>\n";
	html += "<meta charset=\"UTF-8\">\n";
	html += "<title>Index of ";
	html += path;
	html += "</title>\n";
	html += "</head>\n";
	html += "<body>\n";
	html += "<h1>Index of ";
	html += path;
	html += "</h1>\n";
	html += "<hr>\n";
	html += "<ul>\n";
	for (auto& entry : _folderContent)
	{
		html += "<li>";
		html += "<a href=\"" + entry.first + "\">";
		html += entry.first;
		html += "</a>";
		html += "</li>\n";
	}
	html += "</ul>\n";
	html += "<hr>\n";
	html += "</body>\n";
	html += "</html>\n";
	return html;
}

void Folder::createEntry(const std::string &path, std::string content)
{
	std::ofstream file("." + path);
	if (!file.is_open())
	{
		throw std::runtime_error("500");
	}
	file.close();
	_folderContent.emplace(path, DirOrFile(false, content));
}

void	Folder::deleteEntry(const std::string &path)
{
	auto it = _folderContent.find(path);
	if (it == _folderContent.end())
	{
		throw std::runtime_error("404");
	}
	_folderContent.erase(it);
}
