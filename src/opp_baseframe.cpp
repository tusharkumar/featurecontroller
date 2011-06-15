// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#include <vector>

#include "opp.h"
#include "opp_baseframe.h"

/////////////////////////////
//class BaseFrame definitions
/////////////////////////////

namespace Opp {
std::vector<BaseFrame *> vBaseFrames;

BaseFrame::BaseFrame()
	: id(vBaseFrames.size())
{ vBaseFrames.push_back(this); }

BaseFrame::~BaseFrame()
{
	assert(vBaseFrames.at(id) != 0);
	vBaseFrames.at(id) = 0;
}

} //namespace Opp
