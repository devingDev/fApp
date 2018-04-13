
#include "header/Rule34XXXImage.hpp"



using namespace std;


Rule34XXXImage::Rule34XXXImage(string u, int w, int h, string pu, int pw, int ph , string md5S) {
	this->fullurl = u;
	this->width = w;
	this->height = h;
	this->previewurl = pu;
	this->previewwidth = pw;
	this->previewheight = ph;
	this->md5 = md5S;
};
