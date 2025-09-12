#ifndef RASTERIZE_BARY_H__
#define RASTERIZE_BARY_H__

#include "lib_sculptbrush.h"
#include "c4d.h"
#include "paintchannels.h"

/*
	The following code uses the same approach as the simple rasterization algorithm described on Josh Beams website.
	http://joshbeam.com/articles/triangle_rasterization/
*/

struct EdgeBary
{
	EdgeBary(const cinema::Vector &p1, cinema::Int32 x1, cinema::Int32 y1, const cinema::Vector &p2, cinema::Int32 x2, cinema::Int32 y2)
	{
		if (y1 < y2)
		{
			point1 = p1;
			X1 = x1;
			Y1 = y1;
			point2 = p2;
			X2 = x2;
			Y2 = y2;
		}
		else
		{
			point1 = p2;
			X1 = x2;
			Y1 = y2;
			point2 = p1;
			X2 = x1;
			Y2 = y1;
		}
	}

	cinema::Vector point1, point2;
	cinema::Int32 X1, Y1, X2, Y2;
};

struct SpanBary
{
	SpanBary(const cinema::Vector &p1, cinema::Int32 x1, const cinema::Vector &p2, cinema::Int32 x2)
	{
		if (x1 < x2)
		{
			point1 = p1;
			X1 = x1;
			point2 = p2;
			X2 = x2;
		}
		else
		{
			point1 = p2;
			X1 = x2;
			point2 = p1;
			X2 = x1;
		}
	}
	cinema::Vector point1, point2;
	cinema::Int32 X1, X2;
};

inline cinema::Float32 GetBaryValue(const cinema::Vector32 &a, const cinema::Vector32 &b, const cinema::Vector32 &c)
{
	return 1.0f / ((b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y));
}

inline cinema::Vector32 GetBaryCoords(cinema::Float32 den, cinema::Float32 vecX, cinema::Float32 vecY, const cinema::Vector32 &a, const cinema::Vector32 &b, const cinema::Vector32 &c)
{
	cinema::Vector32 lambda;
	lambda.x = ((b.y - c.y) * (vecX - c.x) + (c.x - b.x) * (vecY - c.y)) * den;
	lambda.y = ((c.y - a.y) * (vecX - c.x) + (a.x - c.x) * (vecY - c.y)) * den;
	lambda.z = 1.0f - lambda.x - lambda.y;
	return lambda;
}

inline void DoLineBary(cinema::BrushDabData *dab, PaintChannels *channels, const SpanBary &span, cinema::Int32 y, cinema::Float32 factorStep, const cinema::Vector &posDiff)
{
	cinema::Float32 factor = 0;
	cinema::PaintLayerBmp *paintBmp = channels->channel;
	if (paintBmp)
	{
		cinema::Int32 bitdepth = 8;
		cinema::Int32 numChannels = 0;
		cinema::COLORMODE colorMode = (cinema::COLORMODE)paintBmp->GetColorMode();
		cinema::Bool supported = GetChannelInfo(*paintBmp, bitdepth, numChannels);
		if (!supported)
			return;

		cinema::Int32 bpp = (bitdepth / 8) * numChannels;
		const cinema::Vector &defaultColor = channels->fgColor;

		cinema::Float32 tempBuff[4]; // Use a buffer big enough for 4 32bit values
		tempBuff[0] = (cinema::Float32)defaultColor.x;
		tempBuff[1] = (cinema::Float32)defaultColor.y;
		tempBuff[2] = (cinema::Float32)defaultColor.z;
		tempBuff[3] = 1.0f;


		cinema::UChar colBuffer[16]; // Use a buffer big enough for 4 32bit values
		if (!cinema::PaintBitmap::ConvertBits(1, (cinema::PIX*)tempBuff, 1, cinema::COLORMODE::RGBf, (cinema::PIX*)colBuffer, 1, colorMode, NOTOK, NOTOK))
		{
			return;
		}

		cinema::Int32 numBytes = (span.X2 - span.X1) * bpp;
		iferr (cinema::PIX *pBuffer = NewMem(cinema::PIX, numBytes))
			return;

		paintBmp->GetPixelCnt(span.X1, y, span.X2-span.X1, pBuffer, colorMode, cinema::PIXELCNT::NONE);

		cinema::UInt idx = 0;
		for (cinema::Int32 x = span.X1; x < span.X2; x++)
		{
			cinema::Vector pos = span.point1 + (posDiff * factor);

			cinema::Float32 falloff = 0;
			if (channels->fillTool)
				falloff = (cinema::Float32)channels->strength;
			else
				falloff = (cinema::Float32)(dab->GetBrushFalloffFromPos(pos) * channels->strength);

			if (channels->fillTool && !dab->IsPointInFillArea(pos))
				falloff = 0;

			SetPixel(colorMode, idx, pBuffer, (cinema::Float32)falloff, colBuffer);

			factor += factorStep;
		}
		paintBmp->SetPixelCnt(span.X1, y, span.X2-span.X1, pBuffer, bpp, colorMode, cinema::PIXELCNT::NONE);
		DeleteMem(pBuffer);
	}
}

