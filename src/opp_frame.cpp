// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#include <iostream>
#include <cstdlib>
#include <vector>

#include "opp.h"
#include "opp_timing.h"
#include "opp_baseframe.h"
#include "opp_frame_info.h"
#include "opp_frame.h"
#include "opp_execframe.h"

namespace Opp {



/////////////////////////////
//class Frame definitions
/////////////////////////////

Frame::Frame()
	: BaseFrame()
{ frame_info = new FrameInfo(this); }

Frame::Frame(const Objective& obj)
	: BaseFrame()
{ frame_info = new FrameInfo(this, obj); }


Frame::~Frame() {
	delete frame_info;
	frame_info = 0;
}


//Frame(const std::string& frame_name, const Objective& obj);



/////////////////////////////
//API function-calls related to Frame
/////////////////////////////

std::vector<Frame *> vFrameStack;
	//Only contains non-zero entries for currently active frames

Frame * get_innermost_executing_frame() {
	for(int i=(int)vFrameStack.size()-1; i >= 0; i--) {
		if(vFrameStack[i] != 0 && FrameInfo::get_frame_info(vFrameStack[i])->bIsSuspended == false)
			return vFrameStack[i];
	}

	return 0; //no frame found Executing
}


std::vector<Frame *> get_dynamically_enclosing_frames(BaseFrame * active_base_frame) {
	Frame * frame = dynamic_cast<Frame *>(active_base_frame);
	ExecFrame * execframe = dynamic_cast<ExecFrame *>(active_base_frame);

	assert(frame != 0 || execframe != 0);

	Frame * parent_frame = 0;
	if(frame != 0) {
		FrameInfo * frame_info = FrameInfo::get_frame_info(frame);
		if(frame_info->bIsActive == false) {
			std::cerr << "get_dynamically_enclosing_frames(): ERROR: called for an Inactive frame:"
					<< "\n    frame id = " << active_base_frame->id
					<< std::endl;
			exit(1);
		}
		parent_frame = frame_info->curr_parent_frame;
	}
	else //execframe != 0
	{ parent_frame = ExecFrameInfo::get_execframe_info(execframe)->curr_parent_frame; }

	std::vector<Frame *> context;

	while(parent_frame != 0) {
		context.push_back(parent_frame);
		parent_frame = FrameInfo::get_frame_info(parent_frame)->curr_parent_frame;
	}

	return context;
}

Frame * get_frame_from_frame_id(FrameID_t frame_id)
{
	BaseFrame * base_frame = vBaseFrames.at(frame_id);
	Frame * frame = dynamic_cast<Frame *>(base_frame);
	assert(frame != 0);
	return frame;
}



void frame_enter(FrameID_t frame_id) {
	FrameID_t parent_frame_id = -1; //assume top-level

	Frame * innermost_executing_frame = get_innermost_executing_frame();
	if(innermost_executing_frame != 0)
		parent_frame_id = innermost_executing_frame->id;

	frame_enter(frame_id, parent_frame_id);
}

void frame_enter(FrameID_t frame_id, FrameID_t chosen_parent_frame_id) {
	timeval curr_timeval = get_curr_timeval();

	assert(frame_id >= 0 && frame_id < vBaseFrames.size());

	Frame * frame = dynamic_cast<Frame *>(vBaseFrames[frame_id]);
	assert(frame != 0);
	FrameInfo * frame_info = FrameInfo::get_frame_info(frame);

	if(frame_info->bIsActive == false) { //Inactive -> Executing
		frame_info->bIsActive = true;
		frame_info->bIsSuspended = false;
		frame_info->current_invocation_exec_time = 0.0;

		frame_info->stack_index = (int)vFrameStack.size();
		vFrameStack.push_back(frame);

		frame_info->curr_parent_frame = 0;
		if(chosen_parent_frame_id != -1) { //not top-level, has a definite parent frame
			Frame * parent_frame = dynamic_cast<Frame *>(vBaseFrames[chosen_parent_frame_id]);
			assert(parent_frame != 0);

			FrameInfo * parent_frame_info = FrameInfo::get_frame_info(parent_frame);
			if(parent_frame_info->bIsActive == false || parent_frame_info->bIsSuspended == true) {
				std::cerr << "frame_enter(): ERROR: chosen parent frame is not curently Executing:"
					<< "\n    chosen_parent_frame_id = " << chosen_parent_frame_id
					<< "\n       for frame_id = " << frame_id << std::endl;
				exit(1);
			}	
			frame_info->curr_parent_frame = parent_frame;

			parent_frame_info->list_active_child_frames.push_back(frame);
				//assume: 'frame' cannot already be present in 'parent_frame_info->list_active_child_frames'
		}

		activate_decision_model_and_decide_setting(frame);
	}

	else if(frame_info->bIsSuspended == true) { //Suspended -> Executing
		frame_info->bIsSuspended = false;

		assert(frame_info->stack_index >= 0 && frame_info->stack_index < (int)vFrameStack.size());
		assert(vFrameStack[frame_info->stack_index]->id == frame->id);

		if(chosen_parent_frame_id != -1) {
			if(frame_info->curr_parent_frame->id != chosen_parent_frame_id) {
				std::cerr << "frame_enter(): ERROR: frame Resumed with different chosen-parent:"
					<< "\n  existing parent_frame id = " << frame_info->curr_parent_frame->id
					<< "\n  invoked with chosen_parent_frame_id = " << chosen_parent_frame_id
					<< "\n    on frame_id = " << frame_id
					<< std::endl;
				exit(1);
			}
		}
		else { //top-level frame
			if(frame_info->curr_parent_frame != 0) {
				std::cerr << "frame_enter(): ERROR: inner frame Resumed as top-level frame:"
					<< "\n  existing parent_frame id = " << frame_info->curr_parent_frame->id
					<< std::endl;
				exit(1);
			}
		}
	}

	else {
		std::cerr << "frame_enter(): ERROR: Entered frame was neither Inactive nor Suspended"
					<< "\n   frame_id = " << frame_id
					<< "\n   chosen_parent_frame_id = " << chosen_parent_frame_id << std::endl;
		exit(1);
	}

	frame_info->curr_enter_timeval = curr_timeval;

	assert(frame == get_innermost_executing_frame());
}

ExecTime_t frame_exit_complete(FrameID_t frame_id) {
	assert(frame_id >= 0 && frame_id < vBaseFrames.size());

	Frame * frame = dynamic_cast<Frame *>(vBaseFrames[frame_id]);
	assert(frame != 0);
	FrameInfo * frame_info = FrameInfo::get_frame_info(frame);

	if(frame_info->bIsActive == false) {
		std::cerr << "frame_exit_complete(): ERROR: invoked on an Inactive frame:"
				<< "\n   frame_id = " << frame_id << std::endl;
		exit(1);
	}

	//Now: frame is Active (Executing or Suspended)

	if(frame_info->bIsSuspended == false) //Executing -> Suspended
		frame_exit_suspend(frame_id); //verifies that all child frames are suspended as well
	

	//Now: frame is Suspended
	
	//Complete all child frames
	for(std::list<Frame *>::iterator it = frame_info->list_active_child_frames.begin();
			it != frame_info->list_active_child_frames.end();
			//
	) {
		std::list<Frame *>::iterator next_it = it;
		next_it++;
		frame_exit_complete((*it)->id); //removes 'it' from 'frame_info->list_active_child_frames'
		it = next_it;
	}

	
	// Update statistics related to completing frame
	update_decision_model_on_completion(frame);
	ExecTime_t total_execution_time_for_invocation = frame_info->current_invocation_exec_time;
	frame_info->current_invocation_exec_time = 0.0;
	
	// - Inactivate and nullify on stack
	if(frame_info->curr_parent_frame != 0) {
		FrameInfo * parent_frame_info = FrameInfo::get_frame_info(frame_info->curr_parent_frame);
		std::list<Frame *>::iterator it_for_frame
			= std::find(
				parent_frame_info->list_active_child_frames.begin(),
				parent_frame_info->list_active_child_frames.end(),
				frame
			);
		assert(it_for_frame != parent_frame_info->list_active_child_frames.end());
			//frame must be found in parent
		parent_frame_info->list_active_child_frames.erase(it_for_frame); //remove it
	}
	frame_info->curr_parent_frame = 0;
	frame_info->bIsActive = false;
	vFrameStack.at(frame_info->stack_index) = 0;
	frame_info->stack_index = -1;

	// - remove nullified entries on top of stack
	int new_size = (int)vFrameStack.size();
	while(new_size > 0) {
		if(vFrameStack[new_size-1] != 0)
			break;
		new_size--;
	}

	if(new_size < (int)vFrameStack.size()) {
		vFrameStack.resize(new_size);
	}

	return total_execution_time_for_invocation;
}


ExecTime_t frame_exit_suspend(FrameID_t frame_id) {
	assert(frame_id >= 0 && frame_id < vBaseFrames.size());

	Frame * frame = dynamic_cast<Frame *>(vBaseFrames[frame_id]);
	assert(frame != 0);
	FrameInfo * frame_info = FrameInfo::get_frame_info(frame);

	if(frame_info->bIsActive == false || frame_info->bIsSuspended == true) {
		std::cerr << "frame_exit_suspend(): ERROR: invoked on a non-Executing frame:"
				<< "\n   frame_id = " << frame_id << std::endl;
		exit(1);
	}

	// - check that contained frames are Suspended (error if any is Executing)
	for(std::list<Frame *>::iterator it = frame_info->list_active_child_frames.begin();
			it != frame_info->list_active_child_frames.end();
			it++
	) {
		FrameInfo * child_frame_info = FrameInfo::get_frame_info(*it);
		assert(child_frame_info->bIsActive == true);
		if(child_frame_info->bIsSuspended == false) { //Contained child is Executing
			std::cerr << "frame_exit_suspend(): ERROR: cannot suspend frame containing an executing child frame:"
				<< "\n     frame id = " << frame_id
				<< "\n     found Executing child frame id = " << (*it)->id
				<< std::endl;
			exit(1);
		}
	}

	// - measure elapsed time
	timeval curr_timeval = get_curr_timeval();
	ExecTime_t elapsed_piece_time = diff_time(frame_info->curr_enter_timeval, curr_timeval);
	frame_info->current_invocation_exec_time += elapsed_piece_time;

	frame_info->bIsSuspended = true;

	return elapsed_piece_time;
}

bool is_frame_active(FrameID_t frame_id) {
	assert(0 <= frame_id);

	if(frame_id >= (int)vBaseFrames.size())
		return false; //never allocated

	if(vBaseFrames[frame_id] == 0)
		return false; //destructed, no longer allocated

	Frame * frame = dynamic_cast<Frame *>(vBaseFrames[frame_id]);
	assert(frame != 0);
	FrameInfo * frame_info = FrameInfo::get_frame_info(frame);

	return frame_info->bIsActive;
}

bool is_frame_executing(FrameID_t frame_id) {
	assert(0 <= frame_id);

	if(frame_id >= (int)vBaseFrames.size())
		return false; //never allocated

	if(vBaseFrames[frame_id] == 0)
		return false; //destructed, no longer allocated

	Frame * frame = dynamic_cast<Frame *>(vBaseFrames[frame_id]);
	assert(frame != 0);
	FrameInfo * frame_info = FrameInfo::get_frame_info(frame);

	return (frame_info->bIsActive) && (not frame_info->bIsSuspended);
}





} //namespace Opp
