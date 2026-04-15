// example code for a menu/manager plugin

#include "activeobject.h"
#include "maxon/sortedarray.h"
#include "maxon/utilities/sprintf_safe.h"
#include "maxon/weakrawptr.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_ACTIVEOBJECT = 1000472;

class ListHeader;
class ActiveObjectDialog;

static void ShowObjectProps(BaseList2D* obj);
static void UpdateDialog(ActiveObjectDialog* dlg);

//---------------------------------------------
class ListObj
{
public:
	ListObj() {}
	virtual ~ListObj();

	Bool AddObj(ListHeader* header);
	Bool RemObj(ListHeader* header, Bool delme);
	Bool FindObj(ListHeader* header);

	ListObj* GetNext();
	ListObj* GetPrev();
	ListHeader* GetUp();

	friend class ListHeader;

private:
	ListObj*		next = nullptr;
	ListObj*		prev = nullptr;
	ListHeader* up = nullptr;
};

//---------------------------------------------
class ListHeader
{
public:
	ListHeader() {}
	~ListHeader();

	Bool AddObj(ListObj* newobj);
	Bool RemObj(ListObj* newobj, Bool delme);
	Bool FreeList(Bool delentries);

	ListObj* GetFirst();

	Int32 GetCount();

private:
	ListObj* first = nullptr;
	ListObj* last = nullptr;
};

//---------------------------------------------
ListObj::~ListObj()
{
	if (up)
		RemObj(up, false);
	prev = next = nullptr;
	up = nullptr;
}
Bool ListObj::AddObj(ListHeader* header)
{
	return header->AddObj(this);
}
Bool ListObj::RemObj(ListHeader* header, Bool delme)
{
	return header->RemObj(this, delme);
}
Bool ListObj::FindObj(ListHeader* header)
{
	return false;
}
ListObj* ListObj::GetNext()
{
	return next;
}
ListObj* ListObj::GetPrev()
{
	return prev;
}
ListHeader* ListObj::GetUp()
{
	return up;
}
//---------------------------------------------
ListHeader::~ListHeader()
{
	FreeList(false);
}
Bool ListHeader::AddObj(ListObj* newobj)
{
	if (last)
		last->next = newobj;
	newobj->up = this;
	newobj->prev = last;
	last = newobj;
	if (!first)
		first = newobj;
	return true;
}
Bool ListHeader::RemObj(ListObj* oldobj, Bool delme)
{
	// ausklinken
	if (oldobj->prev)
		oldobj->prev->next = oldobj->next;
	if (oldobj->next)
		oldobj->next->prev = oldobj->prev;

	// header korrigieren
	if (oldobj == first)
		first = oldobj->next;
	if (oldobj == last)
		last = oldobj->prev;

	oldobj->prev = oldobj->next = nullptr;
	oldobj->up = nullptr;

	// wenn gewnscht, Speicher freigeben
	if (delme)
		DeleteObj(oldobj);

	return true;
}
ListObj* ListHeader::GetFirst()
{
	return first;
}
Bool ListHeader::FreeList(Bool delentries)
{
	while (first)
	{
		RemObj(first, delentries);
	}
	return true;
}
Int32 ListHeader::GetCount()
{
	Int32 count = 0;
	for (ListObj* temp = GetFirst(); temp; temp = temp->GetNext())
		count++;
	return count;
}
//---------------------------------------------




struct DebugNode : public ListObj
{
	ListHeader down;
	GeListNode* ptr = nullptr;
	String name;
	Bool open = false;
	Bool isNew = false;
	maxon::WeakRawPtr<GeListNode> link;

	Bool diff[UInt32(HDIRTY_ID::MAX) + 1];
	UInt32 hdirty[UInt32(HDIRTY_ID::MAX) + 1];
	Bool diffDirty[2] = { };
	UInt32 dirty[2] = { (UInt32)NOTOK, (UInt32)NOTOK };
};

class DebugArray : public maxon::SortedArray<DebugArray, maxon::BaseArray<DebugNode*> >
{
public:
	static Bool LessThan(DebugNode* a, DebugNode* b)
	{
		return a->ptr < b->ptr;
	}

	static Bool LessThan(GeListNode* a, DebugNode* b)
	{
		return a < b->ptr;
	}

	static Bool IsEqual(GeListNode* a, DebugNode* b)
	{
		return a == b->ptr;
	}

	void DeleteAll()
	{
		for (DebugNode*& node : *this)
			DeleteObj(node);

		Flush();
	}
};

