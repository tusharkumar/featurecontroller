// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#ifndef OPP_CONSTRAINT_H
#define OPP_CONSTRAINT_H

#include "opp.h"

#include <set>
#include <map>

namespace Opp {

	class LogicValue { //3 valued logic
		typedef enum {
			0, //Logic 0 or FALSE
			1, //Logic 1 or TRUE
			2, //Logic Don't care
		} Value_e;

		Value_e value;

		LogicValue(Value_e lv)
			: value(lv) { }

		bool isDefinite() const
		{ return (value == 0 || value == 1); }

		bool isUnknown() const 
		{ return (value == 2); }
	};

	LogicValue AND(const LogicValue& lv1, const LogicValue& lv2);
	LogicValue OR(const LogicValue& lv1, const LogicValue& lv2);
	LogicValue XOR(const LogicValue& lv1, const LogicValue& lv2);
	LogicValue NOT(const LogicValue& lv);

	class ConstraintStructure {
	public:
		Constraint::Type_e type;

		 //defined iff type == AND, OR, XOR, NOT. Must zero elements otherwise.
		std::vector<ConstraintStructure> vTreeChildren;

		//defined iff type == GT, GEQ, LT, LEQ, EQ
		int v1; 
		int v2;

		bool isLinear() const
			{ return (type == Constraint::GT || type == Constraint::GEQ || type == Constraint::LT
					|| type == Constraint::LEQ || type == Constraint::EQ); }

		bool isLogical() const
			{ return (type == Constraint::AND || type == Constraint::OR
					|| type == Constraint::XOR || type == Constraint::NOT); }


		std::set<int> sContainedVIDs;
			//VIDs occuring in this subtree

		bool bIsEvaluated; //is this subtree of ConstraintStructure already evaluated?

		bool bEvaluationResult; //defined iff bIsEvaluated == true


		//UNDEF
		ConstraintStructure()
			: type(Constraint::UNDEF), v1(-1), v2(-1), bIsEvaluated(true), bEvaluationResult(true) { }

		//Linear Constraint
		ConstraintStructure(Constraint::Type_e type, int v1, int v2)
			: type(type), v1(v1), v2(v2), bIsEvaluated(false), bEvaluationResult(false)
		{
			assert(type == Constraint::GT || type == Constraint::GEQ || type == Constraint::LT
					|| type == Constraint::LEQ || type == Constraint::EQ);

			sContainedVIDs.push_back(v1);
			sContainedVIDs.push_back(v2);
		}

		//Logical Binary
		ConstraintStructure(Constraint::Type_e type, const ConstraintStructure& cs1, const ConstraintStructure& cs2)
			: type(type), v1(-1), v2(-1), bIsEvaluated(false), bEvaluationResult(false)
		{
			assert(isLogical() && type != Constraint::NOT);

			vTreeChildren.push_back(cs1);
			vTreeChildren.push_back(cs2);

			std::set_union(
				cs1.sContainedVIDs.begin(), cs1.sContainedVIDs.end(),
				cs2.sContainedVIDs.begin(), cs2.sContainedVIDs.end(),
				sContainedVIDs.begin()
			);
		}

		//Logical NOT
		ConstraintStructure(Constraint::Type_e type, const ConstraintStructure& cs)
			: type(type), v1(-1), v2(-1), bIsEvaluated(false), bEvaluationResult(false)
		{
			assert(type == Constraint::NOT);

			vTreeChildren.push_back(cs);

			sContainedVIDs = cs.sContainedVIDs;
		}

		ConstraintStructure(const Constrain& constraint)
		{ *this = recursively_deconstruct_constraint(constraint); }

		void clear_cache() {
			if(type != Constraint::UNDEF) {
				bIsEvaluated = false;
				bEvaluationResult = false;
				for(int i=0; i<(int)vTreeChildren.size(); i++)
					vTreeChildren[i].clear_cache();
			}
		}

	private:
		ConstraintStructure recursively_deconstruct_constraint(const Constraint& con_part) {
			switch(con_part.type) {
				case UNDEF:
					return ConstraintStructure();
					break;
				case GT:
				case GEQ:
				case LT:
				case LEQ:
				case EQ:
					return ConstraintStructure(con_part.type, con_part.v1.select_var_id, con_part.v2.select_var_id);
					break;
				case AND:
				case OR:
				case XOR:
					return ConstraintStructure(
						con_part.type,
						recursively_deconstruct_constraint(con_part.vConstraints.at(0)),
						recursively_deconstruct_constraint(con_part.vConstraints.at(1))
					);
					break;
				case NOT:
					return ConstraintStructure(
						con_part.type,
						recursively_deconstruct_constraint(con_part.vConstraints.at(0))
					);
					break;
				default:
					assert(0);
					break;
			}
			return ConstraintStructure();
		}
	};

	class ConstraintVerifier {
	public:
		const Constraint con;
			//Constraint imposed on corresponding Frame

		ConstraintStructure constraint_structure;


