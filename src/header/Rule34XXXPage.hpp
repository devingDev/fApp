#pragma once
#ifndef RULE34XXXPAGE_HPP
#define RULE34XXXPAGE_HPP


#include <vector>
#include <sstream>
#include "Rule34XXXImage.hpp"


class Rule34XXXPage {
public:
	std::vector<Rule34XXXImage> images;
	Rule34XXXPage();
};

#endif