static Bool BuildTree(DebugNode* parent, DebugArray& oldlist, DebugArray& newlist, GeListNode* node, const String& defname, Bool isElement)
{
	DebugNode** nn = oldlist.FindValue(node);
	DebugNode* n = nn ? *nn : nullptr;
	if (n)
	{
		ListHeader* h = n->GetUp();
		if (h)
			h->RemObj(n, false);

		iferr (oldlist.Erase(oldlist.GetIndex(*nn)))	// GetIndex needs *nn instead of n
			return false;

		if (isElement)
		{
			n->link.Set(node);
		}
		n->isNew = h != &parent->down && node->GetType() != Tbasedocument;
	}

	if (!n)
	{
		n = NewObjClear(DebugNode);
		if (!n)
			return false;

		n->ptr = node;
		n->open = false;
		if (isElement)
			n->link.Set(node);
		n->isNew = true;
	}

	iferr (newlist.Append(n))
		return false;
	parent->down.AddObj(n);

	if (node->IsInstanceOf(Tbasedocument))
		n->name = "DOC: " + static_cast<BaseDocument*>(node)->GetDocumentName().GetString();
	else if (node->IsInstanceOf(Tbaselist2d))
		n->name = defname + static_cast<BaseList2D*>(node)->GetName();
	else
		n->name = defname;

	// UIDs
	if (node->IsInstanceOf(Tbaselist2d))
	{
		const Char* mem = nullptr;
		Int32 uID = 0;
		Int bytes = 0;
		Char cTxt[3];

		// go through all available UIDs
		for (Int32 uIdx = 0; uIdx < static_cast<BaseList2D*>(node)->GetUniqueIDCount(); uIdx++)
		{
			iferr (DebugNode * uidNode = NewObj(DebugNode))
				return false;

			if (!uidNode)
				return false;

			uidNode->open = FALSE;

			// get the UID (size and data)
			if (static_cast<BaseList2D*>(node)->GetUniqueIDIndex(uIdx, uID, mem, bytes))
			{
				uidNode->name = "UID [APPID " + String::IntToString(uID) + "] - [";

				for (Int32 pos = 0; mem && pos < bytes; pos++)
				{
					Char cVal = mem[pos];

					// print as 2 digit hex value
					sprintf_safe(cTxt, sizeof(cTxt), "%02X", (UChar)cVal);
					uidNode->name += String(cTxt);

					// add "-" after 4 digits
					if (((pos + 1) % 2) == 0 && pos + 1 < bytes)
						uidNode->name += "-";
				}
				uidNode->name += "]";
			}
			else
			{
				uidNode->name = "UID <not found at index " + String::IntToString(uIdx) + ">";
			}

			iferr (newlist.Append(uidNode))
				return false;

			n->down.AddObj(uidNode);
		}
	}

	UInt32 sum = 0;
	for (Int32 i = 0; i < Int32(HDIRTY_ID::MAX); i++)
	{
		UInt32 hdirty = node->GetHDirty((HDIRTYFLAGS)((1 << 31) | (1 << i)));
		n->diff[i] = n->hdirty[i] != hdirty;
		n->hdirty[i] = hdirty;
		sum += hdirty;
	}
	sum += node->GetDirty(DIRTYFLAGS::ALL);

	n->diff[Int32(HDIRTY_ID::MAX)] = n->hdirty[Int32(HDIRTY_ID::MAX)] != sum;
	n->hdirty[Int32(HDIRTY_ID::MAX)] = sum;

	Int32 dirtyData = node->GetDirty(DIRTYFLAGS::DATA);
	Int32 dirtyMatrix = node->GetDirty(DIRTYFLAGS::MATRIX);
	n->diffDirty[0] = n->dirty[0] != (UInt32)dirtyData;
	n->diffDirty[1] = n->dirty[1] != (UInt32)dirtyMatrix;

	n->dirty[0] = dirtyData;
	n->dirty[1] = dirtyMatrix;


	// Handle Scene Nodes.
	// See in the plugin Project Hooks -> Scene Nodes -> CLASSIC_REPRESENTATION:
	static constexpr Int32 SCENEHOOK_NODE_SCENE_ID = 1054188;
	static constexpr Int32 MSG_UPDATE_LEGACY_OBJECTS = 180420109;

	if (node->GetType() == SCENEHOOK_NODE_SCENE_ID)
	{
		// Request Legacy Object representation
		BaseObject* sceneNodeRoot = nullptr;
		node->Message(MSG_UPDATE_LEGACY_OBJECTS, &sceneNodeRoot);

		// To have a nullptr here is no error, there's just no node scene graph.
		if (sceneNodeRoot != nullptr)
		{
			if (!BuildTree(n, oldlist, newlist, sceneNodeRoot, String("CLASSIC_REPRESENTATION: "), true))
				return false;
		}
	}

	if (node->IsInstanceOf(Obase))
	{
		BaseObject* obj = static_cast<BaseObject*>(node);
		BaseObject* cc = obj->GetCache();
		if (cc)
			if (!BuildTree(n, oldlist, newlist, cc, String("CACHE: "), true))
				return false;

		cc = obj->GetDeformCache();
		if (cc)
			if (!BuildTree(n, oldlist, newlist, cc, String("DEFORM: "), true))
				return false;

		cc = obj->GetIsoparm();
		if (cc)
			if (!BuildTree(n, oldlist, newlist, cc, String("ISOPARM: "), true))
				return false;

		if (node->GetInfo() & OBJECT_ISSPLINE)
		{
			cc = obj->GetRealSpline();
			if (cc && (cc != obj))
				if (!BuildTree(n, oldlist, newlist, cc, String("REALSPLINE: "), true))
					return false;
		}
	}

	if (node->GetDown())
	{
		GeListNode* c = node->GetDown();
		for (; c; c = c->GetNext())
		{
			if (!BuildTree(n, oldlist, newlist, c, String(), true))
				return false;
		}
	}

	maxon::BufferedBaseArray<BranchInfo, 20> info;
	node->GetBranchInfo(info, GETBRANCHINFO::NONE) iferr_ignore("GetBranchInfo");
	for (Int i = 0; i < info.GetCount(); i++)
	{
		if (info[i].name.IsPopulated())
		{
			if (!BuildTree(n, oldlist, newlist, info[i].head, info[i].name, false))
				return false;
		}
		else
		{
			if (!BuildTree(n, oldlist, newlist, info[i].head, String("ListHead"), false))
				return false;
		}
	}

	if (node->IsInstanceOf(Tbasedocument))
	{
		static constexpr Int32 NEUTRON_SCENEHOOK_ID = 1054188;
		static constexpr Int32 NEUTRON_MSG_GET_LEGACY_VIEWPORT_OBJECTS = 180420110;

		BaseObject* neutronRoot = nullptr;
		BaseDocument* doc = static_cast<BaseDocument*>(node);
		BaseSceneHook* neutron = doc->FindSceneHook(NEUTRON_SCENEHOOK_ID);
		if (neutron)
			neutron->Message(NEUTRON_MSG_GET_LEGACY_VIEWPORT_OBJECTS, &neutronRoot);
		if (neutronRoot)
		{
			if (!BuildTree(n, oldlist, newlist, neutronRoot, String("Neutron objects"), true))
				return false;
		}
	}

	return true;
}


