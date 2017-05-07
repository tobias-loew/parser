/**
 *   Copyright (C) 2017 Zach Laine
 *
 *   Distributed under the Boost Software License, Version 1.0. (See accompanying
 *   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <yaml/parser/flow_styles_def.hpp>
#include <iostream>
#include <fstream>
#include <string>

typedef std::string::const_iterator char_iterator_t;
template struct yaml::parser::flow_styles_t<char_iterator_t>;
