#include "../includes/HttpResponse.hpp"

/******************************************************************************/
/*						CONSTRUCTORS & DESTRUCTORS								*/
/******************************************************************************/

HttpResponse::HttpResponse(HttpRequest &src, Server *parent): 
		_cs(src.getCs()),
		imReady(false),
		isSent(false),
		_reqHeaders(src.getHeaders()),
		_reqMethod(src.getMethod()),
		_reqPath(src.getPath()),
		_reqBody(src.getBody()),
		_parent(parent),
		infoCode(418),
		_readFd(-1),
		_childPid(-1),
		_runningProcess(false)
{
	this->executeCmd();
	if (!_runningProcess)
		this->parseResponse();
}

HttpResponse::~HttpResponse(void)
{
	_reqHeaders.clear();
}

/*make this a error_page generator that modifies the string to have the right stuff*/
/*if not explicitly defined for a code in config*/
std::string	HttpResponse::findErrorPage(int code)
{
	if (this->_parent->_fileSystem->getErrorPage(code) != "") {
		_reqPath = "error.html";
		return this->_parent->_fileSystem->getErrorPage(code);
	}
	if (code >= 400 && code <= 511) {
		std::string error_page_content = R"(
		<!DOCTYPE html>
		<html lang="en">
		<head>
			<meta charset="UTF-8">
			<meta name="viewport" content="width=device-width, initial-scale=1.0">
			<title>Error )";
		error_page_content += std::to_string(code);
		error_page_content += R"(</title>
		</head>
		<body>
			<h1>Error )";
		error_page_content += std::to_string(code);
		error_page_content += R"(</h1>
			<p>)";
		error_page_content += findPhrase(code);
		error_page_content += R"(</p>
		</body>
		</html>
		)";
		_reqPath = "error.html";
		return (error_page_content);
	}
	return "";
}

void	HttpResponse::handleRooted()
{
	std::string path = _reqPath;

	for (size_t i = 0; i < path.size(); i++)
	{
		if (path[i] == '/')
		{
			std::string pathToCheck = path.substr(0, i + 1);
			std::pair<std::string, int> entry = this->_parent->_fileSystem->findWithPath(pathToCheck);
			if (entry.second == IS_ACCESSIBLE_DIRECTORY)
			{
				Folder folder = this->_parent->_fileSystem->getFolder(pathToCheck);
				if (folder._isRooted)
				{
					_reqPath = path.substr(i + 1);
					_reqPath = folder._rootedTo + _reqPath;
					return;
				}
			}
		}
	}
}

std::string HttpResponse::findFreeName(Folder &folder, std::string path)
{
	std::string name = path;
	size_t dotPos = name.find_last_of('.');
	std::string extension = "";
	if (dotPos != std::string::npos)
	{
		extension = name.substr(dotPos);
		name = name.substr(0, dotPos);
	}
	else {
		extension = ".txt";
	}
	int i = 1;
	while (true)
	{
		try {
			folder.findEntry(name + std::to_string(i) + extension);
			i++;
			continue;
		}
		catch (std::exception)
		{
			return (name + std::to_string(i) + extension);
		}
		i++;
	}
}

void	HttpResponse::doGet()
{
	std::pair<std::string, int> entry = this->_parent->_fileSystem->findWithPath(_reqPath);
	if (entry.second == IS_ACCESSIBLE_DIRECTORY || entry.second == IS_DENIED_DIRECTORY)
	{
		if (entry.second == IS_ACCESSIBLE_DIRECTORY)
		{
			Folder &folder = this->_parent->_fileSystem->getFolder(_reqPath);

			if (folder._isRedirected)
			{
				_reqPath = folder._redirectPath;
				infoCode = 307;
				return;
			}
			if (folder._hasDefaultPage || this->_parent->_fileSystem->hasIndex())
			{
				if (!folder._getAllowed)
					throw std::runtime_error("403");
				if (folder._hasDefaultPage)
				{
					_body = folder._defaultPage;
					infoCode = 200;
					_reqPath = "index.html";
					return ;
				}
				else
				{
					_body = this->_parent->_fileSystem->getIndex();
					infoCode = 200;
					_reqPath = "index.html";
					return;
				}
			}
			if (folder._autoIndexOn)
			{
				if (!folder._getAllowed)
					throw std::runtime_error("403");
				_body = folder.autoIndexGenerator(_reqPath);
				infoCode = 200;
				_reqPath = "autoindex.html";
				return;
			}
			else {
				throw std::runtime_error("404");
			}
		}
		else {
			throw std::runtime_error("403");
		}
	}
	else if (entry.second == IS_FILE)
	{
		Folder &dir = this->_parent->_fileSystem->getFileParentFolder(_reqPath);
		if (!dir._getAllowed)
			throw std::runtime_error("403");
		_body = entry.first;
		infoCode = 200;
	}
	else {
		throw std::runtime_error("404");
	}
}

