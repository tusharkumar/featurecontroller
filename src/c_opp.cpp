// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#include <iostream>
#include <cstdlib>

#include "c_opp.h"
#include "opp.h"

#include "opp_baseframe.h"

void opp_frame_enter(Opp_FrameID_t frame_id)
{ Opp::frame_enter(frame_id); }

void opp_frame_enter_chosen_parent(Opp_FrameID_t frame_id, Opp_FrameID_t chosen_parent_frame_id)
{ Opp::frame_enter(frame_id, chosen_parent_frame_id); }

Opp_ExecTime_t opp_frame_exit_complete(Opp_FrameID_t frame_id)
{ return Opp::frame_exit_complete(frame_id); }

Opp_ExecTime_t opp_frame_exit_suspend(Opp_FrameID_t frame_id)
{ return Opp::frame_exit_suspend(frame_id); }


void opp_execframe_run(Opp_FrameID_t execframe_id) {
	if(execframe_id < 0 || execframe_id >= (int)Opp::vBaseFrames.size())
	{
		std::cerr << "opp_execframe_run(): ERROR: execframe_id = "
			<< execframe_id << " does not exist" << std::endl;
		exit(1);
	}
	Opp::ExecFrame * ef = dynamic_cast<Opp::ExecFrame *>(Opp::vBaseFrames.at(execframe_id));
	if(ef == 0) {
		std::cerr << "opp_execframe_run(): ERROR: execframe_id = "
			<< execframe_id << " is not an ExecFrame" << std::endl;
		exit(1);
	}

	ef->run();
}


void opp_rebind_func(Opp_Caller caller, Opp_Func func) {
	Opp::Caller * real_caller = reinterpret_cast<Opp::Caller *>(caller.caller_info);

	real_caller->rebind(OPP_FUNC_HANDLE0(func));
}



namespace Opp {

Opp_Caller get_c_handle_for_caller(Opp::Caller * caller) {
	Opp_Caller c_caller;
	c_caller.caller_info = reinterpret_cast<void *>(caller);

	return c_caller;
}

} //namespace Opp
