#include "HttpRequest.hpp"

std::vector<std::string> splitLines(const std::string& input)
{
	std::vector<std::string> lines;
	size_t start = 0;
	size_t end;

	while ((end = input.find("\r\n", start)) != std::string::npos) {
		lines.push_back(input.substr(start, end - start));
		start = end + 2;
	}
	// Handle the last line (including CRLF if present)
	if (start < input.size())
		lines.push_back(input.substr(start));
	return lines;
}

std::vector<std::string> splitToTokens(const std::string& input, char delimiter)
{
	std::vector<std::string> tokens;
	std::istringstream stream(input);
	std::string token;

	while (std::getline(stream, token, delimiter))
		tokens.push_back(token);
	return tokens;
}

std::string trimWhitespace(const std::string& str)
{
	size_t first = str.find_first_not_of(" \t\r\n");
	if (std::string::npos == first)
		return "";
	return str;
}

std::string removeTrailing(const std::string& str)
{
	std::string endBoundary = str.substr(0);
	while (endBoundary[endBoundary.size() - 1] == '\n' || endBoundary[endBoundary.size() - 1] == '\r')
		endBoundary = endBoundary.substr(0, endBoundary.size() - 1);
	return endBoundary;
}

HttpRequest::~HttpRequest()
{
	headers.clear();
}

HttpRequest::HttpRequest(std::string buffer, int cs, long long maxBodySize) :
	hasBoundary(false),
	hasLeftOverBuffer(false),
	_cs(cs),
	_cmdReceived(false),
	_headersReceived(false),
	_bodyReceived(false),
	body(""),
	_maxBodySize(maxBodySize),
	_isChunked(false),
	_previousChunkMissingContent(false),
	_unReceviedChunkSize(0),
	_removedBodySize(0)
{
	this->_cs = cs;
	try {
		this->parseCurrentBuffer(buffer);
	}
	catch (std::exception &e) {
		std::string error = e.what();
		throw std::runtime_error(error);
	}
}

HttpRequest::HttpRequest(const HttpRequest& other) :
		hasBoundary(other.hasBoundary),
		hasLeftOverBuffer(other.hasLeftOverBuffer),
		_cs(other._cs),
		_cmdReceived(other._cmdReceived),
		_headersReceived(other._headersReceived),
		_bodyReceived(other._bodyReceived),
		method(other.method),
		path(other.path),
		protocol(other.protocol),
		headers(other.headers),
		body(other.body),
		_boundary(other._boundary),
		_leftOverBuffer(other._leftOverBuffer),
		_maxBodySize(other._maxBodySize),
		_isChunked(other._isChunked),
		_previousChunkMissingContent(other._previousChunkMissingContent),
		_unReceviedChunkSize(other._unReceviedChunkSize),
		_removedBodySize(other._removedBodySize)
{}

HttpRequest::HttpRequest(unsigned long error_code, int cs) :
		hasBoundary(false),
		hasLeftOverBuffer(false),
		_cs(cs),
		_cmdReceived(true),
		_headersReceived(true),
		_bodyReceived(true),
		method(""),
		path(""),
		protocol(""),
		body(""),
		_boundary(""),
		_leftOverBuffer(""),
		_maxBodySize(0),
		_isChunked(false)
{
	std::string error = std::to_string(error_code);
	headers.insert(std::make_pair("InternalServerErrorHeader", error));
}

std::string HttpRequest::getHost()
{
	auto it = headers.find("Host");
	return it->second;
}

int	HttpRequest::getCs()
{
	return _cs;
}

std::pair<std::string, std::string> HttpRequest::parseHeaderPair(std::string line)
{
	size_t colonPos = line.find(':');

	if (colonPos != std::string::npos && colonPos > 0 && colonPos < line.length() - 1) {
		std::string key = line.substr(0, colonPos);
		std::string value = line.substr(colonPos + 2);

		return std::make_pair(key, value);
	}

	throw std::runtime_error("Error: Invalid header format");

}

bool HttpRequest::compareHeaderAndBody()
{
	if (headers.find("Content-Length") == headers.end())
		return false;
	size_t expectedBodyLength = std::stoul(headers["Content-Length"]) - this->_removedBodySize;
	return (this->body.size() >= expectedBodyLength);
}


bool	HttpRequest::hasContentLengthHeader()
{
	std::map<std::string, std::string>::iterator it;

	it = this->headers.find("Content-Length");
	if (it == this->headers.end())
		return (false);
	else
		return (true);
}

