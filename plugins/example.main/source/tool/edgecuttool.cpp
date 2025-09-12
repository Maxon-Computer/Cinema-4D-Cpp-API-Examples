#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_modeling.h"
#include "main.h"

#include "tooledgecutsdk.h"


using namespace cinema;

static constexpr const Float SUBDIV_DELTA = 30.0;

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_MODELING_EDGECUT_TOOL_SDK = 450000025;

class EdgeCutTool : public DescriptionToolData
{
	Bool ModelingEdgeCut(AtomArray* arr, MODELINGCOMMANDMODE mode, BaseContainer* data, BaseDocument* doc, EditorWindow* win, const BaseContainer* msg, Bool undo, EdgeCutTool* tool);

public:
	EdgeCutTool();

	virtual Int32	GetToolPluginId() const { return ID_MODELING_EDGECUT_TOOL_SDK; }
	virtual const String GetResourceSymbol() const { return String("ToolEdgeCutSDK"); }

	virtual Bool MouseInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg);
	virtual Int32	GetState(BaseDocument* doc);

	virtual void InitDefaultSettings(BaseDocument* doc, BaseContainer& data);
	virtual Bool DoCommand(ModelingCommandData& mdat);
	virtual Bool GetCursorInfo(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, Float x, Float y, BaseContainer& bc);

	virtual Bool GetDEnabling(const BaseDocument* doc, const BaseContainer& data, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc) const;
	virtual TOOLDRAW Draw(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, TOOLDRAWFLAGS flags);

protected:
	Bool											isdragging;
	maxon::BaseArray<Vector>  cutpoints;
};

EdgeCutTool::EdgeCutTool()
{
	isdragging = false;
}

Bool EdgeCutTool::GetCursorInfo(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, Float x, Float y, BaseContainer& bc)
{
	if (bc.GetId() == BFM_CURSORINFO_REMOVE)
		return true;
	bc.SetString(RESULT_BUBBLEHELP, GeLoadString(IDS_HLP_EDGECUT_SDK));
	return true;
}

void EdgeCutTool::InitDefaultSettings(BaseDocument* doc, BaseContainer& data)
{
	data.SetInt32(MDATA_EDGECUTSDK_SUBDIV, 1);
	data.SetFloat(MDATA_EDGECUTSDK_OFFSET, 0.5);
	data.SetFloat(MDATA_EDGECUTSDK_SCALE, 1.0);
	data.SetBool(MDATA_EDGECUTSDK_CREATENGONS, true);
	DescriptionToolData::InitDefaultSettings(doc, data);
}

Bool EdgeCutTool::GetDEnabling(const BaseDocument* doc, const BaseContainer& data, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc) const
{
	return DescriptionToolData::GetDEnabling(doc, data, id, t_data, flags, itemdesc);
}

Int32 EdgeCutTool::GetState(BaseDocument* doc)
{
	AutoAlloc<AtomArray> arr;
	if (!doc || !arr)
		return 0;
	if (doc->GetMode() != Medges)
		return 0;
	doc->GetActivePolygonObjects(*arr, true);
	if (arr->GetCount() == 0)
		return 0;
	return CMD_ENABLED;
}

Bool EdgeCutTool::DoCommand(ModelingCommandData& mdat)
{
	return ModelingEdgeCut(mdat.arr, mdat.mode, mdat.bc, mdat.doc, nullptr, nullptr, (mdat.doc != nullptr) && (mdat.flags & MODELINGCOMMANDFLAGS::CREATEUNDO), nullptr);
}

TOOLDRAW EdgeCutTool::Draw(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, TOOLDRAWFLAGS flags)
{
	if (!(flags & TOOLDRAWFLAGS::HIGHLIGHT))
		return TOOLDRAW::NONE;

	if (!isdragging)
		return TOOLDRAW::HANDLES;

	bd->LineZOffset(3);
	bd->SetPen(GetViewColor(VIEWCOLOR_SELECTION_PREVIEW), 0);
	bd->SetMatrix_Matrix(nullptr, Matrix());
	Int32	cnt = (Int32)cutpoints.GetCount();
	for (Int32 a = 0; a < cnt; a++)
		bd->DrawHandle(cutpoints[a], DRAWHANDLE::MIDDLE, 0);
	bd->LineZOffset(0);

	return TOOLDRAW::HANDLES;
}


struct EdgeCutPoint
{
	Int32 ptind;	// index of the point in the final object
	Int32 pos;		// position on the edge
	Int32 pt1a, pt2;
	Char	reverse;
};

