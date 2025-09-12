#include "viewporttextureprovider_impl.h"
#include "thumbnailgenerator_impl.h"

namespace maxonsdk
{
	
MAXON_METHOD maxon::Result<maxon::nodes::PreviewImageProviderOutput> ViewportTextureProviderExample::TakeOutput()
{
	iferr_scope;

	maxon::nodes::PreviewImageProviderOutput output;

	const maxon::Bool isFinal = (_textureResolutions.GetCount() - _lastFinishedIndex) <= 1;

	for (const IterationResult& result : _pendingResults)
	{
		maxon::nodes::PreviewImageProviderOutputImage outputImage;
		outputImage._imageSize = result._textureSize;
		outputImage._samplesFloat32Linear = result._texture;
		outputImage._isFinal = isFinal;
		output.SetArrayResult(result._id, outputImage) iferr_return;
	}

	// Clear the set of pending changes, we have handed them over.
	_pendingResults.Reset();

	// For now we only do one iteration with all textures.
	output._currentIterationIndex = _lastFinishedIndex;
	output._iterationCount = _textureResolutions.GetCount();
	output._isFinal = isFinal;
	return output;
}

MAXON_METHOD maxon::Result<void> ViewportTextureProviderExample::Initialize(maxon::DataDictionaryObjectRef request, maxon::Int numThreads)
{
	iferr_scope;
	
	_bakeColors = request.Get(maxon::Id("net.maxonexample.viewporttextureprovider.bakecolors"), maxon::DataDictionary());
	
	_requestedIds = request.Get(maxon::nodes::PREVIEWIMAGEREQUEST::TEXTUREIDS, maxon::Array<maxon::Id>());
	CheckState(_requestedIds.IsPopulated() == true, "Nothing to bake.");
	const maxon::IntVector2d requestedTextureSize = request.Get(maxon::nodes::PREVIEWIMAGEREQUEST::TEXTURESIZE, maxon::IntVector2d());
	const maxon::Bool renderProgressively = request.Get<maxon::Int>(maxon::nodes::PREVIEWIMAGEREQUEST::PROGRESSIONTHRESHOLD, maxon::Int(1)) != maxon::Int(0);
	
	// In order to provide the user with an immediate visual response we provide lower resolution textures quickly.
	// This minimizes the time to first sample if users set very high 'Editor resolutions' which may be up to 16K x 16K.
	if (requestedTextureSize.x > 64 && requestedTextureSize.y > 64 && renderProgressively == true)
	{
		// For a real production case we should do a bit better and make sure to re-use previously computed samples where we
		// only fill in the blanks in later resolutions, as well as making sure we always bake power-of-2 textures until we reach the final resolution.
		const maxon::Int minimumTextureSize = 64;
		maxon::IntVector2d currentResolution = requestedTextureSize;
		
		maxon::BaseArray<maxon::IntVector2d> reversedTextureResolutions;
		
		while (currentResolution.x >= minimumTextureSize && currentResolution.y >= minimumTextureSize)
		{
			reversedTextureResolutions.Append(currentResolution) iferr_return;
			currentResolution.x = currentResolution.x / maxon::UInt32(2);
			currentResolution.y = currentResolution.y / maxon::UInt32(2);
		}
		
		// We create a sequence of increasing texture resolutions by reverting our current result.
		while (reversedTextureResolutions.Pop(&currentResolution) == true)
		{
			_textureResolutions.Append(std::move(currentResolution)) iferr_return;
		}
	}
	else
	{
		_textureResolutions.Append(requestedTextureSize) iferr_return;
	}
	
	return maxon::OK;
}
		
MAXON_METHOD maxon::Result<void> ViewportTextureProviderExample::ComputeIteration(const maxon::JobRef parentThread)
{
	iferr_scope;
	
	const maxon::Int currentIndex = _lastFinishedIndex + 1;
	CheckState(currentIndex < _textureResolutions.GetCount(), "Invalid iteration.");
	const maxon::IntVector2d& currentTextureSize = _textureResolutions[currentIndex];
	
	// We compute all textures in lock-step. Note that we don't have to, we could also return one after another.
	for (const maxon::Id& materialSlotId : _requestedIds)
	{
		const maxon::Color bakeColor = _bakeColors.Get(materialSlotId, maxon::Color(1, 0, 1));
		// Ideally, we would spawn a multi-threaded shader baking mechanism.
		maxon::nodes::SamplesFloat32ConstRef image = renderer::CircleBarTextureSampler::BakeColoredTexture(maxon::Color32(bakeColor), currentTextureSize, parentThread) iferr_return;
		
		if (parentThread.IsCancelled())
		{
			return maxon::OK;
		}
		
		IterationResult result;
		result._textureSize = currentTextureSize;
		result._id = materialSlotId;
		result._texture = image;
		_pendingResults.Append(std::move(result)) iferr_return;
	}
	
	// Proceed with the next iteration.
	++_lastFinishedIndex;
	
	return maxon::OK;
}

} // namespace maxonsdk
