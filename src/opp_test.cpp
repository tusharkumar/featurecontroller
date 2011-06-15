// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#include <iostream>

#include "opp.h"
#include "opp_debug_control.h"

void f1(int x) {
	std::cout << "Inside f1" << std::endl;
	for(int i=0; i<1000000; i++)
		x = (x * 10 + 5) / 9;
	std::cout << "x = " << x << std::endl;
}

void f2(int x) {
	std::cout << "Inside f2" << std::endl;
	for(int i=0; i<500000; i++)
		x = (x * 10 + 5) / 9;
	std::cout << "x = " << x << std::endl;
}

void f3(int x) {
	std::cout << "Inside f3" << std::endl;
	for(int i=0; i<100000; i++)
		x = (x * 10 + 5) / 9;
	std::cout << "x = " << x << std::endl;
}

void f4(int x) {
	std::cout << "Inside f4" << std::endl;
	for(int i=0; i<50000; i++)
		x = (x * 10 + 5) / 9;
	std::cout << "x = " << x << std::endl;
}


void ww() {
	static Opp::Frame f_ww(Opp::Objective(0.005, 0.3, 0.3, 0.9, 3));
	Opp::FrameStatistics stats_f_ww(f_ww.id);
	Opp::frame_enter(f_ww.id);

	std::cout << "Inside f_ww" << std::endl;

	std::cout << "JJ" << std::endl;
	static Opp::Caller caller1;
	static Opp::Caller caller2;
	static Opp::Caller caller3;
	static Opp::Caller caller4;
	static Opp::Model model;

	std::cout << "KK" << std::endl;

	static bool bInitialized = false;
	if(bInitialized == false) {
		std::vector<Opp::Model> v;
		v.push_back(Opp::Model(&caller1));
		v.push_back(Opp::Model(&caller2));
		v.push_back(Opp::Model(&caller3));
		v.push_back(Opp::Model(&caller4));

		std::vector<Opp::Model> vSeq;
		vSeq.push_back( Opp::Model(0, v, 1) );   //select model for variable 0
		model = Opp::Model(vSeq); //overall sequence model

		bInitialized = true;
	}

	std::cout << "LL" << std::endl;
	static Opp::ExecFrame ex(model, 0); //stickiness
	//ex.force_default_selection(true);

	int x = 5;
	caller1.rebind(OPP_FUNC_HANDLE(f1, x));
	caller2.rebind(OPP_FUNC_HANDLE(f2, x));
	caller3.rebind(OPP_FUNC_HANDLE(f3, x));
	caller4.rebind(OPP_FUNC_HANDLE(f4, x));

	ex.run();

	Opp::frame_exit_complete(f_ww.id);
	std::cout << Opp::ExecFrameStatistics(ex.id).refresh().print_string() << std::endl;
	std::cout << stats_f_ww.refresh().print_string() << std::endl;
}

int main() {
	Opp::feature_control_use_fast_reaction_strategy(true);

	static Opp::Frame f_main;
	Opp::FrameStatistics stats_f_main(f_main.id);
	std::cout << stats_f_main.refresh().print_string() << std::endl;
	Opp::frame_enter(f_main.id);

	std::cout << "Inside f_main" << std::endl;

	for(int i=0; i<40; i++)
		ww();

	Opp::frame_exit_complete(f_main.id);
	std::cout << stats_f_main.refresh().print_string() << std::endl;
	return 0;
}
