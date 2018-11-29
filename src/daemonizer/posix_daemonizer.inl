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
#ifdef GULPS_CAT_MAJOR
	#undef GULPS_CAT_MAJOR
	#define GULPS_CAT_MAJOR "posix_dmnzer"
#endif

#pragma once

#include "common/scoped_message_writer.h"
#include "common/util.h"
#include "daemonizer/posix_fork.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#define GULPS_PRINT_OK(...) GULPS_PRINT(__VA_ARGS__)

namespace daemonizer
{
namespace
{
const command_line::arg_descriptor<bool> arg_detach = {
	"detach", "Run as daemon"};
const command_line::arg_descriptor<std::string> arg_pidfile = {
	"pidfile", "File path to write the daemon's PID to (optional, requires --detach)"};
const command_line::arg_descriptor<bool> arg_non_interactive = {
	"non-interactive", "Run non-interactive"};
const command_line::arg_descriptor<std::string> arg_log_level = {
	"log-level", "Screen log level, 0-4 or categories", "0"};
const command_line::arg_descriptor<std::string> arg_file_level = {
	"log-file-level", "File log level, 0-4 or categories", "0"};
const command_line::arg_descriptor<std::string> arg_log_file = {
	"log-file", /*TODO tr?*/"Specify log file", /*TODO default_log_name*/ "deamonizer_log.txt"};
}


inline void init_options(
	boost::program_options::options_description &hidden_options, boost::program_options::options_description &normal_options)
{
	command_line::add_arg(normal_options, arg_detach);
	command_line::add_arg(normal_options, arg_pidfile);
	command_line::add_arg(normal_options, arg_non_interactive);
	command_line::add_arg(normal_options, arg_log_level);
	command_line::add_arg(normal_options, arg_file_level);
	command_line::add_arg(normal_options, arg_log_file);
}

inline boost::filesystem::path get_default_data_dir()
{
	return boost::filesystem::absolute(tools::get_default_data_dir());
}

inline boost::filesystem::path get_relative_path_base(
	boost::program_options::variables_map const &vm)
{
	return boost::filesystem::current_path();
}
//gulps_log_level log_scr, log_dsk; COMMENTED OUT BEACAUSE WE GOT "MULTIPLE DEFINITIONS OF" BOTH ERROR 
template <typename T_executor>
inline bool daemonize(
	int argc, char* argv[], T_executor &&executor // universal ref
	,
	boost::program_options::variables_map const &vm)
{	
	
	std::unique_ptr<gulps::gulps_output> file_out;
	/*if(!log_scr1.parse_cat_string(command_line::get_arg(vm, arg_log_level).c_str()))
	{
		GULPS_ERROR("Failed to parse filter string ", command_line::get_arg(vm, arg_log_level).c_str());
		return false;
	}

	if(!log_dsk1.parse_cat_string(command_line::get_arg(vm, arg_file_level).c_str()))
	{
		GULPS_ERROR("Failed to parse filter string ", command_line::get_arg(vm, arg_file_level).c_str());
		return false;
	}
	
	try
	{
		file_out = std::unique_ptr<gulps::gulps_output>(new gulps::gulps_async_file_output(command_line::get_arg(vm, arg_log_file)));
	}
	catch(const std::exception& ex)
	{
		GULPS_ERROR("Could not open file '", command_line::get_arg(vm, arg_log_file), "' error: ", ex.what());
		return false;
	}
	
	if(log_scr.is_active())
	{
		std::unique_ptr<gulps::gulps_output> out(new gulps::gulps_print_output(false, gulps::COLOR_WHITE));
		out->add_filter([](const gulps::message& msg, bool printed, bool logged) -> bool { 
				if(msg.out != gulps::OUT_LOG_0 && msg.out != gulps::OUT_USER_0)
					return false;
				if(printed)
					return false;
				return log_scr.match_msg(msg);
				});
		gulps::inst().add_output(std::move(out));
	}

	if(log_dsk.is_active())
	{
		file_out->add_filter([](const gulps::message& msg, bool printed, bool logged) -> bool { 
				if(msg.out != gulps::OUT_LOG_0 && msg.out != gulps::OUT_USER_0)
					return false;
				return log_dsk.match_msg(msg);
				});
		gulps::inst().add_output(std::move(file_out));
	}*/
	
	if(command_line::has_arg(vm, arg_detach))
	{
		GULPS_PRINT_OK("Forking to background...");
		std::string pidfile;
		if(command_line::has_arg(vm, arg_pidfile))
		{
			pidfile = command_line::get_arg(vm, arg_pidfile);
		}
		posix::fork(pidfile);
		auto daemon = executor.create_daemon(vm);
		return daemon.run();
	}
	else if(command_line::has_arg(vm, arg_non_interactive))
	{
		return executor.run_non_interactive(vm);
	}
	else
	{
		//GULPS_PRINT("Ryo '", RYO_RELEASE_NAME, " (", RYO_VERSION_FULL, ")");
		return executor.run_interactive(vm);
	}
}
}
