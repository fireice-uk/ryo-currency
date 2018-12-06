// Copyright (c) 2018, Ryo Currency Project
// Portions copyright (c) 2014-2018, The Monero Project
//
// Portions of this file are available under BSD-3 license. Please see ORIGINAL-LICENSE for details
// All rights reserved.
//
// Authors and copyright holders give permission for following:
//
// 1. Redistribution and use in source and binary forms WITHOUT modification.
//
// 2. Modification of the source form for your own personal use.
//
// As long as the following conditions are met:
//
// 3. You must not distribute modified copies of the work to third parties. This includes
//    posting the work online, or hosting copies of the modified work for download.
//
// 4. Any derivative version of this work is also covered by this license, including point 8.
//
// 5. Neither the name of the copyright holders nor the names of the authors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// 6. You agree that this licence is governed by and shall be construed in accordance
//    with the laws of England and Wales.
//
// 7. You agree to submit all disputes arising out of or in connection with this licence
//    to the exclusive jurisdiction of the Courts of England and Wales.
//
// Authors and copyright holders agree that:
//
// 8. This licence expires and the work covered by it is released into the
//    public domain on 1st of February 2019
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers
#ifdef GULPS_CAT_MAJOR
    #undef GULPS_CAT_MAJOR
#endif
#define GULPS_CAT_MAJOR "cmd_line"

#pragma once

#include <array>
#include <functional>
#include <iostream>
#include <sstream>
#include <type_traits>

#include "include_base_utils.h"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "common/gulps.hpp"


namespace command_line
{

//! \return True if `str` is `is_iequal("y" || "yes" || `tr("yes"))`.
bool is_yes(const std::string &str);
//! \return True if `str` is `is_iequal("n" || "no" || `tr("no"))`.
bool is_no(const std::string &str);

template <typename T, bool required = false, bool dependent = false, int NUM_DEPS = 1>
struct arg_descriptor;

template <typename T>
struct arg_descriptor<T, false>
{
	typedef T value_type;

	const char *name;
	const char *description;
	T default_value;
	bool not_use_default;
};

template <typename T>
struct arg_descriptor<std::vector<T>, false>
{
	typedef std::vector<T> value_type;

	const char *name;
	const char *description;
};

template <typename T>
struct arg_descriptor<T, true>
{
	static_assert(!std::is_same<T, bool>::value, "Boolean switch can't be required");

	typedef T value_type;

	const char *name;
	const char *description;
};

template <typename T>
struct arg_descriptor<T, false, true>
{
	typedef T value_type;

	const char *name;
	const char *description;

	T default_value;

	const arg_descriptor<bool, false> &ref;
	std::function<T(bool, bool, T)> depf;

	bool not_use_default;
};

template <typename T, int NUM_DEPS>
struct arg_descriptor<T, false, true, NUM_DEPS>
{
	typedef T value_type;

	const char *name;
	const char *description;

	T default_value;

	std::array<const arg_descriptor<bool, false> *, NUM_DEPS> ref;
	std::function<T(std::array<bool, NUM_DEPS>, bool, T)> depf;

