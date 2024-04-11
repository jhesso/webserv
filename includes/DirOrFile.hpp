#ifndef DIRORFILE_HPP
# define DIRORFILE_HPP

#include <map>
#include <iostream>
#include <string>
#include "FileSystem.hpp"

/*if this is a directory, _dirContent has stuff in it and the bool _imDir is true. if this is
a file, the bool _imDir is false and _fileContent has stuff in it and the map is empty*/

class DirOrFile
{
	private:
	/*directory specific*/
		bool _imDirectory;
	/*file specific*/
		std::string		_fileContent;

	public:
		bool	amIDir();
		std::string getFileContent();
		DirOrFile(bool imADir, std::string name, std::string path);
		DirOrFile(const DirOrFile& other);
		DirOrFile(bool imADir, std::string content);
		~DirOrFile();
};

#endif