void	HttpResponse::doPost()
{
	if (_reqPath.find("upload") != std::string::npos && _reqPath.find("upload") == _reqPath.size() - 6)
	{
		_reqPath = _reqPath.substr(0, _reqPath.size() - 6);
	}
	else {
		throw std::runtime_error("403");
	}

	std::pair<std::string, int> entry = this->_parent->_fileSystem->findWithPath(_reqPath);
	if (entry.second == IS_ACCESSIBLE_DIRECTORY || entry.second == IS_DENIED_DIRECTORY || entry.second == IS_FILE)
	{
		if (entry.second == IS_ACCESSIBLE_DIRECTORY)
		{
			Folder &folder = this->_parent->_fileSystem->getFolder(_reqPath);
			std::string uploadPath = _reqPath;

			if (folder._isRedirected)
			{
				_reqPath = folder._redirectPath;
				infoCode = 307;
				return ;
			}
			if (!folder._postAllowed)
			{
				throw std::runtime_error("403");
			}
			if (folder._uploadPass)
			{
				uploadPath = folder._uploadPath;
			}
			try {
				Folder &activeFolder = this->_parent->_fileSystem->getFolder(uploadPath);
				std::string fileName = "default";
				if (_reqHeaders.find("Content-Disposition") != _reqHeaders.end())
				{
					auto it = _reqHeaders.find("Content-Disposition");
					std::string contentDisposition = it->second;
					size_t namePos = contentDisposition.find("filename=");
					if (namePos != std::string::npos)
					{
						size_t start = contentDisposition.find("\"", namePos);
						size_t end = contentDisposition.find("\"", start + 1);
						fileName = contentDisposition.substr(start + 1, end - start - 1);
					}
				}
				std::string path = uploadPath + fileName;

				if (fileName == "default")
				{
					fileName = findFreeName(activeFolder, path);
					path = fileName;
				}
				try {
					activeFolder.findEntry(path);
				}
				catch (std::exception)
				{
					activeFolder.createEntry(path, _reqBody);
					this->_parent->_fileSystem->_pendingEntries.emplace(path, _reqBody);
					this->_parent->_fileSystem->_numPendingEntries++;
					infoCode = 201;
					return;
				}
				throw std::runtime_error("409");
			}
			catch (std::exception &e) {
				throw std::runtime_error(e.what());
			}

		}
		else {
			throw std::runtime_error("403");
		}
	}
	else {
		throw std::runtime_error("404");
	}
}

void	HttpResponse::doDelete()
{
	std::pair<std::string, int> entry = this->_parent->_fileSystem->findWithPath(_reqPath);
	if (entry.second == IS_ACCESSIBLE_DIRECTORY || entry.second == IS_DENIED_DIRECTORY || entry.second == IS_FILE)
	{
		if (entry.second == IS_ACCESSIBLE_DIRECTORY)
		{
			Folder &folder = this->_parent->_fileSystem->getFolder(_reqPath);
			if (folder._isRedirected)
			{
				_reqPath = folder._redirectPath;
				infoCode = 307;
				return;
			}
			else {
				throw std::runtime_error("403");
			}
		}
		else if (entry.second == IS_DENIED_DIRECTORY) {
			throw std::runtime_error("403");
		}
		else if (entry.second == IS_FILE)
		{
			Folder &folder = this->_parent->_fileSystem->getFileParentFolder(_reqPath);
			if (!folder._deleteAllowed)
				throw std::runtime_error("403");
			folder.deleteEntry(_reqPath);
			this->_parent->_fileSystem->_pendingDeletes.emplace(_reqPath, "");
			this->_parent->_fileSystem->_numPendingDeletes++;
			infoCode = 204;
		}
		else {
			throw std::runtime_error("403");
		}
	}
	else {
		throw std::runtime_error("404");
	}
}

