#include "header/helperfunctions.hpp"
#include <openssl/md5.h>
#include <algorithm>
#include <openssl/sha.h>



std::string md5hash(const std::string & filePath) {
	SceUID fileHandle = sceIoOpen(filePath.c_str(), SCE_O_RDONLY, 0777);
	if (fileHandle < 0) {
		// couldn't open file
		sceIoClose(fileHandle);
		return "DEADBEEFDEADBEEFDEADBEEFDEADBEEF";
	}

	int n;
	MD5_CTX c;
	char buf[512];
	ssize_t bytes;
	unsigned char out[MD5_DIGEST_LENGTH];

	MD5_Init(&c);
	bytes = sceIoRead(fileHandle, buf, 512);
	while (bytes > 0)
	{
		MD5_Update(&c, buf, bytes);
		bytes = sceIoRead(fileHandle, buf, 512);
	}
	sceIoClose(fileHandle);
	MD5_Final(out, &c);

	std::stringstream strStream;
	strStream << std::hex << std::setfill('0');
	for (n = 0; n<MD5_DIGEST_LENGTH; n++)
		strStream << std::setw(2) << static_cast<unsigned>(out[n]);
	std::string hexString = strStream.str();

	return hexString;

}

std::string sha512HashString( std::string const&  dataMsg){
	
	int dataLength = dataMsg.length();
	const char * dataArr = dataMsg.c_str();
	
	unsigned char digest[SHA512_DIGEST_LENGTH];
	SHA512_CTX ctx;
    SHA512_Init(&ctx);
    SHA512_Update(&ctx, dataArr, dataLength);
    SHA512_Final(digest, &ctx);
	
	std::stringstream strStream;
	strStream << std::hex << std::setfill('0');
	for (int n = 0; n < SHA512_DIGEST_LENGTH; n++)
		strStream << std::setw(2) << static_cast<unsigned>(digest[n]);
	std::string hexString = strStream.str();

	return hexString;
}

std::string md5HashString( std::string const& dataMsg ){

	int dataLength = dataMsg.length();
	const char * dataArr = dataMsg.c_str();

	unsigned char digest[MD5_DIGEST_LENGTH]; // 16
	MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, dataArr, dataLength);
    MD5_Final(digest, &ctx);
	
	std::stringstream strStream;
	strStream << std::hex << std::setfill('0');
	for (int n = 0; n < MD5_DIGEST_LENGTH; n++)
		strStream << std::setw(2) << static_cast<unsigned>(digest[n]);
	std::string hexString = strStream.str();

	return hexString;
	
}

struct MatchPathSeparator
{
    bool operator()( char ch ) const
    {
        return ch == '/';
    }
};
std::string basename( std::string const& pathname )
{
    return std::string( 
        std::find_if( pathname.rbegin(), pathname.rend(),
                      MatchPathSeparator() ).base(),
        pathname.end() );
}


int checkFileExist(const char *file) {
	SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0);
	if (fd < 0)
		return 0;

	sceIoClose(fd);
		return 1;
}

int checkFolderExist(const char *folder) {
	SceUID dfd = sceIoDopen(folder);
	if (dfd < 0)
		return 0;

	sceIoDclose(dfd);
	return 1;
}

bool isDir(std::string const& fullPath){
	SceIoStat fstat;
	sceIoGetstat(fullPath.c_str() , &fstat);
	
	return SCE_SO_ISDIR( fstat.st_mode );
	
}

bool checkIsValidPathChar(char c){
	if(c == '/' || c <= 31)
		return false;
	return true;
}

std::vector <std::string> getPathParts(std::string const& fullPath){
	std::vector <std::string> parts;
	std::string s = "";
	for(int i = 0; i < fullPath.length() ; i++){
		if(fullPath.at(i) == '/' || fullPath.at(i) == '\\' || fullPath.at(i) == ':' ){
			parts.push_back(s);
			s = "";
		}else if(checkIsValidPathChar(fullPath.at(i))){
			s += fullPath.at(i);
		}
	}
	if(s.length() > 0){
		parts.push_back(s);
	}
	return parts;
}

std::string createDir(std::string path){
	if(!checkFolderExist(path.c_str()) && !checkFileExist(path.c_str())){
		sceIoMkdir(path.c_str(), 0777);
	}
	return path;
}

std::string creatDirs(std::string fullPath , std::string prependDir , bool lastOfFullPathIsFile ){
	std::vector <std::string> parts = getPathParts(fullPath);
	std::string path = "";
	bool prepended = false;
	std::string endDir = "";
	if(prependDir.length() > 0){
		prepended = true;
		path += prependDir ;
	}else{
		path = parts[0] + ":" ;
	}
	int i = 0;
	
	if(prepended){
		i=0;
	}else{
		i=1;
	}
	
	int loopTo = 0;
	if(lastOfFullPathIsFile){
		loopTo = parts.size() -1;
	}else{
		loopTo = parts.size();
	}
	
	for( ; i < loopTo ; i++){
		if(parts[i].length() > 0){
			if(i > 1 || prepended){
				path += "/";
			}
			
			path += parts[i];
			
			if(!checkFolderExist(path.c_str()) && !checkFileExist(path.c_str())){
				sceIoMkdir(path.c_str(), 0777);
			}
		}
		
	}
	endDir = path + "/";
	
	if(lastOfFullPathIsFile){
		endDir += parts[parts.size() - 1];
	}
	
	return endDir;
}

std::string xorString( const std::string & key , const std::string & message)
{
	std::string enc = message;
    int i;
	int messageLength = message.length();
    int keyLength = key.length();
    for( i = 0 ; i < messageLength ; i++ )
    {
        enc.at(i) = message.at(i)^key.at(i%keyLength);
    }
	return enc;
}



