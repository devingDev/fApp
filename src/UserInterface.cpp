#include "header/UserInterface.hpp"
#include "header/helperfunctions.hpp"
#include <cstring>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/kernel/threadmgr.h> 
#include <psp2/kernel/processmgr.h>
#include "header/Logger.hpp"





void UserInterface::AddPreviewImageRule34XXX(Rule34XXXImage r34xxxImage){
	
	
	AddPreviewImage(r34xxxImage.previewurl , r34xxxImage.previewwidth , r34xxxImage.previewheight ,
						r34xxxImage.fullurl , r34xxxImage.width , r34xxxImage.height , r34xxxImage.md5 );

	
}



struct ImageDownloadPack{
	std::string url;
	char * dataLocation;
};
struct AddImageEntryHelper{
	UserInterface * ui;
	ImageEntry iE;
};
struct AddImageThreadHelper{
	int threadIndex;
	UserInterface * ui;
};
static int StartAddPreviewImageThread(unsigned int args, void* argp){
	//AddImageEntryHelper * aiP = (AddImageEntryHelper*)argp;
	//aiP->ui->ThreadAddPreviewImage(aiP->iE);
	
	debugNetPrintf(DEBUG, "In StartAddPreviewImageThread\r\n");
	AddImageThreadHelper * ait = (AddImageThreadHelper*)argp;
	debugNetPrintf(DEBUG, "Passing threadindex to threaddd : %d \r\n" , ait->threadIndex);
	ait->ui->ThreadAddPreviewImage( ait->threadIndex);
	
}

bool Check_ext(const std::string& filename)
{
    size_t pos = filename.rfind('.');
    if (pos == std::string::npos)
        return false;

    std::string ext = filename.substr(pos+1);

    if (ext == "gif" )
        return true;

    return false;
}


void UserInterface::AddPreviewImage(std::string pUrl , int pWidth , int pHeight , std::string surl , int width , int height , std::string fullImageMD5S){
	
	ImageEntry iE;
	
	
	iE.previewurl = pUrl;
	iE.fullURL = surl;
	iE.pWidth = pWidth;
	iE.pHeight = pHeight;
	iE.width = width;
	iE.height = height;
	iE.fullImage = NULL;
	iE.previewImage = NULL;
	iE.fullImageLoaded  = false;
	iE.previewImageLoaded  = false;
	iE.fullImageMD5 = fullImageMD5S;
	iE.isAnimatedGif = Check_ext(surl);
	debugNetPrintf(DEBUG, "In AddPreviewImage,       pUrl now : %s \r\n", pUrl.c_str());
	debugNetPrintf(DEBUG, "In AddPreviewImage,    ie.pUrl now : %s \r\n", iE.previewurl.c_str());
	
	
	
	//AddImageEntryHelper aiHelper;
	//aiHelper.ui = this;
	//aiHelper.iE = iE;
	//debugNetPrintf(DEBUG, "In AddPreviewImage, ai.ie.pUrl now : %s \r\n", aiHelper.iE.previewurl.c_str());
	
	AddImageThreadHelper ait;
	ait.ui = this;
	

	
	debugNetPrintf(DEBUG, "Lock mutex threads\r\n");
	sceKernelLockMutex(imageEntriesLock , 1  , NULL );
	int threadIndex = 0;
	for(int i = 0 ; i < MAX_DOWNLOAD_THREADS ; i++){
		if(!threadEntriesHelper[i].inUse){
			threadIndex = i;
			
			break;
		}
	}
	
	sceKernelLockMutex(changeIntThreadsRunningLock , 1  , NULL );
	threadsRunning++;
	sceKernelUnlockMutex ( changeIntThreadsRunningLock, 1 );
	
	debugNetPrintf(DEBUG, "Found thread helper empty at %d \r\n" , threadIndex);
	threadEntriesHelper[threadIndex].inUse = true;
	debugNetPrintf(DEBUG, "Set iE\r\n");
	threadEntriesHelper[threadIndex].iE = iE;
	sceKernelUnlockMutex ( imageEntriesLock, 1 );
	ait.threadIndex = threadIndex;
	debugNetPrintf(DEBUG, "create thread\r\n");
	SceUID threadID = sceKernelCreateThread ("dlthread", &StartAddPreviewImageThread, 0x40, 1024*1024, 0 , 0 , NULL);
	debugNetPrintf(DEBUG, "Start thread id : %d \r\n" , threadID);
	sceKernelStartThread	(	threadID, sizeof(ait), &ait );	
	
}

