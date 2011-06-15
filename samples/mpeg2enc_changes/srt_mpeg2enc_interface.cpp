#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include "srt_mpeg2enc_interface.h"
#include "opp.h"
#include "opp_debug_control.h"

Opp_Caller oc_window_0;
Opp_Caller oc_window_1;
Opp_Caller oc_window_2;
Opp_Caller oc_window_3;
Opp_Caller oc_window_4;
Opp_Caller oc_window_5;
Opp_Caller oc_window_6;
Opp_Caller oc_window_7;

int multi_frame_count = 1; //default

void fmain_fractional_enter() {
	static int count = 0;
	count++;
	if(count == multi_frame_count) {
		count = 0;
		std::cout << "fMAIN multi-frame enter" << std::endl;
		opp_frame_enter(frame_name_to_id[fMAIN]);
	}
}


void fmain_fractional_exit() {
	static int count = 0;
	count++;
	if(count == multi_frame_count) {
		count = 0;
		std::cout << "fMAIN multi-frame exit" << std::endl;
		opp_frame_exit_complete(frame_name_to_id[fMAIN]);
	}
}

char * plot_filename = 0;

void init_srt_interface() {
	Opp::feature_control_use_fast_reaction_strategy(true);


	const char * srt_exp_plot_filename = std::getenv("SRT_EXP_PLOT_FILENAME");
	std::cout << "Environment variable SRT_EXP_PLOT_FILENAME = " << (srt_exp_plot_filename != 0 ? srt_exp_plot_filename : "") << std::endl;
	plot_filename = "plot.dat";
	if(srt_exp_plot_filename != 0) {
		plot_filename = (char *)malloc( strlen(srt_exp_plot_filename) + 1 );
		strcpy(plot_filename, srt_exp_plot_filename);
	}
	std::cout << "  plot_filename = " << plot_filename << std::endl;

	const char * srt_exp_multi_frame_count = std::getenv("SRT_EXP_MULTI_FRAME_COUNT");
	std::cout << "Environment variable SRT_EXP_MULTI_FRAME_COUNT = " << (srt_exp_multi_frame_count != 0 ? srt_exp_multi_frame_count : "") << std::endl;
	if(srt_exp_multi_frame_count != 0) {
		int temp_mfc;
		if( std::sscanf(srt_exp_multi_frame_count, "%d", &temp_mfc) == 1)
			multi_frame_count = temp_mfc;
		assert(multi_frame_count > 0);
	}
	std::cout << "For Frame fMAIN multi_frame_count = " << multi_frame_count << std::endl;


	const char * srt_exp_mean = std::getenv("SRT_EXP_MEAN");
	std::cout << "Environment variable SRT_EXP_MEAN = " << (srt_exp_mean != 0 ? srt_exp_mean : "") << std::endl;
	double mean = 0.53; //default value
	if(srt_exp_mean != 0) {
		double temp_mean;
		if( std::sscanf(srt_exp_mean, "%lf", &temp_mean) == 1 )
			mean = temp_mean;
		assert(mean >= 0.0);
	}
	mean = mean * multi_frame_count;
	std::cout << "For Frame fMAIN using (multi-frame) mean = " << mean << std::endl;

	const char * srt_exp_window_frac = std::getenv("SRT_EXP_WINDOW_FRAC");
	std::cout << "Environment variable SRT_EXP_WINDOW_FRAC = " << (srt_exp_window_frac != 0 ? srt_exp_window_frac : "") << std::endl;
	double window_frac = 0.20; //default value
	if(srt_exp_window_frac != 0) {
		double temp_window_frac;
		if( std::sscanf(srt_exp_window_frac, "%lf", &temp_window_frac) == 1 )
			window_frac = temp_window_frac;
		assert(window_frac >= 0.0);
	}
	std::cout << "For Frame fMAIN using (multi-frame) window_frac = " << window_frac << std::endl;

	const char * srt_exp_sliding_window = std::getenv("SRT_EXP_SLIDING_WINDOW");
	int sliding_window_size = 1;
	if(srt_exp_sliding_window != 0) {
		int temp_sliding_window_size;
		if( std::sscanf(srt_exp_sliding_window, "%d", &temp_sliding_window_size) == 1 )
			sliding_window_size = temp_sliding_window_size;
		assert(sliding_window_size >= 1);
	}
	std::cout << "Environment variable SRT_EXP_SLIDING_WINDOW = " << (srt_exp_sliding_window != 0 ? srt_exp_sliding_window: "") << std::endl;
	std::cout << "SRT for mpeg2enc ExecFrame: sliding_window_size = " << sliding_window_size << std::endl;

	double lower_window_frac = window_frac;
	double upper_window_frac = window_frac;
	std::cout << "  lower_window_frac = " << lower_window_frac << std::endl;
	std::cout << "  upper_window_frac = " << upper_window_frac << std::endl;
	static Opp::Frame f_main(Opp::Objective(mean, lower_window_frac, upper_window_frac, 0.90, sliding_window_size));
	frame_name_to_id[fMAIN] = f_main.id;

	static Opp::Frame f_motion_est;
	frame_name_to_id[fMOTION_EST] = f_motion_est.id;

	static Opp::Caller window_0;
	static Opp::Caller window_1;
	static Opp::Caller window_2;
	static Opp::Caller window_3;
	static Opp::Caller window_4;
	static Opp::Caller window_5;
	static Opp::Caller window_6;
	static Opp::Caller window_7;

	oc_window_0 = Opp::get_c_handle_for_caller(&window_0);
	oc_window_1 = Opp::get_c_handle_for_caller(&window_1);
	oc_window_2 = Opp::get_c_handle_for_caller(&window_2);
	oc_window_3 = Opp::get_c_handle_for_caller(&window_3);
	oc_window_4 = Opp::get_c_handle_for_caller(&window_4);
	oc_window_5 = Opp::get_c_handle_for_caller(&window_5);
	oc_window_6 = Opp::get_c_handle_for_caller(&window_6);
	oc_window_7 = Opp::get_c_handle_for_caller(&window_7);

	std::vector<Opp::Model> vM;
	vM.push_back( Opp::Model(&window_0) );
	vM.push_back( Opp::Model(&window_1) );
	vM.push_back( Opp::Model(&window_2) );
	vM.push_back( Opp::Model(&window_3) );
	vM.push_back( Opp::Model(&window_4) );
	vM.push_back( Opp::Model(&window_5) );
	vM.push_back( Opp::Model(&window_6) );
	vM.push_back( Opp::Model(&window_7) );

	const char * srt_mode_char = std::getenv("SRT_EXP_MODE");
	bool bFixedMode = false;
	int fixedLevel = -1;
	if(srt_mode_char != 0) {
		char mode_char = srt_mode_char[0];
		if(mode_char >= '0' && mode_char <= '7') {
			bFixedMode = true;
			fixedLevel = mode_char - '0';
		}
	}
	std::cout << "Environment variable SRT_EXP_MODE = " << (srt_mode_char != 0 ? srt_mode_char : "") << std::endl;
	std::cout << "SRT mode control for mpeg2enc: bFixedMode = " << bFixedMode << " fixedLevel = " << fixedLevel << std::endl;

	const char * srt_exp_stickiness_length = std::getenv("SRT_EXP_STICKINESS_LENGTH");
	int stickiness_length = 0;
	if(srt_exp_stickiness_length != 0) {
		int temp_stickiness_length;
		if( std::sscanf(srt_exp_stickiness_length, "%d", &temp_stickiness_length) == 1 )
			stickiness_length = temp_stickiness_length;
		assert(stickiness_length >= 0);
	}
	std::cout << "Environment variable SRT_EXP_STICKINESS_LENGTH = " << (srt_exp_stickiness_length != 0 ? srt_exp_stickiness_length : "") << std::endl;
	std::cout << "SRT for mpeg2enc ExecFrame: stickiness_length = " << stickiness_length << std::endl;

	const char * srt_exp_fast_reaction_strategy_coeff = std::getenv("SRT_EXP_FAST_REACTION_STRATEGY_COEFF");
	double fast_reaction_strategy_coeff = 0.0;
	if(srt_exp_fast_reaction_strategy_coeff != 0) {
		double temp_fast_reaction_strategy_coeff;
		if( std::sscanf(srt_exp_fast_reaction_strategy_coeff, "%lf", &temp_fast_reaction_strategy_coeff) == 1 )
			fast_reaction_strategy_coeff = temp_fast_reaction_strategy_coeff;
		assert(fast_reaction_strategy_coeff >= 0.0);
	}
	std::cout << "Environment variable SRT_EXP_FAST_REACTION_STRATEGY_COEFF = " << (srt_exp_fast_reaction_strategy_coeff != 0 ? srt_exp_fast_reaction_strategy_coeff : "") << std::endl;
	std::cout << "SRT for mpeg2enc ExecFrame: fast_reaction_strategy_coeff = " << fast_reaction_strategy_coeff << std::endl;

	const char * srt_exp_force_fixed_frs_coeff = std::getenv("SRT_EXP_FORCE_FIXED_FRS_COEFF");
	int force_fixed_frs_coeff = 0;
	if(srt_exp_force_fixed_frs_coeff != 0) {
		int temp_force_fixed_frs_coeff;
		if( std::sscanf(srt_exp_force_fixed_frs_coeff, "%d", &temp_force_fixed_frs_coeff) == 1 )
			force_fixed_frs_coeff = temp_force_fixed_frs_coeff;
		assert(force_fixed_frs_coeff == 0 || force_fixed_frs_coeff == 1);
	}
	std::cout << "Environment variable SRT_EXP_FORCE_FIXED_FRS_COEFF = " << (srt_exp_force_fixed_frs_coeff != 0 ? srt_exp_force_fixed_frs_coeff : "") << std::endl;
	std::cout << "SRT for mpeg2enc ExecFrame: force_fixed_frs_coeff = " << force_fixed_frs_coeff << std::endl;

	////
	Opp::Model model_choose_search_window(0, vM, fixedLevel, fast_reaction_strategy_coeff);

	static Opp::ExecFrame ef_choose_search_window(model_choose_search_window, stickiness_length);
	ef_choose_search_window.force_default_selection(bFixedMode);
	ef_choose_search_window.force_fast_reaction_strategy_fixed_coeff( (force_fixed_frs_coeff == 1) );
	execframe_name_to_id[efCHOOSE_SEARCH_WINDOW] = ef_choose_search_window.id;


}

void print_srt_stats_for_frame(SRT_Frame_Names_e frame_name) {
	Opp::FrameID_t frame_id = frame_name_to_id[frame_name];
	std::cout << Opp::FrameStatistics(frame_id).refresh().print_string() << std::endl;
}

void print_srt_stats_for_execframe(SRT_ExecFrame_Names_e execframe_name) {
	Opp::FrameID_t execframe_id = execframe_name_to_id[execframe_name];
	std::cout << Opp::ExecFrameStatistics(execframe_id).refresh().print_string() << std::endl;
}