inline void DoTextureLineBary(cinema::BrushDabData *dab, PaintChannels *channels, const SpanBary &span, cinema::Int32 y, cinema::Float32 factorStep, const cinema::Vector &posDiff, const TriangleData *triP, const cinema::BaseBitmap *pBitmap)
{
	cinema::Float32 factor = 0;
	cinema::PaintLayerBmp *paintBmp = channels->channel;
	if (paintBmp)
	{
		cinema::Bool useStampFalloff = true;
		if (channels->useStamp)
		{
			useStampFalloff = dab->GetData()->GetBool(MDATA_SCULPTBRUSH_STAMP_USEFALLOFF);
		}

		cinema::Bool stencilTileX = dab->GetData()->GetBool(MDATA_SCULPTBRUSH_STENCIL_TILEX);
		cinema::Bool stencilTileY = dab->GetData()->GetBool(MDATA_SCULPTBRUSH_STENCIL_TILEY);

		cinema::Int32 bitdepth = 8;
		cinema::Int32 numChannels = 0;
		cinema::COLORMODE colorMode = (cinema::COLORMODE)paintBmp->GetColorMode();
		cinema::Bool supported = GetChannelInfo(*paintBmp, bitdepth, numChannels);
		if (!supported)
			return;

		cinema::Int32 bpp = (bitdepth / 8) * numChannels;
		cinema::Int32 numBytes = (span.X2 - span.X1) * bpp;
		iferr (cinema::PIX *pBuffer = NewMem(cinema::PIX, numBytes))
			return;

		cinema::UChar colBuffer[16]; // Use a buffer big enough for 4 32bit values

		paintBmp->GetPixelCnt(span.X1, y, span.X2-span.X1, pBuffer, colorMode, cinema::PIXELCNT::NONE);

		cinema::Int32 sourceWidth = pBitmap->GetBw();
		cinema::Int32 sourceHeight = pBitmap->GetBh();

		cinema::Float32 BaryValue = GetBaryValue(triP->destBitmapCoords[0], triP->destBitmapCoords[1], triP->destBitmapCoords[2]);

		cinema::UInt idx = 0;
		for (cinema::Int32 x = span.X1; x < span.X2; x++)
		{
			cinema::Vector32 BaryCoords = GetBaryCoords(BaryValue, (cinema::Float32)x, (cinema::Float32)y, triP->destBitmapCoords[0], triP->destBitmapCoords[1], triP->destBitmapCoords[2]);
			cinema::Vector32 sourcePos = BaryCoords.x * triP->sourceBitmapCoords[0] + BaryCoords.y * triP->sourceBitmapCoords[1] + BaryCoords.z * triP->sourceBitmapCoords[2];

			cinema::Vector pos = span.point1 + (posDiff * factor);

			// Skip the stamp if it is outside of it.
			cinema::Bool skip = false;
			if (sourcePos.x < 0 || sourcePos.x >= cinema::Float32(sourceWidth) || sourcePos.y < 0 || sourcePos.y >= cinema::Float32(sourceHeight))
			{
				if (channels->useStamp)
				{
					skip = true;
				}
				else if (channels->useStencil)
				{
					if (stencilTileX)
					{
						if (sourcePos.x < 0)
							sourcePos.x = cinema::Float32(sourceWidth) + sourcePos.x;
						if (sourcePos.x >= cinema::Float32(sourceWidth))
							sourcePos.x = sourcePos.x - cinema::Float32(sourceWidth);
					}
					else
					{
						skip = true;
					}
					if (stencilTileY)
					{
						if (sourcePos.y < 0)
							sourcePos.y = cinema::Float32(sourceHeight) + sourcePos.y;
						if (sourcePos.y >= cinema::Float32(sourceHeight))
							sourcePos.y = sourcePos.y - cinema::Float32(sourceHeight);
					}
					else
					{
						skip = true;
					}
				}
			}

			cinema::Float32 falloff = 0;
			if (!skip)
			{
				if (channels->fillTool || !useStampFalloff)
					falloff = (cinema::Float32)channels->strength;
				else
					falloff = (cinema::Float32)(dab->GetBrushFalloffFromPos(pos) * channels->strength);
			}

			if (channels->fillTool && !dab->IsPointInFillArea(pos))
				falloff = 0;

			SetPixel(colorMode, idx, pBuffer, (cinema::Float32)falloff, colBuffer, pBitmap, (cinema::Int32)sourcePos.x, (cinema::Int32)sourcePos.y);

			factor += factorStep;
		}
		paintBmp->SetPixelCnt(span.X1, y, span.X2-span.X1, pBuffer, bpp, colorMode, cinema::PIXELCNT::NONE);
		DeleteMem(pBuffer);
	}
}

