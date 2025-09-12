// example code for a menu plugin and multiprocessing
#include "maxon/backgroundentry.h"
#include "maxon/conditionvariable.h"
#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"

using namespace cinema;

class ProgressTest : public CommandData
{
public:
	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager);
	virtual Bool ExecuteOptionID(BaseDocument* doc, Int32 plugid, Int32 subid, GeDialog* parentManager);

	static maxon::Result<void>	ComputationalTaskWithProgress(Bool spinning);
};

// Atomic (thread safe) integer, storing global entry count added by this example
static maxon::AtomicInt g_globalCount;

maxon::Result<void> ProgressTest::ComputationalTaskWithProgress(Bool spinning)
{
	iferr_scope;

	// Increments the number of entry
	Int newId = g_globalCount.SwapIncrement() + 1;

	// Adds to the Task Manager, a new entry visible to the user
	maxon::BackgroundProgressRef backgroundJobs = maxon::BackgroundProgressInterface::GetMaster();
	maxon::BackgroundProgressRef backgroundJobEntry = backgroundJobs.AddActiveEntry(FormatString("SDK Async Job Demo @", newId)) iferr_return;

	// This lambda simulates a computational task which updates the progress indicator in the Task Manager.
	// Enqueue it as a concurrent job which can be cancelled by the Task Manager cancellation delegate.
	maxon::JobRef computationalTask = maxon::JobRef::Enqueue([backgroundJobs, backgroundJobEntry, spinning]() -> maxon::Result<void>
	{
		iferr_scope;

		// Delete the entry once it's done, finally will be executed even in case of error.
		finally
		{
			backgroundJobs.RemoveActiveEntry(backgroundJobEntry) iferr_ignore("ignore on destruction");
		};

		// This is just for the purpose of simulating a workload.
		maxon::ConditionVariableRef simulateWorkload = maxon::ConditionVariableRef::Create() iferr_return;

		// Simulates a processing by doing a 20sec loop
		maxon::TimeValue time = maxon::TimeValue::GetTime();

		// random time between 1 and 29 seconds
		Random rnd;
		UInt32 seed = UInt32(time.GetHashCode());
		rnd.Init(seed);
		maxon::TimeValue duration = maxon::Seconds((rnd.Get01() * 29.0) + 5.0);

		// Adds a new job, to the visible entry. This is technically not mandatory in this case,
		// since we only add one job. By default there is always at least one job in an entry.
		// This default job have an index of zero and a weight of one.
		const Float jobWeight = 1.0;
		Int backgroundJobIdx = backgroundJobEntry.AddProgressJob(jobWeight, FormatString("SDK Demo Progress (@{s})", duration)) iferr_return;

		Float progress = 0.0;

		// Loops, if the progress is not 100% or the user did not canceled the task from the Task Manager
		while (!maxon::JobRef::IsCurrentJobCancelled() && progress < 1.0)
		{
			progress = (maxon::TimeValue::GetTime() - time) / duration;

			// Updates the progress of the current job, and taking into account the weight allocated to this job,
			// the general progress bar of the entry is updated in the user interface
			backgroundJobEntry.SetProgressAndCheckBreak(backgroundJobIdx, spinning ? maxon::UNKNOWNPROGRESS:progress) iferr_return;

			// Simulate workload ...
			simulateWorkload.Wait(maxon::Milliseconds(100));

			// Retrieve if the WARNING and ERROR bit is set in the entry state
			Bool isCurrentStateWarning = Int(backgroundJobEntry.GetState()) & Int(maxon::BackgroundEntryInterface::STATE::WARNING);
			Bool isCurrentStateError =	 Int(backgroundJobEntry.GetState()) & Int(maxon::BackgroundEntryInterface::STATE::ERROR);

			// Defines a warning if the task is longer than 15 sec
			if (!isCurrentStateWarning && (maxon::TimeValue::GetTime() - time) > maxon::Seconds(15))
			{
				backgroundJobEntry.AddState(maxon::BackgroundEntryInterface::STATE::WARNING, "Function took longer than 15 seconds"_s) iferr_return;
			}

			// Defines an error if the task is longer than 24 sec
			if (!isCurrentStateError && (maxon::TimeValue::GetTime() - time) > maxon::Seconds(24))
			{
				backgroundJobEntry.AddState(maxon::BackgroundEntryInterface::STATE::ERROR, "Function took longer than 24 seconds"_s) iferr_return;
			}
		}
		return maxon::OK;
	}).GetValue();
	
	// Remove the entry from the task manager on failure.
	if (computationalTask == nullptr)
	{
		backgroundJobs.RemoveActiveEntry(backgroundJobEntry) iferr_cannot_fail("job must be valid");
		return maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION);
	}

	// The Task manager will invoke this delegate if the user to wants to abort this entry.
	backgroundJobEntry.SetCancelJobDelegate(
		[computationalTask](const maxon::ProgressRef& job) mutable -> maxon::Result<void>
		{
			// Cancel execution of the job.
			computationalTask.Cancel();
			return maxon::OK;
		}) iferr_return;

	return maxon::OK;
}

Bool ProgressTest::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	iferr_scope_handler
	{
		DiagnosticOutput("@", err);
		return false;
	};

	const Bool spinning = false;
	ComputationalTaskWithProgress(spinning) iferr_return;

	return true;
}

Bool ProgressTest::ExecuteOptionID(BaseDocument* doc, Int32 plugid, Int32 subid, GeDialog* parentManager)
{
	iferr_scope_handler
	{
		DiagnosticOutput("@", err);
		return false;
	};

	const Bool spinning = true;
	ComputationalTaskWithProgress(spinning) iferr_return;

	return true;
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_PROGRESS_DEMO_SDK = 1059073;

Bool RegisterProgressTest()
{
	// be sure to use a unique ID obtained from developers.maxon.net
	return RegisterCommandPlugin(ID_PROGRESS_DEMO_SDK, "ProgressTest"_s, PLUGINFLAG_COMMAND_OPTION_DIALOG, nullptr, "C++ SDK Progress Demo"_s, NewObjClear(ProgressTest));
}

