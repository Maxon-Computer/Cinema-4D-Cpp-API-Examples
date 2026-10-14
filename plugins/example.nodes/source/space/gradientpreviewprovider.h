#ifndef GRADIENTPREVIEWPROVIDER_H__
#define GRADIENTPREVIEWPROVIDER_H__

#include "maxon/previewimageprovider.h"
#include "maxon/gradientmath.h"

namespace maxonsdk
{

struct GradientBakingData
{
	struct KnotData
	{
		maxon::ColorA32 _value;
		maxon::Float32 _bias = 0.0;
		maxon::UInt _interpolationMode = 0;
	};

	maxon::BaseArray<KnotData> _knotsData;
	maxon::SortedGradientKnots<maxon::Float32, maxon::Int32> _sortedKnots;
};

class GradientPreviewProvider : public maxon::Component<GradientPreviewProvider, maxon::nodes::PreviewImageProviderInterface>
{
	MAXON_COMPONENT(NORMAL);
public:

	GradientPreviewProvider();

	MAXON_METHOD maxon::Result<maxon::nodes::PreviewImageProviderOutput> TakeOutput();

	MAXON_METHOD maxon::Result<void> Initialize(maxon::DataDictionaryObjectRef request, maxon::Int numThreads);

	MAXON_METHOD maxon::Result<void> ComputeIteration(const maxon::JobRef parentThread);

private:

	maxon::IntVector2d _gradientPreviewSize;
	maxon::Range<maxon::Float> _gradientURange;
	maxon::GraphNode _clonedGradientBundle;

	static maxon::Result<GradientBakingData> InitializeGradientData(const maxon::GraphNode& gradientBundle);

	static maxon::Result<maxon::nodes::SamplesFloat32Ref> BakeGradient(const GradientBakingData& gradientData, const maxon::IntVector2d& gradientPreviewSize, const maxon::Range<maxon::Float>& gradientURange);

	maxon::nodes::SamplesFloat32Ref _gradientBufferRef;
};

} // namespace maxonsdk

#endif // GRADIENTPREVIEWPROVIDER_H__