int UserInterface::ThreadAddPreviewImage(int threadIndex){
	if(threadIndex >= MAX_DOWNLOAD_THREADS || threadIndex < 0){
		sceKernelLockMutex(changeIntThreadsRunningLock , 1  , NULL );
		threadsRunning--;
		debugNetPrintf(ERROR, "Remaining threads : %d \r\n" , threadsRunning);
		sceKernelUnlockMutex ( changeIntThreadsRunningLock, 1 );
		return -1;
	}
	
	char * imageData;
	debugNetPrintf(DEBUG, "assign iE th\r\n");
	
	
	ImageEntry iE;
	sceKernelLockMutex(changeIntThreadsRunningLock , 1  , NULL );
	iE = threadEntriesHelper[threadIndex].iE;
	sceKernelUnlockMutex ( changeIntThreadsRunningLock, 1 );
	debugNetPrintf(DEBUG, "lock mutext th\r\n");
	sceKernelLockMutex(allocateDataImageLock , 1  , NULL );
	debugNetPrintf(DEBUG, "In ThreadAddPreviewImage, threadsRunning now : %d \r\n", threadsRunning);
	debugNetPrintf(DEBUG, "In ThreadAddPreviewImage, currentURL now : %s \r\n", iE.previewurl.c_str());
	imageData  = new char[MAX_THUMBNAIL_SIZE  +  1];
	//memset(imageData, 0, sizeof(char)*(MAX_THUMBNAIL_SIZE + 1));
	sceKernelUnlockMutex ( allocateDataImageLock, 1 );
	
	
	debugNetPrintf(DEBUG, "Downloading image T: %d \r\n", threadIndex);
	unsigned long bytesDownloaded;

	std::string fileSaveName = "ux0:data/fApp/thumbnails/" + iE.fullImageMD5 + ".png";
	bool fileExist = false;
	if(checkFileExist(fileSaveName.c_str())){
		
		fileExist = true;
	}
	if(!fileExist){
		fileSaveName = "ux0:data/fApp/thumbnails/" + iE.fullImageMD5 + ".jpg";
		if(checkFileExist(fileSaveName.c_str())){
		
			fileExist = true;
		}
	}
	if(!fileExist){
		fileSaveName = "ux0:data/fApp/thumbnails/" + iE.fullImageMD5 + ".jpeg";
		if(checkFileExist(fileSaveName.c_str())){
		
			fileExist = true;
		}
	}
	if(!fileExist){
		fileSaveName = "ux0:data/fApp/thumbnails/" + iE.fullImageMD5 + ".bmp";
		if(checkFileExist(fileSaveName.c_str())){
		
			fileExist = true;
		}
	}
	if(!fileExist){
		fileSaveName = "ux0:data/fApp/thumbnails/" + iE.fullImageMD5 + ".gif";
		if(checkFileExist(fileSaveName.c_str())){
		
			fileExist = true;
		}
	}
	if(!fileExist){
		fileSaveName = "ux0:data/fApp/thumbnails/" + iE.fullImageMD5 + ".unk";
		if(checkFileExist(fileSaveName.c_str())){
		
			fileExist = true;
		}
	}
	
	if(fileExist){
		debugNetPrintf(DEBUG, "File found : %s \r\n " , fileSaveName.c_str());
		int fileHandle = sceIoOpen(fileSaveName.c_str() , SCE_O_RDONLY, 0777);
		debugNetPrintf(DEBUG, "File handle : %d \r\n " , fileHandle);

		long fileSize = sceIoLseek(fileHandle, 0 , SCE_SEEK_END);
		sceIoLseek(fileHandle, 0 , SCE_SEEK_SET);
		
		sceIoRead(fileHandle, imageData, fileSize);
		//imageData[fileSize] = 0;
		sceIoClose(fileHandle);
		
		bytesDownloaded = fileSize;
	}else{
		debugNetPrintf(DEBUG, "File NOT found : %s \r\n " , fileSaveName.c_str());
		bytesDownloaded = downloadImage(iE.previewurl.c_str() , imageData);
		fileSaveName = "ux0:data/fApp/debugthumbnails/" + iE.fullImageMD5 ;
		
			debugNetPrintf(DEBUG, "Writing debug preview to file ! \r\n ");
			SceUID fileHandle = sceIoOpen(fileSaveName.c_str(), SCE_O_WRONLY|SCE_O_CREAT, 0777);
			sceIoWrite(fileHandle , imageData , bytesDownloaded); 
			sceIoClose(fileHandle);
		
	}
	
	
				
	
	debugNetPrintf(DEBUG, "Parsing image T: %d \r\n", threadIndex);
	debugNetPrintf(DEBUG, "Image bytes: %d \r\n", bytesDownloaded);
	if(bytesDownloaded <= 0){
		debugNetPrintf(DEBUG, "Failed ! preview bytesDownloaded <= 0 \r\n ");
		iE.previewImage = NULL;
		iE.previewImageLoaded = false;
		iE.previewImageBytes = 0;
		//delete[] imageData;
	}else{
					bool isbmp = false;
					bool ispng = false;
					bool isjpg = false;
					bool isgif87a = false;
					bool isgif89a = false;
		iE.previewImageBytes = bytesDownloaded;
		if( imageData[0] == (char)0x42 ){
			if( imageData[1] == 0x4D ){
				sceKernelLockMutex(vita2dLoadImageLock , 1  , NULL );
				debugNetPrintf(DEBUG, "Loading image to vita2d_texture bmp \r\n ");
				iE.previewImage = vita2d_load_BMP_buffer( imageData );
				sceKernelUnlockMutex ( vita2dLoadImageLock, 1 );
				if( iE.previewImage != NULL){
					iE.previewImageLoaded = true;
					isbmp = true;
				}
			}
		} // now PNG :
		else if ( imageData[0] == (char)0x89 ){
			if ( imageData[1] == 0x50 ){
				if ( imageData[2] == 0x4E ){
					if ( imageData[3] == 0x47 ){
						if ( imageData[4] == 0x0D ){
							if ( imageData[5] == 0x0A ){
								if ( imageData[6] == 0x1A ){
									if ( imageData[7] == 0x0A ){
										sceKernelLockMutex(vita2dLoadImageLock , 1  , NULL );
										debugNetPrintf(DEBUG, "Loading image to vita2d_texture png \r\n ");
										iE.previewImage = vita2d_load_PNG_buffer( imageData );
										sceKernelUnlockMutex ( vita2dLoadImageLock, 1 );
										if( iE.previewImage != NULL){
											iE.previewImageLoaded = true;
											ispng = true;
										}
									}
								}
							}
						}
					}
				}
			}
		}// GIF87a and GIF89a7
		else if(imageData[0] == (char)0x47){
			if(imageData[1] == (char)0x49){
				if(imageData[2] == (char)0x46){
					if(imageData[3] == (char)0x38){
						if(imageData[4] == (char)0x37){
							if(imageData[5] == (char)0x61){
								isgif87a = true;
								debugNetPrintf(DEBUG, "image is gif87a \r\n ");
								
							}
						}else if(imageData[4] == (char)0x39){
							if(imageData[5] == (char)0x61){
								debugNetPrintf(DEBUG, "image is gif98a \r\n ");
								isgif89a = true;
							}
						}
					}
				}
			}
		}
		// now 3 different JPG magic numbers  [ raw | Exif | JFIF ]
		else if( imageData[0] == (char)0xFF ){
			if( imageData[1] == 0xD8 ){
				if( imageData[2] == 0xFF ){
					if( imageData[3] == 0xD8 ){
						// ÿØÿÛ
						sceKernelLockMutex(vita2dLoadImageLock , 1  , NULL );
						debugNetPrintf(DEBUG, "Loading image to vita2d_texture jpg 1 \r\n ");
						iE.previewImage = vita2d_load_JPEG_buffer( imageData , bytesDownloaded);
						debugNetPrintf(DEBUG, "Loaded image to vita2d_texture jpg 1 \r\n ");
						sceKernelUnlockMutex ( vita2dLoadImageLock, 1 );
						if( iE.previewImage != NULL){
							iE.previewImageLoaded = true;
							isjpg = true;
						}

					}
					else if( imageData[3] == (char)0xE0 ){
						if( imageData[6] == 0x4A ){
							if( imageData[7] == 0x46 ){
								if( imageData[8] == 0x49 ){
									if( imageData[9] == 0x46 ){
										if( imageData[10] == 0x00 ){
											if( imageData[11] == 0x01 ){
												//ÿØÿà ..JFIF..
												sceKernelLockMutex(vita2dLoadImageLock , 1  , NULL );
												debugNetPrintf(DEBUG, "Loading image to vita2d_texture jpg 2 \r\n ");
												iE.previewImage = vita2d_load_JPEG_buffer( imageData , bytesDownloaded );
												debugNetPrintf(DEBUG, "Loaded image to vita2d_texture jpg 2 \r\n ");
												sceKernelUnlockMutex ( vita2dLoadImageLock, 1 );
												if( iE.previewImage != NULL){
													iE.previewImageLoaded = true;
													isjpg = true;
												}
											}
										}
									}
								}
							}
						}


					}else if( imageData[3] == (char)0xE1 ){
						if( imageData[6] == 0x45 ){
							if( imageData[7] == 0x78 ){
								if( imageData[8] == 0x69 ){
									if( imageData[9] == 0x66 ){
										if( imageData[10] == 0x00 ){
											if( imageData[11] == 0x00 ){
												//ÿØÿá ..Exif..
												sceKernelLockMutex(vita2dLoadImageLock , 1  , NULL );
												debugNetPrintf(DEBUG, "Loading image to vita2d_texture jpg 3 \r\n ");
												iE.previewImage = vita2d_load_JPEG_buffer( imageData , bytesDownloaded);
												debugNetPrintf(DEBUG, "Loaded image to vita2d_texture jpg 3 \r\n ");
												sceKernelUnlockMutex ( vita2dLoadImageLock, 1 );
												if( iE.previewImage != NULL){
													iE.previewImageLoaded = true;
													isjpg = true;
												}
											}
										}
									}
								}
							}
						}


					}

				}

			}
		}
		debugNetPrintf(DEBUG, "About to save preview to file ! \r\n ");
				fileSaveName = "ux0:data/fApp/thumbnails/" + iE.fullImageMD5 ;
				if(ispng){
					debugNetPrintf(DEBUG, "is png \r\n");
					fileSaveName += ".png";
				}else if(isbmp){
					debugNetPrintf(DEBUG, "is bmp \r\n");
					fileSaveName += ".bmp";
				}else if(isjpg){
					debugNetPrintf(DEBUG, "is jpeg \r\n");
					fileSaveName += ".jpeg";
				}else if(isgif87a){
					debugNetPrintf(DEBUG, "is gif87a \r\n");
					fileSaveName += ".gif";
				}else if(isgif89a){
					debugNetPrintf(DEBUG, "is gif89a \r\n");
					fileSaveName += ".gif";
				}else{
					fileSaveName += ".unk";
					debugNetPrintf(DEBUG, "is unknown \r\n");
				}
				if(!fileExist && iE.previewImageLoaded){
					debugNetPrintf(DEBUG, "Writing preview to file ! \r\n ");
					SceUID fileHandle = sceIoOpen(fileSaveName.c_str(), SCE_O_WRONLY|SCE_O_CREAT, 0777);
					sceIoWrite(fileHandle , imageData , bytesDownloaded); 
					sceIoClose(fileHandle);
				} 
	}
	
	
	
	
	debugNetPrintf(DEBUG, "Lock entries mutex T: %d \r\n", threadIndex);
	sceKernelLockMutex(imageEntriesLock , 1  , NULL );
	debugNetPrintf(DEBUG, "Appending entries image T: %d \r\n", threadIndex);
	if(iE.previewImageLoaded){
		
		if(iE.previewImage != NULL && (iE.pWidth == 0 || iE.pHeight == 0) ){
			iE.pWidth  = vita2d_texture_get_width(  iE.previewImage  );
			iE.pHeight = vita2d_texture_get_height( iE.previewImage  );
		}
		
		if(nextPreviewX + iE.pWidth > 960){
			iE.previewX = 32;
			iE.previewY = currentPreviewY + lastRowMaxHeight + 24;
			currentPreviewY = currentPreviewY + lastRowMaxHeight + 24 ; // some padding
			lastRowMaxHeight = iE.pHeight;
			nextPreviewX =  iE.previewX + iE.pWidth + 32;
		}else{
			iE.previewX = nextPreviewX;
			if(iE.pHeight > lastRowMaxHeight){
				lastRowMaxHeight = iE.pHeight;
			}
			iE.previewY = currentPreviewY;
			nextPreviewX = iE.pWidth + nextPreviewX + 32;
		}
	
	
		estimatedTotalImageHeapUsage += iE.previewImageBytes;
		imageEntries.push_back((iE));
	}
	sceKernelUnlockMutex ( imageEntriesLock, 1 );
	debugNetPrintf(DEBUG, "Estimated heap usage from images : %ld  T: %d \r\n" , estimatedTotalImageHeapUsage , threadIndex);
	sceKernelLockMutex(allocateDataImageLock , 1  , NULL );
	delete[] imageData;
	sceKernelUnlockMutex ( allocateDataImageLock, 1 );
	sceKernelLockMutex(changeIntThreadsRunningLock , 1  , NULL );
	threadsRunning--;
	sceKernelUnlockMutex ( changeIntThreadsRunningLock, 1 );
	debugNetPrintf(DEBUG, "Remaining threads : %d \r\n" , threadsRunning);
	debugNetPrintf(DEBUG, "Exiting ThreadAddPreviewImage\r\n");
	sceKernelLockMutex(imageEntriesLock , 1  , NULL );
	threadEntriesHelper[threadIndex].inUse = false;
	sceKernelUnlockMutex ( imageEntriesLock, 1 );
	
	return 0;
	
}



