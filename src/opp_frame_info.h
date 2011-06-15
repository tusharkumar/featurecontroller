// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#ifndef OPP_FRAME_INFO_H
#define OPP_FRAME_INFO_H

#include <list>
#include <sys/time.h>
#include "opp.h"
#include "opp_decision_model.h"

namespace Opp {
	class FrameInfo {
	public:
		Objective objective;

		Frame * my_frame; //the frame corresponding to this FrameInfo instance
		FrameDecisionModel decision_model;

		//FIXME: add element for Constraints

		bool bIsActive; //frame has been started but not yet completed


		//Following defined only if bIsActive == true
		bool bIsSuspended;
			//an active frame has been suspended, else is executing
		ExecTime_t current_invocation_exec_time;
			//cumulative time spent in current invocation of frame,
			//including all suspends and resumes of a piecewise frame
		int stack_index;
			//location of an active frame in vFrameStack
		Frame * curr_parent_frame;
			//curr_parent_frame = 0 for a top-level frame
		std::list<Frame *> list_active_child_frames;
			//direct children that are currently active (atmost one can be Executing, rest Suspended)


		//Following defined only if frame is currently executing
		//  i.e., bIsActive == true and bIsSuspended = false
		timeval curr_enter_timeval;


		FrameInfo(Frame * my_frame)
			: my_frame(my_frame), decision_model(my_frame), bIsActive(false),
				bIsSuspended(false), current_invocation_exec_time(0.0),
				stack_index(-1), curr_parent_frame(0)
		{ }

		FrameInfo(Frame * my_frame, const Objective& obj)
			: objective(obj),
				my_frame(my_frame), decision_model(my_frame), bIsActive(false),
				bIsSuspended(false), current_invocation_exec_time(0.0),
				stack_index(-1), curr_parent_frame(0)
		{ }


		//being a friend of class Frame
		static FrameInfo * get_frame_info(Frame * frame)
			{ return frame->frame_info; }

	};
}

#endif //OPP_FRAME_INFO_H
