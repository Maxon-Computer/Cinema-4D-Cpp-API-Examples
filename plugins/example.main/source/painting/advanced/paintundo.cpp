#include "paintundo.h"
#include "paintchannels.h"
#include "registeradvancedpaint.h"

using namespace cinema;

static constexpr const Int32 SCENEHOOK_VERSION =  1;

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_PAINT_UNDOREDO_SCENEHOOK = 1031368;

//=============================================================================================================

static constexpr const Char* SCULPTPAINTUNDOSTRING  = "SculptPaintUndo";


static Bool CreateSculptUndo(BaseDocument& doc)
{
	BaseSceneHook *sceneHook = doc.FindSceneHook(ID_PAINT_UNDOREDO_SCENEHOOK);
	if (sceneHook)
	{
		BaseContainer *bc = sceneHook->GetDataInstance();
		bc->SetString((Int32)UNDOTYPE::PRIVATE_STRING, String(SCULPTPAINTUNDOSTRING));
		doc.StartUndo();
		doc.AddUndo(UNDOTYPE::PRIVATE_STRING, sceneHook);
		doc.EndUndo();
		return true;
	}
	return false;
}

Bool PaintUndoTile::Init(PaintLayerBmp& bitmap, Int x, Int y)
{
	iferr_scope_handler { return false; };
	
	_pDestBitmap = maxon::UniqueRef<cinema::BaseLink>::Create() iferr_return;

	_pDestBitmap->SetLink(&bitmap);

	_xTile = (Int)(x * Int(PAINTTILEINV));
	_yTile = (Int)(y * Int(PAINTTILEINV));

	_x = _xTile * PAINTTILESIZE;
	_y = _yTile * PAINTTILESIZE;

	int bitdepth, numChannels;
	if (!GetChannelInfo(bitmap, bitdepth, numChannels))
		return false;

	COLORMODE colorMode = (COLORMODE)bitmap.GetColorMode();
	Int32 bitsPerPixel = (bitdepth / 8) * numChannels;
	_data.Resize(PAINTTILESIZE * PAINTTILESIZE * bitsPerPixel) iferr_return;

	// Copy the data across
	UChar *pos = _data.GetFirst();
	for (Int32 yy = 0; yy < PAINTTILESIZE; yy++)
	{
		bitmap.GetPixelCnt((Int32)_x, (Int32)_y + yy, (Int32)PAINTTILESIZE, &pos[yy*PAINTTILESIZE*bitsPerPixel], colorMode, PIXELCNT::NONE);
	}

	return true;
}

PaintLayerBmp *PaintUndoTile::GetBitmap()
{
	return  (PaintLayerBmp*) (_pDestBitmap ? _pDestBitmap->ForceGetLink() : nullptr);
}

void PaintUndoTile::Apply()
{
	PaintLayerBmp *bitmap = GetBitmap();
	if (!bitmap)
		return;

	int bitdepth, numChannels;
	if (!GetChannelInfo(*bitmap, bitdepth, numChannels))
		return;

	COLORMODE colorMode = (COLORMODE)bitmap->GetColorMode();
	Int32 bitsPerPixel = (bitdepth / 8) * numChannels;

	// Copy the data across
	UChar *pos = _data.GetFirst();
	for (Int32 y = 0; y < PAINTTILESIZE; y++)
	{
		bitmap->SetPixelCnt((Int32)_x, (Int32)_y + y, PAINTTILESIZE, &pos[y*PAINTTILESIZE*bitsPerPixel], bitsPerPixel, colorMode , PIXELCNT::NONE);
	}

	bitmap->UpdateRefresh((Int32)_x, (Int32)_y, (Int32)_x+PAINTTILESIZE, (Int32)_y+PAINTTILESIZE, UPDATE_STD);
}

PaintUndoTile *PaintUndoTile::GetCurrentStateClone()
{
	PaintLayerBmp *bitmap = GetBitmap();
	if (!bitmap)
		return nullptr;
	iferr (PaintUndoTile *tile = NewObj(PaintUndoTile))
		return nullptr;
	tile->Init(*bitmap, _x, _y);
	return tile;
}

