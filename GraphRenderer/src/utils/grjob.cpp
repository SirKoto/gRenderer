#include "grjob.h"

#include "Fibers/FScheduler.h"

namespace gr {
namespace grjob {


typedef std::aligned_storage<sizeof(FScheduler)>::type SchedulerStorage;
SchedulerStorage scheduler;

void createSystem(uint32_t maxThreads)
{
	new(&scheduler) FScheduler(maxThreads);
}

void destroySystem()
{
	reinterpret_cast<FScheduler&>(scheduler).~FScheduler();
}

void startRunningJobSystem()
{
	reinterpret_cast<FScheduler&>(scheduler).startJobSystem();
}

void stopRunningJobSystem()
{
	reinterpret_cast<FScheduler&>(scheduler).stopSystem();
}

uint32_t getNumThreads()
{
	return reinterpret_cast<FScheduler&>(scheduler).getNumThreads();
}

uint32_t getThreadId()
{
	return FScheduler::getThreadId();
}

void runJob(Priority priority, const Job& job, Counter** pCounter, bool needsBigStack)
{
	FScheduler::scheduleJob(priority, job, pCounter, needsBigStack);
}

void runJobBatch(Priority priority, const Job* jobs, uint32_t numJobs, Counter** pCounter)
{
	FScheduler::scheduleBatch(priority, jobs, numJobs, pCounter);
}

void runJobOnMainThread(const Job& job, Counter** pCounter, bool needsBigStack)
{
	FScheduler::scheduleJobForMainThread(job, pCounter, needsBigStack);
}

void waitForCounterAndFree(const Counter* counter, uint32_t value)
{
	FScheduler::waitForCounterAndFree(counter, value);
}

void waitForCounter(const Counter* counter, uint32_t value)
{
	FScheduler::waitForCounter(counter, value);
}

void setExceptionCatch(void(*function)(const std::exception&))
{
	reinterpret_cast<FScheduler&>(scheduler).setExceptionCatch(function);
}

} // namespace grjob

} // namespace gr