	bool not_use_default;
};

template <typename T>
boost::program_options::typed_value<T, char> *make_semantic(const arg_descriptor<T, true> & /*arg*/)
{
	return boost::program_options::value<T>()->required();
}

template <typename T>
boost::program_options::typed_value<T, char> *make_semantic(const arg_descriptor<T, false> &arg)
{
	auto semantic = boost::program_options::value<T>();
	if(!arg.not_use_default)
		semantic->default_value(arg.default_value);
	return semantic;
}

template <typename T>
boost::program_options::typed_value<T, char> *make_semantic(const arg_descriptor<T, false, true> &arg)
{
	auto semantic = boost::program_options::value<T>();
	if(!arg.not_use_default)
	{
		std::ostringstream format;
		format << arg.depf(false, true, arg.default_value) << ", "
			   << arg.depf(true, true, arg.default_value) << " if '"
			   << arg.ref.name << "'";
		semantic->default_value(arg.depf(arg.ref.default_value, true, arg.default_value), format.str());
	}
	return semantic;
}

template <typename T, int NUM_DEPS>
boost::program_options::typed_value<T, char> *make_semantic(const arg_descriptor<T, false, true, NUM_DEPS> &arg)
{
	auto semantic = boost::program_options::value<T>();
	if(!arg.not_use_default)
	{
		std::array<bool, NUM_DEPS> depval;
		depval.fill(false);
		std::ostringstream format;
		format << arg.depf(depval, true, arg.default_value);
		for(size_t i = 0; i < depval.size(); ++i)
		{
			depval.fill(false);
			depval[i] = true;
			format << ", " << arg.depf(depval, true, arg.default_value) << " if '" << arg.ref[i]->name << "'";
		}
		for(size_t i = 0; i < depval.size(); ++i)
			depval[i] = arg.ref[i]->default_value;
		semantic->default_value(arg.depf(depval, true, arg.default_value), format.str());
	}
	return semantic;
}

template <typename T>
boost::program_options::typed_value<T, char> *make_semantic(const arg_descriptor<T, false> &arg, const T &def)
{
	auto semantic = boost::program_options::value<T>();
	if(!arg.not_use_default)
		semantic->default_value(def);
	return semantic;
}

template <typename T>
boost::program_options::typed_value<std::vector<T>, char> *make_semantic(const arg_descriptor<std::vector<T>, false> & /*arg*/)
{
	auto semantic = boost::program_options::value<std::vector<T>>();
	semantic->default_value(std::vector<T>(), "");
	return semantic;
}

template <typename T, bool required, bool dependent, int NUM_DEPS>
void add_arg(boost::program_options::options_description &description, const arg_descriptor<T, required, dependent, NUM_DEPS> &arg, bool unique = true)
{
	if(0 != description.find_nothrow(arg.name, false))
	{
		CHECK_AND_ASSERT_MES(!unique, void(), "Argument already exists: " << arg.name);
		return;
	}

	description.add_options()(arg.name, make_semantic(arg), arg.description);
}

template <typename T>
void add_arg(boost::program_options::options_description &description, const arg_descriptor<T, false> &arg, const T &def, bool unique = true)
{
	if(0 != description.find_nothrow(arg.name, false))
	{
		CHECK_AND_ASSERT_MES(!unique, void(), "Argument already exists: " << arg.name);
		return;
	}

	description.add_options()(arg.name, make_semantic(arg, def), arg.description);
}

template <>
inline void add_arg(boost::program_options::options_description &description, const arg_descriptor<bool, false> &arg, bool unique)
{
	if(0 != description.find_nothrow(arg.name, false))
	{
		CHECK_AND_ASSERT_MES(!unique, void(), "Argument already exists: " << arg.name);
		return;
	}

	description.add_options()(arg.name, boost::program_options::bool_switch(), arg.description);
}

template <typename charT>
boost::program_options::basic_parsed_options<charT> parse_command_line(int argc, const charT *const argv[],
																	   const boost::program_options::options_description &desc, bool allow_unregistered = false)
{
	auto parser = boost::program_options::command_line_parser(argc, argv);
	parser.options(desc);
	if(allow_unregistered)
	{
		parser.allow_unregistered();
	}
	return parser.run();
}

template <typename F>
bool handle_error_helper(const boost::program_options::options_description &desc, F parser)
{
	try
	{
		return parser();
	}
	catch(const std::exception &e)
	{
		GULPS_ERRORF( "Failed to parse arguments: {}", e.what());
		GULPS_ERROR(desc);
		return false;
	}
	catch(...)
	{
		GULPS_ERROR( "Failed to parse arguments: unknown exception");
		GULPS_ERROR(desc);
		return false;
	}
}

template <typename T, bool required, bool dependent, int NUM_DEPS>
typename std::enable_if<!std::is_same<T, bool>::value, bool>::type has_arg(const boost::program_options::variables_map &vm, const arg_descriptor<T, required, dependent, NUM_DEPS> &arg)
{
	auto value = vm[arg.name];
	return !value.empty();
}

template <typename T, bool required, bool dependent, int NUM_DEPS>
bool is_arg_defaulted(const boost::program_options::variables_map &vm, const arg_descriptor<T, required, dependent, NUM_DEPS> &arg)
{
	return vm[arg.name].defaulted();
}

template <typename T>
T get_arg(const boost::program_options::variables_map &vm, const arg_descriptor<T, false, true> &arg)
{
	return arg.depf(get_arg(vm, arg.ref), is_arg_defaulted(vm, arg), vm[arg.name].template as<T>());
}

template <typename T, int NUM_DEPS>
T get_arg(const boost::program_options::variables_map &vm, const arg_descriptor<T, false, true, NUM_DEPS> &arg)
{
	std::array<bool, NUM_DEPS> depval;
	for(size_t i = 0; i < depval.size(); ++i)
		depval[i] = get_arg(vm, *arg.ref[i]);
	return arg.depf(depval, is_arg_defaulted(vm, arg), vm[arg.name].template as<T>());
}

template <typename T, bool required>
T get_arg(const boost::program_options::variables_map &vm, const arg_descriptor<T, required> &arg)
{
	return vm[arg.name].template as<T>();
}

template <bool dependent, int NUM_DEPS>
inline bool has_arg(const boost::program_options::variables_map &vm, const arg_descriptor<bool, false, dependent, NUM_DEPS> &arg)
{
	return get_arg(vm, arg);
}

#ifdef WIN32
bool get_windows_args(std::vector<char*>& argptrs);
void set_console_utf8();
#endif

extern const arg_descriptor<bool> arg_help;
extern const arg_descriptor<bool> arg_version;
}