class Function2 : public TreeViewFunctions
{
public:
	virtual void*	GetFirst(void* root, void* userdata)
	{
		DebugNode* node = (DebugNode*)root;
		return node->down.GetFirst();
	}

	virtual void*	GetNext(void* root, void* userdata, void* obj)
	{
		DebugNode* node = (DebugNode*)obj;
		return node->GetNext();
	}

	virtual void* GetPred(void* root, void* userdata, void* obj)
	{
		DebugNode* node = (DebugNode*)obj;
		return node->GetPrev();
	}

	virtual void*	GetDown(void* root, void* userdata, void* obj)
	{
		DebugNode* node = (DebugNode*)obj;
		return node->down.GetFirst();
	}

	virtual Bool IsSelected(void* root, void* userdata, void* obj)
	{
		//DebugNode *node = (DebugNode*)obj;
		return false;
	}

	virtual String GetName(void* root, void* userdata, void* obj)
	{
		DebugNode* node = (DebugNode*)obj;
		return node->name;
	}

	virtual Int GetId(void* root, void* userdata, void* obj)
	{
		return (Int)obj;
	}

	virtual Bool IsOpened(void* root, void* userdata, void* obj)
	{
		DebugNode* node = (DebugNode*)obj;
		return node->open;
	}

	virtual void Open(void* root, void* userdata, void* obj, Bool onoff)
	{
		DebugNode* node = (DebugNode*)obj;
		node->open = onoff;
	}

	virtual void Select(void* root, void* userdata, void* obj, Int32 mode)
	{
		DebugNode*	 node = (DebugNode*)obj;
		GeListNode* link = node->link.Get();

		if (!link || !link->IsInstanceOf(Tbaselist2d))
			return;

		if (link->IsInstanceOf(Obase))
		{
			BaseObject*		op	= (BaseObject*)link;
			BaseDocument* doc = op->GetDocument();
			if (doc)
				doc->SetActiveObject(op, mode);
			EventAdd();
		}
		else if (link->IsInstanceOf(Mbase))
		{
			BaseMaterial* op	= (BaseMaterial*)link;
			BaseDocument* doc = op->GetDocument();
			if (doc)
				doc->SetActiveMaterial(op);
			EventAdd();
		}
		ShowObjectProps(static_cast<BaseList2D*>(link));
	}

