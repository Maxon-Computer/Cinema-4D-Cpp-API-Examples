#include "lib_sculptbrush.h"
#include "c4d_symbols.h"
#include "c4d.h"
#include "main.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_SCULPT_BRUSH_MULTISTAMP = 1030685;


class SculptBrushMultiStamp : public SculptBrushToolData
{
public:
	explicit SculptBrushMultiStamp(SculptBrushParams *pParams);
	virtual ~SculptBrushMultiStamp();

	virtual Int32 GetToolPluginId() const;
	virtual const String GetResourceSymbol() const;
	virtual Bool HasDrawMode(Int32 mode)
	{
		return true; // Has all Draw Modes
	}

	virtual void StartDab(Int32 strokeInstanceID);
	virtual Bool GetCustomData(Int32 strokeInstanceID, SculptCustomData *pCustom);

	virtual Bool SetDParameter(BaseDocument* doc, BaseContainer& data, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags);

	void InitStamps(const Filename &folder);
	void FreeStamps();

	static Bool MovePointsFunc(BrushDabData *dab);

private:
	maxon::BaseArray<BaseBitmap*> _bitmaps;
	Int32 _currentMap = 0;
};

SculptBrushMultiStamp::SculptBrushMultiStamp(SculptBrushParams *pParams) : SculptBrushToolData(pParams)
{
}

SculptBrushMultiStamp::~SculptBrushMultiStamp()
{
	FreeStamps();
}

void SculptBrushMultiStamp::FreeStamps()
{
	BaseBitmap *pBitmap = nullptr;
	while (_bitmaps.Pop(&pBitmap))
	{
		BaseBitmap::Free(pBitmap);
	}

	_currentMap = 0;
}

void SculptBrushMultiStamp::InitStamps(const Filename &folder)
{
	if (!folder.IsPopulated())
	{
		FreeStamps();
		return;
	}

	AutoAlloc<BrowseFiles> files;
	files->Init(folder, 0);
	Int count = 0;
	Int i = 0;
	while (files->GetNext())
	{
		count++;
	}
	if (count > 0)
	{
		files->Init(folder, 0);
		StatusSetText("Loading Stamps"_s);
		while (files->GetNext())
		{
			StatusSetBar(Int32(i / count));
			i++;

			if (_bitmaps.EnsureCapacity(_bitmaps.GetCount() + 1) == maxon::OK)
			{
				maxon::UniqueRef<BaseBitmap> pBitmap = BaseBitmap::Alloc();
				if (pBitmap && pBitmap->Init(folder + files->GetFilename()) == IMAGERESULT::OK)
				{
					_bitmaps.Append(pBitmap.Disconnect()) iferr_cannot_fail("Entry was reserved");
				}
			}
		}
	}
	StatusClear();
}

Bool SculptBrushMultiStamp::SetDParameter(BaseDocument* doc, BaseContainer& data, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags)
{
	if (!SculptBrushToolData::SetDParameter(doc, data, id, t_data, flags)) 
		return false;

	// Load in all the stamps that are in the same directory as the selected one
	if (id[0].id == MDATA_SCULPTBRUSH_STAMP_TEXTUREFILENAME)
	{
		FreeStamps();
		Filename fileName = t_data.GetFilename();
		InitStamps(fileName.GetDirectory());
	}
	return true;
}


Int32 SculptBrushMultiStamp::GetToolPluginId() const
{
	return ID_SCULPT_BRUSH_MULTISTAMP;
}

const String SculptBrushMultiStamp::GetResourceSymbol() const
{
	return String("ToolSculptBrushMultiStamp");
}

void SculptBrushMultiStamp::StartDab(Int32 strokeInstanceID)
{
	// Swap the bitmaps
	_currentMap++;
	if (_currentMap >= _bitmaps.GetCount())
	{
		_currentMap = 0;
	}
}

Bool SculptBrushMultiStamp::GetCustomData(Int32 strokeInstanceID, SculptCustomData *pCustom)
{
	if (_currentMap >= 0 && _currentMap < _bitmaps.GetCount())
	{
		pCustom->pStamp = _bitmaps[_currentMap];
		return pCustom->pStamp != nullptr;
	}
	return false;
}

Bool SculptBrushMultiStamp::MovePointsFunc(BrushDabData *dab)
{
	if (dab->GetBrushStrength() == 0) 
		return true;

	const BaseContainer *brushData = dab->GetData();

	Int32 a;
	Float buildup = brushData->GetFloat(MDATA_SCULPTBRUSH_SETTINGS_BUILDUP) * 0.002;

	Bool usePreview = dab->IsPreviewDab();

	Float brushRadius = dab->GetBrushRadius();
	if (brushRadius <= 0) 
		return false;

	PolygonObject *polyObj = dab->GetPolygonObject();
	if (!polyObj) 
		return false;

	Float dim = polyObj->GetRad().GetLength() * 0.005;

	Vector normal = dab->GetDrawDirectionNormal();
	Float pressurePreMult = dab->GetBrushStrength() * 10.0 * buildup * dim;
	Vector multPreMult = normal * pressurePreMult;
	if (dab->GetBrushOverride() & OVERRIDE::INVERT)
	{
		multPreMult = -multPreMult;
	}

	if (brushData->GetBool(MDATA_SCULPTBRUSH_SETTINGS_INVERT))
	{
		multPreMult = -multPreMult;
	}

	Int32 count = dab->GetPointCount();
	const BrushPointData *pPointData = dab->GetPointData();
	for (a = 0; a < count; ++a)
	{
		Int32 pointIndex = pPointData[a].pointIndex;
		Float fallOff = dab->GetBrushFalloff(a);
		if (fallOff == 0) 
			continue;
		Vector res = fallOff * multPreMult;

		if (!usePreview)
			dab->OffsetPoint(pointIndex, res);
		else
			dab->OffsetPreviewPoint(pointIndex, res);
	}
	return true;
}

Bool RegisterSculptBrushMultiStamp()
{
	String name = GeLoadString(IDS_SCULPT_BRUSH_MULTISTAMP); 
	if (!name.IsPopulated()) 
		return true;
	SculptBrushParams *pParams = SculptBrushParams::Alloc();
	if (!pParams) 
		return false;

	pParams->EnableInvertCheckbox(true);
	pParams->EnableBuildup(true);
	pParams->EnableDrawDirection(true);
	pParams->EnableCustomStamp(true);
	pParams->EnableBrushAccess(true);
	pParams->EnableRespectSelections(true);

	pParams->SetUndoType(SCULPTBRUSHDATATYPE::POINT);
	pParams->SetMovePointFunc(&SculptBrushMultiStamp::MovePointsFunc);

	return RegisterToolPlugin(ID_SCULPT_BRUSH_MULTISTAMP, name, PLUGINFLAG_HIDEPLUGINMENU | PLUGINFLAG_TOOL_SCULPTBRUSH | PLUGINFLAG_TOOL_NO_OBJECTOUTLINE | PLUGINFLAG_TOOL_NO_TOPOLOGY_EDIT, nullptr, String(), NewObjClear(SculptBrushMultiStamp, pParams));
}
