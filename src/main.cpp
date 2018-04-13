#define VITASDK

#include <psp2/sysmodule.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/display.h>

#include <psp2/libssl.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/net/http.h>

#include <psp2/io/fcntl.h>

#include <stdio.h>
#include <malloc.h> 
#include <string>
#include <vector>
#include <cstring>


#include "header/json.hpp"
#include "header/rapidxml.hpp"
#include "header/rapidxml_utils.hpp"
#include "header/Rule34XXXPage.hpp"
#include "header/Rule34XXXImage.hpp"


#include "header/VitaIME.hpp"
#include "header/controls.hpp"
#include "header/UserInterface.hpp"
#include "header/Logger.hpp"

#define MAX_XML_SIZE  2097152

//#include "header/common/debugScreen.h"
int psvDebugScreenPrintf(const char *format, ...){
	return 0;
}

extern "C" {
	int _newlib_heap_size_user = 128 * 1024 * 1024;
}

using namespace rapidxml;

char * lastSiteRequestContent;
std::vector<Rule34XXXPage> rule34XXXPages;

void LoadNewPageRule34XXXXml(char * url);
void ParseRule34XXXXml(char * xmlData);
void ParseDanbooruJson(char * jsonData);

void netInit() {
	psvDebugScreenPrintf("Loading module SCE_SYSMODULE_NET\n");
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	
	psvDebugScreenPrintf("Running sceNetInit\n");
	SceNetInitParam netInitParam;
	int size = 8*1024*1024;
	netInitParam.memory = malloc(size);
	netInitParam.size = size;
	netInitParam.flags = 0;
	sceNetInit(&netInitParam);

	psvDebugScreenPrintf("Running sceNetCtlInit\n");
	sceNetCtlInit();
}

void netTerm() {
	psvDebugScreenPrintf("Running sceNetCtlTerm\n");
	sceNetCtlTerm();

	psvDebugScreenPrintf("Running sceNetTerm\n");
	sceNetTerm();

	psvDebugScreenPrintf("Unloading module SCE_SYSMODULE_NET\n");
	sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
}

void httpInit() {
	psvDebugScreenPrintf("Loading module SCE_SYSMODULE_HTTP\n");
	sceSysmoduleLoadModule(SCE_SYSMODULE_HTTP);

    sceSysmoduleLoadModule(SCE_SYSMODULE_SSL);
    sceSslInit(8*1024 * 1024);
	psvDebugScreenPrintf("Running sceHttpInit\n");
	sceHttpInit(8*1024*1024);
	 
	sceHttpsDisableOption(SCE_HTTPS_FLAG_SERVER_VERIFY);
}

void httpTerm() {
	psvDebugScreenPrintf("Running sceHttpTerm\n");
	sceHttpTerm();

    sceSysmoduleUnloadModule(SCE_SYSMODULE_SSL);
	psvDebugScreenPrintf("Unloading module SCE_SYSMODULE_HTTP\n");
	sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTP);
}

#define 	RESOLVETIMEOUT   (1 * 1000 * 1000U)
#define 	CONNECTTIMEOUT   (15* 1000 * 1000U)
#define 	SENDTIMEOUT   (15* 1000 * 1000U)
#define 	RECVTIMEOUT   (15* 1000 * 1000U)




struct DownloadHelper{
	std::string url;
	UserInterface * ui;
	char * dataDest;
};

VitaIME vitaIME;
UserInterface * userInterface;
SceUID lockDownloadThread;
SceUID lockIsDownloadingBool;
std::string pagenum  = "0";
std::string tags = "";
std::string api = "";
bool failNum = false;
int loadedTimes = 0;
int currentPage = 0;
int currentAPI = 0; // 0 = danbooru , 1 = rule34
bool isDownloading = false;

std::string danbooruSite = "https://danbooru.donmai.us/posts.json?limit=10";
std::string rule34site = "https://rule34.xxx/index.php?page=dapi&s=post&q=index&limit=10";
std::string siteBaseUrl = "";
DownloadHelper dlHelper;
using json = nlohmann::json;
json danbooruJSON;