	virtual Int32 AcceptDragObject(void* root, void* userdata, void* obj, Int32 dragtype, void* dragobject, Bool& bAllowCopy)
	{
		return 0;
	}

	virtual void InsertObject(void* root, void* userdata, void* obj, Int32 dragtype, void* dragobject, Int32 insertmode, Bool bCopy)
	{
	}

	virtual Int32 GetDragType(void* root, void* userdata, void* obj)
	{
		return NOTOK;
	}

	virtual Int32 DoubleClick(void* root, void* userdata, void* obj, Int32 col, MouseInfo* mouseinfo)
	{
		return false;
	}

	virtual Bool MouseDown(void* root, void* userdata, void* obj, Int32 col, MouseInfo* mouseinfo, Bool rightButton)
	{
		return false;
	}

	virtual Int32 GetHeaderColumnWidth(void* root, void* userdata, Int32 col, GeUserArea* ua)
	{
		if (!ua)
			return 0;
		return ua->DrawGetTextWidth("8888"_s) + 4;
	}

	virtual Int32 GetColumnWidth(void* root, void* userdata, void* obj, Int32 col, GeUserArea* pArea)
	{
		if (col == 'icon')
		{
			return pArea->DrawGetFontHeight() + 4;
		}
		else if (col == 'refc')
		{
			return pArea->DrawGetTextWidth("8888888x"_s) + 12;
		}
		else if (col == 'addr' || col == 'drtL')
		{
			return pArea->DrawGetTextWidth("0x8888888888888888"_s) + 12;
		}
		else if (col == 'type')
		{
			return pArea->DrawGetTextWidth("9999999"_s) + 12;
		}
		else if ((col >> 8) == C4D_FOUR_BYTE(0, 'd', 'r', 't'))
		{
			return pArea->DrawGetTextWidth("8888"_s);
		}
		return 0;
	}

	virtual Int32 GetLineHeight(void* root, void* userdata, void* obj, Int32 col, GeUserArea* pArea)
	{
		return pArea->DrawGetFontHeight();
	}

	Bool DrawHeaderCell(void* root, void* userdata, Int32 col, DrawInfo* drawinfo)
	{
		String name;
		drawinfo->frame->DrawSetPen(COLOR_BG);
		drawinfo->frame->DrawRectangle(drawinfo->xpos, drawinfo->ypos, drawinfo->xpos + drawinfo->width - 1, drawinfo->ypos + drawinfo->height - 1);
		drawinfo->frame->DrawSetFont(FONT_STANDARD);

		switch (col)
		{
			case 'tree':	name = "Scene"; break;
			case 'icon':	name = String(); break;
			case 'type':	name = "Type"; break;
			case 'addr':	name = "Address"; break;
			case 'refc':	name = "RefCnt"; break;
			case 'bits':	name = "Gen"; break;
			case 'drtd':	name = "Data"; break;
			case 'drtm':	name = "Mat"; break;
			case 'drt0':	name = "All"; break;
			case 'drt1':	name = "Anim"; break;
			case 'drt2':	name = "Obj"; break;
			case 'drt3':	name = "OMatrix"; break;
			case 'drt4':	name = "OHierarchie"; break;
			case 'drt5':	name = "Tag"; break;
			case 'drt6':	name = "HMat"; break;
			case 'drt7':	name = "Shd"; break;
			case 'drt8':	name = "Rend"; break;
			case 'drt9':	name = "VP"; break;
			case 'drta':	name = "Filter"; break;
			case 'drtb':	name = "NBit"; break;
			case 'drtL':	name = "Mask"; break;
		}

		drawinfo->frame->DrawSetTextCol(COLOR_TEXT, COLOR_BG);
		drawinfo->frame->DrawText(name, drawinfo->xpos + 4, drawinfo->ypos);

		drawinfo->frame->DrawSetPen(COLOR_EDGEWH);
		drawinfo->frame->DrawLine(drawinfo->xpos, drawinfo->ypos, drawinfo->xpos + drawinfo->width - 1, drawinfo->ypos);
		drawinfo->frame->DrawLine(drawinfo->xpos, drawinfo->ypos, drawinfo->xpos, drawinfo->ypos + drawinfo->height - 1);

		drawinfo->frame->DrawSetPen(COLOR_EDGEDK);
		drawinfo->frame->DrawLine(drawinfo->xpos + drawinfo->width - 1, drawinfo->ypos, drawinfo->xpos + drawinfo->width - 1, drawinfo->ypos + drawinfo->height - 1);
		drawinfo->frame->DrawLine(drawinfo->xpos, drawinfo->ypos + drawinfo->height - 1, drawinfo->xpos + drawinfo->width - 1, drawinfo->ypos + drawinfo->height - 1);

		return true;
	}

