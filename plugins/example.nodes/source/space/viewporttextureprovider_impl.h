#ifndef VIEWPORTTEXTUREPROVIDER_IMPL_H__
#define VIEWPORTTEXTUREPROVIDER_IMPL_H__

#include "maxon/array.h"
#include "maxon/previewimageprovider.h"

namespace maxonsdk
{

class ViewportTextureProviderExample : public maxon::Component<ViewportTextureProviderExample, maxon::nodes::PreviewImageProviderInterface>
{
	struct IterationResult
	{
		maxon::nodes::SamplesFloat32ConstRef _texture;
		maxon::IntVector2d _textureSize;
		maxon::Id _id;
	};
	
	MAXON_COMPONENT(NORMAL);
public:
	
	MAXON_METHOD maxon::Result<maxon::nodes::PreviewImageProviderOutput> TakeOutput();
	
	MAXON_METHOD maxon::Result<void> Initialize(maxon::DataDictionaryObjectRef request, maxon::Int numThreads);
	
	MAXON_METHOD maxon::Result<void> ComputeIteration(const maxon::JobRef parentThread);
	
private:
	
	maxon::BaseArray<maxon::IntVector2d> _textureResolutions;
	maxon::Int _lastFinishedIndex = -1;
	
	maxon::Array<maxon::Id> _requestedIds;
	
	maxon::DataDictionary _bakeColors;
	
	maxon::BaseArray<IterationResult> _pendingResults;
};
	
} // namespace maxonsdk
#endif // VIEWPORTTEXTUREPROVIDER_IMPL_H__