UserInterface::UserInterface(std::function<unsigned long(const char * , char *)> downloadFunction){
	createDir("ux0:data/fApp/");
	createDir("ux0:data/fApp/thumbnails/");
	createDir("ux0:data/fApp/debugthumbnails/");
	
	downloadImage = downloadFunction;
	imageDataFull = new char[MAX_IMAGE_SIZE + 1];
	isViewingImage = false;
	isViewingGifImage = false;
	selectedImage = 0;
	fullScaleX = 1;
	fullScaleY = 1;
	scrollX = 0;
	scrollY = 0;
	nextPreviewX = 32;
	currentPreviewY = 24;
	lastRowMaxHeight = 0;
	lastRowMaxWidth = 0;
	estimatedTotalImageHeapUsage = 0;
	threadsRunning = 0;
	
	threadEntriesHelper = new ThreadImageEntryHelper[MAX_DOWNLOAD_THREADS];
	for(int i = 0; i < MAX_DOWNLOAD_THREADS;i++){
		threadEntriesHelper[i].inUse = false;
	}
	
	imageEntriesVectorLock = sceKernelCreateMutex ("entriesvectorl", 0 , 0 , 0);
	imageEntriesLock = sceKernelCreateMutex ("mydownloads", 0 , 0 , 0);
	vita2dLoadImageLock  = sceKernelCreateMutex ("vitatwodadd", 0 , 0 , 0);
	allocateDataImageLock  = sceKernelCreateMutex ("allocateimd", 0 , 0 , 0);
	changeIntThreadsRunningLock  = sceKernelCreateMutex ("changerunnint", 0 , 0 , 0);
	
	vita2d_init();
	//vita2d_set_clear_color(RGBA8(32, 34, 37, 0xFF));
	//vita2d_set_clear_color(RGBA8(0xFF , 0xFF , 0xFF , 0xFF)); 
	vita2d_set_clear_color(RGBA8(0x2F , 0x31 , 0x36 , 0xFF));
	
	backgroundTexture = vita2d_load_PNG_file("app0:assets/background.png");
	
	//vita2dFontBig = vita2d_load_font_file("app0:assets/font/whitney-book.ttf");
	//std::string bgPath = "app0:assets/images/Vitacord-Background-8BIT.png";
	//backgroundImage = vita2d_load_PNG_file(bgPath.c_str());
}

