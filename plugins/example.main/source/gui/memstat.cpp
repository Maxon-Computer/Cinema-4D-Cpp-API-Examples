// be sure to use a unique ID obtained from developers.maxon.net
#define ID_MEMSTAT 200000072

#include "c4d.h"
#include "main.h"
#include "maxon/memoizationcache.h"
#include "maxon/dll.h"
#include "maxon/parallelfor.h"

using namespace cinema;

class MemStatDialog : public GeDialog
{
private:
	void CheckMaxMemory(Int32 mbblocks);
	void CheckParallelAllocation();
	
public:
	MemStatDialog();
	virtual Bool CreateLayout();

	void UpdateMemoizationCache(Bool createDialog);

	virtual Bool InitValues();
	virtual Bool Command(Int32 id, const BaseContainer& msg);
	virtual Int32 Message(const BaseContainer& msg, BaseContainer& result);
	virtual Bool CoreMessage  (Int32 id, const BaseContainer& msg);
	virtual void Timer(const BaseContainer& msg);

private:
	maxon::HashMap<maxon::Id, maxon::Tuple<Int /*cnt*/, Int /*mem*/>> _lastStat;
};

enum
{
	IDC_MEMORY_STAT_MEMORY_INUSE = 1000,
	IDC_MEMORY_STAT_MEMORY_PEAK,
	IDC_MEMORY_STAT_NO_OF_ALLOCATIONS_TOTAL,
	IDC_MEMORY_STAT_NO_OF_ALLOCATIONS_CURRENT,
	IDC_MEMORY_STAT_OGL_MEMORY,
	IDC_MEMORY_STAT_EOGL_TEXBUFFER,
	IDC_MEMORY_STAT_EOGL_VERTEXBUFFER,

	IDC_MEMORY_TEST_CLEAR_MEMOIZATION_GROUP,
	IDC_MEMORY_TEST_CLEAR_MEMOIZATION_CACHES_ALL,
	IDC_MEMORY_TEST_CLEAR_MEMOIZATION_CACHES_LASTFRAME,

	IDC_MEMORY_TEST_1MB,
	IDC_MEMORY_TEST_10MB,
	IDC_MEMORY_TEST_100MB,
	IDC_MEMORY_TEST_PARALLEL,

	IDC_MEMORY_TEST_1MB_RES,
	IDC_MEMORY_TEST_10MB_RES,
	IDC_MEMORY_TEST_100MB_RES,
	IDC_MEMORY_TEST_PARALLEL_RES,

	IDC_MEMORY_STAT_
};

MemStatDialog::MemStatDialog()
{
}

