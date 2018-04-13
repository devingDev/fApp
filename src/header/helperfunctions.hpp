#ifndef HELPERFUNCTIONS_H
#define HELPERFUNCTIONS_H

#include <string>
#include <iostream>     // std::cout, std::endl
#include <iomanip>      // std::setfill, std::setw
#include <sstream>      // std::stringstream
#include <vector>

#include <map>

#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h> 
#include <psp2/io/stat.h> 
 


std::string md5hash(const std::string & filePath);
std::string basename( std::string const& pathname );
std::string md5HashString( std::string const& dataMsg);
std::string sha512HashString( std::string const&  dataMsg);

int checkFileExist(const char *file);
int checkFolderExist(const char *folder);
bool isDir(std::string const& fullPath);

std::vector <std::string> getPathParts(std::string const& fullPath);
std::string createDir(std::string path);
std::string creatDirs(std::string fullPath , std::string prependDir , bool lastOfFullPathIsFile );



std::string xorString( const std::string & key , const std::string & message);


#endif