unsigned long downloadString(const char *url , char * dest) {
	//debugNetPrintf(INFO,"\n\nDownloading %s s\n", url);

	// Create template with user agend "PS Vita Sample App"
	int tpl = sceHttpCreateTemplate("Mozilla/5.0 (iPhone; CPU iPhone OS 10_3 like Mac OS X) AppleWebKit/602.1.50 (KHTML, like Gecko) CriOS/56.0.2924.75 Mobile/14E5239e Safari/602.1"
										, SCE_HTTP_VERSION_1_1, SCE_TRUE);
	//debugNetPrintf(INFO,"0x%08X sceHttpCreateTemplate\n", tpl);

	sceHttpSetResolveTimeOut (tpl, RESOLVETIMEOUT);
	sceHttpSetConnectTimeOut (tpl, CONNECTTIMEOUT);
	sceHttpSetSendTimeOut (tpl, SENDTIMEOUT);
	sceHttpSetRecvTimeOut (tpl, RECVTIMEOUT);
	
	// set url on the template
	int conn = sceHttpCreateConnectionWithURL(tpl, url, SCE_FALSE);
	//debugNetPrintf(INFO,"0x%08X sceHttpCreateConnectionWithURL\n", conn);

	// create the request with the correct method
	int request = sceHttpCreateRequestWithURL(conn, SCE_HTTP_METHOD_GET, url, 0);
	//debugNetPrintf(INFO,"0x%08X sceHttpCreateRequestWithURL\n", request);

	// send the actual request. Second parameter would be POST data, third would be length of it.
	int handle = sceHttpSendRequest(request, NULL, 0);
	//debugNetPrintf(INFO,"0x%08X sceHttpSendRequest\n", handle);
	
	if(handle < 0){
		debugNetPrintf(ERROR,"Fail in downloadString %s . ERROR CODE : 0x%08X sceHttpSendRequest  \r\n", url, handle);
		return 0;
	}

	unsigned long read = 0;
	read = sceHttpReadData(request, dest, MAX_IMAGE_SIZE );
	//dest[read+1] = 0;
	
	// open destination file
	//////int fh = sceIoOpen(dest, SCE_O_WRONLY | SCE_O_CREAT, 0777);
	//////psvDebugScreenPrintf("0x%08X sceIoOpen\n", fh);
    //////
	//////// create buffer and counter for read bytes.
	//////unsigned char data[16*1024];
	//////int read = 0;
    //////
	//////// read data until finished
	//////while ((read = sceHttpReadData(request, &data, sizeof(data))) > 0) {
	//////	psvDebugScreenPrintf("read %d bytes\n", read);
    //////
	//////	// writing the count of read bytes from the data buffer to the file
	//////	int write = sceIoWrite(fh, data, read);
	//////	psvDebugScreenPrintf("wrote %d bytes\n", write);
	//////}
    //////
	//////// close file
	//////sceIoClose(fh);
	//////psvDebugScreenPrintf("sceIoClose\n");
	
	sceHttpDeleteRequest (request);
	sceHttpDeleteConnection (conn);
	sceHttpDeleteTemplate (tpl);
	psvDebugScreenPrintf("\n\n");
	return read;
}
bool LoadNewPage(const char * url) {

	//int fileHandle = sceIoOpen("app0:assets/demo.xml", SCE_O_RDONLY, 0777);
	//psvDebugScreenPrintf("filehandle : %d \n " , fileHandle);
	//sceKernelDelayThread(2 * 1000 * 1000);
	//	SceIoStat uploadFileStat;
	//	sceIoGetstatByFd (fileHandle, &uploadFileStat);
	//psvDebugScreenPrintf("getting filesize\n\n");
	//long fileSize = sceIoLseek(fileHandle, 0 , SCE_SEEK_END);
	//psvDebugScreenPrintf("filesize : %ld\n\n" , fileSize);
	//psvDebugScreenPrintf("seeking back\n\n");
	//sceIoLseek(fileHandle, 0 , SCE_SEEK_SET);
	//
	//
	//psvDebugScreenPrintf("calling new char\n\n");
	//lastSiteRequestContent = new char[fileSize + 1]; // 'new' crashes ..
	////lastSiteRequestContent = (char*)malloc( sizeof(char) * ( fileSize + 1 ) ); //new char[fileSize + 1]; // 'new' crashes ..
	//psvDebugScreenPrintf("sceIoReading\n\n");
	//sceIoRead(fileHandle, lastSiteRequestContent, fileSize);
	//psvDebugScreenPrintf("null terminating\n\n");
	//lastSiteRequestContent[fileSize] = 0;
	//psvDebugScreenPrintf("sceioclose\n\n");
	//sceIoClose(fileHandle);
	//
	//psvDebugScreenPrintf("r3 contn\n\n");
	////lastSiteRequestContent = std::string(buffer.str());
	//psvDebugScreenPrintf("%s\n\n" , lastSiteRequestContent);
	//sceKernelDelayThread(2 * 1000 * 1000);
	
	//char * lastSiteRequestContent = new char[MAX_XML_SIZE + 1];
	memset(lastSiteRequestContent, 0, sizeof(char)*(MAX_XML_SIZE + 1));
	Logger::Info("Calling download\r\n");
	unsigned long downloadedBytes = downloadString(url , lastSiteRequestContent);
	lastSiteRequestContent[downloadedBytes] = 0;
	debugNetPrintf(INFO,"Got data : %s\r\n"  , lastSiteRequestContent);
	
	if(downloadedBytes > 16){ // some random value to check against if json or xml has actually data in it
		int hand = sceIoOpen("ux0:data/fApp/dbgxml.txt", SCE_O_WRONLY | SCE_O_CREAT, 0777);
		sceIoWrite(hand , lastSiteRequestContent , strlen(lastSiteRequestContent));
		sceIoClose(hand);
		
		//sceKernelDelayThread(2*1000*1000);
		Logger::Info("Calling parseRule\r\n");
		if(currentAPI == 0){
			ParseDanbooruJson(lastSiteRequestContent);
		}else if(currentAPI == 1){
			ParseRule34XXXXml(lastSiteRequestContent);
		}
		
		//delete[] lastSiteRequestContent;
		return true;
	}
	return false;
}


