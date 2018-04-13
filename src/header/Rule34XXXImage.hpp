#pragma once
#ifndef RULE34XXXIMAGE_HPP
#define RULE34XXXIMAGE_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>




class Rule34XXXImage {
public:
	std::string fullurl;
	int height;
	int width;
	std::string previewurl;
	int previewheight;
	int previewwidth;
	std::string md5;
	Rule34XXXImage(std::string u, int h, int w, std::string pu, int pw, int ph, std::string md5S);

};


#endif