char **HttpResponse::createArgv()
{
	std::string fileName = _reqPath.substr(_reqPath.find_last_of('/') + 1);
	char **ret;
	if (_reqMethod == "POST")
	{
		ret = new char*[4];

		std::string interpreter = INTERPRETER_PYTHON;
		ret[0] = new char[interpreter.size() + 1];
		std::strcpy(ret[0], interpreter.c_str());
		ret[1] = new char[fileName.size() + 1];
		std::strcpy(ret[1], fileName.c_str());
		ret[2] = new char[_reqBody.size() + 1];
		std::strcpy(ret[2], _reqBody.c_str());
		ret[3] = NULL;
	}
	else
	{
		ret = new char*[3];

		std::string interpreter = INTERPRETER_PYTHON;
		ret[0] = new char[interpreter.size() + 1];
		std::strcpy(ret[0], interpreter.c_str());
		ret[1] = new char[fileName.size() + 1];
		std::strcpy(ret[1], fileName.c_str());
		ret[2] = NULL;
	}
	return ret;
}

char **HttpResponse::createEnvs()
{
	std::map<std::string, std::string> envs;

	auto it = _reqHeaders.find("Content-Length");
	if (it != _reqHeaders.end())
		envs["CONTENT_LENGTH"] = it->second;
	else
		envs["CONTENT_LENGTH"] = "0";

	auto it2 = _reqHeaders.find("Content-Type");
	if (it2 != _reqHeaders.end())
		envs["CONTENT_TYPE"] = it2->second;
	else
		envs["CONTENT_TYPE"] = "text/plain";

	envs["AUTH_TYPE"] = "basic";
	envs["REDIRECT_STATUS"] = "200";
	envs["GATEWAY_INTERFACE"] = "CGI/1.1";
	envs["SCRIPT_NAME"] = _reqPath;
	envs["REQUEST_METHOD"] = _reqMethod;
	envs["SERVER_NAME"] = this->_parent->getValue("serverName");
	envs["SERVER_PORT"] = std::to_string(this->_parent->getSocket());
	envs["SERVER_PROTOCOL"] = "HTTP/1.1";
	envs["SERVER_SOFTWARE"] = "webserv404";

	char **ret = new char*[envs.size() + 1];
	ret[envs.size()] = NULL;
	int i	= 0;
	for (auto& iter : envs)
	{
		std::string entry = iter.first + "=" + iter.second;
		ret[i] = new char[entry.size() + 1];
		std::strcpy(ret[i], entry.c_str());
		i++;
	}
	return ret;
}

void	HttpResponse::doCgi()
{
	Folder &folder = this->_parent->_fileSystem->getFileParentFolder(_reqPath);
	if (folder._isRedirected)
	{
		_reqPath = folder._redirectPath;
		infoCode = 307;
		return;
	}
	int fds[2];
	if (pipe(fds) == -1)
		throw std::runtime_error("500");
	_readFd = fds[0];
	int pid	= fork();
	if (pid == -1)
	{
		close(fds[0]);
		close(fds[1]);
		throw std::runtime_error("500");
	}
	if (pid == 0)
	{
		std::string pathWithoutEndPoint = _reqPath.substr(0, _reqPath.find_last_of('/'));
		pathWithoutEndPoint += "/";
		if (chdir(("." + pathWithoutEndPoint).c_str()) != 0)
			std::exit(EXIT_FAILURE);
		if (close(fds[0]) == -1 || dup2(fds[1], STDOUT_FILENO) == -1 || close(fds[1]) == -1)
			std::exit(EXIT_FAILURE);
		char **argv = createArgv();
		char **envs = createEnvs();
		if (execve(INTERPRETER_PYTHON, argv, envs) == -1)
		{
			delete[] argv;
			delete[] envs;
			std::exit(EXIT_FAILURE);
		}
		delete[] argv;
		delete[] envs;
		std::exit(EXIT_SUCCESS);
	}
	else
	{
		_childPid = pid;
		close(fds[1]);
		_time = std::chrono::steady_clock::now();
		_runningProcess = true;
	}
}