void ParseDanbooruJson(char * jsonData){
	danbooruJSON = json::parse(jsonData);
	
	Rule34XXXPage rPage;
	
	if( !danbooruJSON.is_null() ){
		int postCount = danbooruJSON.size();
		
		std::string fileUrl = "";
		int fileWidth = 0;
		int fileHeight = 0;
		std::string previewUrl = "";
		int previewWidth = 0;
		int previewHeight = 0;
		std::string md5S = "";
		
		std::string httpsString = "https://";
		
		for(int i = 0; i < postCount ; i++){
			if( !danbooruJSON[i].is_null() ){
				fileUrl = "";
				previewUrl = "";
				previewWidth = 0;
				previewHeight = 0;
				fileHeight = 0;
				fileWidth = 0;
				md5S = "";
				if( !danbooruJSON[i]["file_url"].is_null() ){
					std::string file_tmp_url = danbooruJSON[i]["file_url"].get<std::string>();
					
					if(strncmp(file_tmp_url.c_str() , httpsString.c_str() , httpsString.length()) == 0){
						fileUrl = file_tmp_url;
					debugNetPrintf(ERROR , " file_url contained 'https://'  : %s \r\n\r\n " , danbooruJSON[i]["file_url"].get<std::string>().c_str());
					}else{
						fileUrl = "https://danbooru.donmai.us" + file_tmp_url;
					}
				}
				if( !danbooruJSON[i]["preview_file_url"].is_null() ){
					previewUrl = "https://danbooru.donmai.us" + danbooruJSON[i]["preview_file_url"].get<std::string>();
				}
				if( !danbooruJSON[i]["image_width"].is_null() ){
					fileWidth = danbooruJSON[i]["image_width"].get<int>();
				}
				if( !danbooruJSON[i]["image_height"].is_null() ){
					fileHeight = danbooruJSON[i]["image_height"].get<int>();
				}
				if( !danbooruJSON[i]["md5"].is_null() ){
					md5S = danbooruJSON[i]["md5"].get<std::string>();
				}
				Rule34XXXImage rImage( fileUrl, fileWidth, fileHeight, previewUrl, previewWidth, previewHeight , md5S );
				rPage.images.push_back(rImage);
			}
			
			
		}
	}
	
	rule34XXXPages.push_back(rPage);
	
	
}


