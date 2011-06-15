// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#ifndef OPP_H
#define OPP_H

#include <list>
#include <vector>
#include <string>
#include <map>
#include <assert.h>

#include "c_opp.h" //C language API

//C++ language API
namespace Opp {
	////////// Setup Opportunistic Execution of Functions ///////////

	class Func {
	public:
		virtual void eval() = 0;

		const std::string fname;

		Func(const std::string& fname);
		virtual ~Func();
	};

	#define OPP_FUNC_HANDLE(fname, args...) Opp::func_handle(#fname, fname, ##args)

	#define OPP_FUNC_HANDLE0(fname) Opp::func_handle(#fname, fname)
		//special case for 0-ary function fname

} //namespace Opp

#include "opp_func.h"



namespace Opp {

	////////// Identify Scalable Functionality in Application ///////////

	// Model Creation
	class Caller {
		friend class RunModel;
		Func * func;
	public:
		Caller() : func(0) { }

		void rebind(Func * new_func) {
			if(func != 0)
				delete func;
			func = new_func;
		}

		~Caller() {
			if(func != 0)
				delete func;
			func = 0;
		}
	};

	Opp_Caller get_c_handle_for_caller(Caller * caller);
		//returns a handle so that C code can bind functions to callers


	class Model {
	public:
		friend class RunModel;
		typedef enum {None, Binder, Sequence, Select} ModelType;

	private:
		ModelType _type;

		Caller * caller;                  //defined iff type == Binder
		std::vector<Model> modelList;     //defined iff type == Sequence OR Select
		int select_var_id;                //defined iff type == Select
		std::vector<int> select_priority; //defined iff type == Select
			//length of select_priority must be same as modelList. Gives priority for corresponding choice.

		int default_choice_index_for_select_var_id; //defined iff type == Select
			//Defines a fixed index into modelList. Normally an ExecFrame invoking this model can pick any choice from modelList.
			// But in a "fixed-choice" mode, the ExecFrame will only pick the choice indexed by default_choice_index_for_select_var_id,
			//   provided default_choice_index_for_select_var_id != -1.

		double fast_reaction_strategy_coeff; //defined iff type == Select
			//initial value of coefficient to be used if Fast Reaction Strategy is used
			// = 0.0 => coefficient not specified

	public:
		//NOP model (do nothing)
		Model()
			: _type(None), caller(0), select_var_id(-1) { }

		//Leaf model
		Model(Caller * caller)
			: _type(Binder), caller(caller), select_var_id(-1) { }

		//Sequence Model
		Model(const std::vector<Model>& modelList)
			: _type(Sequence), caller(0), modelList(modelList), select_var_id(-1) { }

		//Select Model
		Model(int select_var_id, const std::vector<Model>& modelList, int default_choice_index_for_select_var_id = -1, double fast_reaction_strategy_coeff = 0.0)
			: _type(Select), caller(0), modelList(modelList), select_var_id(select_var_id),
				default_choice_index_for_select_var_id(default_choice_index_for_select_var_id), fast_reaction_strategy_coeff(fast_reaction_strategy_coeff)
		{ select_priority.resize(modelList.size(), 0); }

		//Select Model with non-standard priorities for choices
		Model(int select_var_id, const std::vector<Model>& modelList, const std::vector<int>& select_priority, int default_choice_index_for_select_var_id = -1, double fast_reaction_strategy_coeff = 0.0)
			: _type(Select), caller(0), modelList(modelList), select_var_id(select_var_id), select_priority(select_priority),
				default_choice_index_for_select_var_id(default_choice_index_for_select_var_id), fast_reaction_strategy_coeff(fast_reaction_strategy_coeff)
		{ assert(select_priority.size() == modelList.size()); }

		ModelType type() const { return _type; }

		std::vector<Model>& access_modelList() {
			assert(_type == Sequence || _type == Select);
			return modelList;
		}

		int& access_select_var_id() {
			assert(_type == Select);
			return select_var_id;
		}