		std::map< int, std::set<int> > map_VID_to_ValueObservations;
			//History of values observed so far for each select_var_id that occurs in a Model directly under
			//  corresponding Frame's scope, History is retained only for the progress made so far in the
			//  current invocation of the Frame.
			//
			//This history is used to verify the correctness of any new choice-value being evaluated
			//  for a Model within the dynamic scope of the current frame. The correctness is verified
			//  against the constraint imposed by this Frame. Therefore, the history only needs to record
			//  values for VIDs being tracked as captured by the root of constraint_structure.


		ConstraintVerifier(const Constraint& con)
			: con(con), constraint_structure(con) { }

		void clear_verifier_history() {
			constraint_structure.clear_cache();
			map_VID_to_ValueObservations.clear();
		}

		//Fast Verfication Strategy
		//1. Assume that existing history does not violate Constraint (inductive correctness)
		//   And then nothing that violates the constraint is allowed to be added to history.
		//
		//2. Lazy Evaluation: Evaluate only those clauses in the constraint that involve the
		//    given select_var_id. Evaluate other clauses only if they are needed to establish
		//    satisfaction of the overall constraint. (FIXME: not implemented)
		//
		//3. Cache evaluation results of clauses

		bool verify_decisions(const std::map<int, int>& map_DecisionVID_to_Value) {
			std::set<int> sDecisionVIDs;
			for(std::map<int, int>::const_iterator mit = map_DecisionVID_to_Value.begin();
					mit != map_DecisionVID_to_Value.end();
					mit++
			)
			{ sDecisionVIDs.insert(mit->first); }

			std::set<int> common_VIDs;
			std::set_intersection(constraint_structure.sContainedVIDs.begin(), constraint_structure.sContainedVIDs.end(),
					sDecisionVIDs.begin(), sDecisionVIDs.end(), common_VIDs.begin());
			if(common_VIDs.size() == 0) //no DecisionVIDs occur in this constraint
				return true; //this constraint does not affect validity of values chosen for given VIDs

			return recursive_verify(common_VIDs, map_DecisionVID_to_Value, constraint_structure);
		}

		bool recursive_verify(
			const std::set<int>& sRelevent_DecisionVIDs,
			const std::map<int, int>& map_DecisionVID_to_Value,
			const ConstraintStructure& cs
		) {
			if
		}

	private:
		void add_observation(int select_var_id, int observed_value) {
			if(constraint_structure.sContainedVIDs.count(select_var_id) > 0) { //VID is being tracked by this frame
				if(map_VID_to_ValueObservations.count(select_var_id) == 0)
					map_VID_to_ValueObservations[select_var_id] = std::set<int>();
				map_VID_to_ValueObservations[select_var_id].insert(observed_value);
			}
		}


		bool verifier(ConstraintStructure& cs, int select_var_id, int chosen_value, bool bUpdateVerifierCache) {
			if(cs.sContainedVIDs.count(select_var_id) == 0) { //select_var_id does not occur is subtree
				if(cs.bIsEvaluated)
					return cs.bEvaluationResult;
				else
					return true; //Lazy assumption: not evaluated, but does not involve select_var_id ==> assume satisfiable
			}

			//Now, select_var_id occurs in this subtree

			bool bResult = true;
			if(cs.isLogical()) {
				bool bSubtree1 = lazy_verifier(cs.vTreeChildren.at(0), select_var_id, chosen_value, bUpdateVerifierCache);
				bool bSubtree2 = true;
				if(cs.type != Constraint::NOT)
					bSubtree2 = lazy_verifier(cs.vTreeChildren.at(1), select_var_id, chosen_value, bUpdateVerifierCache);

				if(cs.type == Constraint::AND)
					bResult = bSubtree1 and bSubtree2;
				else if(cs.type == Constraint::OR)
					bResult = bSubtree1 or bSubtree2;
				else //XOR or NOT
					bResult = (bSubtree1 and not bSubtree2) or (not bSubtree1 and bSubtree2);
			}

			else if(cs.isLinear()) {
				std::set<int> value1_set;
				std::set<int> value2_set;

				if(select_var_id == v1) {
					value1_set.insert(chosen_value);
					value2_set = map_VID_to_ValueObservations[v2];
				}
				else if(select_var_id == v2) {
					value1_set = map_VID_to_ValueObservations[v1];
					value2_set.insert(chosen_value);
				}
				else
				{ assert(0); }

				bResult = true; //assume true, unless an explicit counter-example is found
				for(std::set<int>::iterator sit1 = value1_set.begin(); sit1 != value1_set.end(); sit1++) {
					for(std::set<int>::iterator sit2 = value2_set.begin(); sit2 != value2_set.end(); sit2++) {
						if(cs.type == Constraint::GT)
							bResult = (*sit1) > (*sit2);
						else if(cs.type == Constraint::GEQ)
							bResult = (*sit1) >= (*sit2);
						else if(cs.type == Constraint::LT)
							bResult = (*sit1) < (*sit2);
						else if(cs.type == Constraint::LEQ)
							bResult = (*sit1) <= (*sit2);
						else if(cs.type == Constraint::EQ)
							bResult = (*sit1) == (*sit2);
						else
							assert(0);

						if(bResult == false)
							break; //test failed
					}
				}
			}

			if(bUpdateVerifierCache) {
				cs.bIsEvaluated = true;
				cs.bEvaluationResult = bResult;
			}

			return bResult;
		}

	};

}

#endif //OPP_CONSTRAINT_H
