// This is an example of a brush that will draw polygon strips on top of the underlying object.
// It is a brush implementation of the "sculpting/drawpoly.cpp" tool from the SDK.

#include "lib_sculptbrush.h"
#include "c4d_symbols.h"
#include "c4d.h"
#include "lib_modeling.h"
#include "main.h"

#define SCULPTDRAWPOLYBRUSH_SDK_EXAMPLE 1029861	//You MUST get your own ID from developers.maxon.net

using namespace cinema;

struct StrokeData
{
	Vector p1, p2, p3, p4;
	Int32	 index1, index2, index3, index4;

	Bool	 firstHit;
	Bool	 firstPointDone;
	Vector hitPoint;

	Int32	 strokeId;

	StrokeData()
	{
		strokeId = -1;
		index1 = index2 = index3 = index4 = 0;
		firstHit = false;
		firstPointDone = false;
	}
};

class ExampleSculptDrawPolyBrush : public SculptBrushToolData
{
public:
	explicit ExampleSculptDrawPolyBrush(SculptBrushParams* pParams) : SculptBrushToolData(pParams) { }
	~ExampleSculptDrawPolyBrush() { }

	virtual Int32 GetToolPluginId() const;
	virtual const String GetResourceSymbol() const;

	virtual void StartStroke(Int32 strokeCount, const BaseContainer& data);
	virtual void EndStroke();

	virtual void StartStrokeInstance(Int32 strokeInstanceID);
	virtual void EndStrokeInstance(Int32 strokeInstanceID);

	static Bool MovePointsFunc(BrushDabData* dab);

public:
	maxon::BaseArray<StrokeData> _strokeData;
	BaseDocument*					_doc;
	Int32									_strokeCounter;
	PolygonObject*				_poly;
	AutoAlloc<Modeling>		_mod;
};

Int32 ExampleSculptDrawPolyBrush::GetToolPluginId() const
{
	return SCULPTDRAWPOLYBRUSH_SDK_EXAMPLE;
}

const String ExampleSculptDrawPolyBrush::GetResourceSymbol() const
{
	// Return the name of the .res file, in the res/description and res/strings folder, for this tool.
	return String("toolsculptdrawpolybrush");
}


void ExampleSculptDrawPolyBrush::StartStroke(Int32 strokeCount, const BaseContainer& data)
{
	_strokeCounter = 0;

	iferr (_strokeData.Resize(strokeCount))
		return;
	// At the start of the brush stroke we get the active document and call StartUndo on it since we are handling Undo ourselves.
	_doc = GetActiveDocument();
	_doc->StartUndo();

	_poly = PolygonObject::Alloc(0, 0);

	// Add the null object to the document.
	_doc->InsertObject(_poly, nullptr, nullptr);

	// Add an undo event for this null object.
	_doc->AddUndo(UNDOTYPE::NEWOBJ, _poly);

	if (!_mod || !_mod->InitObject(_poly))
	{
		return;
	}
}

void ExampleSculptDrawPolyBrush::EndStroke()
{
	_strokeData.Reset();
	// When the stroke ends (which happens on mouse up) we end the Undo for this brush stroke.
	_doc->EndUndo();
	_doc = nullptr;
}

void ExampleSculptDrawPolyBrush::StartStrokeInstance(Int32 strokeInstanceID)
{
	if (_strokeCounter < _strokeData.GetCount())
	{
		_strokeData[_strokeCounter++].strokeId = strokeInstanceID;
	}
}

void ExampleSculptDrawPolyBrush::EndStrokeInstance(Int32 strokeInstanceID)
{
	// Nothing to do here.
}