void ParseRule34XXXXml(char * xmlData) {

	psvDebugScreenPrintf("doc\n\n");
	xml_document<> doc;
	psvDebugScreenPrintf("docparse\n\n");
	doc.parse<0>(xmlData);

	psvDebugScreenPrintf("xml doc.firstnode\n\n");
	xml_node<> *pRoot = doc.first_node();

	
	psvDebugScreenPrintf("pusback page\n\n");
	Rule34XXXPage rPage;

	psvDebugScreenPrintf("parse xml node\n\n");
	for (xml_node<> *pNode = pRoot->first_node("post"); pNode; pNode = pNode->next_sibling())
	{
		// This loop will walk you through two nodes:
		//node attribute = "0" and then node attribute = "1"
		// Do something here
		psvDebugScreenPrintf("get attribs\n\n");
		xml_attribute<> *pAttrFile = pNode->first_attribute("file_url");
		std::string fileUrl = pAttrFile->value();
		pAttrFile = pNode->first_attribute("width");
		int fileWidth = atoi(pAttrFile->value());
		pAttrFile = pNode->first_attribute("height");
		int fileHeight = atoi(pAttrFile->value());
		pAttrFile = pNode->first_attribute("preview_url");
		std::string previewUrl = pAttrFile->value();
		pAttrFile = pNode->first_attribute("preview_width");
		int previewWidth = atoi(pAttrFile->value());
		pAttrFile = pNode->first_attribute("preview_height");
		int previewHeight = atoi(pAttrFile->value());
		pAttrFile = pNode->first_attribute("md5");
		std::string md5S = pAttrFile->value();
		
		psvDebugScreenPrintf("push back new image\n\n");

		Rule34XXXImage rImage(fileUrl, fileWidth, fileHeight, previewUrl, previewWidth, previewHeight , md5S);
		rPage.images.push_back(rImage);

	}
	
	rule34XXXPages.push_back(rPage);

	//free(lastSiteRequestContent);

}

static int StartDownloadPreviewThread(unsigned int args, void* argp){
	sceKernelLockMutex(lockDownloadThread , 1  , NULL );
	
	int timeoutPageLoad = 10 * 1000 * 1000;
	
	if(LoadNewPage(dlHelper.url.c_str())){
		int currentIndex = loadedTimes;
		loadedTimes++;
		for (int i = 0; i < rule34XXXPages.at(currentIndex).images.size(); i++) {
			debugNetPrintf(INFO,"%d : %s\n", i , rule34XXXPages.at(currentIndex).images.at(i).previewurl.c_str() );
			debugNetPrintf(DEBUG, "Checking for emptry threads ( current running : %d ) \r\n" , userInterface->threadsRunning);
			int timeoutInMicroSeconds = 5 * 1000 * 1000;
			while(userInterface->threadsRunning >= MAX_DOWNLOAD_THREADS && timeoutInMicroSeconds > 0){	
				sceKernelDelayThread(100*1000);
				timeoutInMicroSeconds -= 100*1000;
				timeoutPageLoad -= 100*1000 ;
			}
			if(timeoutInMicroSeconds <= 0){
				continue;
			}else if(timeoutPageLoad <= 0){
				sceKernelLockMutex(lockIsDownloadingBool , 1  , NULL );
				isDownloading = false;
				sceKernelUnlockMutex ( lockIsDownloadingBool, 1 );
				return -1;
			}else{
				userInterface->AddPreviewImageRule34XXX(rule34XXXPages.at(currentIndex).images.at(i) );
				timeoutPageLoad = 10 * 1000 * 1000;
			}
			
			
			timeoutPageLoad -= 100*1000;
			sceKernelDelayThread(100*1000);
		}
	}else{
		currentPage--;
		if(currentPage <= 0){
			if(currentAPI == 0){
				currentPage = 1;
			}else if(currentAPI == 0){
				currentPage = 0;
			}
			
		}
	}
	sceKernelUnlockMutex ( lockDownloadThread, 1 );
	sceKernelLockMutex(lockIsDownloadingBool , 1  , NULL );
	isDownloading = false;
	sceKernelUnlockMutex ( lockIsDownloadingBool, 1 );
	return 0;
}
void StartDownloadPreviews(std::string url){
	sceKernelLockMutex(lockDownloadThread , 1  , NULL );
	dlHelper.url = url;
	dlHelper.ui = userInterface;
	dlHelper.dataDest = lastSiteRequestContent;
	sceKernelUnlockMutex ( lockDownloadThread, 1 );
	SceUID threadID = sceKernelCreateThread ("dlthread", &StartDownloadPreviewThread, 0x40, 1024*1024, 0 , 0 , NULL);
	debugNetPrintf(DEBUG, "Start thread\r\n");
	sceKernelStartThread	(	threadID, 0, NULL );	
}