typedef maxon::BaseArray<EdgeCutPoint, maxon::BASEARRAY_DEFAULT_CHUNK_SIZE, maxon::BASEARRAYFLAGS::MOVEANDCOPYOBJECTS> EdgeCutPointArray;

class ActiveEdgeCutObject
{
	MAXON_DISALLOW_COPY_AND_ASSIGN(ActiveEdgeCutObject)

public:
	ActiveEdgeCutObject() : op(nullptr), seledges(nullptr), mkernel(nullptr)
	{
	}

	~ActiveEdgeCutObject()
	{
		Modeling::Free(mkernel);
	}

	ActiveEdgeCutObject(ActiveEdgeCutObject && src) : op(src.op), cutpoints(std::move(src.cutpoints)), seledges(src.seledges), mkernel(src.mkernel)
	{
		src.op = nullptr;
		src.seledges = nullptr;
		src.mkernel	 = nullptr;
	}

	MAXON_OPERATOR_MOVE_ASSIGNMENT(ActiveEdgeCutObject)

	Bool Prepare(Bool selall, MODELINGCOMMANDMODE mode, Int32 subdiv, Bool subdivide);
	Bool ReInit(Bool selall, MODELINGCOMMANDMODE mode, Int32 subdiv, Bool subdivide);

	PolygonObject*		op;
	EdgeCutPointArray cutpoints;
	const BaseSelect*	seledges;
	Modeling*					mkernel;
};

class ActiveEdgeCutArray
{
	MAXON_DISALLOW_COPY_AND_ASSIGN(ActiveEdgeCutArray)

public:
	ActiveEdgeCutArray() : selall(true)
	{
	}

	~ActiveEdgeCutArray()
	{
	}

	ActiveEdgeCutArray(ActiveEdgeCutArray && src) : selall(src.selall), objects(std::move(src.objects))
	{
	}

	MAXON_OPERATOR_MOVE_ASSIGNMENT(ActiveEdgeCutArray)

	Bool Init(AtomArray* arr)
	{
		selall = true;

		iferr (objects.Resize(arr->GetCount()))
			return false;

		for (Int32 a = 0; a < arr->GetCount(); a++)
		{
			objects[a].mkernel = Modeling::Alloc();
			if (!objects[a].mkernel)
				return false;

			C4DAtom* at = arr->GetIndex(a);
			if (!at->IsInstanceOf(Opolygon))
			{
				objects.Reset();
				DebugAssert(false);
				return false;
			}

			objects[a].op = (PolygonObject*)at;

			if (static_cast<PolygonObject*>(at)->GetEdgeS()->GetCount() != 0)
			{
				selall = false;
			}
		}
		return GetCount() > 0;
	}

	Bool ReInit(AtomArray* arr, MODELINGCOMMANDMODE mode, Int32 subdiv, Bool subdivide)
	{
		objects.Reset();

		if (!Init(arr))
			return false;

		for (Int32 a = 0; a < GetCount(); a++)
		{
			if (!GetObject(a).Prepare(selall, mode, subdiv, subdivide))
				return false;
		}
		return true;
	}
	Bool Prepare(MODELINGCOMMANDMODE mode, Int32 subdiv, Bool subdivide)
	{
		for (Int32 a = 0; a < GetCount(); a++)
		{
			if (!GetObject(a).Prepare(selall, mode, subdiv, subdivide))
				return false;
		}
		return true;
	}

	Int32 GetCount() { return (Int32)objects.GetCount(); }
	ActiveEdgeCutObject& GetObject(Int32 a) { return objects[a]; }

	Bool selall;
	maxon::BaseArray<ActiveEdgeCutObject> objects;
};

