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
	#define GULPS_CAT_MAJOR "win_dmnzer"
#endif

#pragma once

#include "common/util.h"
#include "daemonizer/windows_service.h"
#include "daemonizer/windows_service_runner.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <shlobj.h>

#define GULPS_PRINT_OK(...) GULPS_PRINT(__VA_ARGS__)

namespace daemonizer
{
namespace
{
const command_line::arg_descriptor<bool> arg_install_service = {
	"install-service", "Install Windows service"};
const command_line::arg_descriptor<bool> arg_uninstall_service = {
	"uninstall-service", "Uninstall Windows service"};
const command_line::arg_descriptor<bool> arg_start_service = {
	"start-service", "Start Windows service"};
const command_line::arg_descriptor<bool> arg_stop_service = {
	"stop-service", "Stop Windows service"};
const command_line::arg_descriptor<bool> arg_is_service = {
	"run-as-service", "Hidden -- true if running as windows service"};
	const command_line::arg_descriptor<std::string> arg_log_level = {
	"log-level", "Screen log level, 0-4 or categories", "0"};
const command_line::arg_descriptor<std::string> arg_file_level = {
	"log-file-level", "File log level, 0-4 or categories", "0"};
const command_line::arg_descriptor<std::string> arg_log_file = {
	"log-file", /*TODO tr?*/"Specify log file", /*TODO default_log_name*/ "deamonizer_log.txt"};

std::string get_argument_string(int argc, char* argv[])
{
	std::string result = "";
	for(int i = 1; i < argc; ++i)
	{
		result += " " + std::string{argv[i]};
	}
	return result;
}
}

inline void init_options(
	boost::program_options::options_description &hidden_options, boost::program_options::options_description &normal_options)
{
	command_line::add_arg(normal_options, arg_install_service);
	command_line::add_arg(normal_options, arg_uninstall_service);
	command_line::add_arg(normal_options, arg_start_service);
	command_line::add_arg(normal_options, arg_stop_service);
	command_line::add_arg(hidden_options, arg_is_service);
}

inline boost::filesystem::path get_default_data_dir()
{
	bool admin;
	if(!windows::check_admin(admin))
	{
		admin = false;
	}
	if(admin)
	{
		return boost::filesystem::absolute(
			tools::get_special_folder_path(CSIDL_COMMON_APPDATA, true) + "\\" + CRYPTONOTE_NAME);
	}
	else
	{
		return boost::filesystem::absolute(
			tools::get_special_folder_path(CSIDL_APPDATA, true) + "\\" + CRYPTONOTE_NAME);
	}
}

inline boost::filesystem::path get_relative_path_base(
	boost::program_options::variables_map const &vm)
{
	if(command_line::has_arg(vm, arg_is_service))
	{
		if(command_line::has_arg(vm, cryptonote::arg_data_dir))
		{
			return command_line::get_arg(vm, cryptonote::arg_data_dir);
		}
		else
		{
			return tools::get_default_data_dir();
		}
	}
	else
	{
		return boost::filesystem::current_path();
	}
}

gulps_log_level log_scr, log_dsk;
template <typename T_executor>
inline bool daemonize(
	int argc, char* argv[], T_executor &&executor // universal ref
	,
	boost::program_options::variables_map const &vm)
{
	std::string arguments = get_argument_string(argc, argv);

	std::unique_ptr<gulps::gulps_output> file_out;
	if(!log_scr.parse_cat_string(command_line::get_arg(vm, arg_log_level).c_str()))
	{
		GULPS_ERROR("Failed to parse filter string ". command_line::get_arg(vm, arg_log_level).c_str());
		return false;
	}

	if(!log_dsk.parse_cat_string(command_line::get_arg(vm, arg_file_level).c_str()))
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
	}
	
	if(command_line::has_arg(vm, arg_is_service))
	{
		// TODO - Set the service status here for return codes
		windows::t_service_runner<typename T_executor::t_daemon>::run(
			executor.name(), executor.create_daemon(vm));
		return true;
	}
	else if(command_line::has_arg(vm, arg_install_service))
	{
		if(windows::ensure_admin(arguments))
		{
			arguments += " --run-as-service";
			return windows::install_service(executor.name(), arguments);
		}
	}
	else if(command_line::has_arg(vm, arg_uninstall_service))
	{
		if(windows::ensure_admin(arguments))
		{
			return windows::uninstall_service(executor.name());
		}
	}
	else if(command_line::has_arg(vm, arg_start_service))
	{
		if(windows::ensure_admin(arguments))
		{
			return windows::start_service(executor.name());
		}
	}
	else if(command_line::has_arg(vm, arg_stop_service))
	{
		if(windows::ensure_admin(arguments))
		{
			return windows::stop_service(executor.name());
		}
	}
	else // interactive
	{
		//GULPS_PRINT("Ryo '" << RYO_RELEASE_NAME << "' (" << RYO_VERSION_FULL);
		return executor.run_interactive(vm);
	}

	return false;
}
}