// This method does all the work for the brush. Every time a dab is placed down on the surface this method is called. It will
// be called for every symmetrical dab as well, but you do not need to worry about them at all.
Bool ExampleSculptDrawPolyBrush::MovePointsFunc(BrushDabData* dab)
{
	// Since we have enabled brush access via the call to EnableBrushAccess(true) we can now access the brush directly from this static MovePointFunc method.
	// This lets us access the member variables of the brush.
	ExampleSculptDrawPolyBrush* pBrush = (ExampleSculptDrawPolyBrush*)dab->GetBrush();
	if (!pBrush)
		return false;

	// Get the correct StrokeData for this dab
	StrokeData* pData = nullptr;
	Int32				count = (Int32)pBrush->_strokeData.GetCount();
	Int32				i;
	for (i = 0; i < count; i++)
	{
		if (pBrush->_strokeData[i].strokeId == dab->GetStrokeInstanceID())
		{
			pData = &pBrush->_strokeData[i];
		}
	}

	if (!pData || !pBrush->_poly || !pBrush->_mod)
	{
		return false;
	}

	Float distance = dab->GetBrushRadius();

	if (!pData->firstHit)
	{
		pData->firstHit = true;
		pData->hitPoint = dab->GetHitPoint() + dab->GetNormal();
		return true;
	}

	Vector normal = dab->GetNormal();
	Vector newP = dab->GetHitPoint() + normal;
	Vector diff = pData->hitPoint - newP;
	Float	 len	= diff.GetLength();
	if (len > distance)
	{
		Vector cross = Cross(normal, diff.GetNormalized());
		Vector gap = cross * distance * 0.5;
		if (!pData->firstPointDone)
		{
			pData->firstPointDone = true;
			pData->p1 = newP - gap;
			pData->p2 = newP + gap;

			pData->index1 = pBrush->_mod->AddPoint(pBrush->_poly, pData->p1);
			pData->index2 = pBrush->_mod->AddPoint(pBrush->_poly, pData->p2);
		}

		pData->p3 = newP + gap;
		pData->p4 = newP - gap;

		pData->index3 = pBrush->_mod->AddPoint(pBrush->_poly, pData->p3);
		pData->index4 = pBrush->_mod->AddPoint(pBrush->_poly, pData->p4);

		Int32 padr[4] = { pData->index1, pData->index2, pData->index3, pData->index4 };

		pData->index1 = (-pData->index4) - 1;
		pData->index2 = (-pData->index3) - 1;
		pData->hitPoint = newP;

		i = pBrush->_mod->CreateNgon(pBrush->_poly, padr, 4, MODELING_SETNGON_FLAG_FIXEDQUADS);
		if (i == 0)
			return true;

		if (!pBrush->_mod->Commit(pBrush->_poly, MODELING_COMMIT_UPDATE))
			return true;
	}
	return true;
}

Bool RegisterSculptDrawPolyBrush()
{
	SculptBrushParams* pParams = SculptBrushParams::Alloc();
	if (!pParams)
		return false;

	// This brush does not use stencils
	pParams->EnableStencil(false);

	// This brush does not use stamps
	pParams->EnableStamp(false);

	// Since we are using StartStroke/EndStroke calls, and also because we need access to the brush
	// itself from within the MovePointFunc (dab->GetBrush()), we need to set this to true.
	pParams->EnableBrushAccess(true);

	// Tell the system we are going to handle undo ourselves
	pParams->SetUndoType(SCULPTBRUSHDATATYPE::NONE);

	// Pass in the pointer to the static MovePointsFunc. This will get called for every dab.
	pParams->SetMovePointFunc(&ExampleSculptDrawPolyBrush::MovePointsFunc);

	// Register the tool with Cinema4D.
	return RegisterToolPlugin(SCULPTDRAWPOLYBRUSH_SDK_EXAMPLE, GeLoadString(IDS_SCULPTDRAWPOLYBRUSH_TOOL), PLUGINFLAG_TOOL_SCULPTBRUSH | PLUGINFLAG_TOOL_NO_OBJECTOUTLINE, nullptr, GeLoadString(IDS_SCULPTDRAWPOLYBRUSH_TOOL), NewObjClear(ExampleSculptDrawPolyBrush, pParams));
}
