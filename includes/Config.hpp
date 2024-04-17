#ifndef CONFIG_HPP
# define CONFIG_HPP

#include <iostream>
#include <string>
#include <sstream>
#include <cctype>
#include <vector>
#include <map>

class Config
{
	public:
		Config(std::string configData); // read config upon construction // the { } check comes in main
		~Config(void);

		int	getListenPort();
		std::string	getHost();
		std::string getValue(std::string what);
		std::map<std::string, std::string> &getDirectives(){return _serverDirectives;};
		std::vector<std::pair<std::string, std::map<std::string, std::string>>> getLocations();
	private:
		int	_port;
		std::string _host;

		std::vector<std::pair<std::string, std::map<std::string, std::string>>>	_locations;
		std::map<std::string, std::string>	_serverDirectives;

		std::string trimLeadingSpaces(const std::string& input);
		std::string trimTrailingSpaces(const std::string& input);
};

std::vector<Config> readAndSetConfigs(const char* pathToConfig);

#endif