Bool MemStatDialog::CreateLayout()
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	SetTitle("Memory Statistics"_s);

	GroupBegin(0, BFH_SCALEFIT, 2, 0, String("Memory"), 0);
	{
		GroupBorder(BORDER_THIN_IN);
		GroupBorderSpace(4, 4, 4, 4);
		AddStaticText(0, BFH_FIT, 0, 0, "Memory In Use"_s, 0); AddStaticText(IDC_MEMORY_STAT_MEMORY_INUSE, BFH_SCALEFIT, SizeChr(140), 0, String(), 0);
		AddStaticText(0, BFH_FIT, 0, 0, "Memory Peak"_s, 0); AddStaticText(IDC_MEMORY_STAT_MEMORY_PEAK, BFH_SCALEFIT, SizeChr(140), 0, String(), 0);
		AddStaticText(0, BFH_FIT, 0, 0, "Number of Allocations (Current)"_s, 0); AddStaticText(IDC_MEMORY_STAT_NO_OF_ALLOCATIONS_CURRENT, BFH_SCALEFIT, SizeChr(100), 0, String(), 0);
		AddStaticText(0, BFH_FIT, 0, 0, "Number of Allocations (Total)"_s, 0); AddStaticText(IDC_MEMORY_STAT_NO_OF_ALLOCATIONS_TOTAL, BFH_SCALEFIT, SizeChr(100), 0, String(), 0);
		AddStaticText(0, BFH_FIT, 0, 0, "Viewport memory (allocated/used) MiB"_s, 0); AddStaticText(IDC_MEMORY_STAT_OGL_MEMORY, BFH_SCALEFIT, SizeChr(100), 0, String(), 0);
		// AddStaticText(0, BFH_FIT, 0, 0, "EOGL Texture Cache"_s, 0); AddStaticText(IDC_MEMORY_STAT_EOGL_TEXBUFFER, BFH_SCALEFIT, SizeChr(100), 0, String(), 0);
		// AddStaticText(0, BFH_FIT, 0, 0, "EOGL VBO Cache"_s, 0); AddStaticText(IDC_MEMORY_STAT_EOGL_VERTEXBUFFER, BFH_SCALEFIT, SizeChr(100), 0, String(), 0);
	}
	GroupEnd();

	GroupBegin(IDC_MEMORY_TEST_CLEAR_MEMOIZATION_GROUP, BFH_SCALEFIT, 2, 0, "Memoization Caches (cache count/reuse count)"_s, 0);
	{
		GroupBorder(BORDER_THIN_IN);
		UpdateMemoizationCache(true);
	}
	GroupEnd();

	GroupBegin(0, BFH_LEFT, 2, 0, String(), 0);
	{
		AddButton(IDC_MEMORY_TEST_1MB, BFH_FIT, 0, 0, "Test Max Memory Alloc (  1 MB)"_s);
		AddStaticText(IDC_MEMORY_TEST_1MB_RES, BFH_FIT, SizeChr(140), 0, String(), 0);

		AddButton(IDC_MEMORY_TEST_10MB, BFH_FIT, 0, 0, "Test Max Memory Alloc ( 10 MB)"_s);
		AddStaticText(IDC_MEMORY_TEST_10MB_RES, BFH_FIT, SizeChr(140), 0, String(), 0);

		AddButton(IDC_MEMORY_TEST_100MB, BFH_FIT, 0, 0, "Test Max Memory Alloc (100 MB)"_s);
		AddStaticText(IDC_MEMORY_TEST_100MB_RES, BFH_FIT, SizeChr(140), 0, String(), 0);

		AddButton(IDC_MEMORY_TEST_PARALLEL, BFH_FIT, 0, 0, "Test Parallel Memory Allocation"_s);
		AddStaticText(IDC_MEMORY_TEST_PARALLEL_RES, BFH_FIT, SizeChr(140), 0, String(), 0);
	}
	GroupEnd();

	SetTimer(500);

	return res;
}

void MemStatDialog::UpdateMemoizationCache(Bool createDialog)
{
	UpdateDialogHelper helper;

	using namespace maxon;
	HashMap<Id, Tuple<Int /*cnt*/, Int /*mem*/>> stat;

	ifnoerr (MemoizationCacheInterface::GetStatistics(stat))
	{
		if (!createDialog && _lastStat == stat)
		{
			return;
		}

		if (!createDialog)
		{
			helper = BeginLayoutChange(IDC_MEMORY_TEST_CLEAR_MEMOIZATION_GROUP, false);
		}

		if (stat.IsPopulated())
		{
			for (const auto& e : stat)
			{
				AddStaticText(0, BFH_FIT, 0, 0, FormatString("@", e.GetKey()), 0);
				AddStaticText(0, BFH_SCALEFIT, SizeChr(140), 0, FormatString("@", e.GetValue()), 0);
			}
		}
		else
		{
			AddStaticText(0, BFH_SCALEFIT, 0, 0, "No caches used"_s, 0);
			AddStaticText(0, BFH_SCALEFIT, 0, 0, ""_s, 0);
		}
		_lastStat = std::move(stat);
	}

	if (_lastStat.IsPopulated())
	{
		AddButton(IDC_MEMORY_TEST_CLEAR_MEMOIZATION_CACHES_ALL, BFH_FIT, 0, 0, "Clear all caches"_s);
		AddButton(IDC_MEMORY_TEST_CLEAR_MEMOIZATION_CACHES_LASTFRAME, BFH_FIT, 0, 0, "Clear old caches"_s);
	}

	helper.CommitChanges();
}

