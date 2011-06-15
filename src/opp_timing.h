// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#ifndef OPP_TIMING_H
#define OPP_TIMING_H

#include <sys/time.h> 

namespace Opp {
#if 0
	double get_curr_time_seconds();
		//returns current time in seconds
#endif

	timeval get_curr_timeval();

	double diff_time(timeval start, timeval end);
}

#endif //OPP_TIMING_H
