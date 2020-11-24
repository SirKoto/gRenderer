#pragma once

#include "Fibers/Job.h"
#include "Fibers/Counter.h"

namespace gr
{
namespace grjob
{

enum class Priority {
	eHigh,
	eMid,
	eLow
};

void createSystem(uint32_t maxThreads);

void destroySystem();

void startRunningJobSystem();

void stopRunningJobSystem();

void runJob(Priority priority, const Job& job, Counter** pCounter = nullptr, bool needsBigStack = false);

void runJobBatch(Priority priority, const Job* jobs, uint32_t numJobs, Counter** pCounter = nullptr);

void runJobOnMainThread(const Job& job, Counter** pCounter = nullptr, bool needsBigStack = false);

void waitForCounterAndFree(const Counter* counter, uint32_t value);

void waitForCounter(const Counter* counter, uint32_t value);

void setExceptionCatch(void(*function)(const std::exception&));


} // namespace grjob

} // namespace gr
