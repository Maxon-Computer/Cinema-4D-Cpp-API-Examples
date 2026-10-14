#ifndef THUMBNAILGENERATOR_IMPL_H__
#define THUMBNAILGENERATOR_IMPL_H__

#include "maxon/previewimageprovider.h"
#include "maxon/range.h"
#include "maxon/job.h"

#include "maxon/lib_math.h"

namespace maxonsdk
{
	
namespace renderer
{
	
class CircleBarNode
{
public:
	CircleBarNode(const maxon::Color32& foreground, const maxon::Color32& background, maxon::Float32 radius) :
	_foreground(foreground),
	_background(background),
	_radiusSquared(radius * radius){}
	
	maxon::Color32 Sample(const maxon::Vector2d32& uv) const;
	
private:
	const maxon::Color32 _foreground;
	const maxon::Color32 _background;
	const maxon::Float32 _radiusSquared;
};

class CircleBarTextureSampler
{
public:
	
	CircleBarTextureSampler(const maxon::Color32& circleColor, const maxon::IntVector2d& textureSize,
													const maxon::Range<maxon::Float32>& uRange = maxon::Range<maxon::Float32>(0, 1),
													const maxon::Range<maxon::Float32>& vRange = maxon::Range<maxon::Float32>(0, 1)) :
	_circleBar(circleColor, maxon::Color32(0), (maxon::Float32)0.45),
	_textureSize(textureSize),
	_uRange(uRange),
	_vRange(vRange),
	_uStep((uRange.GetMax() - uRange.GetMin()) / maxon::Float32(textureSize.x)),
	_vStep((vRange.GetMax() - vRange.GetMin()) / maxon::Float32(textureSize.y)),
	_halfSample(maxon::Vector2d32((maxon::Float32)0.5*_uStep, (maxon::Float32)0.5*_vStep)){}
	
	CircleBarTextureSampler() : CircleBarTextureSampler(maxon::Color32(1), maxon::IntVector2d(1)){}
	
	maxon::Color32 Sample(maxon::Float32 u, maxon::Float32 v) const;
	
	maxon::Float32 GetU(maxon::Int pixelX) const;
	
	maxon::Float32 GetV(maxon::Int pixelY) const;
	
	static maxon::Result<maxon::nodes::SamplesFloat32ConstRef> BakeColoredTexture(const maxon::Color32& color, const maxon::IntVector2d& textureSize, const maxon::JobRef& parentThread);
	
private:
	
	const CircleBarNode _circleBar;
	const maxon::IntVector2d _textureSize;
	
	const maxon::Range<maxon::Float32> _uRange;
	const maxon::Range<maxon::Float32> _vRange;
	
	const maxon::Float32 _uStep;
	const maxon::Float32 _vStep;
	const maxon::Vector2d32 _halfSample;
};
	
} // namespace renderer
	
} // namespace maxonsdk
#endif // THUMBNAILGENERATOR_IMPL_H__
