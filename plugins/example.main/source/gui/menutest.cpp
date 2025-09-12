// example code for a menu plugin and multiprocessing
#include "maxon/parallelfor.h"
#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"

using namespace cinema;

class MenuTest : public CommandData
{
public:
	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager);
};

Bool MenuTest::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	GeShowMouse(MOUSE_BUSY);

	Int cnt = maxon::ThreadRef::GetCurrentThreadCount();
	String msgResult = "Multiprocessing Test on " + String::IntToString(cnt) + " CPUs returns:";
	String msgDuration = "| Duration per thread:"_s;

	struct MyContext : public maxon::ParallelFor::BaseContext
	{
		Random _rnd;
		Float	 _duration;
	};

	Float64 startTime = GeGetMilliSeconds();

	// This example uses a ParallelFor loop to replace a MPThreadPool. 
	maxon::ParallelFor::Dynamic<MyContext>(0, cnt,
		[](MyContext& context)
		{
			// ... init thread local data ...
			context._rnd.Init((Int32)context.GetLocalThreadIndex());
			context._duration = 0.0;
			// ... init thread local data ...
		},
		[](Int, MyContext& context)
		{
			Float64 t = GeGetMilliSeconds();

			// calculate the 10,000,000 th random number
			for (Int j = 0; j < 10000000; j++)
				context._rnd.Get01();

			context._duration = GeGetMilliSeconds() - t;
		},
		[&msgResult, &msgDuration, &cnt](MyContext& context)
		{
			// ... finalize thread local data ...
			msgResult += " " + String::FloatToString(context._rnd.Get01());
			msgDuration += " " + String::FloatToString(context._duration) + " ms";
			if (context.GetLocalThreadIndex() < cnt - 1)
				msgDuration += ",";
		}, maxon::ParallelFor::Granularity(1));

	Float64 totalDuration = GeGetMilliSeconds() - startTime;

	String msg = msgResult + " " + msgDuration + "|Total duration: " + String::FloatToString(totalDuration) + " ms";

	GeShowMouse(MOUSE_NORMAL);

	MessageDialog(msg);

	return true;
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_MENUTEST_SDK = 1000956;

Bool RegisterMenuTest()
{
	// be sure to use a unique ID obtained from developers.maxon.net
	return RegisterCommandPlugin(ID_MENUTEST_SDK, GeLoadString(IDS_MENUTEST), 0, AutoBitmap("icon.tif"_s), "C++ SDK Menu Test Plugin"_s, NewObjPtr(MenuTest));
}

