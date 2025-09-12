// ------------------------------------------------------------------------
/// This file contains implementations of CommandClassInterface and CommandDataInterface.
///
/// "ExampleDataImpl" is an standard implementation of a context.
/// "MeanAverageCommandImpl" is an implementation calculating the average value of a given array.
/// "MeanMedianCommandImpl" is an implementation calculating the median value of a given array.
// ------------------------------------------------------------------------

// Maxon API header files
#include "maxon/lib_math.h"
#include "maxon/sort.h"
#include "maxon/array.h"

// local header files
#include "command_declaration.h"
#include "maxon/commandobservable.h"

namespace maxonsdk
{
// ------------------------------------------------------------------------
/// An implementation of CommandDataInterface with standard functionality.
// ------------------------------------------------------------------------
class ExampleDataImpl : public maxon::Component<ExampleDataImpl, maxon::CommandDataInterface>
{
	MAXON_COMPONENT(NORMAL, maxon::DataDictionaryObjectClass);

public:
	MAXON_METHOD maxon::Result<void> SetData(maxon::ForwardingDataPtr&& key, maxon::Data&& data)
	{
		return super.SetData(std::move(key), std::move(data));
	}
	MAXON_METHOD maxon::Result<maxon::Data> GetData(const maxon::ConstDataPtr& key) const
	{
		return super.GetData(std::move(key));
	}
};

// register data
MAXON_COMPONENT_CLASS_REGISTER(ExampleDataImpl, maxon::CommandDataClasses, "net.maxonexample.commanddata.example");

// ------------------------------------------------------------------------
/// An implementation of CommandClassInterface that calculates the average value of the given array.
// ------------------------------------------------------------------------
class MeanAverageCommandImpl : public maxon::Component<MeanAverageCommandImpl, maxon::CommandClassInterface>
{
	MAXON_COMPONENT();

public:
	MAXON_METHOD maxon::Result<maxon::COMMANDSTATE> GetState(maxon::CommandDataRef& data) const
	{
		iferr_scope;

		// get data from context
		const maxon::BaseArray<maxon::Float> values = data.Get(MEANSETTINGS::VALUES) iferr_return;

		// check count
		const maxon::Int count = values.GetCount();
		if (count == 0)
			return maxon::COMMANDSTATE::DISABLED;

		return maxon::COMMANDSTATE::ENABLED;
	}

	MAXON_METHOD maxon::Result<maxon::COMMANDRESULT> Execute(maxon::CommandDataRef& data) const
	{
		iferr_scope;
		const maxon::COMMANDSTATE commandState = self.GetState(data) iferr_return;
		if (commandState != maxon::COMMANDSTATE::ENABLED)
			return maxon::COMMANDRESULT::SKIP;

		// get data from context
		const maxon::BaseArray<maxon::Float> values = data.Get(MEANSETTINGS::VALUES) iferr_return;

		// calculate average
		const maxon::Float result = maxon::GetAverage(values);

		// store average
		data.Set(MEANSETTINGS::RESULT, result) iferr_return;

		return maxon::COMMANDRESULT::OK;
	}
};

// ------------------------------------------------------------------------
/// Register command.
// ------------------------------------------------------------------------
MAXON_COMPONENT_OBJECT_REGISTER(MeanAverageCommandImpl, maxon::CommandClasses, "net.maxonexample.command.mean.average");

// ------------------------------------------------------------------------
/// An implementation of CommandClassInterface that calcualtes the median value of the given array.
// ------------------------------------------------------------------------
class MeanMedianCommandImpl : public maxon::Component<MeanMedianCommandImpl, maxon::CommandClassInterface>
{
	MAXON_COMPONENT();

public:
	MAXON_METHOD maxon::Result<maxon::COMMANDSTATE> GetState(maxon::CommandDataRef& data) const
	{
		iferr_scope;

		// get data from context
		const maxon::BaseArray<maxon::Float> values = data.Get(MEANSETTINGS::VALUES) iferr_return;

		// check count
		const maxon::Int count = values.GetCount();
		if (count == 0)
			return maxon::COMMANDSTATE::DISABLED;

		return maxon::COMMANDSTATE::ENABLED;
	}

	MAXON_METHOD maxon::Result<maxon::COMMANDRESULT> Execute(maxon::CommandDataRef& data) const
	{
		iferr_scope;
		const maxon::COMMANDSTATE commandState = self.GetState(data) iferr_return;
		if (commandState != maxon::COMMANDSTATE::ENABLED)
			return maxon::COMMANDRESULT::SKIP;

		// get data from context
		maxon::BaseArray<maxon::Float> values = data.Get(MEANSETTINGS::VALUES) iferr_return;

		// get count
		const maxon::Int count = values.GetCount();

		// sort values
		maxon::SimpleSort<> sort;
		sort.Sort(values);

		maxon::Float median = 0.0;

		if (count % 2)
		{
			// odd
			const maxon::Int center = count / 2;
			median = values[center];
		}
		else
		{
			// even
			const maxon::Int middleIndex2 = count / 2;
			const maxon::Int middleIndex1 = middleIndex2 - 1;

			const maxon::Float v1 = values[middleIndex1];
			const maxon::Float v2 = values[middleIndex2];
			median = (v1 + v2) / 2.0;
		}

		// store median
		data.Set(MEANSETTINGS::RESULT, median) iferr_return;

		return maxon::COMMANDRESULT::OK;
	}
};

// ------------------------------------------------------------------------
/// Register command.
// ------------------------------------------------------------------------
MAXON_COMPONENT_OBJECT_REGISTER(MeanMedianCommandImpl, maxon::CommandClasses, "net.maxonexample.command.mean.median");
}

