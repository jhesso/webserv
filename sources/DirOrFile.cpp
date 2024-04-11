#include "../includes/DirOrFile.hpp"

/******************************************************************************/
/*						CONSTRUCTORS & DESTRUCTORS							  */
/******************************************************************************/

DirOrFile::DirOrFile(bool imADir, std::string content)
{
	_imDirectory = imADir;
	_fileContent = content;
}

DirOrFile::DirOrFile(bool imADir, std::string name, std::string path):
	_imDirectory(imADir)
{
	if (!imADir)
	{
		std::ifstream file("." + path + name);
		if (!file.is_open())
		{
			std::cout << "Error opening file: " << path + name << ". File wont be included in the servers filesystem" << std::endl;
			throw std::runtime_error("Error opening file");
		}
		std::string content;
		_fileContent = "";
		while (std::getline(file, content))
		{
			_fileContent += content + "\n";
		}
		file.close();
	}	
}

DirOrFile::DirOrFile(const DirOrFile& other):
    _imDirectory(other._imDirectory),
    _fileContent(other._fileContent) 
{
}

DirOrFile::~DirOrFile(void)
{
}

/******************************************************************************/
/*							PUBLIC FUNCTIONS								  */
/******************************************************************************/

bool DirOrFile::amIDir()
{
	return (_imDirectory);
}

std::string DirOrFile::getFileContent()
{
	return (_fileContent);
}