void UserInterface::ResetPreviews(){
	for(int i = 0; i < imageEntries.size() ; i++){
		if(imageEntries[i].previewImageLoaded){
			if(imageEntries[i].previewImage != NULL){
				vita2d_free_texture(imageEntries[i].previewImage);
			}
		}
	}
	UnshowImage();
	isViewingImage = false;
	selectedImage = 0;
	scrollX = 0;
	scrollY = 0;
	fullScaleX = 1;
	fullScaleY = 1;
	nextPreviewX = 0;
	currentPreviewY = 0;
	lastRowMaxHeight = 0;
	lastRowMaxWidth = 0;
	imageEntries.clear();
}



void UserInterface::ShowImage(){
	if(!isViewingImage){
		sceKernelLockMutex( imageEntriesVectorLock , 1 , NULL );
		if(selectedImage < imageEntries.size() && selectedImage >= 0){
			int i = selectedImage;
			if(imageEntries[i].fullImageLoaded){
				isViewingImage = true;
			}
			else{
				
				std::string fileSaveName = "ux0:data/fApp/" + imageEntries[i].fullImageMD5 + ".png";
				bool fileExist = false;
				if(checkFileExist(fileSaveName.c_str())){
					
					fileExist = true;
				}
				if(!fileExist){
					fileSaveName = "ux0:data/fApp/" + imageEntries[i].fullImageMD5 + ".jpg";
					if(checkFileExist(fileSaveName.c_str())){
					
						fileExist = true;
					}
				}
				if(!fileExist){
					fileSaveName = "ux0:data/fApp/" + imageEntries[i].fullImageMD5 + ".jpeg";
					if(checkFileExist(fileSaveName.c_str())){
					
						fileExist = true;
					}
				}
				if(!fileExist){
					fileSaveName = "ux0:data/fApp/" + imageEntries[i].fullImageMD5 + ".bmp";
					if(checkFileExist(fileSaveName.c_str())){
					
						fileExist = true;
					}
				}
				if(!fileExist){
					fileSaveName = "ux0:data/fApp/" + imageEntries[i].fullImageMD5 + ".gif";
					if(checkFileExist(fileSaveName.c_str())){
					
						fileExist = true;
					}
				}
				if(!fileExist){
					fileSaveName = "ux0:data/fApp/" + imageEntries[i].fullImageMD5 + ".unk";
					if(checkFileExist(fileSaveName.c_str())){
					
						fileExist = true;
					}
				}
				
				memset(imageDataFull, 0, sizeof(char)*(MAX_IMAGE_SIZE + 1));
				unsigned long bytesDownloaded = 0;
				if(fileExist){
					debugNetPrintf(DEBUG, "File found : %s \r\n " , fileSaveName.c_str());
					int fileHandle = sceIoOpen(fileSaveName.c_str() , SCE_O_RDONLY, 0777);
					debugNetPrintf(DEBUG, "File handle : %d \r\n " , fileHandle);
	
					long fileSize = sceIoLseek(fileHandle, 0 , SCE_SEEK_END);
					sceIoLseek(fileHandle, 0 , SCE_SEEK_SET);
					
					sceIoRead(fileHandle, imageDataFull, fileSize);
					//imageDataFull[fileSize] = 0;
					sceIoClose(fileHandle);
					
					bytesDownloaded = fileSize;
				}else{
					debugNetPrintf(DEBUG, "File NOT found : %s \r\n " , fileSaveName.c_str());
					bytesDownloaded = downloadImage(imageEntries[i].fullURL.c_str() , imageDataFull);
					
				}
				
				if(bytesDownloaded <= 0){
					debugNetPrintf(DEBUG, "Failed ! bytesDownloaded <= 0 \r\n ");
					imageEntries[i].fullImage = NULL;
					imageEntries[i].fullImageLoaded = false;
					imageEntries[i].fullImageBytes = 0;
				}else{
					debugNetPrintf(DEBUG, "Loaded ! ! bytesDownloaded > 0 \r\n ");
					imageEntries[i].fullImageBytes = bytesDownloaded;
					bool isbmp = false;
					bool ispng = false;
					bool isjpg = false;
					bool isgif87a = false;
					bool isgif89a = false;
					debugNetPrintf(DEBUG, "Find out what type \r\n ");
					if( imageDataFull[0] == (char)0x42 ){
						if( imageDataFull[1] == 0x4D ){
							imageEntries[i].fullImage = vita2d_load_JPEG_buffer( imageDataFull , bytesDownloaded);
							estimatedTotalImageHeapUsage += bytesDownloaded;
							if( imageEntries[i].fullImage != NULL){
								imageEntries[i].fullImageLoaded = true;
								isViewingImage = true;
								isbmp = true;
							}
						}
					} // now PNG :
					else if ( imageDataFull[0] == (char)0x89 ){
						if ( imageDataFull[1] == 0x50 ){
							if ( imageDataFull[2] == 0x4E ){
								if ( imageDataFull[3] == 0x47 ){
									if ( imageDataFull[4] == 0x0D ){
										if ( imageDataFull[5] == 0x0A ){
											if ( imageDataFull[6] == 0x1A ){
												if ( imageDataFull[7] == 0x0A ){
													imageEntries[i].fullImage = vita2d_load_PNG_buffer( imageDataFull );
													if( imageEntries[i].fullImage != NULL){
														imageEntries[i].fullImageLoaded = true;
														isViewingImage = true;
														ispng = true;
													}
												}
											}
										}
									}
								}
							}
						}
					}// GIF87a and GIF89a7
					else if(imageDataFull[0] == (char)0x47){
						if(imageDataFull[1] == (char)0x49){
							if(imageDataFull[2] == (char)0x46){
								if(imageDataFull[3] == (char)0x38){
									if(imageDataFull[4] == (char)0x37){
										if(imageDataFull[5] == (char)0x61){
											isgif87a = true;
											imageEntries[i].fullImage = vita2d_create_empty_texture( imageEntries[i].width , imageEntries[i].height );
											imageEntries[i].fullImageLoaded = true;
											isViewingImage = true;
											isViewingGifImage = true;
										}
									}else if(imageDataFull[4] == (char)0x39){
										if(imageDataFull[5] == (char)0x61){
											isgif89a = true;
											imageEntries[i].fullImage = vita2d_create_empty_texture( imageEntries[i].width , imageEntries[i].height );
											imageEntries[i].fullImageLoaded = true;
											isViewingImage = true;
											isViewingGifImage = true;
										}
									}
								}
							}
						}
					}
					// now 3 different JPG magic numbers  [ raw | Exif | JFIF ]
					else if( imageDataFull[0] == (char)0xFF ){
						if( imageDataFull[1] == 0xD8 ){
							if( imageDataFull[2] == 0xFF ){
								if( imageDataFull[3] == 0xD8 ){
									// ÿØÿÛ
									imageEntries[i].fullImage = vita2d_load_JPEG_buffer( imageDataFull , bytesDownloaded);
									if( imageEntries[i].fullImage != NULL){
										imageEntries[i].fullImageLoaded = true;
										isViewingImage = true;
										isjpg = true;
									}

								}
								else if( imageDataFull[3] == (char)0xE0 ){
									if( imageDataFull[6] == 0x4A ){
										if( imageDataFull[7] == 0x46 ){
											if( imageDataFull[8] == 0x49 ){
												if( imageDataFull[9] == 0x46 ){
													if( imageDataFull[10] == 0x00 ){
														if( imageDataFull[11] == 0x01 ){
															//ÿØÿà ..JFIF..
															imageEntries[i].fullImage = vita2d_load_JPEG_buffer( imageDataFull , bytesDownloaded);
															if( imageEntries[i].fullImage != NULL){
																imageEntries[i].fullImageLoaded = true;
																isViewingImage = true;
																isjpg = true;
															}
														}
													}
												}
											}
										}
									}


								}else if( imageDataFull[3] == (char)0xE1 ){
									if( imageDataFull[6] == 0x45 ){
										if( imageDataFull[7] == 0x78 ){
											if( imageDataFull[8] == 0x69 ){
												if( imageDataFull[9] == 0x66 ){
													if( imageDataFull[10] == 0x00 ){
														if( imageDataFull[11] == 0x00 ){
															//ÿØÿá ..Exif..
															imageEntries[i].fullImage = vita2d_load_JPEG_buffer( imageDataFull , bytesDownloaded);
															if( imageEntries[i].fullImage != NULL){
																imageEntries[i].fullImageLoaded = true;
																isViewingImage = true;
																isjpg = true;
															}
														}
													}
												}
											}
										}
									}


								}

							}

						}
					}
					
					fileSaveName = "ux0:data/fApp/" + imageEntries[i].fullImageMD5 ;
					if(ispng){
						debugNetPrintf(DEBUG, "is png \r\n");
						fileSaveName += ".png";
					}else if(isbmp){
						debugNetPrintf(DEBUG, "is bmp \r\n");
						fileSaveName += ".bmp";
					}else if(isjpg){
						debugNetPrintf(DEBUG, "is jpeg \r\n");
						fileSaveName += ".jpeg";
					}else if(isgif87a){
						debugNetPrintf(DEBUG, "is gif87a \r\n");
						fileSaveName += ".gif";
					}else if(isgif89a){
						debugNetPrintf(DEBUG, "is gif89a \r\n");
						fileSaveName += ".gif";
					}else{
						fileSaveName += ".unk";
						debugNetPrintf(DEBUG, "is unknown \r\n");
					}
					if(!fileExist && imageEntries[i].fullImageLoaded){
					
						
						SceUID fileHandle = sceIoOpen(fileSaveName.c_str(), SCE_O_WRONLY|SCE_O_CREAT, 0777);
						sceIoWrite(fileHandle , imageDataFull , bytesDownloaded); 
						sceIoClose(fileHandle);
					} 
					if(isgif87a || isgif89a){
						myGifDisplayer.ShowImage(fileSaveName , imageEntries[i].fullImage);
					}
					
					
				}
				
				if(imageEntries[i].fullImageLoaded ){
					estimatedTotalImageHeapUsage += imageEntries[i].fullImageBytes;
				}
			} // ENd of parsing image
			
			if(isViewingImage){
				float newPosX = 0;
				float newPosY = 0;
				float newSizeY = imageEntries[i].height;
				float newSizeX = imageEntries[i].width;
				if(newSizeY > 544){
					fullScaleY = 544 / newSizeY;
					fullScaleX = fullScaleY;
					newSizeY = newSizeY  * fullScaleY;
					newSizeX = newSizeX  * fullScaleX;
				}
				if(newSizeX > 960){
					fullScaleX = 960 / newSizeX;
					fullScaleY = fullScaleX;
					newSizeY = newSizeY  * fullScaleY;
					newSizeX = newSizeX  * fullScaleX;
				}
				
				newPosX = (960 / 2)  -  (newSizeX / 2);
				newPosY = (544 / 2)  -  (newSizeY / 2);
				
				imageEntries[i].fullX = newPosX;
				imageEntries[i].fullY = newPosY;
				
			}
				
			
		}
		sceKernelUnlockMutex ( imageEntriesVectorLock , 1 );
	}
	debugNetPrintf(DEBUG, "Estimated heap usage from images : %ld \r\n" , estimatedTotalImageHeapUsage);
}

