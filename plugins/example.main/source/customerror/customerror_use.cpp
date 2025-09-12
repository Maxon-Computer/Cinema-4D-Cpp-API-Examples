#include "c4d.h"

using namespace cinema;


/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_CUSTOM_ERROR_SDK = 99990000;

#include "customerror_interface.h"

// Dummy test function
static maxon::Result<void> TestFunction(maxon::Int * val)
{
	iferr_scope;
	
	if (!val)
		return CustomError(MAXON_SOURCE_LOCATION, 4242);
	
	ApplicationOutput("Value is @", *val);
	
	return maxon::OK;
}

// Command to test the custom error
class CustomErrorExample : public CommandData
{
public:
	
#if API_VERSION >= 20000 && API_VERSION < 21000
	Bool Execute(BaseDocument* doc)
#elif API_VERSION >= 21000
	Bool Execute(BaseDocument* doc, GeDialog* parentManager)
#endif
	{
		iferr_scope_handler
		{
			return false;
		};
		
		if (!doc)
			return false;
		
		iferr (TestFunction(nullptr))
			ApplicationOutput("Error: @", err);
		
		maxon::Int a = 100;
		
		iferr (TestFunction(&a))
			ApplicationOutput("Error: @", err);
				
		return true;
	}
	
	static CommandData* Alloc() { return NewObj(CustomErrorExample) iferr_ignore("Unexpected failure on allocating CustomErrorExample plugin."_s); }
};

Bool RegisterCustomErrorExample();
Bool RegisterCustomErrorExample()
{
	return RegisterCommandPlugin(ID_CUSTOM_ERROR_SDK, "C++ SDK - Custom Error Test"_s, PLUGINFLAG_COMMAND_OPTION_DIALOG, nullptr, ""_s, CustomErrorExample::Alloc());
}