	virtual void DrawCell(void* root, void* userdata, void* obj, Int32 col, DrawInfo* drawinfo, const GeData& bgColor)
	{
		DebugNode*	 node = (DebugNode*)obj;
		GeListNode* link = node->link.Get();

		Int32 wx = drawinfo->xpos;
		Int32 wy = drawinfo->ypos;
		Int32 ww = drawinfo->width;
		Int32 wh = drawinfo->height;

		if (col == 'icon')
		{
			if (link && link->IsInstanceOf(Tbaselist2d))
			{
				BaseObject* op = (BaseObject*)link;
				IconData		icon;
				op->GetIcon(&icon);

				if (ww > 24)
				{
					wx += (ww - 24) / 2; ww = 24;
				}
				if (wh > 24)
				{
					wy += (wh - 24) / 2; wh = 24;
				}

				Int32 drawflags = BMP_ALLOWALPHA | BMP_NORMALSCALED;
				if (icon.flags & ICONDATAFLAGS::APPLYCOLORPROFILE)
					drawflags |= BMP_APPLY_COLORPROFILE;
				if (icon.flags & ICONDATAFLAGS::DISABLED)
					drawflags |= BMP_GRAYEDOUT;
				drawinfo->frame->DrawSetPen(bgColor);
				drawinfo->frame->DrawBitmap(icon.bmp, wx, wy, ww, wh, icon.x, icon.y, icon.w, icon.h, drawflags);
			}
		}
		else if (col == 'type')
		{
			if (link)
			{
				String t = String::IntToString(link->GetType());
				drawinfo->frame->DrawSetTextCol(COLOR_TEXT_DISABLED, bgColor);
				drawinfo->frame->DrawText(t, wx, wy);
			}
		}
		else if (col == 'addr')
		{
			if (node->isNew)
				drawinfo->frame->DrawSetTextCol(Vector(1, 0, 0), COLOR_EDGEWH);
			else
				drawinfo->frame->DrawSetTextCol(COLOR_TEXT_DISABLED, bgColor);

			GeListNode* o = link ? link : node->ptr;
			String hex = String::HexToString((UInt)o);
			hex = String::HexToString((UInt)o);
			drawinfo->frame->DrawText(hex, wx, wy);
		}
		else if (col == 'refc')
		{
			GeListNode* o = link;
			if (o && o->IsInstanceOf(Tvariable))
			{
				GeData data;
				o->GetParameter(ConstDescID(DescLevel((Int32)0xdeadbeef)), data, DESCFLAGS_GET::NONE);
				maxon::StrongCOWRef<Int>* ref = reinterpret_cast<maxon::StrongCOWRef<Int>*>(data.GetVoid());
				UInt											refCnt = ref && ref->GetPointer() ? (UInt)maxon::details::PrivateGetReferenceCounter(ref->GetPointer()) : 0;
				String										hex = String::UIntToString(refCnt) + "x";

				if (node->isNew)
					drawinfo->frame->DrawSetTextCol(Vector(1, 0, 0), COLOR_EDGEWH);
				else
					drawinfo->frame->DrawSetTextCol(COLOR_TEXT_DISABLED, bgColor);

				drawinfo->frame->DrawText(hex, wx, wy);
			}
		}
		else if (col == 'drtL')
		{
			if (link)
			{
				GeData data;
				link->GetParameter(CreateDescID(DescLevel(999999)), data, DESCFLAGS_GET::NONE);
				String hex = String::HexToString(UInt32(data.GetInt32()));
				drawinfo->frame->DrawSetTextCol(COLOR_TEXT_DISABLED, bgColor);
				drawinfo->frame->DrawText(hex, wx, wy);
			}
		}
		else if (col == 'bits')
		{
			if (link && link->IsInstanceOf(Tbaselist2d))
			{
				Bool	 bit = static_cast<BaseList2D*>(link)->GetBit(BIT_CONTROLOBJECT);
				String hex = String::IntToString(bit);
				drawinfo->frame->DrawSetTextCol(COLOR_TEXT_DISABLED, bgColor);
				drawinfo->frame->DrawText(hex, wx, wy);
			}
		}
		else if (col == 'drtm' || col == 'drtd')
		{
			if (link)
			{
				Int id = col == 'drtd' ? 0 : 1;
				UInt32 dirty_cnt = node->dirty[id];
				Bool dirty = node->diffDirty[id];

				if (dirty)
					drawinfo->frame->DrawSetTextCol(Vector(1, 0, 0), COLOR_EDGEWH);
				else
					drawinfo->frame->DrawSetTextCol(COLOR_TEXT, bgColor);

				drawinfo->frame->DrawText(String::UIntToString(dirty_cnt), wx, wy);
			}
		}
		else if ((col >> 8) == C4D_FOUR_BYTE(0, 'd', 'r', 't'))
		{
			if (link)
			{
				Int32	 id = 0;
				UInt32 dirty_cnt = (UInt32) - 1;
				Bool	 dirty = false;

				switch (col)
				{
					case 'drt0':	id = (Int32)HDIRTY_ID::MAX; break;
					case 'drt1':	id = (Int32)HDIRTY_ID::ANIMATION; break;
					case 'drt2':	id = (Int32)HDIRTY_ID::OBJECT; break;
					case 'drt3':	id = (Int32)HDIRTY_ID::OBJECT_MATRIX; break;
					case 'drt4':	id = (Int32)HDIRTY_ID::OBJECT_HIERARCHY; break;
					case 'drt5':	id = (Int32)HDIRTY_ID::TAG; break;
					case 'drt6':	id = (Int32)HDIRTY_ID::MATERIAL; break;
					case 'drt7':	id = (Int32)HDIRTY_ID::SHADER; break;
					case 'drt8':	id = (Int32)HDIRTY_ID::RENDERSETTINGS; break;
					case 'drt9':	id = (Int32)HDIRTY_ID::VP; break;
					case 'drta':	id = (Int32)HDIRTY_ID::FILTER; break;
					case 'drtb':	id = (Int32)HDIRTY_ID::NBITS; break;
				}
				dirty_cnt = node->hdirty[id];
				dirty = node->diff[id];

				if (dirty)
					drawinfo->frame->DrawSetTextCol(Vector(1, 0, 0), COLOR_EDGEWH);
				else
					drawinfo->frame->DrawSetTextCol(COLOR_TEXT, bgColor);

				drawinfo->frame->DrawText(String::UIntToString(dirty_cnt), wx, wy);
			}
		}
	}

