#include "c4d.h"
#include "paintchannels.h"
#include "paintundo.h"

using namespace cinema;

Bool PaintChannels::Init()
{
	minmax[0] = LIMIT<Int32>::MAX;
	minmax[1] = LIMIT<Int32>::MAX;
	minmax[2] = LIMIT<Int32>::MIN;
	minmax[3] = LIMIT<Int32>::MIN;
	return true;
}

void PaintChannels::UpdateBitmaps()
{
	if (channel)
		channel->UpdateRefresh(minmax[0], minmax[1], minmax[2], minmax[3], UPDATE_STD);
}

void PaintChannels::SetupPoly_Pixel(BrushDabData* dab, const CPolygon &p, const UVWStruct &polyUVs, const Vector* polyPoints)
{
	if (channel)
	{
		BaseDocument* doc = GetActiveDocument();
		if (doc == nullptr)
			return;

		Int32 w = channel->GetBw();
		Int32 h = channel->GetBh();

		Float32 aX = (Float32)(polyUVs.a.x * w);
		Float32 aY = (Float32)(polyUVs.a.y * h);
		Float32 bX = (Float32)(polyUVs.b.x * w);
		Float32 bY = (Float32)(polyUVs.b.y * h);
		Float32 cX = (Float32)(polyUVs.c.x * w);
		Float32 cY = (Float32)(polyUVs.c.y * h);
		Float32 dX = (Float32)(polyUVs.d.x * w);
		Float32 dY = (Float32)(polyUVs.d.y * h);

		PaintUndoSystem *pPaintSystem = GetPaintUndoSystem(doc);
		if (pPaintSystem)
		{
			pPaintSystem->AddUndoRedo(*channel, (int)aX, (int)aY);
			pPaintSystem->AddUndoRedo(*channel, (int)bX, (int)bY);
			pPaintSystem->AddUndoRedo(*channel, (int)cX, (int)cY);
			pPaintSystem->AddUndoRedo(*channel, (int)dX, (int)dY);
		}

		minmax[0] = maxon::Min(minmax[0], maxon::Min((Int32)aX, maxon::Min((Int32)bX, (Int32)cX)));
		minmax[1] = maxon::Min(minmax[1], maxon::Min((Int32)aY, maxon::Min((Int32)bY, (Int32)cY)));
		minmax[2] = maxon::Max(minmax[2], maxon::Max((Int32)aX, maxon::Max((Int32)bX, (Int32)cX)));
		minmax[3] = maxon::Max(minmax[3], maxon::Max((Int32)aY, maxon::Max((Int32)bY, (Int32)cY)));

		if (p.c != p.d)
		{
			minmax[0] = maxon::Min(minmax[0], (Int32)dX);
			minmax[1] = maxon::Min(minmax[1], (Int32)dY);
			minmax[2] = maxon::Max(minmax[2], (Int32)dX);
			minmax[3] = maxon::Max(minmax[3], (Int32)dY);
		}

		triangle[0].destBitmapCoords[0].x = aX;
		triangle[0].destBitmapCoords[0].y = aY;
		triangle[0].destBitmapCoords[1].x = bX;
		triangle[0].destBitmapCoords[1].y = bY;
		triangle[0].destBitmapCoords[2].x = cX;
		triangle[0].destBitmapCoords[2].y = cY;

		triangle[0].points3D[0] = polyPoints[p.a];
		triangle[0].points3D[1] = polyPoints[p.b];
		triangle[0].points3D[2] = polyPoints[p.c];

		if (p.c != p.d)
		{
			triangle[1].destBitmapCoords[0].x = aX;
			triangle[1].destBitmapCoords[0].y = aY;
			triangle[1].destBitmapCoords[1].x = cX;
			triangle[1].destBitmapCoords[1].y = cY;
			triangle[1].destBitmapCoords[2].x = dX;
			triangle[1].destBitmapCoords[2].y = dY;

			triangle[1].points3D[0] = polyPoints[p.a];
			triangle[1].points3D[1] = polyPoints[p.c];
			triangle[1].points3D[2] = polyPoints[p.d];
		}
	}
}


void PaintChannels::SetupPoly_Bary(BrushDabData* dab, const CPolygon &p, const UVWStruct &polyUVs, const Vector* polyPoints)
{
	SetupPoly_Pixel(dab, p, polyUVs, polyPoints);
	if (channel)
	{
		if (useStamp)
		{
			dab->GetStampColor(triangle[0].points3D[0], 1.0, nullptr, &triangle[0].sourceBitmapCoords[0]);
			dab->GetStampColor(triangle[0].points3D[1], 1.0, nullptr, &triangle[0].sourceBitmapCoords[1]);
			dab->GetStampColor(triangle[0].points3D[2], 1.0, nullptr, &triangle[0].sourceBitmapCoords[2]);
		}
		else if (useStencil)
		{
			dab->GetStencilColor(triangle[0].points3D[0], nullptr, &triangle[0].sourceBitmapCoords[0]);
			dab->GetStencilColor(triangle[0].points3D[1], nullptr, &triangle[0].sourceBitmapCoords[1]);
			dab->GetStencilColor(triangle[0].points3D[2], nullptr, &triangle[0].sourceBitmapCoords[2]);
		}
		if (p.c != p.d)
		{
			if (useStamp)
			{
				dab->GetStampColor(triangle[1].points3D[0], 1.0, nullptr, &triangle[1].sourceBitmapCoords[0]);
				dab->GetStampColor(triangle[1].points3D[1], 1.0, nullptr, &triangle[1].sourceBitmapCoords[1]);
				dab->GetStampColor(triangle[1].points3D[2], 1.0, nullptr, &triangle[1].sourceBitmapCoords[2]);
			}
			else if (useStencil)
			{
				dab->GetStencilColor(triangle[1].points3D[0], nullptr, &triangle[1].sourceBitmapCoords[0]);
				dab->GetStencilColor(triangle[1].points3D[1], nullptr, &triangle[1].sourceBitmapCoords[1]);
				dab->GetStencilColor(triangle[1].points3D[2], nullptr, &triangle[1].sourceBitmapCoords[2]);
			}
		}
	}
}