std::vector<std::string> HttpRequest::splitPipelines(std::string buffer)
{
	std::vector<std::string> lines;
	std::istringstream stream(buffer);
	std::string line;

	while (std::getline(stream, line))
		lines.push_back(line);
	return lines;
}

void HttpRequest::handleMultipartFormData()
{
	std::string endBoundary = "--" + _boundary + "--";
	size_t endBoundaryStartIndex = this->body.find(endBoundary);
	if (endBoundaryStartIndex == std::string::npos)
		throw std::exception();
	this->body = this->body.substr(0, endBoundaryStartIndex); //remove everything after the last boundary

	std::vector<std::string> parts = splitPipelines(this->body);
	std::string finalBody = "";

	while (!parts.empty()) {
		std::string trimmedPart = removeTrailing(parts[0]);
		std::string part = parts[0];
		if (trimmedPart.find(_boundary) != std::string::npos) {
			parts.erase(parts.begin());
			continue;
		}
		std::vector<std::string> tokens = splitToTokens(part, ':');
		if (tokens.size() == 2) {
			this->headers.insert(std::make_pair(tokens[0], tokens[1]));
			parts.erase(parts.begin());
			if (parts.size() > 0) {
				std::string nextPart = removeTrailing(parts[0]);
				if (nextPart.empty())
					parts.erase(parts.begin());
			}
			continue;
		}
		if (parts.size() > 1)
			finalBody += part + "\n";
		else
			finalBody += part;
		parts.erase(parts.begin());
	}
	this->body = finalBody;
}

bool	HttpRequest::contentShortEnough()
{
	if (auto it = this->headers.find("Content-Length"); it != this->headers.end()) {
		try {
			if (static_cast<long long>(this->body.size()) > _maxBodySize)
				return false;
			else if (std::stoll(it->second) > _maxBodySize)
				return false;
			return true;
		}
		catch (std::exception &e) {
			(void)e;
			return true;
		}
	}
	return true;
}

std::string	HttpRequest::getBody()
{
	return this->body;
}

std::map<std::string, std::string>	HttpRequest::getHeaders()
{
	return this->headers;
}

std::string	HttpRequest::getHeaderValue(std::string key)
{
	std::map<std::string, std::string>::iterator it;

	it = this->headers.find(key);
	if (it == this->headers.end())
		return ("");
	else
		return this->headers[key];
}

bool		HttpRequest::compareCs(int cs)
{
	if (cs == _cs)
		return (true);
	return (false);
}

bool HttpRequest::seeIfComplete()
{
	if (!_cmdReceived || !_headersReceived || !_bodyReceived)
		return (false);
	return (true);
}

void HttpRequest::processChunkedBody(std::vector<std::string> &lines)
{
	std::string chunkSizeStr;
	unsigned long chunkSize;
	bool isPartialChunk = false;

	if (_previousChunkMissingContent) {
		std::string chunkData = "";
		while (_unReceviedChunkSize > 0 && chunkData.size() < _unReceviedChunkSize && lines.size() > 0) {
			chunkData += lines[0];
			lines.erase(lines.begin());
		}
		this->body += chunkData;
		if (lines.size() > 0)
			lines.erase(lines.begin());
		if (chunkData.size() < _unReceviedChunkSize) {
			_previousChunkMissingContent = true;
			_unReceviedChunkSize = _unReceviedChunkSize - chunkData.size();
			return;
		}
		_previousChunkMissingContent = false;
	}

	while (!lines.empty()) {
		chunkSizeStr = lines[0];
		lines.erase(lines.begin());

		chunkSize = std::stoul(chunkSizeStr, nullptr, 16);
		std::string chunkData = "";
		while (chunkSize > 0 && chunkData.size() < chunkSize && lines.size() > 0) {
			chunkData += lines[0];
			lines.erase(lines.begin());
		}
		if (chunkData.size() < chunkSize) {
			isPartialChunk = true;
		}
		this->body += chunkData;
		if (lines.size() > 0)
			lines.erase(lines.begin());
		if (chunkSize == 0) {
			_bodyReceived = true;
			return;
		}
		if (isPartialChunk) {
			_previousChunkMissingContent = true;
			_unReceviedChunkSize = chunkSize - chunkData.size();
			return;
		}
	}
}