bool HttpResponse::completeMe(int status)
{
	if (status == EXIT_SUCCESS)
	{
		char buffer[4096];
		ssize_t bytesRead;
		while ((bytesRead = read(_readFd, buffer, 4096)) > 0)
		{
			if (bytesRead == -1)
			{
				infoCode = 500;
				this->_body = findErrorPage(this->infoCode);
				break;
			}
			else if (bytesRead == 0)
			{
				infoCode = 200;
				break;
			}
			_body += std::string(buffer, bytesRead);
		}
	}
	else {
		infoCode = 500;
		this->_body = findErrorPage(this->infoCode);
	}
	close(_readFd);
	parseResponse();
	return true;
}

std::string	HttpResponse::findPhrase(int code)
{
	const std::map<int, std::string> phrases = {
		{100, "Continue"},
		{101, "Switching Protocols"},
		{102, "Processing"},
		{200, "OK"},
		{201, "Created"},
		{202, "Accepted"},
		{203, "Non-Authoritative Information"},
		{204, "No Content"},
		{205, "Reset Content"},
		{206, "Partial Content"},
		{300, "Multiple Choices"},
		{301, "Moved Permanently"},
		{302, "Found"},
		{303, "See Other"},
		{304, "Not Modified"},
		{305, "Use Proxy"},
		{307, "Temporary Redirect"},
		{308, "Permanent Redirect"},
		{400, "Bad Request"},
		{401, "Unauthorized"},
		{402, "Payment Required"},
		{403, "Forbidden"},
		{404, "Not Found"},
		{405, "Method Not Allowed"},
		{406, "Not Acceptable"},
		{407, "Proxy Authentication Required"},
		{408, "Request Timeout"},
		{409, "Conflict"},
		{410, "Gone"},
		{411, "Length Required"},
		{412, "Precondition Failed"},
		{413, "Payload Too Large"},
		{414, "URI Too Long"},
		{415, "Unsupported Media Type"},
		{416, "Range Not Satisfiable"},
		{417, "Expectation Failed"},
		{418, "I'm a teapot"},
		{421, "Misdirected Request"},
		{422, "Unprocessable Entity"},
		{423, "Locked"},
		{424, "Failed Dependency"},
		{425, "Unordered Collection"},
		{426, "Upgrade Required"},
		{428, "Precondition Required"},
		{429, "Too Many Requests"},
		{431, "Request Header Fields Too Large"},
		{451, "Unavailable For Legal Reasons"},
		{500, "Internal Server Error"},
		{501, "Not Implemented"},
		{502, "Bad Gateway"},
		{503, "Service Unavailable"},
		{504, "Gateway Timeout"},
		{505, "HTTP Version Not Supported"},
		{506, "Variant Also Negotiates"},
		{507, "Insufficient Storage"},
		{508, "Loop Detected"},
		{510, "Not Extended"},
		{511, "Network Authentication Required"}
	};
	std::map<int, std::string>::const_iterator it = phrases.find(code);
	if (it != phrases.end()) {
		return it->second;
	}
	else {
		return "Unknown Status Code";
	}
}

bool	HttpResponse::isCgi()
{
	std::string pathToCheck = _reqPath;
	size_t dotPos = pathToCheck.find_last_of('.');
	if (dotPos != std::string::npos)
	{
		std::string fileExtension = pathToCheck.substr(dotPos + 1);
		std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);
		if (fileExtension != "py") // if want more cgi. add extensions here
			return false;
	}
	std::pair<std::string, int> entry = this->_parent->_fileSystem->findWithPath(_reqPath);
	if (entry.second == IS_FILE)
	{
		Folder &folder = this->_parent->_fileSystem->getFileParentFolder(_reqPath);
		if (folder._cgiAllowed) {
			return true;
		}
		else {
			return false;
		}
	}
	return false;
}