		std::vector<int>& access_select_priority() {
			assert(_type == Select);
			return select_priority;
		}

		int& access_default_choice_index_for_select_var_id() {
			assert(_type == Select);
			return default_choice_index_for_select_var_id;
		}

		double& access_fast_reaction_strategy_coeff() {
			assert(_type == Select);
			return fast_reaction_strategy_coeff;
		}
	};


	////////// User-Defined Correctness Constraints on Functionality Scaling ///////////
	

	class SelectVariableID {
	public:
		int select_var_id;

		SelectVariableID(int select_var_id = -1)
			: select_var_id(select_var_id) { }
	};
	typedef SelectVariableID VID;

	//Constraint applies to the constructed Frame and all its dynamic sub-frames.
	//
	//  Use of VID in the Constraint applies to any select-Model with matching select_var_id
	//  that may get invoked within the current invocation of the constructed Frame (i.e. the
	//  Frame with which this Constraint is associated).
	//
	//   ASSUMPTION: Specified constraints should not be unsatisfiable. Unsatisfiable
	//     constraints will not be found to be so until evaluated on VIDs that relate to
	//     the unsatisfiability.
	//   Examples: 
	//      a) (x and y) and (x and not y), for x = (VID_1 > VID_2), y = (VID_3 > VID_4)
	//         is always unsatisfiable, but will evaluate to Unknown unless y takes a definite 0/1 value
	//
	//      b) (x or y) and (x or not y), for x = (VID_1 > VID_2), y = (VID_3 > VID_4)
	//         is unsatisfiable when x = 0, but will evaluate to Unknown until y = 0 or 1.
	//
	class Constraint {
		friend class ConstraintStructure;
		friend class ConstraintVerifier;
	public:
		//post-dependence: can be best-effort or required (FIXME: incorporate, maybe??)

		typedef enum {UNDEF, GT, GEQ, LT, LEQ, EQ, AND, OR, XOR, NOT} ConstraintType;

		Constraint()
			: type(UNDEF) { }

		Constraint(ConstraintType type, VID v1, VID v2) //Linear Constraint Constructor
			: type(type), v1(v1), v2(v2)
		{ assert(type == GT || type == GEQ || type == LT || type == LEQ || type == EQ); }

		Constraint(ConstraintType type, const Constraint& c1, const Constraint& c2) //Binary Logical Constraint Constructor
			: type(type)
		{
			assert(type == AND || type == OR || type == XOR);
			vConstraints.push_back(c1);
			vConstraints.push_back(c2);
		}

		Constraint(ConstraintType type, const Constraint& c) //Logical NOT Constraint constructor
			: type(type)
		{
			assert(type == NOT);
			vConstraints.push_back(c);
		}

		//read-only accessors
		ConstraintType get_type() const { return type; }
		VID get_v1() const
			{ assert(isLinear()); return v1; }
		VID get_v2() const
			{ assert(isLinear()); return v2; }
		std::vector<Constraint> get_vConstraints() const
			{ assert(isLogical() || vConstraints.size() == 0); return vConstraints; }

		//Categorize
		bool isLinear() const { return (type == GT || type == GEQ || type == LT || type == LEQ || type == EQ); }
		bool isLogical() const { return (type == AND || type == OR || type == XOR || type == NOT); }

	private:
		ConstraintType type;

		//defined for type == GT, GEQ, LT, LEQ, EQ
		VID v1;
		VID v2;

		//defined for type = AND, OR, NOT
		std::vector<Constraint> vConstraints;
	};

	Constraint operator>(VID v1, VID v2);
	Constraint operator>=(VID v1, VID v2);
	Constraint operator<(VID v1, VID v2);
	Constraint operator<=(VID v1, VID v2);
	Constraint operator==(VID v1, VID v2);
	Constraint operator&&(Constraint c1, Constraint c2);
	Constraint operator||(Constraint c1, Constraint c2);
	Constraint operator^(Constraint c1, Constraint c2);
	Constraint operator~(Constraint c);


	////////// Specify Performance Objectives ///////////