//=============================================================================================================

PaintUndoStroke::PaintUndoStroke()
{
}

PaintUndoStroke::~PaintUndoStroke()
{
}

void PaintUndoStroke::Init()
{
}

void PaintUndoStroke::Init(PaintUndoStroke& stroke)
{
	for (auto& a : stroke._tiles)
	{
		AddUndoTile(a.GetCurrentStateClone());
	}
}

inline Int GetTileHash(maxon::Pair<Int, Int>&& xy)
{
	Int xx = (Int)(xy.first * Int(PAINTTILEINV));
	Int yy = (Int)(xy.second * Int(PAINTTILEINV));
	return xx + PAINTTILESIZE * yy;
}

void PaintUndoStroke::AddUndoTile(PaintUndoTile *pTile)
{
  iferr_scope_handler
  {
    return;
  };

	if (pTile)
	{
		_tiles.AppendPtr(pTile) iferr_return;
		_tileMap.Insert(GetTileHash(pTile->GetXY()), pTile) iferr_return;
	}
}

void PaintUndoStroke::Apply()
{
	for (auto& a : _tiles)
	{
		a.Apply();
	}
	DrawViews(DRAWFLAGS::PRIVATE_NO_WAIT_GL_FINISHED|DRAWFLAGS::ONLY_ACTIVE_VIEW|DRAWFLAGS::NO_THREAD|DRAWFLAGS::NO_ANIMATION);
}

PaintUndoTile* PaintUndoStroke::Find(Int x, Int y)
{
	auto* entry = _tileMap.Find(GetTileHash({ x, y }));
	return entry ? const_cast<PaintUndoTile*>(entry->GetValue()) : nullptr;
}


//=============================================================================================================


PaintUndoRedo::~PaintUndoRedo()
{
	ClearRedos();
	ClearUndos();
}

void PaintUndoRedo::ClearRedos()
{
	for (auto& stroke : _redoStrokes)
	{
		DeleteObj(stroke);
	}
	_redoStrokes.Reset();
}

void PaintUndoRedo::ClearUndos()
{
	for (auto& stroke : _undoStrokes)
	{
		DeleteObj(stroke);
	}
	_undoStrokes.Reset();
}

void PaintUndoRedo::Undo()
{
	if (_undoStrokes.GetCount() > 0)
	{
		PaintUndoStroke *lastStroke = nullptr;
		if (_undoStrokes.Pop(&lastStroke))
		{
			DebugAssert(lastStroke != nullptr);
			iferr (PaintUndoStroke *redoStroke = NewObj(PaintUndoStroke))
				return;
			redoStroke->Init(*lastStroke);
			iferr (_redoStrokes.Append(redoStroke))
        return;
			lastStroke->Apply();
			DeleteObj(lastStroke);
			lastStroke = nullptr;
		}
	}
}

void PaintUndoRedo::Redo()
{
	if (_redoStrokes.GetCount() > 0)
	{
		PaintUndoStroke *lastStroke = nullptr;
		if (_redoStrokes.Pop(&lastStroke))
		{
			DebugAssert(lastStroke != nullptr);
			iferr (PaintUndoStroke *undoStroke = NewObj(PaintUndoStroke))
				return;
			undoStroke->Init(*lastStroke);
			iferr (_undoStrokes.Append(undoStroke))
        return;
			lastStroke->Apply();
			DeleteObj(lastStroke);
			lastStroke = nullptr;
		}
	}
}

void PaintUndoRedo::FlushUndoBuffer()
{
	ClearRedos();
	ClearUndos();
}


Bool PaintUndoRedo::StartUndoStroke()
{
	ClearRedos();
	if (!_currentStroke)
	{
		_currentStroke = NewObjClear(PaintUndoStroke);
		if (!_currentStroke)
			return false;
	}
	return true;
}