void UserInterface::UnshowImage(){
	if(isViewingImage){
		if(isViewingGifImage == true){
			myGifDisplayer.UnshowCurrentImage();
			debugNetPrintf(DEBUG , "Unshown Gif\r\n");
		}
		
		sceKernelLockMutex( imageEntriesVectorLock , 1 , NULL );
		if(selectedImage >= 0 && selectedImage < imageEntries.size()){
			if(imageEntries[selectedImage].fullImage != NULL){
				vita2d_free_texture(imageEntries[selectedImage].fullImage);
				imageEntries[selectedImage].fullImageLoaded = false;
				estimatedTotalImageHeapUsage -= imageEntries[selectedImage].fullImageBytes;
				imageEntries[selectedImage].fullImageBytes = 0;
				debugNetPrintf(DEBUG, "Estimated heap usage from images : %ld \r\n" , estimatedTotalImageHeapUsage);
			}
		}
		sceKernelUnlockMutex(imageEntriesVectorLock , 1 );
		fullScrollX = 0;
		fullScrollY = 0;
		fullScaleX = 1;
		fullScaleY = 1;
		isViewingImage = false;
	}
}
int drawnThisFrame = 0;
void UserInterface::Draw(){
	vita2d_start_drawing();
	vita2d_clear_screen();
	
	if(backgroundTexture != NULL){
		vita2d_draw_texture(backgroundTexture , 0 ,0);
	}
	
	sceKernelLockMutex( imageEntriesVectorLock , 1 , NULL );
	if(!isViewingImage){
		drawnThisFrame = 0;
		for(int i = 0; i < imageEntries.size() ; i++){
			if(imageEntries[i].previewImageLoaded){
				if( (scrollY + imageEntries[i].previewY) <= 544 && (scrollY + imageEntries[i].previewY + imageEntries[i].pHeight) >= 0  ){
					if(imageEntries[i].previewImage != NULL){
						if(selectedImage == i){
							vita2d_draw_rectangle(scrollX + imageEntries[i].previewX - 8, scrollY + imageEntries[i].previewY - 8, imageEntries[i].pWidth + 16, imageEntries[i].pHeight + 19, 
														RGBA8(0xFF , 0xFF , 0xFF , 0xCF));
						}else{
							vita2d_draw_rectangle(scrollX + imageEntries[i].previewX - 8, scrollY + imageEntries[i].previewY - 8, imageEntries[i].pWidth + 16, imageEntries[i].pHeight + 16, 
														RGBA8(0xFF , 0xFF , 0xFF , 0x2F));
						}
						
						
						vita2d_draw_texture(imageEntries[i].previewImage, scrollX + imageEntries[i].previewX, scrollY + imageEntries[i].previewY);
						drawnThisFrame ++;
						
						if(imageEntries[i].isAnimatedGif){
							vita2d_draw_fill_circle(scrollX + imageEntries[i].previewX + 2, scrollY + imageEntries[i].previewY + 2, 6 , RGBA8(0xF7 , 0x3F , 0x1F , 0xBF));
							//vita2d_draw_rectangle(scrollX + imageEntries[i].previewX + 4, scrollY + imageEntries[i].previewY + 4, 8, 8, 
							//							RGBA8(0xF7 , 0x7F , 0x1F , 0x9F));
						}
					}
				}
			}
		}
		//debugNetPrintf(DEBUG, "Images drawn this frame : %d \r\n" , drawnThisFrame);  // around 30 or so at max what i saw
	}else{
		if(selectedImage >= 0 && selectedImage < imageEntries.size()){
			if(imageEntries[selectedImage].fullImageLoaded){
				if(imageEntries[selectedImage].fullImage != NULL){
					vita2d_draw_texture_scale(imageEntries[selectedImage].fullImage , fullScrollX + imageEntries[selectedImage].fullX , fullScrollY + imageEntries[selectedImage].fullY
												, fullScaleX , fullScaleY);
				}
			}
		}
	}
	sceKernelUnlockMutex(imageEntriesVectorLock , 1 );
	
	vita2d_end_drawing();
	vita2d_swap_buffers();
	vita2d_wait_rendering_done();
	sceDisplayWaitVblankStart();
}