	typedef double ExecTime_t;
	typedef long long FrameID_t;

	typedef ExecTime_t (* ImpactRescaler_f)(ExecTime_t measured_execution_time_in_seconds);
		//User-defined function, that re-scales the measured execution time into a uniform scale
		// of its impact. Must be a pure function whose behavior is "stationary", i.e. independent
		// of context and time of invocation.
		//
		//This function must be a continuous map from measured_execution_time_in_seconds >= 0.0 to the
		//  range [0.0, infinity).
		//
		//Example: If frame-rate instead of frame-time is used to define Objective for a frame, then
		//  this function could be 0.001/t, or 0.001/(t + 0.000001) to avoid possibility of division-by-zero.
		//
		//Note: The mean and window of the corresponding Objective must be defined on the same scale as the
		//  range produced by the function above.

	class Objective {
	public:
		const bool isDefined;

		typedef enum {ObjABSOLUTE, ObjRELATIVE} Type_t;
		Type_t type;

		//defined iff type == ObjRELATIVE
		FrameID_t reference_frame_id;    //frame whose objective mean serves as reference value
		double relative_mean_frac;       //gives what fraction of the reference value is to be used as current objective mean


		ExecTime_t mean;                 //must be provided for type == ObjABSOLUTE, else it is calculated using
		                                 //   relative_mean_frac and the reference mean value

		double window_frac_lower;
		double window_frac_upper;

		double prob;
			//probability with which the associated execution time must be
			//  kept in the range [mean - window_lower, mean + window_upper], where window_[lower/upper] = mean * window_frac_[lower/upper]

		int sliding_window_size;

		ImpactRescaler_f impact_rescaler;

		Objective()
			: isDefined(false) { }

		Objective(ExecTime_t mean, double window_frac_lower, double window_frac_upper, double prob, int sliding_window_size = 1, ImpactRescaler_f impact_rescaler = 0)
			: isDefined(true), type(ObjABSOLUTE),
				reference_frame_id(-1), relative_mean_frac(0.0),
				mean(mean), window_frac_lower(window_frac_lower), window_frac_upper(window_frac_upper), prob(prob),
				sliding_window_size(sliding_window_size), impact_rescaler(impact_rescaler) { }

		Objective(FrameID_t reference_frame_id, double relative_mean_frac, double window_frac_lower, double window_frac_upper, double prob, int sliding_window_size = 1, ImpactRescaler_f impact_rescaler = 0)
			: isDefined(true), type(ObjRELATIVE),
				reference_frame_id(reference_frame_id), relative_mean_frac(relative_mean_frac),
				mean(0.0), window_frac_lower(window_frac_lower), window_frac_upper(window_frac_upper), prob(prob),
				sliding_window_size(sliding_window_size), impact_rescaler(impact_rescaler) { }

	};



	////////// Demarcate Frames ///////////
	


	//allocate Frame/ExecFrame as a 'static' object

	class BaseFrame {
	public:
		BaseFrame();

		virtual ~BaseFrame();

		typedef enum {FRAME, EXECFRAME} Type_t;
		virtual inline Type_t get_type() = 0;

		const FrameID_t id; //uniquely assigned ID, assigned automatically
	};

	class FrameInfo;

	class Frame : public BaseFrame {
		friend class FrameInfo;

	public:
		Frame();
		Frame(const Objective& obj);
		Frame(const Constraint& con);
		Frame(const Objective& obj, const Constraint& con);

		virtual ~Frame();

		virtual inline Type_t get_type() { return FRAME; }
		virtual inline bool isModule() { return false; }

	private:
		FrameInfo * frame_info;
	};


#if 0
	//A Module is like a Frame, except that constraints currently active outside the
	//  module are not applied within.
	//The "expose_constraints_funcs" parameter lists function-items. Any parts of currently
	//  active external constraints that involve the listed func-items are applied to the Module.
	class Module : public Frame {
	public:
		Module();
		Module(const Objective& obj);
		Module(const Constraint& con);
		//Module(const Objective& obj, const FuncList& expose_constraints_funcs);