void PaintUndoRedo::EndUndoStroke()
{
	if (_currentStroke)
	{
		if (_undoStrokes.EnsureCapacity(_undoStrokes.GetCount() + 1) == maxon::OK)
			_undoStrokes.Append(_currentStroke.Disconnect()) iferr_cannot_fail("EnsureCapacity was successful");
	}
}


Bool PaintUndoRedo::AddUndoTile(PaintLayerBmp& bitmap, Int x, Int y)
{
	if (_currentStroke)
	{
		if (_currentStroke->Find(x, y))
			return false;

		iferr (auto undoTile = maxon::UniqueRef<PaintUndoTile>::Create())
			return false;

		if (undoTile->Init(bitmap, x, y))
			return false;

		_currentStroke->AddUndoTile(undoTile.Disconnect());
		return true;
	}
	return false;
}

//=======================================================
// Sculpt Undo/Redo Scene Hook
//=======================================================
NodeData *PaintUndoSystem::Alloc()
{
	return NewObjClear(PaintUndoSystem);
}

Bool PaintUndoSystem::Init(GeListNode* node, Bool isCloneInit)
{
	_undoRedo = NewObjClear(PaintUndoRedo);
	return _undoRedo ? true : false;
}

Bool PaintUndoSystem::Message(GeListNode *node, Int32 type, void *data)
{
	switch (type)
	{
		case MSG_STRINGUNDO:
		{
			StringUndo *su = (StringUndo*)data;
			if (su && su->str == SCULPTPAINTUNDOSTRING)
			{
				if (su->redo) 
					Redo();
				else 
					Undo();
			}
			break;
		}
		default: break;
	}
	return SceneHookData::Message(node, type, data);
}

Bool PaintUndoSystem::AddUndoRedo(PaintLayerBmp& bitmap, Int x, Int y)
{
	return _undoRedo->AddUndoTile(bitmap, x, y);
}

Bool PaintUndoSystem::Undo()
{
	if (_undoRedo && _lock.AttemptLock())
	{
		_undoRedo->Undo();
		_lock.Unlock();
	}
	return true;
}

Bool PaintUndoSystem::Redo()
{
	if (_undoRedo && _lock.AttemptLock())
	{
		_undoRedo->Redo();
		_lock.Unlock();
	}
	return true;
}

Bool PaintUndoSystem::StartStroke()
{
	if (_undoRedo == nullptr)
		return false;

	return _undoRedo->StartUndoStroke();
 }

Bool PaintUndoSystem::EndStroke()
{
	if (_undoRedo)
	{
		_undoRedo->EndUndoStroke();
		BaseDocument* doc = GetActiveDocument();
		if (doc)
		{
			CreateSculptUndo(*doc);
			return true;
		}
	}
	return false;
}

PaintUndoStroke *PaintUndoSystem::GetCurrentStroke()
{
	if (_undoRedo)
		return _undoRedo->GetCurrentStroke();

	return nullptr;
}

Bool PaintUndoSystem::FlushUndoBuffer()
{
	if (_undoRedo)
	{
		_undoRedo->FlushUndoBuffer();
		return true;
	}
	return false;
}

Bool RegisterPaintUndoSystem()
{
	return RegisterSceneHookPlugin(ID_PAINT_UNDOREDO_SCENEHOOK, "Paint Brush"_s, PLUGINFLAG_HIDE|PLUGINFLAG_SCENEHOOK_SUPPORT_ANIMATION, PaintUndoSystem::Alloc, EXECUTIONPRIORITY_GENERATOR, SCENEHOOK_VERSION);
}

Bool FreePaintUndoSystem()
{
	return true;
}





PaintUndoSystem* GetPaintUndoSystem(BaseDocument* doc)
{
	if (!doc)  
		return nullptr;

	PaintUndoSystem *undoRedo = nullptr;
	BaseSceneHook *undoRedoHook = doc->FindSceneHook(ID_PAINT_UNDOREDO_SCENEHOOK);
	if (undoRedoHook)
		undoRedo = undoRedoHook->GetNodeData<PaintUndoSystem>();

	return undoRedo;
}