void UserInterface::Check(){
	sceKernelLockMutex( imageEntriesVectorLock , 1 , NULL );
	if(!isViewingImage){
		drawnThisFrame = 0;
		for(int i = 0; i < imageEntries.size() ; i++){
			if(imageEntries[i].previewImageLoaded){
				if( (scrollY + imageEntries[i].previewY) <= -1000  ){
					if(imageEntries[i].previewImage != NULL){
						imageEntries[i].previewImageLoaded = false;
						vita2d_free_texture(imageEntries[i].previewImage);
					}
				}
			}
		}
		//debugNetPrintf(DEBUG, "Images drawn this frame : %d \r\n" , drawnThisFrame);  // around 30 or so at max what i saw
	}
	sceKernelUnlockMutex(imageEntriesVectorLock , 1 );
}

const float zoomChange = 0.04f;

void UserInterface::Zoom(int direction){
	if(direction > 0){
		fullScaleX += zoomChange;
		fullScaleY += zoomChange;
	}else if(direction < 0){
		fullScaleX -= zoomChange;
		fullScaleY -= zoomChange;
	}
	
	float newPosX = 0;
	float newPosY = 0;
	float newSizeY = imageEntries[selectedImage].height;
	float newSizeX = imageEntries[selectedImage].width;

	
	newSizeY = newSizeY  * fullScaleY;
	newSizeX = newSizeX  * fullScaleX;
	
	newPosX = (960 / 2)  -  (newSizeX / 2);
	newPosY = (544 / 2)  -  (newSizeY / 2);
	
	imageEntries[selectedImage].fullX = newPosX;
	imageEntries[selectedImage].fullY = newPosY;
	
}

