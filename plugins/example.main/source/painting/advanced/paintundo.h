#ifndef PAINTUNDO_H__
#define PAINTUNDO_H__

#include "c4d.h"
#include "maxon/pointerarray.h"
#include "maxon/hashmap.h"

static constexpr const cinema::Int32 PAINTTILESIZE = 64;
static constexpr const cinema::Float PAINTTILEINV = 1.0/64.0;

class PaintUndoTile
{
public:
	PaintUndoTile() = default;
	virtual ~PaintUndoTile() = default;

	cinema::Bool Init(cinema::PaintLayerBmp& bitmap, cinema::Int x, cinema::Int y);
	cinema::PaintLayerBmp* GetBitmap();
	void Apply();

	PaintUndoTile *GetCurrentStateClone();
	maxon::Pair<cinema::Int, cinema::Int> GetXY() const { return { _x, _y }; }

private:
	maxon::UniqueRef<cinema::BaseLink> _pDestBitmap;
	maxon::BaseArray<cinema::UChar> _data;
	cinema::Int _x = 0;
	cinema::Int _y = 0;
	cinema::Int _xTile = 0;
	cinema::Int _yTile = 0;
};

class PaintUndoStroke
{
public:
	PaintUndoStroke();
	~PaintUndoStroke();

	void Init();
	void Init(PaintUndoStroke& stroke);
	PaintUndoTile* Find(cinema::Int x, cinema::Int y);

	void AddUndoTile(PaintUndoTile *pTile);
	void Apply();

	cinema::Int GetTileCount() { return _tiles.GetCount(); }
	PaintUndoTile* GetUndoData(cinema::Int32 a) { return &_tiles[a]; }

public:
	maxon::PointerArray<PaintUndoTile> _tiles;
	maxon::HashMap<cinema::Int, PaintUndoTile*> _tileMap;
};

class PaintUndoRedo
{
public:
	PaintUndoRedo() = default;
	~PaintUndoRedo();

	cinema::Bool StartUndoStroke();
	void EndUndoStroke();

	cinema::Bool AddUndoTile(cinema::PaintLayerBmp& bitmap, cinema::Int x, cinema::Int y);
	cinema::Bool ApplyStroke(PaintUndoStroke& stroke);

	void Undo();
	void Redo();

	void ClearUndos();
	void ClearRedos();

	void FlushUndoBuffer();

	PaintUndoStroke *GetCurrentStroke() { return _currentStroke; }

private:
	maxon::BaseArray<PaintUndoStroke*> _undoStrokes;
	maxon::BaseArray<PaintUndoStroke*> _redoStrokes;
	maxon::UniqueRef<PaintUndoStroke> _currentStroke;
};

class PaintUndoSystem : public cinema::SceneHookData
{
	INSTANCEOF(PaintUndoSystem , cinema::SceneHookData)

private:
	PaintUndoSystem() = default;
	~PaintUndoSystem() = default;

public:
	virtual cinema::Bool Init(cinema::GeListNode* node, cinema::Bool isCloneInit);
	virtual cinema::Bool Message(cinema::GeListNode *node, cinema::Int32 type, void *data);

public:
	static cinema::NodeData *Alloc();

	cinema::Bool AddUndoRedo(cinema::PaintLayerBmp& bitmap, cinema::Int x, cinema::Int y);
	cinema::Bool Undo();
	cinema::Bool Redo();

	cinema::Bool FlushUndoBuffer();

	cinema::Bool StartStroke();
	cinema::Bool EndStroke();

	PaintUndoStroke *GetCurrentStroke();

private:
	maxon::UniqueRef<PaintUndoRedo> _undoRedo;
	cinema::Bool _undoEvent = true;

	maxon::Spinlock _lock; // Avoid multiple calls to Undo or Redo
};

PaintUndoSystem* GetPaintUndoSystem(cinema::BaseDocument* doc);

#endif // PAINTUNDO_H__