Bool ActiveEdgeCutObject::Prepare(Bool selall, MODELINGCOMMANDMODE mode, Int32 subdiv, Bool subdivide)
{
	if (!mkernel->InitObject(op))
		return false;

	Int32	polycnt = op->GetPolygonCount();
	Bool	ok	= false;
	Bool	all = mode != MODELINGCOMMANDMODE::EDGESELECTION || selall;
	if (all)
	{
		op->GetWritableEdgeS()->SelectAll(0, 4 * polycnt - 1);
		op->GetWritableEdgeS()->SelectAll(0, 4 * polycnt - 1);
	}
	seledges = op->GetEdgeS();

	Int32				 seg = 0, a, b, i, p1, p2, lp, steps, stp;
	EdgeCutPoint ep;

	iferr (cutpoints.EnsureCapacity(seledges->GetCount() * (subdiv + 1)))
		return false;

	Bool					isfirst = true;
	Char					reverse;
	Vector				v1, v2, v3, v4, vDir1, vDir2;
	const Vector* points = op->GetPointR();

	while (seledges->GetRange(seg++, polycnt * 4, &a, &b))
	{
		for (i = a; i <= b; i++)
		{
			if (mkernel->GetOriginalEdgePoints(op, i, p1, p2))
			{
				if (isfirst)
				{
					v1 = points[p1];
					v2 = points[p2];
					isfirst = false;
					reverse = false;
				}
				else
				{
					v3 = points[p1];
					v4 = points[p2];
					vDir1 = !Cross(v2 - v1, v3 - v1);
					vDir2 = !Cross(v3 - v1, v4 - v1);
					if (Dot(vDir1, vDir2) <= 0.0)
						reverse = false;
					else
						reverse = true;
				}
				ep.pt1a = p1;
				ep.pt2	= p2;
				Int32 nindex = i / 4;	//mkernel->TranslateNgonIndex(op,i/4);
				if (mkernel->IsValidEdge(op, nindex, p1, p2))
				{
					/*
								if (subdivide)
								{
									Int32 lEdgeNgons;
									Int32* plNgons = mkernel->GetEdgeNgons(op, p1, p2, lEdgeNgons);
									while (--lEdgeNgons >= 0)
										mkernel->SetNgonFlags(op, plNgons[lEdgeNgons], MODELING_SETNGON_FLAG_TRIANGULATE);
									DeleteMem(plNgons);
								}
					*/
					steps = subdiv + 1;
					lp = p1;
					for (stp = 0; stp < subdiv; stp++, steps--)
					{
						if ((lp = mkernel->SplitEdge(op, lp, p2, 1.0f / Float(steps))) == 0)
							goto Exit;
						ep.ptind = lp;
						ep.pos = stp;
						ep.reverse = reverse;
						iferr (cutpoints.Append(ep))
							goto Exit;
					}
				}
			}
		}
	}
	if (!mkernel->Commit(op, MODELING_COMMIT_UPDATE))
		goto Exit;

	ok = true;
Exit:
	return ok;
}

static constexpr const Float POINT_MIN = 0.0;
static constexpr const Float POINT_MAX = 1.0;

