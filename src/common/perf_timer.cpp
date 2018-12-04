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
#define GULPS_CAT_MAJOR "perf_timer"


#include "perf_timer.h"
#include "misc_os_dependent.h"
#include <vector>

#include "common/gulps.hpp"


//#undef RYO_DEFAULT_LOG_CATEGORY
//#define RYO_DEFAULT_LOG_CATEGORY "perf"

namespace
{
uint64_t get_tick_count()
{
#if defined(__x86_64__)
	uint32_t hi, lo;
	__asm__ volatile("rdtsc"
					 : "=a"(lo), "=d"(hi));
	return (((uint64_t)hi) << 32) | (uint64_t)lo;
#else
	return epee::misc_utils::get_ns_count();
#endif
}

#ifdef __x86_64__
uint64_t get_ticks_per_ns()
{
	uint64_t t0 = epee::misc_utils::get_ns_count(), t1;
	uint64_t r0 = get_tick_count();

	while(1)
	{
		t1 = epee::misc_utils::get_ns_count();
		if(t1 - t0 > 1 * 1000000000)
			break; // work one second
	}

	uint64_t r1 = get_tick_count();
	uint64_t tpns256 = 256 * (r1 - r0) / (t1 - t0);
	return tpns256 ? tpns256 : 1;
}
#endif

#ifdef __x86_64__
uint64_t ticks_per_ns = get_ticks_per_ns();
#endif

uint64_t ticks_to_ns(uint64_t ticks)
{
#if defined(__x86_64__)
	return 256 * ticks / ticks_per_ns;
#else
	return ticks;
#endif
}
}

namespace tools
{

el::Level performance_timer_log_level = el::Level::Debug;

static __thread std::vector<PerformanceTimer *> *performance_timers = NULL;

void set_performance_timer_log_level(el::Level level)
{
	if(level != el::Level::Debug && level != el::Level::Trace && level != el::Level::Info && level != el::Level::Warning && level != el::Level::Error && level != el::Level::Fatal)
	{
		GULPS_ERRORF("Wrong log level: {}, using Debug", el::LevelHelper::convertToString(level));
		level = el::Level::Debug;
	}
	performance_timer_log_level = level;
}

PerformanceTimer::PerformanceTimer(const std::string &s, uint64_t unit, el::Level l) : name(s), unit(unit), level(l), started(false), paused(false)
{
	ticks = get_tick_count();
	if(!performance_timers)
	{	
		//TODO SHOULD WE LOG BASED ON GIVEN LEVEL?
		GULPS_LOG_L1("PERF             ----------");
		performance_timers = new std::vector<PerformanceTimer *>();
	}
	else
	{
		PerformanceTimer *pt = performance_timers->back();
		if(!pt->started && !pt->paused)
		{
			size_t size = 0;
			for(const auto *tmp : *performance_timers)
				if(!tmp->paused)
					++size;
			//TODO SHOULD WE LOG BASED ON GIVEN LEVEL?
			GULPS_LOGF_L1("PERF           {} {}", std::string((size - 1) * 2, ' '), pt->name);
			pt->started = true;
		}
	}
	performance_timers->push_back(this);
}

PerformanceTimer::~PerformanceTimer()
{
	performance_timers->pop_back();
	if(!paused)
		ticks = get_tick_count() - ticks;

	size_t size = 0;
	for(const auto *tmp : *performance_timers)
		if(!tmp->paused || tmp == this)
			++size;
	//TODO SHOULD WE LOG BASED ON GIVEN LEVEL?
	GULPS_LOGF_L1("PERF {}{} {}", ticks_to_ns(ticks) / (1000000000 / unit), std::string(size * 2, ' '), name);
	if(performance_timers->empty())
	{
		delete performance_timers;
		performance_timers = NULL;
	}
}

void PerformanceTimer::pause()
{
	if(paused)
		return;
	ticks = get_tick_count() - ticks;
	paused = true;
}

void PerformanceTimer::resume()
{
	if(!paused)
		return;
	ticks = get_tick_count() - ticks;
	paused = false;
}
}