std::string	HttpResponse::getContentType()
{
	std::string pathToCheck = _reqPath;

	size_t dotPos = pathToCheck.find_last_of('.');

	if (dotPos != std::string::npos)
	{
		std::string fileExtension = pathToCheck.substr(dotPos + 1);

		std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);

		const std::map<std::string, std::string> contentTypes = {
			{"html", "text/html"},
			{"htm", "text/html"},
			{"txt", "text/plain"},
			{"jpg", "image/jpeg"},
			{"jpeg", "image/jpeg"},
			{"png", "image/png"},
			{"gif", "image/gif"},
		};

		std::map<std::string, std::string>::const_iterator it = contentTypes.find(fileExtension);
		if (it != contentTypes.end()) {
			return it->second; // Return the mapped content type
		}
		else
		{
			std::string contentLine = _body.substr(0, _body.find("\n"));
			std::transform(contentLine.begin(), contentLine.end(), contentLine.begin(), ::tolower);
			if (contentLine.find("content-type:") != std::string::npos)
			{
				size_t start = contentLine.find(":") + 1;
				size_t end = contentLine.size();
				std::string contentType = contentLine.substr(start, end - start);
				if (contentType[0] == ' ')
					contentType = contentType.substr(contentType.find_first_not_of(' '));
				return contentType;
			}
			return "application/octet-stream"; // Default for unknown types
		}
	}
	else {
		return "application/octet-stream";
	}
}

std::string	HttpResponse::getContentLength()
{
	try {
		return std::to_string(this->_body.size());
	} catch (std::exception &e) {
		return ("0");
	}
}

std::string	HttpResponse::getConnection()
{
	return ("Keep-Alive");
}

bool	HttpResponse::hasBeenSent()
{
	return isSent;
}

void	HttpResponse::parseResponse()
{
	if (infoCode == 307 || infoCode == 308) //parse redirection
	{
		/*status line*/
		this->_status = "HTTP/1.1 ";
		this->_status += std::to_string(infoCode);
		this->_status += " ";
		this->_status += this->findPhrase(infoCode);
		this->_status += "\r\n";

		this->_headers = "Location:";
		this->_headers += "http://localhost:" + std::to_string(this->_parent->getPort()) + _reqPath;
		this->_headers += "\r\n";

		this->_headers += "Connection:";
		this->_headers += getConnection();
		this->_headers += "\r\n\r\n";
		_response = _status + _headers;
		imReady = true;
		return;
	}
	/*status line*/
	this->_status = "HTTP/1.1 ";
	this->_status += std::to_string(infoCode);
	this->_status += " ";
	this->_status += this->findPhrase(infoCode);
	this->_status += "\r\n";

	/*headers: content-type, cntent-length, Connection*/
	this->_headers = "Content-Type:";
	this->_headers += getContentType();
	this->_headers += "\r\n";

	this->_headers += "Content-Length:";
	this->_headers += getContentLength();
	this->_headers += "\r\n";

	this->_headers += "Connection:";
	this->_headers += getConnection();
	this->_headers += "\r\n\r\n";
	
	/*Note: body is parsed in executeCmd*/
	std::string	combined = _status + _headers + _body;
	_response = combined;
	imReady = true;
}

void	HttpResponse::executeCmd()
{
	try {
		handleRooted();
		if (_reqHeaders.find("InternalServerErrorHeader") != _reqHeaders.end())
		{
			auto it = _reqHeaders.find("InternalServerErrorHeader");
			throw std::runtime_error(it->second);
		}
		else if ((_reqMethod == "GET" || _reqMethod == "POST") && isCgi())
		{
			doCgi();
		}
		else if (_reqMethod == "GET")
		{
			doGet();
		}
		else if (_reqMethod == "POST")
		{
			doPost();
		}
		else if (_reqMethod == "DELETE")
		{
			doDelete();
		}
		else
		{
			throw std::runtime_error("405");
		}
	}
	catch (std::exception &e)
	{
		this->infoCode = std::stoi(e.what());
		this->_body = findErrorPage(this->infoCode);
	}
}

bool	HttpResponse::isReady()
{
	if (imReady)
		return (true);
	return (false);
}

void HttpResponse::sendResponse()
{
	ssize_t bytesSent;
	size_t bytesRemaining;

	bytesRemaining = _response.size();
	if (bytesRemaining > 0)
	{
		bytesSent = send(_cs, _response.c_str(), bytesRemaining, 0);
		if (bytesSent == -1)
		{
			throw std::runtime_error("Error sending response");
		}
		bytesRemaining -= bytesSent;
		if (!bytesRemaining || bytesSent == 0)
		{
			isSent = true;
			return ;
		}
		else
		{
			_response = _response.substr(bytesSent, _response.size());
		}
	}
}
