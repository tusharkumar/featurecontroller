// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com


#ifndef C_OPP_H
#define C_OPP_H


///////////////////////
// API for C programs
///////////////////////

#ifdef __cplusplus
extern "C" {
#endif

typedef double Opp_ExecTime_t;
typedef long long Opp_FrameID_t;


//Frame interface
void opp_frame_enter(Opp_FrameID_t frame_id);

void opp_frame_enter_chosen_parent(Opp_FrameID_t frame_id, Opp_FrameID_t chosen_parent_frame_id);

Opp_ExecTime_t opp_frame_exit_complete(Opp_FrameID_t frame_id);

Opp_ExecTime_t opp_frame_exit_suspend(Opp_FrameID_t frame_id);

void opp_execframe_run(Opp_FrameID_t execframe_id); //FIXME: return Opp_ExecTime_t


//Func interface

typedef struct Opp_Caller {
	void * caller_info;
} Opp_Caller;

typedef void (* Opp_Func)(void);

void opp_rebind_func(Opp_Caller caller, Opp_Func func); //binds func, so SRT can invoke it via caller




#ifdef __cplusplus
} //extern "C"
#endif

#endif //C_OPP_H
