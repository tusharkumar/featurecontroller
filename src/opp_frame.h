// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#ifndef OPP_FRAME_H
#define OPP_FRAME_H

#include <vector>
#include "opp.h"

namespace Opp {

	std::vector<Frame *> get_dynamically_enclosing_frames(BaseFrame * active_base_frame);
	//returns the stack of frames (top-level frame last) that are currently
	//  dynamically enclosing the execution of the 'active_base_frame'.
	//'active_base_frame' must correspond to a Frame that is currently
	//  in Active state, or, must be an ExecFrame that is currently Executing.

	Frame * get_innermost_executing_frame();
	//Gets innermost Frame that is currently executing.
	//Returns 0 if no frame is executing.

	Frame * get_frame_from_frame_id(FrameID_t frame_id);
}

#endif //OPP_FRAME_H
