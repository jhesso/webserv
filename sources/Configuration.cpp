#include "../includes/Configuration.hpp"

/******************************************************************************/
/*						CONSTRUCTORS & DESTRUCTORS							  */
/******************************************************************************/

Configuration::Configuration(std::string configData)
{
	std::string line;
	std::istringstream configStream(configData);

	std::string locatioonName;
	std::map<std::string, std::string> locationDirectives;

	while (std::getline(configStream, line))
	{
		line = trimLeadingSpaces(line);
		line = trimTrailingSpaces(line);

		if (line.empty() || line[0] == '#')
			continue;
		if (line.substr(0, line.find_first_of(" \t")) == "location")
		{
			locatioonName = line.substr(line.find_first_of(" \t") + 1);
			locatioonName = locatioonName.substr(0, locatioonName.find_first_of(" \t"));
			std::getline(configStream, line);
			line = trimLeadingSpaces(line);
			line = trimTrailingSpaces(line);
			if (line != "{")
				throw std::runtime_error("Error: location block must start with {");
			while (std::getline(configStream, line))
			{
				line = trimLeadingSpaces(line);
				line = trimTrailingSpaces(line);
				if (line.empty() || line[0] == '#')
					continue;
				if (line[0] == '{')
					throw std::runtime_error("Error: location block format issue");
				if (line == "}")
				{
					_locations.push_back(std::make_pair(locatioonName, locationDirectives));
					locationDirectives.clear();
					break;
				}
				std::string key = line.substr(0, line.find_first_of(" \t"));
				std::string value = line.substr(line.find_first_of(" \t") + 1);
				for (int i = value.size() - 1; i > 0; i--)
				{
					if (value[i] == ' ' || value[i] == '\t' || value[i] == ';')
						value.pop_back();
					else
						break;
				}
				if (key.find("POST") != std::string::npos || key.find("GET") != std::string::npos || key.find("DELETE") != std::string::npos)
				{
					if (key.find("POST") != std::string::npos)
						locationDirectives["POST"] = "";
					else if (key.find("GET") != std::string::npos)
						locationDirectives["GET"] = "";
					else if (key.find("DELETE") != std::string::npos)
						locationDirectives["DELETE"] = "";

					if (value.find("POST") != std::string::npos && key.find("POST") == std::string::npos)
						locationDirectives["POST"] = "";
					if (value.find("GET") != std::string::npos && key.find("GET") == std::string::npos)	
						locationDirectives["GET"] = "";
					if (value.find("DELETE") != std::string::npos && key.find("DELETE") == std::string::npos)
						locationDirectives["DELETE"] = "";
				}
				else
				{
					locationDirectives[key] = value;
				}
			}
			continue;
		}
		if (line[0] == '{' || line[0] == '}')
			throw std::runtime_error("Error: server block format issue");
		else
		{
			std::string key = line.substr(0, line.find_first_of(" \t"));
			std::string value = line.substr(line.find_first_of(" \t") + 1);
			value = trimLeadingSpaces(value);
			for (int i = value.size() - 1; i > 0; i--)
			{
				if (value[i] == ' ' || value[i] == '\t' || value[i] == ';')
					value.pop_back();
				else
					break;
			}
			if (_serverDirectives.find(key) == _serverDirectives.end() && key != "error_page")
			{
				_serverDirectives[key] = value;
			}
			else if (key == "listen" || key == "host" || key == "server_name" || key == "root" || key == "index" || key == "client_max_body_size")
			{
				std::string message = "Error: multiple " + key + " directives";
				throw std::runtime_error(message);
			}
			else if (key == "error_page")
			{
				std::string errorNum = value.substr(0, value.find_first_of(" \t"));
				for (unsigned long i = 0; i < errorNum.size(); i++)
				{
					if (!std::isdigit(errorNum[i]))
					{
						throw std::runtime_error("Error: error code not a digit"); 
					}
				}
				key = key + " " + errorNum;
				_serverDirectives[key] = value;
			}
			else
			{
				_serverDirectives[key] = value;
			}
				
		}
	}
	auto it = _serverDirectives.find("listen");
	if (it != _serverDirectives.end())
	{
		_port = std::stoi(it->second);
	}
	else
		throw std::runtime_error("Error: no listen directive found");
	auto it2 = _serverDirectives.find("host");
	if (it2 != _serverDirectives.end())
	{
		_host = it2->second;
	}
	else
		throw std::runtime_error("Error: no host directive found");
}

Configuration::~Configuration(void)
{
}

/******************************************************************************/
/*							PRIVATE FUNCTIONS								  */
/******************************************************************************/

std::string Configuration::trimLeadingSpaces(const std::string& input)
{
    size_t startPos = input.find_first_not_of(" \t");
    if (startPos == std::string::npos)
	{
        return "";
    }
    return input.substr(startPos);
}

std::string Configuration::trimTrailingSpaces(const std::string& input)
{
	size_t endPos = input.find_last_not_of(" \t");
	if (endPos == std::string::npos)
	{
		return "";
	}
	return input.substr(0, endPos + 1);
}

/******************************************************************************/
/*							PUBLIC FUNCTIONS								  */
/******************************************************************************/

void Configuration::printMyVals()
{
	/*print all server directives. then print all locations*/
	std::cout << "##server directives##" << std::endl;
	for (auto& entry : _serverDirectives)
	{
		std::cout << entry.first << " : " << entry.second << std::endl;
	}
	std::cout << "\n##locations##" << std::endl;
	for (auto& location : _locations)
	{
		std::cout << "\nlocation: " << location.first << std::endl;
		for (auto& entry : location.second)
		{
			std::cout << entry.first << " : " << entry.second << std::endl;
		}
	}
}

std::string	Configuration::getValue(std::string what)
{
	auto it = _serverDirectives.find(what);
	if (it != _serverDirectives.end())
	{
		return it->second;
	}
	return "";
}

std::vector<std::pair<std::string, std::map<std::string, std::string>>> Configuration::getLocations()
{
	return this->_locations;
}

int	Configuration::getListenPort()
{
	return _port;
}

std::string	Configuration::getHost()
{
	return _host;
}