bool HttpRequest::isForThisServer(std::string &host, std::vector<std::string> &otherHosts)
{
	auto it = headers.find("Host");
	if (it != headers.end()) {
		if (it->second.find(host) != std::string::npos) {
			return true;
		}
		for (unsigned long i = 0; i < otherHosts.size(); i++) {
			if (it->second.find(otherHosts[i]) != std::string::npos)
				return false;
		}
	}
	return true;
}

void HttpRequest::parseCurrentBuffer(std::string buffer)
{
	std::vector<std::string> lines;
	lines = splitLines(buffer);
	if (hasLeftOverBuffer) {
		lines[0] = _leftOverBuffer + lines[0];
		_leftOverBuffer = "";
		hasLeftOverBuffer = false;
	}

	if (!lines.empty() && !_cmdReceived) {
		std::vector<std::string> tokens = splitToTokens(lines[0], ' ');

		if (tokens.size() >= 3) {
			this->method = tokens[0];
			this->path = tokens[1];
			this->protocol = tokens[2];
			_cmdReceived = true;
			lines.erase(lines.begin());
		}
		else
			throw std::runtime_error("400");
	}

	while (lines.size() > 0 && !_headersReceived && _cmdReceived)
	{
		std::string lineWithoutWhiteSpaces = trimWhitespace(lines[0]);
		if (lineWithoutWhiteSpaces.empty()) {
			_headersReceived = true;
			lines.erase(lines.begin());
			if (method != "POST") {
				_bodyReceived = true;
				return;
			}
			break;
		}
		try {
			std::pair<std::string, std::string> header = parseHeaderPair(lines[0]);
			this->headers.insert(header);
			lines.erase(lines.begin());
		}
		catch (std::exception &e) {
				_leftOverBuffer = lines[0]; // remove \n
				hasLeftOverBuffer = true;
				break;
		}
	}
	/*handle chunked*/
	if (!_isChunked && _headersReceived && method == "POST" && this->headers.count("Transfer-Encoding") > 0) {
		std::string transferEncoding = this->headers["Transfer-Encoding"];
		if (transferEncoding.find("chunked") != std::string::npos)
			_isChunked = true;
	}
	/*handle pipelining*/
	if (!_isChunked	&& _headersReceived && !hasBoundary && method == "POST" && this->headers.count("Content-Type") > 0) {
		std::string contentType = this->headers["Content-Type"];
		if (contentType.find("multipart/form-data") != std::string::npos) {
			size_t pos = contentType.find("boundary=");
			if (pos != std::string::npos) {
				size_t boundaryStartIndex = pos + strlen("boundary=");
				_boundary = contentType.substr(boundaryStartIndex);
				_boundary = removeTrailing(_boundary);
				hasBoundary = true;
			}
		}
	}

	if (_headersReceived && method == "POST" && lines.size() > 0 && !_bodyReceived) {
		std::string requestBody = "";
		if (_isChunked) {
			try {
				processChunkedBody(lines);
			}
			catch (std::exception &e) {
				throw std::runtime_error("400");
			}
		}
		else if (hasContentLengthHeader() && contentShortEnough()) {
			while (lines.size() > 0) {
				requestBody += lines[0] + "\n";
				lines.erase(lines.begin());
				_removedBodySize += 1; // for the \r at end of each line that got removed earlier
			}
			this->body += requestBody;
			if (contentShortEnough() && compareHeaderAndBody()) {
				_bodyReceived = true;
				if (hasBoundary) {
					try {
						handleMultipartFormData();
					}
					catch (std::exception &e) {
						throw std::runtime_error("400");
					}
				}
			}
			else if (contentShortEnough() && !hasBoundary)
				_bodyReceived = true;
			else if (!contentShortEnough())
				throw std::runtime_error("413");
			else
				this->body = this->body.substr(0, this->body.size() - 1);
		}
		else {
			if (!contentShortEnough())
				throw std::runtime_error("413");
			else
				throw std::runtime_error("400");
		}
	}
}

void HttpRequest::printAttributes()
{
	std::cout << "Method: " << method << std::endl;
	std::cout << "Path: " << path << std::endl;
	std::cout << "Protocol: " << protocol << std::endl;

	// Print headers
	std::cout << "Headers:" << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
		std::cout << it->first << ": " << it->second << std::endl;
	}

	// Print body (if not empty)
	if (!body.empty()) {
		std::cout << "Body: " << body << std::endl;
	}
}

std::string HttpRequest::getMethod()
{
	return this->method;
}

std::string HttpRequest::getPath()
{
	return this->path;
}