Bool EdgeCutTool::ModelingEdgeCut(AtomArray* arr, MODELINGCOMMANDMODE mode, BaseContainer* data, BaseDocument* doc, EditorWindow* win, const BaseContainer* msg, Bool undo, EdgeCutTool* tool)
{
	if (!data)
		return false;
	arr->FilterObject(-1, Opolygon);
	if (arr->GetCount() < 1)
		return true;

	ActiveEdgeCutArray active;
	if (!active.Init(arr))
		return false;

	Bool	ok = false;
	Int32 i, j;

	Int32	 subdiv = LMax(1, data->GetInt32(MDATA_EDGECUTSDK_SUBDIV));
	Float	 offset = data->GetFloat(MDATA_EDGECUTSDK_OFFSET, .5f);
	Float	 scale	= data->GetFloat(MDATA_EDGECUTSDK_SCALE, 1.0f);
	Bool	 subdivide = !data->GetBool(MDATA_EDGECUTSDK_CREATENGONS);
	maxon::BaseArray<Float> pos1, pos2;
	iferr (pos1.Resize(subdiv))
		return false;
	iferr (pos2.Resize(subdiv))
		return false;
	Float	 temp1, temp2;
	Vector tmp(DC);

	if (scale < .0001)
		scale = 0.0001;

	if (undo)
		AddUndo(doc, arr, UNDOTYPE::CHANGE);

	if (!active.Prepare(mode, subdiv, subdivide))
		goto Exit;

	if (!win)
	{
		for (i = 0; i < subdiv; i++)
		{
			pos1[i] = offset + (Float(1 + i) / Float(subdiv + 1)) * scale - .5f;
			pos2[i] = 1 - offset + (Float(1 + i) / Float(subdiv + 1)) * scale - .5f;
		}
		for (i = 0; i < active.GetCount(); i++)
		{
			ActiveEdgeCutObject& obj = active.GetObject(i);
			EdgeCutPointArray& cutpts = obj.cutpoints;
			Int32	ptcnt = (Int32)obj.cutpoints.GetCount();
			for (j = 0; j < ptcnt; j++)
			{
				Float rPos = cutpts[j].reverse ? pos1[cutpts[j].pos] : pos2[cutpts[j].pos];
				if (rPos < POINT_MIN || rPos > POINT_MAX)
				{
					if (!obj.mkernel->DeletePoint(obj.op, obj.mkernel->TranslatePointIndex(obj.op, cutpts[j].ptind)))
						goto Exit;
				}
				else
				{
					if (!obj.mkernel->SetEdgePoint(obj.op, cutpts[j].ptind, rPos))
						goto Exit;
				}
			}
			if (!obj.mkernel->Commit(obj.op, MODELING_COMMIT_UPDATE | ((subdivide) ? MODELING_COMMIT_TRINGONS | MODELING_COMMIT_QUADS : 0)))
				goto Exit;
		}
	}
	else
	{
		BaseContainer backup = *data;
		Bool					first	 = true;
		Float					dx, dy;
		BaseContainer device;
		scale	 = 1.0f;
		offset = .5f;
		Float mousex = msg->GetInt32(BFM_INPUT_X);
		Float mousey = msg->GetInt32(BFM_INPUT_Y);
		win->MouseDragStart(KEY_MLEFT, mousex, mousey, MOUSEDRAGFLAGS::NOMOVE);
		Bool					shift, ctrl;
		Float					subdivchange = 0;
		const Vector* points;
		if (!tool)
		{
			DebugAssert(false);
			win->MouseDragEnd();
			goto Exit;
		}
		tool->isdragging = true;
		while (win->MouseDrag(&dx, &dy, &device) == MOUSEDRAGRESULT::CONTINUE)
		{
			if (dx == 0 && dy == 0 && !first)
				continue;
			shift = (device.GetInt32(BFM_INPUT_QUALIFIER) & QSHIFT) != 0;
			ctrl	= (device.GetInt32(BFM_INPUT_QUALIFIER) & QCTRL) != 0;
			if (!first)
			{
				if (shift && !ctrl)
				{
					dx /= 200.0f;
					Float tempscale = scale + dx;
					if (tempscale < .0001)
						tempscale = 0.0001;

					// check that all points are in the [0.05, 0.95] range
					temp1 = offset + (Float(subdiv) / Float(subdiv + 1) - .5f) * tempscale;
					temp2 = offset + (1.0_f / Float(subdiv + 1) - .5f) * tempscale;
					if (temp1 < POINT_MIN || temp1 > POINT_MAX || temp2 < POINT_MIN || temp2 > POINT_MAX)
						continue;
					scale = tempscale;
				}
				else if (!shift && ctrl)
				{
					dx /= 200.0f;
					Float tempoffset = offset + dx;
					// check that all points are in the [0.05, 0.95] range
					temp1 = tempoffset + (Float(subdiv) / Float(subdiv + 1) - .5f) * scale;
					temp2 = tempoffset + (1.0_f / Float(subdiv + 1) - .5f) * scale;
					if (temp1 < POINT_MIN || temp1 > POINT_MAX || temp2 < POINT_MIN || temp2 > POINT_MAX)
						continue;
					offset = tempoffset;
				}
				else
				{
					Int32 lSubdivChange = 0;
					subdivchange += dx;
					if (subdivchange <= -SUBDIV_DELTA)
					{
						lSubdivChange = -1;
						while (subdivchange <= -SUBDIV_DELTA)
							subdivchange += SUBDIV_DELTA;
					}
					else if (subdivchange >= SUBDIV_DELTA)
					{
						lSubdivChange = 1;
						while (subdivchange >= SUBDIV_DELTA)
							subdivchange -= SUBDIV_DELTA;
					}
					if (lSubdivChange)
					{
						if (lSubdivChange < 0 && subdiv == 1)
							continue;
						subdiv += lSubdivChange;
						if (!undo || !doc)
						{
							DebugAssert(false);
							win->MouseDragEnd();
							goto Exit;
						}
						doc->EndUndo();
						doc->DoUndo();
						doc->GetActivePolygonObjects(*arr, true);
						if (arr->GetCount() < 1)
						{
							win->MouseDragEnd();
							goto Exit;
						}
						doc->StartUndo();
						AddUndo(doc, arr, UNDOTYPE::CHANGE);
						doc->EndUndo();

						if (!active.ReInit(arr, mode, subdiv, subdivide))
						{
							win->MouseDragEnd();
							goto Exit;
						}
						iferr (pos1.Resize(subdiv))
						{
							win->MouseDragEnd();
							goto Exit;
						}
						iferr (pos2.Resize(subdiv))
						{
							win->MouseDragEnd();
							goto Exit;
						}

						for (i = 0; i < subdiv; i++)
						{
							pos1[i] = offset + ((Float(1 + i) / Float(subdiv + 1)) - .5f) * scale;
							pos2[i] = 1 - offset + ((Float(1 + i) / Float(subdiv + 1)) - .5f) * scale;
						}
					}
				}
			}
			first = false;
			for (i = 0; i < subdiv; i++)
			{
				pos1[i] = offset + ((Float(1 + i) / Float(subdiv + 1)) - .5f) * scale;
				pos2[i] = 1 - offset + ((Float(1 + i) / Float(subdiv + 1)) - .5f) * scale;
				DebugAssert(pos1[i] >= POINT_MIN && pos1[i] <= POINT_MAX);
				DebugAssert(pos2[i] >= POINT_MIN && pos2[i] <= POINT_MAX);
			}

			j = 0;
			for (i = 0; i < active.GetCount(); i++)
			{
				ActiveEdgeCutObject& obj = active.GetObject(i);
				j += (Int32)obj.cutpoints.GetCount();
			}
			iferr (tool->cutpoints.EnsureCapacity(j + 1))
			{
				win->MouseDragEnd();
				goto Exit;
			}

			tool->cutpoints.Flush();
			for (i = 0; i < active.GetCount(); i++)
			{
				ActiveEdgeCutObject& obj = active.GetObject(i);
				EdgeCutPointArray& cutpts = obj.cutpoints;
				Int32	 ptcnt = (Int32)obj.cutpoints.GetCount();
				Matrix mg = obj.op->GetMg();
				points = obj.op->GetPointR();
				for (j = 0; j < ptcnt; j++)
				{
					Float rPos = cutpts[j].reverse ? pos1[cutpts[j].pos] : pos2[cutpts[j].pos];
					if (!obj.mkernel->SetEdgePoint(obj.op, cutpts[j].ptind, rPos))
					{
						win->MouseDragEnd();
						goto Exit;
					}
					tmp = mg * Blend(points[cutpts[j].pt1a], points[cutpts[j].pt2], rPos);
					iferr (tool->cutpoints.Append(tmp))
          {
            win->MouseDragEnd();
            goto Exit;
          }
				}
				if (!obj.mkernel->Commit(obj.op, MODELING_COMMIT_UPDATE | ((subdivide) ? MODELING_COMMIT_TRINGONS | MODELING_COMMIT_QUADS : 0)))
				{
					win->MouseDragEnd();
					goto Exit;
				}
			}
			BaseContainer* writeback = GetToolData(doc, GetToolPluginId());
			if (writeback)
			{
				writeback->SetFloat(MDATA_EDGECUTSDK_OFFSET, offset);
				writeback->SetFloat(MDATA_EDGECUTSDK_SCALE, scale);
				writeback->SetInt32(MDATA_EDGECUTSDK_SUBDIV, subdiv);
				GeSyncMessage(EVMSG_TOOLCHANGED, 0);
			}
			DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
		}
		if (win->MouseDragEnd() != MOUSEDRAGRESULT::FINISHED)
		{
			backup.CopyTo(data, COPYFLAGS::NONE, nullptr);
			goto Exit;
		}
		else
		{
			ok = true;
		}
	}
	EventAdd();

	ok = true;
Exit:
	if (tool)
	{
		tool->isdragging = false;
		tool->cutpoints.Reset();
	}

	if (!ok && undo)
		doc->DoUndo(true);

	return ok;
}


Bool EdgeCutTool::MouseInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
{
	if (!doc)
		return false;

	if (doc->GetMode() == Medges)
	{
		AutoAlloc<AtomArray> arr;
		if (!arr)
			return false;
		doc->GetActivePolygonObjects(*arr, true);

		BaseContainer* localData = GetToolData(doc, ID_MODELING_EDGECUT_TOOL);
		if (!localData)
			return false;

		// undo the step before
		InteractiveModeling_Restart(doc);

		ModelingEdgeCut(arr, MODELINGCOMMANDMODE::EDGESELECTION, localData, doc, win, &msg, true, this);
		EventAdd();
		return true;
	}
	return true;
}

Bool RegisterEdgeCutTool()
{
	return RegisterToolPlugin(ID_MODELING_EDGECUT_TOOL_SDK, GeLoadString(IDS_EDGECUT_SDK), 0, nullptr, GeLoadString(IDS_HLP_EDGECUT_SDK), NewObjClear(EdgeCutTool));
}
