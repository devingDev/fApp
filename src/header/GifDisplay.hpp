#ifndef GIFDISPLAY_HPP
#define GIFDISPLAY_HPP

#include <string>
#include <vita2d.h>
#include "Logger.hpp"
#include "../giflib/gif_lib.h"






class GifDisplay{
	public:
	
	
		struct GifThreadHelper{
			vita2d_texture * myGifVita2D;
			std::string fileName;
			GifDisplay * gifDisplay;
		};
	// the vita2d_texture  *   must be created beforehand!
		void ShowImage(std::string filename , vita2d_texture * texture);
		void UnshowCurrentImage();
		void LoadAnimationGif();
		SceUID animationThreadID; 
		vita2d_texture * myGifVita2D;
		GifFileType * myGif;
		std::string currentFilename;
		bool isShown;
		bool threadExited;
		GifThreadHelper gifThreadHelper;
	
	private:
		
	
};














#endif