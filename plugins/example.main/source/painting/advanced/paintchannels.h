#ifndef PAINTCHANNELS_H__
#define PAINTCHANNELS_H__

#include "c4d.h"
#include "paintbrushbase.h"

namespace cinema
{

class BrushDabData;

} // namespace cinema

struct TriangleData
{
	cinema::Vector32 destBitmapCoords[3];
	cinema::Vector32 sourceBitmapCoords[3];
	cinema::Vector points3D[3];
};

struct PaintChannels
{
	PaintChannels() = default;
	~PaintChannels() = default;

	cinema::Bool Init();
	void UpdateBitmaps();

	void SetupPoly_Pixel(cinema::BrushDabData* dab, const cinema::CPolygon &p, const cinema::UVWStruct &polyUVs, const cinema::Vector* polyPoints);
	void SetupPoly_Bary(cinema::BrushDabData* dab, const cinema::CPolygon &p, const cinema::UVWStruct &polyUVs, const cinema::Vector* polyPoints);

public:
	cinema::PaintLayerBmp* channel = nullptr;
	cinema::Vector fgColor;
	TriangleData triangle[2];

	cinema::Float strength;
	cinema::Bool useStencil;
	cinema::Bool useStamp;
	cinema::Bool fillTool;
	cinema::Int32 minmax[4];
};


inline cinema::Bool GetChannelInfo(cinema::PaintLayerBmp& paintBmp, int &bitdepth, int &numChannels)
{
	cinema::Bool supported = false;

	cinema::COLORMODE colorMode = (cinema::COLORMODE)paintBmp.GetColorMode();
	switch (colorMode)
	{
		// 8Bit
		case cinema::COLORMODE::ALPHA:	// only alpha channel
		{
			bitdepth = 8;
			numChannels = 1;
			supported = true;
			break;
		}
		case cinema::COLORMODE::GRAY:
		{
			bitdepth = 8;
			numChannels = 1;
			supported = true;
			break;
		}
		case cinema::COLORMODE::AGRAY:
		{
			bitdepth = 8;
			numChannels = 2;
			supported = true;
			break;
		}
		case cinema::COLORMODE::RGB:
		{
			bitdepth = 8;
			numChannels = 3;
			supported = true;
			break;
		}
		case cinema::COLORMODE::ARGB:
		{
			bitdepth = 8;
			numChannels = 4;
			supported = true;
			break;
		}
		case cinema::COLORMODE::CMYK:
		{
			bitdepth = 8;
			numChannels = 4;
			break;
		}
		case cinema::COLORMODE::ACMYK:
		{
			bitdepth = 8;
			numChannels = 5;
			break;
		}
		case cinema::COLORMODE::MASK: // gray map as mask
		{
			bitdepth = 8;
			numChannels = 1;
			break;
		}
		case cinema::COLORMODE::AMASK: // gray map as mask
		{
			bitdepth = 8;
			numChannels = 2;
			break;
		}

		// 16 bit modes
		case cinema::COLORMODE::GRAYw:
		{
			bitdepth = 16;
			numChannels = 1;
			supported = true;
			break;
		}
		case cinema::COLORMODE::AGRAYw:
		{
			bitdepth = 16;
			numChannels = 2;
			supported = true;
			break;
		}
		case cinema::COLORMODE::RGBw:
		{
			bitdepth = 16;
			numChannels = 3;
			supported = true;
			break;
		}
		case cinema::COLORMODE::ARGBw:
		{
			bitdepth = 16;
			numChannels = 4;
			supported = true;
			break;
		}
		case cinema::COLORMODE::MASKw:
		{
			bitdepth = 16;
			numChannels = 1;
			break;
		}

		// 32 bit modes
		case cinema::COLORMODE::GRAYf:
		{
			bitdepth = 32;
			numChannels = 1;
			supported = true;
			break;
		}
		case cinema::COLORMODE::AGRAYf:
		{
			bitdepth = 32;
			numChannels = 2;
			supported = true;
			break;
		}
		case cinema::COLORMODE::RGBf:
		{
			bitdepth = 32;
			numChannels = 3;
			supported = true;
			break;
		}
		case cinema::COLORMODE::ARGBf:
		{
			bitdepth = 32;
			numChannels = 4;
			supported = true;
			break;
		}
		case cinema::COLORMODE::MASKf:
		{
			bitdepth = 32;
			numChannels = 1;
			break;
		}
	}

	return supported;
}

