#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <map>

class HttpRequest
{
	public:
		HttpRequest(std::string buffer, int cs, long long maxBodySize);
		HttpRequest(const HttpRequest& other);
		HttpRequest(unsigned long error_code, int cs);
		~HttpRequest(void);
		void printAttributes();

		std::string getMethod();
		std::string getPath();
		std::string	getBody();
		int			getCs();
		std::map<std::string, std::string>	getHeaders();
		std::string	getHeaderValue(std::string key);
		
		bool		seeIfComplete();

		void		parseCurrentBuffer(std::string buffer);
		bool		compareCs(int cs);

		bool		isForThisServer(std::string &host, std::vector<std::string> &otherHosts);

		std::string getHost();
	private:
		bool		hasBoundary;
		bool		hasLeftOverBuffer;

		int	_cs;

		bool	_cmdReceived;
		bool	_headersReceived;
		bool	_bodyReceived;

		std::string method;
		std::string	path;
		std::string	protocol;

		std::map<std::string, std::string> headers;
		
		std::string body;

		std::string _boundary;
		std::string _leftOverBuffer;

		bool	hasContentLengthHeader();
		bool	compareHeaderAndBody();
		std::pair<std::string, std::string> parseHeaderPair(std::string line);
		
		void	handleMultipartFormData();
		bool	contentShortEnough();

		long long _maxBodySize;

		void processChunkedBody(std::vector<std::string> &lines);

		bool _isChunked;

		bool _previousChunkMissingContent;
		unsigned long _unReceviedChunkSize;
		int _removedBodySize;

		std::vector<std::string> splitPipelines(std::string buffer);
};

#endif
