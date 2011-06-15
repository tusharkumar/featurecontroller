// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#include "opp.h"

namespace Opp {


Constraint operator>(VID v1, VID v2)
{ return Constraint(Constraint::GT, v1, v2); }

Constraint operator>=(VID v1, VID v2)
{ return Constraint(Constraint::GEQ, v1, v2); }

Constraint operator<(VID v1, VID v2)
{ return Constraint(Constraint::LT, v1, v2); }

Constraint operator<=(VID v1, VID v2)
{ return Constraint(Constraint::LEQ, v1, v2); }

Constraint operator==(VID v1, VID v2)
{ return Constraint(Constraint::EQ, v1, v2); }

Constraint operator&&(Constraint c1, Constraint c2)
{ return Constraint(Constraint::AND, c1, c2); }

Constraint operator||(Constraint c1, Constraint c2)
{ return Constraint(Constraint::OR, c1, c2); }

Constraint operator^(Constraint c1, Constraint c2)
{ return Constraint(Constraint::XOR, c1, c2); }

Constraint operator~(Constraint c)
{ return Constraint(Constraint::NOT, c); }



//3 valued logic operations

LogicValue AND(const LogicValue& lv1, const LogicValue& lv2) {
	if(lv1.value == 0 or lv2.value == 0)
		return LogicValue(0);
	if(lv1.value == 1 and lv2.value == 1)
		return LogicValue(1);
	return LogicValue(2);
}

LogicValue OR(const LogicValue& lv1, const LogicValue& lv2) {
	if(lv1.value == 1 or lv2.value == 1)
		return LogicValue(1);
	if(lv1.value == 0 and lv2.value == 0)
		return LogicValue(0);
	return LogicValue(2);
}

LogicValue XOR(const LogicValue& lv1, const LogicValue& lv2) {
	if(lv1.isUnknown() or lv2.isUnknown())
		return LogicValue(2);

	//Now, lv1 and lv2 both have definite values (i.e. 0 or 1)
	return LogicValue(lv1.value ^ lv2.value);
}

LogicValue NOT(const LogicValue& lv) {
	if(lv.isUnknown())
		return LogicValue(2);

	return LogicValue(1 - lv.value);
}

}; //namespace Opp