void UserInterface::Scroll(int x , int y){
	if(isViewingImage){
		fullScrollX += x * 2;
		fullScrollY -= y * 2;
	}else{
		//scrollX += x;
		scrollY -= y * 2;
		
		if(x > 0){
			MoveToImage(1);
		}else if(x < 0){
			MoveToImage(-1);
		}
		
	}
}

void UserInterface::MoveToImage(int direction){
	if(isViewingImage){
		return;
	}
	
	sceKernelLockMutex( imageEntriesVectorLock , 1 , NULL );
	selectedImage += direction;
	if(selectedImage < 0){
		selectedImage = imageEntries.size() - 1;
	}else if(selectedImage >= imageEntries.size()) 
	{
		selectedImage = 0;
	}
	
	int currentImagePos = scrollY + imageEntries[selectedImage].previewY;
	int currentImageEndPos = scrollY + imageEntries[selectedImage].previewY + imageEntries[selectedImage].pHeight;
	if( currentImagePos <= 520  && currentImageEndPos >= 16){
		// is good
	}else{
		if(direction > 0){
			for(int i = 0; i < imageEntries.size() ; i++){
				if(imageEntries[i].previewImageLoaded){
					if( (scrollY + imageEntries[i].previewY) <= 520 && (scrollY + imageEntries[i].previewY + imageEntries[i].pHeight) >= 15 ){
						selectedImage = i;
						
						break;
					}
				}
			}
		}else if(direction < 0){
			for(int i = imageEntries.size() - 1; i >= 0  ; i--){
				if(imageEntries[i].previewImageLoaded){
					if( (scrollY + imageEntries[i].previewY) <= 520 && (scrollY + imageEntries[i].previewY + imageEntries[i].pHeight) >= 16 ){
						selectedImage = i;
						
						break;
					}
				}
			}
		}else{
			for(int i = 0; i < imageEntries.size() ; i++){
				if(imageEntries[i].previewImageLoaded){
					if( (scrollY + imageEntries[i].previewY) <= 520 && (scrollY + imageEntries[i].previewY + imageEntries[i].pHeight) >= 16 ){
						selectedImage = i;
						
						break;
					}
				}
			}
		}
	}
	sceKernelUnlockMutex( imageEntriesVectorLock , 1  );
	sceKernelDelayThread(100000);
	
}


