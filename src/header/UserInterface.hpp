#ifndef USERINTERFACE_HPP
#define USERINTERFACE_HPP
#include <string>
#include <vector>
#include <functional>
#include <vita2d.h>
#include <mutex>
#include "Rule34XXXImage.hpp"
#include "GifDisplay.hpp"
#define MAX_IMAGE_SIZE 16 * 1024 * 1024
#define MAX_THUMBNAIL_SIZE 1 * 1024 * 1024
#define MAX_DOWNLOAD_THREADS 7


struct ImageEntry{
	std::string previewurl;
	std::string fullURL;
	vita2d_texture * previewImage;
	vita2d_texture * fullImage;
	int pWidth ;
	int pHeight;
	int width ;
	int height ;
	bool fullImageLoaded;
	bool previewImageLoaded;
	int previewX;
	int previewY;
	int fullX;
	int fullY;
	long fullImageBytes;
	long previewImageBytes;
	std::string fullImageMD5;
	bool isAnimatedGif;
};

struct ThreadImageEntryHelper{
	ImageEntry iE;
	bool inUse;
};

class UserInterface{
	public:
		void Check();
		void Draw();
		void AddPreviewImage(std::string pUrl , int pWidth , int pHeight , std::string fullURL , int width , int height , std::string fullImageMD5S);
		void AddPreviewImageRule34XXX(Rule34XXXImage r34xxxImage);
		int ThreadAddPreviewImage(int threadIndex);
		void ResetPreviews();
		void MoveToImage(int direction);
		void Scroll(int x , int y);
		void ShowImage();
		void Zoom(int direction);
		void UnshowImage();
		UserInterface(std::function<unsigned long(const char * , char *)> downloadFunction);
		bool isViewingImage;
		bool isViewingGifImage;
		int threadsRunning;
		GifDisplay myGifDisplayer;
		
	private:
		long estimatedTotalImageHeapUsage;
		char * imageDataFull;
		vita2d_texture * backgroundTexture;
		int selectedImage;
		int scrollX;
		int scrollY;
		int fullScrollX;
		int fullScrollY;
		float fullScaleX;
		float fullScaleY;
		int nextPreviewX;
		int currentPreviewY;
		int lastRowMaxHeight;
		int lastRowMaxWidth;
		ThreadImageEntryHelper * threadEntriesHelper;
		SceUID imageEntriesVectorLock;
		SceUID imageEntriesLock;
		SceUID vita2dLoadImageLock;
		SceUID allocateDataImageLock;
		SceUID changeIntThreadsRunningLock;
		std::vector<ImageEntry> imageEntries;
		std::function<unsigned long(const char * , char *)> downloadImage;
};


#endif
