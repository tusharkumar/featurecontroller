// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#ifndef OPP_FUNC_H
#define OPP_FUNC_H

#ifndef OPP_H
#error "Cannot include directly. opp_func.h is automatically included from opp.h"
#endif

namespace Opp {

	//Func class defined in opp.h
	
	//0 parameter function
	class FO_0p_c : public Func {
		typedef void (* Func0p_t)(void);

		Func0p_t f;

	public:
		FO_0p_c(const std::string& fname, Func0p_t f)
			: Func(fname), f(f) { }

		void eval() { f(); }
	};

	Func * func_handle(const std::string& fname, void (* f)(void));
	
	//1 parameter function
	template<typename T1>
	class FO_1p_c : public Func {
		typedef void (* Func1p_t)(T1);

		Func1p_t f;
		T1 * pt1;

	public:
		FO_1p_c(const std::string& fname, Func1p_t f, T1 * pt1)
			: Func(fname), f(f), pt1(pt1) { }

		void eval() { f(*pt1); }
	};

	template<typename T1>
	Func * func_handle(const std::string& fname, void (* f)(T1), T1& t1) {
		return new FO_1p_c<T1>(fname, f, &t1);
	};


	//2 parameter function
	template<typename T1, typename T2>
	class FO_2p_c : public Func {
		typedef void (* Func2p_t)(T1, T2);

		Func2p_t f;
		T1 * pt1;
		T2 * pt2;

	public:
		FO_2p_c(const std::string& fname, Func2p_t f, T1 * pt1, T2 * pt2)
			: Func(fname), f(f), pt1(pt1), pt2(pt2) { }

		void eval() { f(*pt1, *pt2); }
	};

	template<typename T1, typename T2>
	Func * func_handle(const std::string& fname, void (* f)(T1, T2), T1& t1, T2& t2) {
		return new FO_2p_c<T1, T2>(fname, f, &t1, &t2);
	};


	//3 parameter function
	template<typename T1, typename T2, typename T3>
	class FO_3p_c : public Func {
		typedef void (* Func3p_t)(T1, T2, T3);

		Func3p_t f;
		T1 * pt1;
		T2 * pt2;
		T3 * pt3;

	public:
		FO_3p_c(const std::string& fname, Func3p_t f, T1 * pt1, T2 * pt2, T3 * pt3)
			: Func(fname), f(f), pt1(pt1), pt2(pt2), pt3(pt3) { }

		void eval() { f(*pt1, *pt2, *pt3); }
	};

	template<typename T1, typename T2, typename T3>
	Func * func_handle(const std::string& fname, void (* f)(T1, T2, T3), T1& t1, T2& t2, T3& t3) {
		return new FO_3p_c<T1, T2, T3>(fname, f, &t1, &t2, &t3);
	};



} //namespace Opp

#endif //OPP_FUNC_H
