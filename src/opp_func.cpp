// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#include <iostream>

#include <pthread.h>
#include <sys/time.h>

#include "opp.h"

namespace Opp {

//Definitions for Func

Func::Func(const std::string& fname)
	: fname(fname)
{ }

Func::~Func()
{ }


//Non-templated 0 parameter function support
Func * func_handle(const std::string& fname, void (* f)(void)) {
	return new FO_0p_c(fname, f);
};

} //namespace Opp