inline void SetPixel(cinema::COLORMODE colorMode, cinema::UInt &idx, cinema::UChar* buffer, cinema::Float32 falloff, cinema::UChar* colBuffer, const cinema::BaseBitmap* bitmap = nullptr, int sourceX = 0, int sourceY = 0)
{
	if (falloff <= 0)
	{
		switch (colorMode)
		{
			// 8Bit
			case cinema::COLORMODE::ALPHA:	// only alpha channel
			{
				idx += 1;
				break;
			}
			case cinema::COLORMODE::GRAY:
			{
				idx += 1;
				break;
			}
			case cinema::COLORMODE::AGRAY:
			{
				idx += 2;
				break;
			}
			case cinema::COLORMODE::RGB:
			{
				idx += 3;
				break;
			}
			case cinema::COLORMODE::ARGB:
			{
				idx += 4;
				break;
			}
			case cinema::COLORMODE::CMYK:
			{
				break;
			}
			case cinema::COLORMODE::ACMYK:
			{
				break;
			}
			case cinema::COLORMODE::MASK: // gray map as mask
			{
				idx += 1;
				break;
			}
			case cinema::COLORMODE::AMASK: // gray map as mask
			{
				idx += 2;
				break;
			}

			// 16 bit modes
			case cinema::COLORMODE::GRAYw:
			{
				idx += 2;
				break;
			}
			case cinema::COLORMODE::AGRAYw:
			{
				idx += 4;
				break;
			}
			case cinema::COLORMODE::RGBw:
			{
				idx += 6;
				break;
			}
			case cinema::COLORMODE::ARGBw:
			{
				idx += 8;
				break;
			}
			case cinema::COLORMODE::MASKw:
			{
				idx += 2;
				break;
			}

			// 32 bit modes
			case cinema::COLORMODE::GRAYf:
			{
				idx += 4;
				break;
			}
			case cinema::COLORMODE::AGRAYf:
			{
				idx += 8;
				break;
			}
			case cinema::COLORMODE::RGBf:
			{
				idx += 12;
				break;
			}
			case cinema::COLORMODE::ARGBf:
			{
				idx += 16;
				break;
			}
			case cinema::COLORMODE::MASKf:
			{
				idx += 4;
				break;
			}
		}

		return;
	}

	cinema::Float32 falloffInterp = 1.0f - falloff;
	switch (colorMode)
	{
		// 8Bit
		case cinema::COLORMODE::ALPHA:	// only alpha channel
		{
			idx += 1;
			break;
		}
		case cinema::COLORMODE::GRAY:
		{
			if (bitmap) 
				bitmap->GetPixelCnt(sourceX, sourceY, 1, (cinema::UChar*)colBuffer, 1, cinema::COLORMODE::GRAY, cinema::PIXELCNT::NONE);
			buffer[idx] = (cinema::UChar)(cinema::Float32(buffer[idx]) * (falloffInterp) +cinema::Float32(colBuffer[0]) * falloff);
			idx += 1;
			break;
		}
		case cinema::COLORMODE::AGRAY:
		{
			if (bitmap) 
				bitmap->GetPixelCnt(sourceX, sourceY, 1, (cinema::UChar*)colBuffer, 1, cinema::COLORMODE::AGRAY, cinema::PIXELCNT::NONE);
			buffer[idx]   = (cinema::UChar)(cinema::Float32(buffer[idx])   * (falloffInterp) + cinema::Float32(colBuffer[0]) * falloff);
			buffer[idx+1] = (cinema::UChar)(cinema::Float32(buffer[idx+1]) * (falloffInterp) + cinema::Float32(colBuffer[1]) * falloff);
			idx += 2;
			break;
		}
		case cinema::COLORMODE::RGB:
		{
			if (bitmap) 
				bitmap->GetPixelCnt(sourceX, sourceY, 1, (cinema::UChar*)colBuffer, 3, cinema::COLORMODE::RGB, cinema::PIXELCNT::NONE);
			buffer[idx]   = (cinema::UChar)(cinema::Float32(buffer[idx])   * (falloffInterp) + cinema::Float32(colBuffer[0]) * falloff);
			buffer[idx+1] = (cinema::UChar)(cinema::Float32(buffer[idx+1]) * (falloffInterp) + cinema::Float32(colBuffer[1]) * falloff);
			buffer[idx+2] = (cinema::UChar)(cinema::Float32(buffer[idx+2]) * (falloffInterp) + cinema::Float32(colBuffer[2]) * falloff);
			idx += 3;
			break;
		}
		case cinema::COLORMODE::ARGB:
		{
			if (bitmap) 
				bitmap->GetPixelCnt(sourceX, sourceY, 1, (cinema::UChar*)colBuffer, 3, cinema::COLORMODE::ARGB, cinema::PIXELCNT::NONE);
			buffer[idx]   = (cinema::UChar)(cinema::Float32(buffer[idx]) * (falloffInterp) + cinema::Float32(colBuffer[0]) * falloff);
			buffer[idx+1] = (cinema::UChar)(cinema::Float32(buffer[idx+1]) * (falloffInterp) + cinema::Float32(colBuffer[1]) * falloff);
			buffer[idx+2] = (cinema::UChar)(cinema::Float32(buffer[idx+2]) * (falloffInterp) + cinema::Float32(colBuffer[2]) * falloff);
			buffer[idx+3] = (cinema::UChar)(cinema::Float32(buffer[idx+3]) * (falloffInterp) + cinema::Float32(colBuffer[3]) * falloff);
			idx += 4;
			break;
		}

		case cinema::COLORMODE::CMYK:		break;
		case cinema::COLORMODE::ACMYK:	break;
		case cinema::COLORMODE::AMASK:	break;

		// 16 bit modes
		case cinema::COLORMODE::GRAYw:
		{
			if (bitmap) 
				bitmap->GetPixelCnt(sourceX, sourceY, 1, (cinema::UChar*)colBuffer, 2, cinema::COLORMODE::GRAYw, cinema::PIXELCNT::NONE);
			cinema::UInt16 *colBuf = (cinema::UInt16*)colBuffer;
			cinema::UInt16 *destBuf = (cinema::UInt16*)&buffer[idx];
			destBuf[0] = (cinema::UInt16)(cinema::Float32(destBuf[0]) * (falloffInterp) + cinema::Float32(colBuf[0]) * falloff);
			idx += 2;
			break;
		}
		case cinema::COLORMODE::AGRAYw:
		{
			if (bitmap) 
				bitmap->GetPixelCnt(sourceX, sourceY, 1, (cinema::UChar*)colBuffer, 2, cinema::COLORMODE::AGRAYw, cinema::PIXELCNT::NONE);
			cinema::UInt16 *colBuf = (cinema::UInt16*)colBuffer;
			cinema::UInt16 *destBuf = (cinema::UInt16*)&buffer[idx];
			destBuf[0] = (cinema::UInt16)(cinema::Float32(destBuf[0]) * (falloffInterp) + cinema::Float32(colBuf[0]) * falloff);
			destBuf[1] = (cinema::UInt16)(cinema::Float32(destBuf[1]) * (falloffInterp) + cinema::Float32(colBuf[1]) * falloff);
			idx += 4;
			break;
		}
		case cinema::COLORMODE::RGBw:
		{
			if (bitmap) 
				bitmap->GetPixelCnt(sourceX, sourceY, 1, (cinema::UChar*)colBuffer, 6, cinema::COLORMODE::RGBw, cinema::PIXELCNT::NONE);
			cinema::UInt16 *colBuf = (cinema::UInt16*)colBuffer;
			cinema::UInt16 *destBuf = (cinema::UInt16*)&buffer[idx];
			destBuf[0] = (cinema::UInt16)(cinema::Float32(destBuf[0]) * (falloffInterp) + cinema::Float32(colBuf[0]) * falloff);
			destBuf[1] = (cinema::UInt16)(cinema::Float32(destBuf[1]) * (falloffInterp) + cinema::Float32(colBuf[1]) * falloff);
			destBuf[2] = (cinema::UInt16)(cinema::Float32(destBuf[2]) * (falloffInterp) + cinema::Float32(colBuf[2]) * falloff);
			idx += 6;
			break;
		}
		case cinema::COLORMODE::ARGBw:
		{
			if (bitmap) 
				bitmap->GetPixelCnt(sourceX, sourceY, 1, (cinema::UChar*)colBuffer, 6, cinema::COLORMODE::ARGBw, cinema::PIXELCNT::NONE);
			cinema::UInt16 *colBuf = (cinema::UInt16*)colBuffer;
			cinema::UInt16 *destBuf = (cinema::UInt16*)&buffer[idx];
			destBuf[0] = (cinema::UInt16)(cinema::Float32(destBuf[0]) * (falloffInterp) + cinema::Float32(colBuf[0]) * falloff);
			destBuf[1] = (cinema::UInt16)(cinema::Float32(destBuf[1]) * (falloffInterp) + cinema::Float32(colBuf[1]) * falloff);
			destBuf[2] = (cinema::UInt16)(cinema::Float32(destBuf[2]) * (falloffInterp) + cinema::Float32(colBuf[2]) * falloff);
			destBuf[3] = (cinema::UInt16)(cinema::Float32(destBuf[3]) * (falloffInterp) + cinema::Float32(colBuf[3]) * falloff);
			idx += 8;
			break;
		}

		// 32 bit modes
		case cinema::COLORMODE::GRAYf:
		{
			if (bitmap) 
				bitmap->GetPixelCnt(sourceX, sourceY, 1, (cinema::UChar*)colBuffer, 4, cinema::COLORMODE::GRAYf, cinema::PIXELCNT::NONE);
			cinema::Float32 *colBuf = (cinema::Float32*)colBuffer;
			cinema::Float32 *destBuf = (cinema::Float32*)&buffer[idx];
			destBuf[0] = cinema::Float32(destBuf[0]) * (falloffInterp) +cinema::Float32(colBuf[0]) * falloff;
			idx += 4;
			break;
		}
		case cinema::COLORMODE::AGRAYf:
		{
			if (bitmap) 
				bitmap->GetPixelCnt(sourceX, sourceY, 1, (cinema::UChar*)colBuffer, 4, cinema::COLORMODE::AGRAYf, cinema::PIXELCNT::NONE);
			cinema::Float32 *colBuf = (cinema::Float32*)colBuffer;
			cinema::Float32 *destBuf = (cinema::Float32*)&buffer[idx];
			destBuf[0] = cinema::Float32(destBuf[0]) * (falloffInterp) + cinema::Float32(colBuf[0]) * falloff;
			destBuf[1] = cinema::Float32(destBuf[1]) * (falloffInterp) + cinema::Float32(colBuf[1]) * falloff;
			idx += 8;
			break;
		}
		case cinema::COLORMODE::RGBf:
		{
			if (bitmap) 
				bitmap->GetPixelCnt(sourceX, sourceY, 1, (cinema::UChar*)colBuffer, 12, cinema::COLORMODE::RGBf, cinema::PIXELCNT::NONE);
			cinema::Float32 *colBuf = (cinema::Float32*)colBuffer;
			cinema::Float32 *destBuf = (cinema::Float32*)&buffer[idx];
			destBuf[0] = cinema::Float32(destBuf[0]) * (falloffInterp) + cinema::Float32(colBuf[0]) * falloff;
			destBuf[1] = cinema::Float32(destBuf[1]) * (falloffInterp) + cinema::Float32(colBuf[1]) * falloff;
			destBuf[2] = cinema::Float32(destBuf[2]) * (falloffInterp) + cinema::Float32(colBuf[2]) * falloff;
			idx += 12;
			break;
		}
		case cinema::COLORMODE::ARGBf:
		{
			if (bitmap) 
				bitmap->GetPixelCnt(sourceX, sourceY, 1, (cinema::UChar*)colBuffer, 12, cinema::COLORMODE::ARGBf, cinema::PIXELCNT::NONE);
			cinema::Float32 *colBuf = (cinema::Float32*)colBuffer;
			cinema::Float32 *destBuf = (cinema::Float32*)&buffer[idx];
			destBuf[0] = cinema::Float32(destBuf[0]) * (falloffInterp) + cinema::Float32(colBuf[0]) * falloff;
			destBuf[1] = cinema::Float32(destBuf[1]) * (falloffInterp) + cinema::Float32(colBuf[1]) * falloff;
			destBuf[2] = cinema::Float32(destBuf[2]) * (falloffInterp) + cinema::Float32(colBuf[2]) * falloff;
			destBuf[3] = cinema::Float32(destBuf[3]) * (falloffInterp) + cinema::Float32(colBuf[3]) * falloff;
			idx += 16;
			break;
		}

		case cinema::COLORMODE::MASK: break;
		case cinema::COLORMODE::MASKw: break;
		case cinema::COLORMODE::MASKf: break;
	}
}

#endif // PAINTCHANNELS_H__