		virtual ~Module();

		virtual inline bool isModule() { return true; }
	};
	//FIXME: Module is currently unimplemented
#endif


	//TERMINOLOGY
	//  Frame: dynamically demarcated component of an application, executed repeatedly
	//
	//  Frame invocation: each time the same frame is executed
	//
	//  Frame States:
	//    - Inactive: frame is not currently invoked, or has already completed previous invocation
	//    - Active
	//       + Executing: frame has been invoked, and currently executing application code falls under it
	//       + Suspended: frame has been invoked, but currently executing application does not fall under it
	//
	//  Events causing State Transition:
	//    - Enter
	//       + Start:  Inactive -> Executing
	//       + Resume: Suspended -> Executing
	//    - Exit
	//       + Suspend:  Executing -> Suspended
	//       + Complete: Active (Executing/Suspended) -> Inactive
	
	void frame_enter(FrameID_t frame_id);
		//Start fresh invocation or resume previously suspended invocation of frame.
		//Entered as sub-frame of most recently entered but not yet exited frame.

	void frame_enter(FrameID_t frame_id, FrameID_t chosen_parent_frame_id);
		//Enter as sub-frame of specified parent-frame, regardless of current frame.
		//Specified parent-frame must be currently executing.
		//
		//chosen_parent_frame_id = -1 implies that this frame is not contained inside any other frame
		//  i.e., this is a top-level frame

	ExecTime_t frame_exit_complete(FrameID_t frame_id);
		//Complete invocation of most recently entered frame. frame_id required to
		//   check correctness against programmer expectations.
		//Returns total active execution time of frame in seconds, cumulative over all suspended segments
	
	ExecTime_t frame_exit_suspend(FrameID_t frame_id);
		//Suspend execution of most recently entered frame. Frame is not completed yet.
		//The next frame_enter() on the same frame_id would continue (resume) execution of
		//   the same invocation of the frame.
		//
		//An invocation of a frame is finally completed when either one of the following occurs
		// - frame is exited using frame_exit_complete() call
		// - some parent-frame completes OR suspends, and therefore implies completion of
		//    current suspended frame
		//
		//Returns active execution time in frame since last resume
	
	//Query state of frame
	bool is_frame_active(FrameID_t frame_id);
		//returns true if active (executing or suspended), false if inactive or if frame isn't allocated
	
	bool is_frame_executing(FrameID_t frame_id);
		//returns true if executing, false if inactive or unallocated

	


	////////// Executable Frame ///////////

	//A ExecFrame is a frame that selectively invokes user-defined functions in order to
	//  maximize slack utilization, and meet the objectives of the frames that contain it.
	//
	//  User-defined model and scoped constraints restrict the selection and execution ordering of
	//     invoked functions in order to ensure correctness.

	class ExecFrameInfo;

	class ExecFrame : public BaseFrame {
		friend class ExecFrameInfo;

		const int stickiness_length;
			//Depending on the application, a model may have to hold its choices for multiple invocations before the choices
			//  starts to show its effect, or the effect of previous choices abates sufficiently. For models that exhibit
			//  such control-lag, the programmer can set stickiness_length to the minimum number of times insufficiently
			//  tested choices must be re-tried before they are rejected. Default value = 0 ( => no stickiness).

	public:
		ExecFrame();
		ExecFrame(const Model& model, int stickiness_length = 0);

		virtual ~ExecFrame();

		virtual inline Type_t get_type() { return EXECFRAME; }

		void run();
			//User must ensure that all Callers' likely to be invoked have been rebound
			//  to uninvoked Funcs, before each invocation to run()

		void force_default_selection(bool bEnable);
			//Turn on (true) or turn off (false) the picking of default model-choices.
			//  If the model run by this ExecFrame contains Select models with default_choice_index_for_select_var_id != -1
			//  then turning this option on forces only those choices to be picked (see description of Model for details).

