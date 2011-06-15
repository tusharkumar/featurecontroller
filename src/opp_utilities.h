// Copyright 2011, Tushar Kumar, Georgia Institute of Technology, under the 3-clause BSD license
//
// Author: Tushar Kumar, tushardeveloper@gmail.com

#ifndef OPP_UTILITIES_H
#define OPP_UTILITIES_H

#include <sstream>
#include <vector>
#include <string>

namespace Opp {

template<typename T>
	std::basic_ostream<char>& operator<<(std::basic_ostream<char>& bos, const std::vector<T>& vT)
{
	bos << "[";
	for(int i=0; i<(int)vT.size(); i++) {
		if(i > 0)
			bos << ", ";
		bos << vT[i];
	}
	bos << "]";
	return bos;
}

template<typename T>
	std::string vector_print_string(const std::vector<T>& vT)
{
	std::ostringstream oss;
	oss << vT;
	return oss.str();
}
	
	
}

#endif //OPP_UTILITIES_H