int main(int argc, char *argv[]) {
	//psvDebugScreenInit();
	Logger::Setup();
	Logger::Info(" ====   fApp  v1   ==== \n\n");
	SetupControls();
	// Initialize
	lockDownloadThread = sceKernelCreateMutex ("maindownloadz", 0 , 0 , 0);
	lockIsDownloadingBool = sceKernelCreateMutex ("maindlisdl", 0 , 0 , 0);
	lastSiteRequestContent = new char[MAX_XML_SIZE + 1]; 
	netInit();
	httpInit();
	std::function<unsigned long(const char * , char *)> downloadStringFunc = downloadString;
	userInterface = new UserInterface(downloadStringFunc);
	
	tags = vitaIME.getUserText("Tags" , "");
	pagenum = vitaIME.getUserText("Which page number" , "0");
	api = vitaIME.getUserText("0 = danbooru / 1 = rule34.xxx" , "0");
	
	failNum = false;
	for ( int i = 0; i < pagenum.length() ; i++ ){
		if(!isdigit(pagenum[i])){
			failNum = true;
		}
	}
	if(!failNum){
		currentPage = atoi(pagenum.c_str());
	}
	
	failNum = false;
	for ( int i = 0; i < api.length() ; i++ ){
		if(!isdigit(api[i])){
			failNum = true;
		}
	}
	if(!failNum){
		currentAPI = atoi(api.c_str());
	}
	
	
	if(currentAPI == 0){
		if(currentPage <= 0){
			currentPage = 1;
		}
		siteBaseUrl = danbooruSite + "&tags=" + tags + "&page=" + std::to_string(currentPage);
	}else if(currentAPI == 1){
		if(currentPage <= 0){
			currentPage = 0;
		}
		siteBaseUrl = rule34site + "&tags=" + tags + "&pid=" + std::to_string(currentPage) ;
	}
	//std::string downloadUrl = "https://rule34.xxx/index.php?page=dapi&s=post&q=index&limit=20&tags=" + tags + "&pid=" + std::to_string(currentPage);

	StartDownloadPreviews(siteBaseUrl);
	
	
	int deltaTimeMicroSecond = 1*1000;
	int waitTimeInput = 100*1000;
	int zoomWaitTime = 1000;
	int deltaTimeSecond = (deltaTimeMicroSecond/1000)/1000;
	if(deltaTimeSecond <= 0) deltaTimeSecond = 1;
	int scrollAmount = 5000;
	int scrollSpeedTime = (int)((float)scrollAmount / deltaTimeMicroSecond);
	
	while(true){
		userInterface->Check();
		userInterface->Draw();
		ReadControls();
		if(PressedUp()){
			userInterface->Scroll(0,scrollSpeedTime);
		}
		if(PressedDown()){
			userInterface->Scroll(0,-scrollSpeedTime);
		}
		if(PressedRight()){
			//userInterface->MoveToImage(1);
			userInterface->Scroll(scrollSpeedTime,0);
		}
		if(PressedLeft()){
			//userInterface->MoveToImage(-1);
			userInterface->Scroll(-scrollSpeedTime,0);
		}
		if(PressedCross()){
			userInterface->ShowImage();
			sceKernelDelayThread(waitTimeInput);
		}else if(PressedCircle()){
			userInterface->UnshowImage();
			sceKernelDelayThread(waitTimeInput);
		}else if(PressedStart()){
			if(userInterface->isViewingImage){
				
			}else{
				sceKernelLockMutex(lockIsDownloadingBool , 1  , NULL );
				if(isDownloading == false){
					isDownloading = true;
					sceKernelUnlockMutex ( lockIsDownloadingBool, 1 );
					tags = vitaIME.getUserText("Tags" , tags.c_str());
					pagenum = vitaIME.getUserText("Which page number" , "0");
					api = vitaIME.getUserText("0 = danbooru / 1 = rule34.xxx" , "0");
					failNum = false;
					for ( int i = 0; i < pagenum.length( ) ; i++ ){
						if(!isdigit(pagenum[i])){
							failNum = true;
						}
					}
					if(!failNum){
						currentPage = atoi(pagenum.c_str());
					}
					
					failNum = false;
					for ( int i = 0; i < api.length() ; i++ ){
						if(!isdigit(api[i])){
							failNum = true;
						}
					}
					if(!failNum){
						currentAPI = atoi(api.c_str());
					}
					if(currentAPI == 0){
						if(currentPage <= 0){
							currentPage = 1;
						}
						siteBaseUrl = danbooruSite + "&tags=" + tags + "&page=" + std::to_string(currentPage);
					}else if(currentAPI == 1){
						if(currentPage <= 0){
							currentPage = 0;
						}
						siteBaseUrl = rule34site + "&tags=" + tags + "&pid=" + std::to_string(currentPage) ;
					}
					StartDownloadPreviews(siteBaseUrl);
					sceKernelDelayThread(500 * 1000);
				}else{
					//sceKernelUnlockMutex ( lockIsDownloadingBool, 1 );
				}
			}
			
			sceKernelDelayThread(waitTimeInput);
		}else if(PressedR1()){
			if(userInterface->isViewingImage){
				userInterface->Zoom(+1);
				sceKernelDelayThread(zoomWaitTime);
			}else{
				sceKernelLockMutex(lockIsDownloadingBool , 1  , NULL );
				if(isDownloading == false){
					isDownloading = true;
					sceKernelUnlockMutex ( lockIsDownloadingBool, 1 );
					currentPage++;
					if(currentAPI == 0){
						if(currentPage <= 0){
							currentPage = 1;
						}
						siteBaseUrl = danbooruSite + "&tags=" + tags + "&page=" + std::to_string(currentPage);
					}else if(currentAPI == 1){
						if(currentPage <= 0){
							currentPage = 0;
						}
						siteBaseUrl = rule34site + "&tags=" + tags + "&pid=" + std::to_string(currentPage) ;
					}
					StartDownloadPreviews(siteBaseUrl);
				}else{
					sceKernelUnlockMutex ( lockIsDownloadingBool, 1 );
				}
				sceKernelDelayThread(500 * 1000);
			}
		}else if(PressedL1()){
			if(userInterface->isViewingImage){
				userInterface->Zoom(-1);
				sceKernelDelayThread(zoomWaitTime);
			}else{
				
				sceKernelDelayThread(waitTimeInput);
			}
		}
		
		sceKernelDelayThread(deltaTimeMicroSecond);
	}
	
	
	psvDebugScreenPrintf("Call Load new page\n\n");
	
	
	




	// Cleanup
	httpTerm();
	netTerm();
	delete[] lastSiteRequestContent;
	
	psvDebugScreenPrintf("This app will close in 10 seconds!\n");
	sceKernelDelayThread(10*1000*1000);
	sceKernelExitProcess(0);
	return 0;
}
