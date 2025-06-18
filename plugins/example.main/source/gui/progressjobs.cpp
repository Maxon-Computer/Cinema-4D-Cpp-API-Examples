// example code for a menu plugin and multiprocessing
#include "maxon/backgroundentry.h"
#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"

using namespace cinema;

class ProgressTest : public CommandData
{
public:
	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager);
	virtual Bool ExecuteOptionID(BaseDocument* doc, Int32 plugid, Int32 subid, GeDialog* parentManager);

	static maxon::Result<void>	ComputationalTaskWithProgress(maxon::JobRef* jobRef, Bool spinning);

private:
};

// Atomic (thread safe) integer, storing global entry count added by this example
static maxon::AtomicInt g_globalCount;

maxon::Result<void> ProgressTest::ComputationalTaskWithProgress(maxon::JobRef* jobRef, Bool spinning)
{
	iferr_scope;

	// Retrieves the Task Manager
	maxon::BackgroundProgressRef backgroundJobs = maxon::BackgroundProgressInterface::GetMaster();

	// Increments the number of entry
	Int newId = g_globalCount.SwapIncrement() + 1;

	// Adds to the Task Manager, a new entry visible to the user
	maxon::BackgroundProgressRef backgroundJob = backgroundJobs.AddActiveEntry(FormatString("SDK Async Job Demo @", newId)) iferr_return;

	// Delete the entry once it's done, finally will be executed even in case of error.
	finally
	{
		backgroundJobs.RemoveActiveEntry(backgroundJob) iferr_ignore("ignore on destruction");
	};

	// Setting this delegate allows the user to cancel an entry in the task manager.
	// In this delegate, we cancel the execution of the current job.
	backgroundJob.SetCancelJobDelegate(
		[jobRef](const maxon::ProgressRef& job) mutable -> maxon::Result<void>
		{
			jobRef->Cancel();
			return maxon::OK;
		}) iferr_return;

	// Simulates a processing by doing a 20sec loop
	MAXON_SCOPE
	{
		maxon::TimeValue time = maxon::TimeValue::GetTime();

		// random time between 1 and 29 seconds
		Random rnd;
		UInt32 seed = (UInt32)time.GetMicroseconds();
		rnd.Init(seed);
		maxon::TimeValue duration = maxon::Seconds(rnd.Get01() * 29.0 + 1.0);

		// Adds a new job, to the visible entry. This is technically not mandatory in this case, 
		// since we only add one job. By default there is always at least one job in an entry.
		// This default job have an index of zero and a weight of one.
		const Float jobWeight = 1.0;
		Int backgroundJobIdx = backgroundJob.AddProgressJob(jobWeight, FormatString("SDK Demo Progress (@{s})", duration)) iferr_return;

		Float progress = 0.0;

		// Loops, if the progress is not 100% or the user did not canceled the task from the Task Manager
		while (!jobRef->IsCancelled() && progress < 1.0)
		{
			progress = (maxon::TimeValue::GetTime() - time) / duration;

			// Updates the progress of the current job, and taking into account the weight allocated to this job,
			// the general progress bar of the entry is updated in the user interface
			backgroundJob.SetProgressAndCheckBreak(backgroundJobIdx, spinning ? maxon::UNKNOWNPROGRESS:progress) iferr_return;
			jobRef->Wait(maxon::Milliseconds(100));

			// Retrieve if the WARNING and ERROR bit is set in the entry state
			Bool isCurrentStateWarning = Int(backgroundJob.GetState()) & Int(maxon::BackgroundEntryInterface::STATE::WARNING);
			Bool isCurrentStateError =	 Int(backgroundJob.GetState()) & Int(maxon::BackgroundEntryInterface::STATE::ERROR);

			// Defines a warning if the task is longer than 15 sec
			if (!isCurrentStateWarning && (maxon::TimeValue::GetTime() - time) > maxon::Seconds(15))
			{
				backgroundJob.AddState(maxon::BackgroundEntryInterface::STATE::WARNING, "Function took longer than 15 seconds"_s) iferr_return;
			}

			// Defines an error if the task is longer than 24 sec
			if (!isCurrentStateError && (maxon::TimeValue::GetTime() - time) > maxon::Seconds(24))
			{
				backgroundJob.AddState(maxon::BackgroundEntryInterface::STATE::ERROR, "Function took longer than 24 seconds"_s) iferr_return;
			}
		}
	}

	return maxon::OK;
}

Bool ProgressTest::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	iferr_scope_handler
	{
		DiagnosticOutput("@", err);
		return false;
	};



	// Background task are scheduled via jobs. Such job will be then enqueue to a pool of jobs.
	// Cinema 4D will then execute all jobs of the pools.
	// Since the execution of the job, will be done when Cinema 4D want, and not directly 
	// all variables from the current scope, must live enough time. 
	// Thus, we use a StrongReference which guarantees that the job will be available 
	// for as long as necessary and will be released when no more references
	// for that job are present in Cinema 4D.
	maxon::StrongRef<maxon::JobRef> jobRef = NewObj(maxon::JobRef) iferr_return;

	// Adds the lambda to the background job queue.
	// Enqueue returns the jobRef, the lambda will be executed on, 
	// Therefor we move this jobRef to our StrongRef<JobRef> ensuring this 
	// returned job will live as long as needed (aka up until the lambda is executed)
	*jobRef = maxon::JobRef::Enqueue(
		[jobRef]() mutable -> maxon::Result<void>
		{
			iferr_scope;

			const Bool spinning = false;
			ComputationalTaskWithProgress(jobRef, spinning) iferr_return;

			// Removes the reference of the job. Since we are not using it anymore
			// Cinema 4D will be able to free the reference.
			jobRef = nullptr;
			return maxon::OK;
		}) iferr_return;

	return true;
}

Bool ProgressTest::ExecuteOptionID(BaseDocument* doc, Int32 plugid, Int32 subid, GeDialog* parentManager)
{
	iferr_scope_handler
	{
		DiagnosticOutput("@", err);
		return false;
	};

	// Background task are scheduled via jobs. Such job will be then enqueue to a pool of jobs.
	// Cinema 4D will then execute all jobs of the pools.
	// Since the execution of the job, will be done when Cinema 4D want, and not directly 
	// all variables from the current scope, must live enough time. 
	// Thus, we use a StrongReference which guarantees that the job will be available 
	// for as long as necessary and will be released when no more references
	// for that job are present in Cinema 4D.
	maxon::StrongRef<maxon::JobRef> jobRef = NewObj(maxon::JobRef) iferr_return;

	// Adds the lambda to the background job queue.
	// Enqueue returns the jobRef, the lambda will be executed on, 
	// Therefor we move this jobRef to our StrongRef<JobRef> ensuring this 
	// returned job will live as long as needed (aka up until the lambda is executed)
	*jobRef = maxon::JobRef::Enqueue(
		[jobRef]() mutable -> maxon::Result<void>
	{
		iferr_scope;

		const Bool spinning = true;
		ComputationalTaskWithProgress(jobRef, spinning) iferr_return;

		// Removes the reference of the job. Since we are not using it anymore
		// Cinema 4D will be able to free the reference.
		jobRef = nullptr;
		return maxon::OK;
	}) iferr_return;

	return true;
}

Bool RegisterProgressTest()
{
	// be sure to use a unique ID obtained from developers.maxon.net
	return RegisterCommandPlugin(1059073, "ProgressTest"_s, PLUGINFLAG_COMMAND_OPTION_DIALOG, nullptr, "C++ SDK Progress Demo"_s, NewObjClear(ProgressTest));
}