	virtual void CreateContextMenu(void* root, void* userdata, void* obj, Int32 lColumn, BaseContainer* bc)
	{
		bc->FlushAll();

		DebugNode* node = (DebugNode*)obj;
		const GeListNode* link = node ? node->link.Get() : nullptr;

		switch (lColumn)
		{
			case 'tree':
			{
				if (link)
				{
					if (link->GetType() == ID_LISTHEAD)
					{
						bc->InsData(REFRESH_TREE, GeData("Refresh Tree"));
					}
					else if (link->IsInstanceOf(Obase))
					{
						bc->InsData(SHOW_OBJECT_INFORMATION, GeData("Show object information..."));
						bc->InsData(EXTRACT_OBJECT_TO_SCENE, GeData("Extract to scene"));
						bc->InsData(SHOW_RELATIONSHIP, GeData("Show Relationship"));
					}
					else if (link->IsInstanceOf(Tvariable))
					{
						bc->InsData(SHOW_VARIABLE_TAG_DATA, GeData("Show variable tag dat"));
					}
				}

				if (node && node->ptr)
				{
					bc->InsData(COPY_ADDRESS, GeData("Copy Address"));
				}
				break;
			}
		}
	}

	Bool ContextMenuCall(void* root, void* userdata, void* obj, Int32 lColumn, Int32 lCommand)
	{
		DebugNode* node = (DebugNode*)obj;
		GeListNode* link = node->link.Get();

		switch (lCommand)
		{
			case REFRESH_TREE:
			{
				ActiveObjectDialog* dlg = static_cast<ActiveObjectDialog*>(userdata);
				UpdateDialog(dlg);
				break;
			}
			case SHOW_OBJECT_INFORMATION:
			{
				if (link && link->IsInstanceOf(Obase))
				{
					BaseObject* object = static_cast<BaseObject*>(link);
					String messageString = FormatString("Type: @", object->GetType());
					messageString += FormatString("\nName: @", object->GetName());
					if (object->IsInstanceOf(Opoint))
						messageString += FormatString("\nPoint Count: @", ToPoint(object)->GetPointCount());
					if (object->IsInstanceOf(Opolygon))
						messageString += FormatString("\nPolygon Count: @", ToPoly(object)->GetPolygonCount());
					GeOutString(messageString, GEMB::OK);
				}
				break;
			}
			case COPY_ADDRESS:
			{
				String hex = String::HexToString((UInt)node->ptr);
				CopyToClipboard(hex);
				StatusSetText("Clipboard: " + hex);
				break;
			}
			case EXTRACT_OBJECT_TO_SCENE:
			{
				Bool needsEventAdd = false;
				if (link && link->IsInstanceOf(Obase))
				{
					BaseObject* object = static_cast<BaseObject*>(link);
					C4DAtom* clone = object->GetClone(COPYFLAGS::NONE, nullptr);
					if (clone)
					{
						BaseDocument* document = object->GetDocument();
						if (!document)
						{
							BaseObject* cacheParent = object->GetCacheTopParent();
							if (cacheParent)
								document = cacheParent->GetDocument();
						}
						if (!document)
							document = GetActiveDocument();

						if (document)
						{
							BaseObject* cloneObject = static_cast<BaseObject*>(clone);
							// BIT_EDITOBJECT is set internally to hide the polygon object in certain cases (e.g. if the object is under a SDS). 
							// Remove the flag here, otherwise the clone would not be visible in the viewport.
							cloneObject->DelBit(BIT_EDITOBJECT);
							document->InsertObject(cloneObject, nullptr, nullptr);
							needsEventAdd = true;
						}
					}
				}
				if (needsEventAdd)
				{
					EventAdd();
				}
				break;
			}
			case SHOW_RELATIONSHIP:
			{
				String outString = "Relationship for: "_s;
				if (link && link->IsInstanceOf(Obase))
				{
					const BaseObject* object = static_cast<BaseObject*>(link);
					const BaseObject* r = nullptr;
					auto AddObject = [&r, &outString](const String& type)
					{
						outString += "|"_s;
						outString += type;
						outString += ": "_s;
						if (r)
							outString += FormatString("0x@: @", (void*)r, r->GetName());
						else
							outString += "nullptr"_s;
					};

					outString += FormatString("0x@: @|", (void*)object, object->GetName());

					r = object->GetOrigin(false);
					AddObject("origin"_s);

					r = object->GetOrigin(true);
					AddObject("origin (safe)"_s);

					r = object->GetCacheParent();
					AddObject("cache parent"_s);
				}
				else
				{
					outString += "(no BaseObject)"_s;
				}
				GeOutString(outString, GEMB::OK);
				break;
			}
			case SHOW_VARIABLE_TAG_DATA:
			{
				if (link && link->IsInstanceOf(Tvariable))
				{
					const VariableTag* tag = static_cast<const VariableTag*>(link);
					ShowVariableTagDataDialog(tag);
				}
				break;
			}
		}

		return true;
	}

private:
	static const Int32 REFRESH_TREE = ID_TREEVIEW_FIRST_NEW_ID + 1;
	static const Int32 SHOW_OBJECT_INFORMATION = ID_TREEVIEW_FIRST_NEW_ID + 2;
	static const Int32 EXTRACT_OBJECT_TO_SCENE = ID_TREEVIEW_FIRST_NEW_ID + 3;
	static const Int32 SHOW_RELATIONSHIP = ID_TREEVIEW_FIRST_NEW_ID + 4;
	static const Int32 COPY_ADDRESS = ID_TREEVIEW_FIRST_NEW_ID + 5;
	static const Int32 SHOW_VARIABLE_TAG_DATA = ID_TREEVIEW_FIRST_NEW_ID + 6;
};