static String GetOGLMemoryString(Int32 l)
{
	return l <= 0 ? "unknown" : String::IntToString(l >> 10);
}

static String GetNoOfAllocationsString(Int64 amount)
{
	return amount <= 0 ? "unknown" : String::IntToString(amount);
}

Bool MemStatDialog::InitValues()
{
	// first call the parent instance
	if (!GeDialog::InitValues())
		return false;

	BaseContainer stat;
	// since this function is slow we have to tell Cinema that we need this information by setting 1
	stat.SetInt32(C4D_MEMORY_STAT_OPENGL_ALLOCATED, 1);
	GeGetMemoryStat(stat);

	SetString(IDC_MEMORY_STAT_MEMORY_INUSE, String::MemoryToString(stat.GetInt64(C4D_MEMORY_STAT_MEMORY_INUSE)));
	SetString(IDC_MEMORY_STAT_MEMORY_PEAK, String::MemoryToString(stat.GetInt64(C4D_MEMORY_STAT_MEMORY_PEAK)));
	SetString(IDC_MEMORY_STAT_NO_OF_ALLOCATIONS_TOTAL, GetNoOfAllocationsString(stat.GetInt64(C4D_MEMORY_STAT_NO_OF_ALLOCATIONS_TOTAL)));
	SetString(IDC_MEMORY_STAT_NO_OF_ALLOCATIONS_CURRENT, GetNoOfAllocationsString(stat.GetInt64(C4D_MEMORY_STAT_NO_OF_ALLOCATIONS_CURRENT)));
	SetString(IDC_MEMORY_STAT_OGL_MEMORY, GetOGLMemoryString(stat.GetInt32(C4D_MEMORY_STAT_OPENGL_ALLOCATED)) + " / " + GetOGLMemoryString(stat.GetInt32(C4D_MEMORY_STAT_OPENGL_USED)));
	//SetString(IDC_MEMORY_STAT_EOGL_TEXBUFFER, String::MemoryToString(stat.GetInt64(C4D_MEMORY_STAT_EOGL_TEXBUFFER)) + String(" (") + String::IntToString(stat.GetInt32(C4D_MEMORY_STAT_EOGL_TEXTUREBUFFER_CNT)) + String(")"));
	//SetString(IDC_MEMORY_STAT_EOGL_VERTEXBUFFER, String::MemoryToString(stat.GetInt64(C4D_MEMORY_STAT_EOGL_VERTEXBUFFER)) + String(" (") + String::IntToString(stat.GetInt32(C4D_MEMORY_STAT_EOGL_VERTEXBUFFER_CNT)) + String(")"));

	UpdateMemoizationCache(false);

	return true;
}

void MemStatDialog::Timer(const BaseContainer& msg)
{
	InitValues();
}

void MemStatDialog::CheckMaxMemory(Int32 mbblocks)
{
	maxon::BaseArray<void*> blocks;
	Int32 i;

	for (i = 0; true; i++)
	{
		void* block = nullptr;
		if (mbblocks > 0)
		{
			iferr (block = NewMem(UChar, mbblocks * 1024 * 1024))
				break;
		}
		if (!block)
			break;
		InitValues();
		iferr (blocks.Append(block))
		{
			DeleteMem(block);
			break;
		}
		InitValues();
	}

	BaseContainer stat;
	GeGetMemoryStat(stat);

	for (i = 0; i < blocks.GetCount(); i++)
	{
		void* block = blocks[i];
		if (block)
			DeleteMem(block);
	}

	InitValues();

	String memstr = String::MemoryToString(stat.GetInt64(C4D_MEMORY_STAT_MEMORY_INUSE));

	switch (mbblocks)
	{
		case 1:			SetString(IDC_MEMORY_TEST_1MB_RES, memstr); break;
		case 10:		SetString(IDC_MEMORY_TEST_10MB_RES, memstr); break;
		case 100:		SetString(IDC_MEMORY_TEST_100MB_RES, memstr); break;
	}

	GeOutString("Max memory allocation: " + memstr, GEMB::OK);
}