inline void DrawSpanBary(cinema::BrushDabData *dab, PaintChannels *channels, const SpanBary &span, cinema::Int32 y, const TriangleData *triP)
{
	cinema::Int32 xdiff = span.X2 - span.X1;
	if (xdiff == 0)
		return;

	cinema::Vector posDiff = span.point2 - span.point1;

	cinema::Float32 factorStep = 1.0f / (cinema::Float32)xdiff;

	const cinema::BaseBitmap *pBitmap = nullptr;
	if (channels->useStencil)
	{
		pBitmap = dab->GetStencil();
	}
	else if (channels->useStamp)
	{
		pBitmap = dab->GetStamp();
	}

	if (pBitmap)
	{
		DoTextureLineBary(dab, channels, span, y, factorStep, posDiff, triP, pBitmap);
	}
	else
	{
		DoLineBary(dab, channels, span, y, factorStep, posDiff);
	}
}

inline void DrawSpansBetweenEdgesBary(cinema::BrushDabData *dab, PaintChannels *channels, const EdgeBary &e1, const EdgeBary &e2, const TriangleData *triP)
{
	cinema::Float32 e1ydiff = (cinema::Float32)(e1.Y2 - e1.Y1);
	if (e1ydiff == 0.0f)
		return;

	cinema::Float32 e2ydiff = (cinema::Float32)(e2.Y2 - e2.Y1);
	if (e2ydiff == 0.0f)
		return;

	cinema::Float32 e1xdiff = (cinema::Float32)(e1.X2 - e1.X1);
	cinema::Float32 e2xdiff = (cinema::Float32)(e2.X2 - e2.X1);
	cinema::Vector e1posDiff = (e1.point2 - e1.point1);
	cinema::Vector e2posDiff = (e2.point2 - e2.point1);

	cinema::Float32 factor1 = (cinema::Float32)(e2.Y1 - e1.Y1) / e1ydiff;
	cinema::Float32 factorStep1 = 1.0f / e1ydiff;
	cinema::Float32 factor2 = 0.0f;
	cinema::Float32 factorStep2 = 1.0f / e2ydiff;

	for (cinema::Int32 y = e2.Y1; y < e2.Y2; y++)
	{
		SpanBary span(
			e1.point1 + (e1posDiff * factor1),
			e1.X1 + (cinema::Int32)(e1xdiff * factor1),
			e2.point1 + (e2posDiff * factor2),
			e2.X1 + (cinema::Int32)(e2xdiff * factor2));

		DrawSpanBary(dab, channels, span, y, triP);

		factor1 += factorStep1;
		factor2 += factorStep2;
	}
}

inline cinema::Int32 iround(cinema::Float32 x)
{
	return (cinema::Int32)floor(x+0.5f);
}

inline void DrawTriangle_Bary(cinema::BrushDabData *dab, PaintChannels *channels, TriangleData *triP)
{
	const cinema::Vector &point1 = triP->points3D[0];
	const cinema::Vector &point2 = triP->points3D[1];
	const cinema::Vector &point3 = triP->points3D[2];

	cinema::Float32 x1 = triP->destBitmapCoords[0].x;
	cinema::Float32 y1 = triP->destBitmapCoords[0].y;
	cinema::Float32 x2 = triP->destBitmapCoords[1].x;
	cinema::Float32 y2 = triP->destBitmapCoords[1].y;
	cinema::Float32 x3 = triP->destBitmapCoords[2].x;
	cinema::Float32 y3 = triP->destBitmapCoords[2].y;

	EdgeBary edges[3] = {
		EdgeBary(point1, iround(x1), iround(y1), point2, iround(x2), iround(y2)),
		EdgeBary(point2, iround(x2), iround(y2), point3, iround(x3), iround(y3)),
		EdgeBary(point3, iround(x3), iround(y3), point1, iround(x1), iround(y1))
	};

	cinema::Int32 maxLength = 0;
	cinema::Int32 longEdge = 0;

	for (cinema::Int32 i = 0; i < 3; i++)
	{
		cinema::Int32 length = edges[i].Y2 - edges[i].Y1;
		if (length > maxLength)
		{
			maxLength = length;
			longEdge = i;
		}
	}

	cinema::Int32 shortEdge1 = (longEdge + 1) % 3;
	cinema::Int32 shortEdge2 = (longEdge + 2) % 3;

	DrawSpansBetweenEdgesBary(dab, channels, edges[longEdge], edges[shortEdge1], triP);
	DrawSpansBetweenEdgesBary(dab, channels, edges[longEdge], edges[shortEdge2], triP);
}

#endif // RASTERIZE_BARY_H__
