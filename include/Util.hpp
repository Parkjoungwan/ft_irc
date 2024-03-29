#ifndef	UTIL_HPP
#define	UTIL_HPP

#include <vector>
#include <iostream>

void						print_stringVector(std::vector<std::string> v);
std::string					appendStringColon(size_t startIndex, std::vector<std::string> msg);
std::vector<std::string>	split(std::string &line, std::string s);

#endif