void MemStatDialog::CheckParallelAllocation()
{
	iferr_scope_handler
	{
		err.DbgStop();
		return;
	};
	maxon::BaseArray<void*> blocks;

	constexpr const Int MAX_CNT = 100000000;
	blocks.Resize(MAX_CNT) iferr_return;
	maxon::AtomicInt totalSize;

	// Allocate many small memory blocks concurrently.
	maxon::ParallelFor::Dynamic(0, MAX_CNT,
		[&blocks, &totalSize](Int i)
		{
			Int size = 31 + (i & 31);
			blocks[i] = maxon::DefaultAllocator::Alloc(size, MAXON_SOURCE_LOCATION);
			totalSize.SwapAdd(size);
		});
	
	GeOutString(FormatString("Allocated @ MB", totalSize.LoadRelaxed() / (1024 * 1024)), GEMB::OK);

	// Free memory from a different thread (at least mostly).
	for (void*& ptr : blocks)
		maxon::DefaultAllocator::Free(ptr);
}

Bool MemStatDialog::Command(Int32 id, const BaseContainer& msg)
{
	iferr_scope_handler
	{
		return false;
	};

	switch (id)
	{
		case IDC_MEMORY_TEST_CLEAR_MEMOIZATION_CACHES_ALL:
		case IDC_MEMORY_TEST_CLEAR_MEMOIZATION_CACHES_LASTFRAME:
		{
			using namespace maxon;
			TimeValue time = TimeValue::GetTime();

			Int deleteRunId = NOTOK;
			if (id == IDC_MEMORY_TEST_CLEAR_MEMOIZATION_CACHES_LASTFRAME)
			{
				deleteRunId = maxon::Max<Int>(GetActiveDocument()->GetCacheRunId() - 1, 0);
			}

			MemoizationCacheInterface::FlushAll(Id(), nullptr, deleteRunId);
			DiagnosticOutput("Clear Cache took: @", time.Stop());
			break;
		}

		case IDC_MEMORY_TEST_1MB:			CheckMaxMemory(1); break;
		case IDC_MEMORY_TEST_10MB:		CheckMaxMemory(10); break;
		case IDC_MEMORY_TEST_100MB:		CheckMaxMemory(100); break;
		case IDC_MEMORY_TEST_PARALLEL:	CheckParallelAllocation(); break;
	}
	return true;
}

Bool MemStatDialog::CoreMessage(Int32 id, const BaseContainer& msg)
{
	switch (id)
	{
		case EVMSG_CHANGE:
			if (CheckCoreMessage(msg))
			{
			}
			break;
	}
	return GeDialog::CoreMessage(id, msg);
}

Int32 MemStatDialog::Message(const BaseContainer& msg, BaseContainer& result)
{
	//switch (msg.GetId())
	{
	}
	return GeDialog::Message(msg, result);
}

class MemStatCommand : public CommandData
{
private:
	MemStatDialog dlg;

public:
	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager);
	virtual Int32 GetState(BaseDocument* doc, GeDialog* parentManager);
	virtual Bool RestoreLayout(void* secret);
};

Int32 MemStatCommand::GetState(BaseDocument* doc, GeDialog* parentManager)
{
	return CMD_ENABLED;
}

Bool MemStatCommand::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	return dlg.Open(DLG_TYPE::ASYNC, ID_MEMSTAT, -1, -1);
}

Bool MemStatCommand::RestoreLayout(void* secret)
{
	return dlg.RestoreLayout(ID_MEMSTAT, 0, secret);
}

Bool RegisterMemoryStat()
{
	return RegisterCommandPlugin(ID_MEMSTAT, String("C++ SDK - Memory Statistics"), 0, nullptr, String(), NewObjClear(MemStatCommand));
}