Function2 g_functable;

enum
{
	IDC_AO_LOCK_ELEMENT = 1001,
	IDC_AO_DESCRIPTION	= 10000,
	IDC_AO_TREEVIEW			= 10001
};


class ActiveObjectDialog : public GeDialog
{
public:
	~ActiveObjectDialog()
	{
		_oldlist.DeleteAll();
	}

	virtual Bool CreateLayout();
	virtual Bool InitValues();
	virtual void DestroyWindow();
	virtual Bool Command(Int32 id, const BaseContainer& msg);
	virtual Bool CoreMessage(Int32 id, const BaseContainer& msg);

	Bool FillTree();

	DescriptionCustomGui* GetActiveCustomGui() { return _gad; }

private:
	DescriptionCustomGui* _gad = nullptr;
	Bool _lock = false;
	TreeViewCustomGui* _tree = nullptr;
	DebugNode _root_of_docs;
	DebugArray _oldlist;
};

void ActiveObjectDialog::DestroyWindow()
{
	_tree = nullptr;
	_gad = nullptr;
}

Bool ActiveObjectDialog::CreateLayout()
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();
	if (res == false)
		return res;

	SetTitle("C++SDK Demo - Active Object Properties"_s);

	GroupBegin(0, BFH_SCALEFIT | BFV_SCALEFIT, 0, 1, String(), BFV_GRIDGROUP_ALLOW_WEIGHTS);
	// you can also write "TREEVIEW id { BORDER; } in the resource file

	GroupBegin(0, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, String(), 0);
	AddCheckbox(IDC_AO_LOCK_ELEMENT, BFH_LEFT, 0, 0, "Lock Element"_s);

	BaseContainer treedata;
	treedata.SetBool(TREEVIEW_BORDER, true);
	treedata.SetBool(TREEVIEW_HAS_HEADER, true);
	treedata.SetBool(TREEVIEW_FIXED_LAYOUT, true);
	_tree = (TreeViewCustomGui*)AddCustomGui(IDC_AO_TREEVIEW, CUSTOMGUI_TREEVIEW, String(), BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, treedata);
	// you can also write "TREEVIEW id { BORDER; } in the resource file
	GroupEnd();

	BaseContainer customgui;
	customgui.SetBool(DESCRIPTION_ALLOWFOLDING, true);
	_gad = (DescriptionCustomGui*)AddCustomGui(IDC_AO_DESCRIPTION, CUSTOMGUI_DESCRIPTION, String(), BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, customgui);

	GroupEnd();

	if (_gad && GetActiveDocument())
	{
		_gad->SetObject(GetActiveDocument()->GetActiveObject());
	}

	if (_tree)
	{
		BaseContainer layout;
		layout.SetInt32('tree', LV_TREE);
		layout.SetInt32('icon', LV_USER);
		layout.SetInt32('type', LV_USER);
		layout.SetInt32('addr', LV_USER);
		layout.SetInt32('refc', LV_USER);
		layout.SetInt32('bits', LV_USER);
		layout.SetInt32('drtd', LV_USER);
		layout.SetInt32('drtm', LV_USER);
		layout.SetInt32('drt0', LV_USER);
		layout.SetInt32('drt1', LV_USER);
		layout.SetInt32('drt2', LV_USER);
		layout.SetInt32('drt3', LV_USER);
		layout.SetInt32('drt4', LV_USER);
		layout.SetInt32('drt5', LV_USER);
		layout.SetInt32('drt6', LV_USER);
		layout.SetInt32('drt7', LV_USER);
		layout.SetInt32('drt8', LV_USER);
		layout.SetInt32('drt9', LV_USER);
		layout.SetInt32('drta', LV_USER);
		layout.SetInt32('drtb', LV_USER);
		layout.SetInt32('drtL', LV_USER);
		_tree->SetLayout(19, layout);
	}

	return res;
}