		void force_fast_reaction_strategy_fixed_coeff(bool bEnable);
			//Turn on (true) or off (false) rescaling of the Fast Reaction Strategy coefficient.
			//  If the model run by this ExecFrame contains Select models with fast_reaction_strategy_coeff != 0.0
			//  then turning this option on makes the coefficient persistent with no rescaling;
			//   otherwise, fast_reaction_strategy_coeff only provides the initial coefficient which can then be rescaled
			//   (see description of Model for details).

	private:
		ExecFrameInfo * execframe_info;
	};



	////////// Collect Statistics ///////////
	
	class FrameStatistics {
	public:
		const FrameID_t frame_id;

		FrameStatistics(FrameID_t frame_to_track)
			: frame_id(frame_to_track),
			satisfaction_ratio_wrt_specified_objective(0.0), satisfaction_ratio_wrt_active_objective(0.0)
		{ }

		FrameStatistics& refresh();
			//Refreshes variables below with current statistics.
			//Nullifies the following variables if frame_id does not correspond to a defined Frame
			//Returns *this for chaining operations.

		std::string print_string() const;
			//Returns formatted statistics ready for printing




		//-- Statistic Variables --
		std::vector<ExecTime_t> vExecTime_bin_centers;
			//bin-centers quantizing execution-time for the Frame
		
		std::vector<double> vExecTime_bin_frequencies;
			//The occurence-counts of each of the bins for a Frame from program start.
			//The bins are identified by vExecTime_bin_centers and the corresponding occurence-counts form a histogram.
		
		std::vector<int> vSpecified_Objective_bin_indices;
			//Bin-indices that represent the Objective set for the Frame by the user. Empty if Frame has no Objective set by user.
		
		std::vector<int> vActive_Objective_bin_indices;
			//Bin-indices that are currently the objective for an Active Frame. These bins may be different from
			//  those in vSpecified_Objective_bin_indices, if a higher level Frame overrides specified objectives.
			//Empty when requested on a Frame that is not currently in Active state.
		

		double satisfaction_ratio_wrt_specified_objective;
			//Fraction of invocations of Frame that achieved the specified objective

		std::vector<long long> vFailure_Runlengths_wrt_specified_objective;
			//Element at index 'i' captures the number of runs-of-continuous-failures of lengths in interval (2^(i-1), 2^i]
			// where the failures were w.r.t. achieving the specified objective.


		double satisfaction_ratio_wrt_active_objective;
			//Fraction of invocations of Frame that achieved the active objective (whatever it was for that invocation)

		std::vector<long long> vFailure_Runlengths_wrt_active_objective;
			//Element at index 'i' captures the number of runs-of-continuous-failures of lengths in interval (2^(i-1), 2^i]
			// where the failures were w.r.t. achieving the active objective.
	};

	class ExecTime_vs_ModelDecision_Distribution {
	public:
		std::vector<ExecTime_t> vExecTime_bin_centers;
		std::vector<int> vModelChoices;

		std::vector< std::vector<double> > vvCounts;
			//Table: (i, j) entry gives the occurence-count of choice vModelChoices[j] under execution-bin vExecTime_bin_centers[i]
	};

	class ExecFrameStatistics {
	public:
		const FrameID_t execframe_id;

		ExecFrameStatistics(FrameID_t execframe_to_track)
			: execframe_id(execframe_to_track) { }

		ExecFrameStatistics& refresh();
			//Refreshes variables below with current statistics.
			//Nullifies the following variables if execframe_id does not correspond to a defined ExecFrame
			//Returns *this for chaining operations.

		std::string print_string();
			//Returns formatted statistics ready for printing


		//-- Statistic Variables --
		std::vector<FrameID_t> vTracking_FrameIDs; //FrameIDs for frames that are tracking the decision's of this execframe

		std::map<FrameID_t, ExecTime_vs_ModelDecision_Distribution> map_tracking_frame_to_exectime_distribution;
			//for each tracking frame in vTracking_FrameIDs, gives the corresponding correlation of model-choices with execution-times of that frame
	};



} //namespace Opp

#endif //OPP_H