Bool ActiveObjectDialog::InitValues()
{
	// first call the parent instance
	if (!GeDialog::InitValues())
		return false;

	return FillTree();
}

Bool ActiveObjectDialog::Command(Int32 id, const BaseContainer& msg)
{
	switch (id)
	{
		case IDC_AO_LOCK_ELEMENT:
		{
			_lock = msg.GetInt32(BFM_ACTION_VALUE) != 0;
			CoreMessage(EVMSG_CHANGE, BaseContainer());
			break;
		}
	}
	return GeDialog::Command(id, msg);
}

Bool ActiveObjectDialog::CoreMessage(Int32 id, const BaseContainer& msg)
{
	switch (id)
	{
		case EVMSG_DOCUMENTRECALCULATED:
		{
			if (CheckCoreMessage(msg) && !_lock)
			{
				if (_gad && GetActiveDocument())
				{
					InitValues();
				}
			}
			break;
		}
	}
	return GeDialog::CoreMessage(id, msg);
}

Bool ActiveObjectDialog::FillTree()
{
	if (_tree)
	{
		_root_of_docs.down.FreeList(false);

		DebugArray newlist;
		BuildTree(&_root_of_docs, _oldlist, newlist, GetActiveDocument()->GetListHead(), String("Documents"), false);
		_oldlist.DeleteAll();
		iferr (_oldlist.CopyFrom(newlist))
			return false;
		_oldlist.Sort();
		_tree->SetRoot(&_root_of_docs, &g_functable, this);
		_tree->Refresh();
	}
	return true;
}

void UpdateDialog(ActiveObjectDialog* dlg)
{
	if (dlg)
		dlg->FillTree();
}

class ActiveObjectDialogCommand : public CommandData
{
public:
	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager)
	{
		return _dlg.Open(DLG_TYPE::ASYNC, ID_ACTIVEOBJECT, -1, -1, 500, 300);
	}

	virtual Int32 GetState(BaseDocument* doc, GeDialog* parentManager)
	{
		return CMD_ENABLED;
	}

	virtual Bool RestoreLayout(void* secret)
	{
		return _dlg.RestoreLayout(ID_ACTIVEOBJECT, 0, secret);
	}

public:
	ActiveObjectDialog _dlg;
};

static ActiveObjectDialogCommand* g_cmd;

static void ShowObjectProps(BaseList2D* obj)
{
	if (!g_cmd)
		return;
	if (!g_cmd->_dlg.GetActiveCustomGui())
		return;
	g_cmd->_dlg.GetActiveCustomGui()->SetObject(obj);
}

Bool RegisterActiveObjectDlg()
{
	g_cmd = NewObjClear(ActiveObjectDialogCommand);
	return RegisterCommandPlugin(ID_ACTIVEOBJECT, GeLoadString(IDS_ACTIVEOBJECT), 0, nullptr, String("C++ SDK Active Object"), g_cmd);
}

