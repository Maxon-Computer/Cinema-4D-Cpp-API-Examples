// example code for a menu/manager plugin

// be sure to use a unique ID obtained from developers.maxon.net
#define ID_VARIABLE_TAG_DIALOG 450000282

#include <stdio.h>
#include "activeobject.h"

using namespace cinema;

enum
{
	IDC_VARIABLE_TAG_INFO1 = 1000,
	IDC_VARIABLE_TAG_INFO2,
	IDC_VARIABLE_TAG_NUM_COLS,
	IDC_VARIABLE_TAG_NUM_ROWS,
	IDC_VARIABLE_TAG_NUM_ROW_OFFSET,
	IDC_VARIABLE_TAG_DATA_FORMAT,
	IDC_VARIABLE_TAG_FLOAT_PRECISION,
	IDC_VARIABLE_TAG_DATA_TREE,
	IDC_VARIABLE_TAG_COLUMN_WIDTH,

	IDC_MEMORY_STAT_
};

class VariableTagDataTreeViewFunctions : public TreeViewFunctions
{
public:
	enum class DATA_TYPE
	{
		NONE				= 0,
		FLOAT32			= 1 << 0,
		INT32				= 1 << 1,
		UINT32			= 1 << 2,
		HEX32				= 1 << 3,
		VECTOR32		= 1 << 4,
		VECTOR64		= 1 << 5,
		POLYGON			= 1 << 6,
		GENERIC_HEX	= 1 << 7
	} MAXON_ENUM_FLAGS_CLASS(DATA_TYPE);

	struct TagData
	{
		const VariableTag* tag = nullptr;
		Int32 type = 0;
		Int32 dataCount = 0;
		Int32 dataSize = 0;
		const void* data = nullptr;	

		Int32 columnWidth = 50;
		Int32 numRows = 1;
		Int32 numColumns = 1;
		Int32 rowOffset = 0;
		DATA_TYPE dataType = DATA_TYPE::FLOAT32;
		DATA_TYPE allowedDataType = DATA_TYPE::NONE;

		char floatLayout[10];
	};

	inline void* LineIndexToVoid(Int32 index) { return (void*)((Int)(index + 1)); }
	inline Int32 VoidToLineIndex(const void* p) { return (Int32)(((Int)p) - 1); }

	virtual void*	GetFirst(void* root, void* userdata)
	{
		const TagData* tagData = static_cast<TagData*>(root);
		return tagData->data ? LineIndexToVoid(tagData->rowOffset) : nullptr;
	}

	virtual void*	GetNext(void* root, void* userdata, void* obj)
	{
		const TagData* tagData = static_cast<TagData*>(root);
		Int32 lineIndex = VoidToLineIndex(obj);
		if (lineIndex < Min((tagData->dataCount + tagData->numColumns) / tagData->numColumns, tagData->rowOffset + tagData->numRows) - 1)
			return LineIndexToVoid(lineIndex + 1);
		return nullptr;
	}

	virtual void* GetPred(void* root, void* userdata, void* obj)
	{
		Int32 lineIndex = VoidToLineIndex(obj);
		if (lineIndex > 0)
			return LineIndexToVoid(lineIndex - 1);
		return nullptr;
	}

	virtual void*	GetDown(void* root, void* userdata, void* obj)
	{
		return nullptr;
	}

	virtual Bool IsSelected(void* root, void* userdata, void* obj)
	{
		return false;
	}

	virtual String GetName(void* root, void* userdata, void* obj)
	{
		const TagData* tagData = static_cast<TagData*>(root);
		Int32 lineIndex = VoidToLineIndex(obj);

		return FormatString("@", lineIndex * tagData->numColumns);
	}

	virtual Int GetId(void* root, void* userdata, void* obj)
	{
		return (Int)obj;
	}

	virtual Bool IsOpened(void* root, void* userdata, void* obj)
	{
		return false;
	}

	virtual void Open(void* root, void* userdata, void* obj, Bool onoff)
	{
	}

	virtual void Select(void* root, void* userdata, void* obj, Int32 mode)
	{
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
		const TagData* tagData = static_cast<TagData*>(root);
		if (col == 'tree')
			return ua ? ua->DrawGetTextWidth("wwwww"_s) : 100;
		return tagData->columnWidth;
	}

	virtual Int32 GetColumnWidth(void* root, void* userdata, void* obj, Int32 col, GeUserArea* pArea)
	{
		const TagData* tagData = static_cast<TagData*>(root);
		if (col == 'tree')
			return pArea ? pArea->DrawGetTextWidth("wwwww"_s) : 100;
		return tagData->columnWidth;
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

		if (col == 'tree')
			name = "Offset"_s;
		else
			name = FormatString("@", col);

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
		const TagData* tagData = static_cast<TagData*>(root);
		Int32 lineIndex = VoidToLineIndex(obj);
		Int32 dataIndex = lineIndex * tagData->numColumns + col;
		if (dataIndex >= 0 && dataIndex < tagData->dataCount)
		{
			Int32 wx, wy, wh, ww;
			wx = drawinfo->xpos;
			wy = drawinfo->ypos;
			ww = drawinfo->width;
			wh = drawinfo->height;

			const void* data = (const void*)((UInt)(tagData->data) + dataIndex * tagData->dataSize);
			drawinfo->frame->DrawSetTextCol(COLOR_TEXT, bgColor);
			String t;
			switch (tagData->dataType)
			{
				case DATA_TYPE::FLOAT32:
					t = FormatString(tagData->floatLayout, *static_cast<const Float32*>(data));
					break;
				case DATA_TYPE::INT32:
					t = FormatString("@", *static_cast<const Int32*>(data));
					break;
				case DATA_TYPE::UINT32:
					t = FormatString("@", *static_cast<const UInt32*>(data));
					break;
				case DATA_TYPE::HEX32:
					t = FormatString("@{x}", *static_cast<const UInt32*>(data));
					break;
				case DATA_TYPE::VECTOR32:
				{
					const Vector32& p = *static_cast<const Vector32*>(data);
					t += FormatString(tagData->floatLayout, p.x);
					t += ", ";
					t += FormatString(tagData->floatLayout, p.y);
					t += ", ";
					t += FormatString(tagData->floatLayout, p.z);
					break;
				}
				case DATA_TYPE::VECTOR64:
				{
					const Vector64& p = *static_cast<const Vector64*>(data);
					t += FormatString(tagData->floatLayout, p.x);
					t += ", ";
					t += FormatString(tagData->floatLayout, p.y);
					t += ", ";
					t += FormatString(tagData->floatLayout, p.z);
					break;
				}
				case DATA_TYPE::POLYGON:
				{
					const CPolygon& p = *static_cast<const CPolygon*>(data);
					if (p.c == p.d)
						t = FormatString("@, @, @", p.a, p.b, p.c);
					else
						t = FormatString("@, @, @, @", p.a, p.b, p.c, p.d);
					break;
				}
				case DATA_TYPE::GENERIC_HEX:
				{
					const UChar* c = static_cast<const UChar*>(data);
					for (Int32 i = 0; i < tagData->dataSize; ++i)
						t += FormatString("@{x}", c[i]);
					break;
				}
				default:
					t = "??"_s;
					break;
			}
			drawinfo->frame->SetClippingRegion(wx, wy, ww, wh);
			drawinfo->frame->DrawText(t, wx, wy);
			drawinfo->frame->ClearClippingRegion();
		}
	}

	virtual void CreateContextMenu(void* root, void* userdata, void* obj, Int32 lColumn, BaseContainer* bc)
	{
	}

	Bool ContextMenuCall(void* root, void* userdata, void* obj, Int32 lColumn, Int32 lCommand)
	{
		return false;
	}
};

static VariableTagDataTreeViewFunctions g_variableTagDataFunctable;

class VariableTagDataDialog : public GeDialog
{
public:
	explicit VariableTagDataDialog(const VariableTag* tag);
	virtual Bool CreateLayout();

	virtual Bool InitValues();
	virtual Bool Command(Int32 id, const BaseContainer& msg);
	virtual Int32 Message(const BaseContainer& msg, BaseContainer& result);
	virtual Bool CoreMessage  (Int32 id, const BaseContainer& msg);

private:
	void SetTreeLayout();

	const VariableTag* _tag;
	VariableTagDataTreeViewFunctions::TagData _tagData;
	TreeViewCustomGui* _tree = nullptr;
	VariableTagDataTreeViewFunctions::DATA_TYPE _preferredDataType = VariableTagDataTreeViewFunctions::DATA_TYPE::NONE;
};

VariableTagDataDialog::VariableTagDataDialog(const VariableTag* tag) :
	_tag(tag)
{
	_tagData.tag = tag;
	_tagData.type = tag->GetType();
	_tagData.dataCount = tag->GetDataCount();
	_tagData.dataSize = tag->GetDataSize();
	_tagData.data = tag->GetLowlevelDataAddressR();

	if (_tagData.dataSize == 4)
		_tagData.allowedDataType |= VariableTagDataTreeViewFunctions::DATA_TYPE::FLOAT32 | VariableTagDataTreeViewFunctions::DATA_TYPE::INT32 | 
			VariableTagDataTreeViewFunctions::DATA_TYPE::UINT32 | VariableTagDataTreeViewFunctions::DATA_TYPE::HEX32;
	else if (_tagData.dataSize == 12)
		_tagData.allowedDataType |= VariableTagDataTreeViewFunctions::DATA_TYPE::VECTOR32;
	else if (_tagData.dataSize == 24)
		_tagData.allowedDataType |= VariableTagDataTreeViewFunctions::DATA_TYPE::VECTOR64;
	else if (_tagData.dataSize == 16)
		_tagData.allowedDataType |= VariableTagDataTreeViewFunctions::DATA_TYPE::POLYGON;

	_tagData.allowedDataType |= VariableTagDataTreeViewFunctions::DATA_TYPE::GENERIC_HEX;
}

Bool VariableTagDataDialog::CreateLayout()
{
	// First call the parent instance.
	Bool res = GeDialog::CreateLayout();

	SetTitle("Variable tag data"_s);
	AddStaticText(IDC_VARIABLE_TAG_INFO1, BFH_SCALEFIT, 0, 0, ""_s, 0);
	AddStaticText(IDC_VARIABLE_TAG_INFO2, BFH_SCALEFIT, 0, 0, ""_s, 0);
	GroupBegin(0, BFH_SCALEFIT, 6, 0, String(), 0);
	{
		AddStaticText(0, BFH_FIT, 0, 0, "Colums:"_s, 0);
		AddEditNumberArrows(IDC_VARIABLE_TAG_NUM_COLS, 0);
		AddStaticText(0, BFH_FIT, 0, 0, "Rows:"_s, 0);
		AddEditNumberArrows(IDC_VARIABLE_TAG_NUM_ROWS, 0);
		AddStaticText(0, BFH_FIT, 0, 0, "Row Offset:"_s, 0);
		AddEditNumberArrows(IDC_VARIABLE_TAG_NUM_ROW_OFFSET, 0);
	}
	GroupEnd();

	GroupBegin(0, BFH_SCALEFIT, 3, 0, String(), 0);
	{
		GroupBegin(0, BFH_LEFT, 2, 0, String(), 0);
		{
			AddStaticText(0, BFH_LEFT, 0, 0, "Show data as:"_s, 0);
			AddComboBox(IDC_VARIABLE_TAG_DATA_FORMAT, BFH_FIT);
	#define ADD_IF_ALLOWED(type, s) \
		if (_tagData.allowedDataType & VariableTagDataTreeViewFunctions::DATA_TYPE::type) { \
			AddChild(IDC_VARIABLE_TAG_DATA_FORMAT, (Int32)VariableTagDataTreeViewFunctions::DATA_TYPE::type, s); \
			if (_preferredDataType == VariableTagDataTreeViewFunctions::DATA_TYPE::NONE) _preferredDataType = VariableTagDataTreeViewFunctions::DATA_TYPE::type; }

			ADD_IF_ALLOWED(FLOAT32, "Float32"_s);
			ADD_IF_ALLOWED(INT32, "Int32"_s);
			ADD_IF_ALLOWED(UINT32, "UInt32"_s);
			ADD_IF_ALLOWED(HEX32, "32 bit hex"_s);
			ADD_IF_ALLOWED(VECTOR32, "Vector32"_s);
			ADD_IF_ALLOWED(VECTOR64, "Vector64"_s);
			ADD_IF_ALLOWED(POLYGON, "Polygon"_s);
			ADD_IF_ALLOWED(GENERIC_HEX, "Generic hex"_s);
		}
		GroupEnd();

		GroupBegin(0, BFH_SCALEFIT, 2, 0, String(), 0);
		{
			AddStaticText(0, BFH_LEFT, 0, 0, "Float precision:"_s, 0);
			AddEditNumberArrows(IDC_VARIABLE_TAG_FLOAT_PRECISION, 0);
		}
		GroupEnd();

		GroupBegin(0, BFH_RIGHT, 2, 0, String(), 0);
		{
			AddStaticText(0, BFH_FIT, 0, 0, "Column width:"_s, 0);
			AddEditNumberArrows(IDC_VARIABLE_TAG_COLUMN_WIDTH, 0);
		}
		GroupEnd();
	}
	GroupEnd();

	BaseContainer treedata;
	treedata.SetBool(TREEVIEW_BORDER, true);
	treedata.SetBool(TREEVIEW_HAS_HEADER, true);
	treedata.SetBool(TREEVIEW_RESIZE_HEADER, true);
	treedata.SetBool(TREEVIEW_FIXED_LAYOUT, true);
	treedata.SetBool(TREEVIEW_ALTERNATE_BG, true);
	_tree = (TreeViewCustomGui*)AddCustomGui(IDC_VARIABLE_TAG_DATA_TREE, CUSTOMGUI_TREEVIEW, String(), BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, treedata);
	if (!_tree)
		return false;

	SetTreeLayout();

	return res;
}

Bool VariableTagDataDialog::InitValues()
{
	// First call the parent instance.
	if (!GeDialog::InitValues())
		return false;

	const int defaultFloatPrecision = 2;
	_tagData.numColumns = 16;
	_tagData.numRows = 16;
	_tagData.rowOffset = 0;
	_tagData.columnWidth = 50;
	_tagData.dataType = _preferredDataType;
	snprintf(_tagData.floatLayout, SIZEOF(_tagData.floatLayout), "@{.%d}", defaultFloatPrecision);

	SetString(IDC_VARIABLE_TAG_INFO1, FormatString("Tag Name: \"@\", Type: @, Address: 0x@", _tagData.tag->GetName(), _tagData.type, (void*)_tagData.tag));
	SetString(IDC_VARIABLE_TAG_INFO2, FormatString("Data address: 0x@, Count: @, Size: @", _tagData.data, _tagData.dataCount, _tagData.dataSize));
	SetInt32(IDC_VARIABLE_TAG_NUM_COLS, _tagData.numColumns, 1, 32);
	SetInt32(IDC_VARIABLE_TAG_NUM_ROWS, _tagData.numRows, 1, LIMIT<Int32>::MAX);
	SetInt32(IDC_VARIABLE_TAG_NUM_ROW_OFFSET, _tagData.rowOffset, 0, LIMIT<Int32>::MAX);
	SetInt32(IDC_VARIABLE_TAG_DATA_FORMAT, (Int32)_preferredDataType);
	SetInt32(IDC_VARIABLE_TAG_COLUMN_WIDTH, _tagData.columnWidth, 30);
	SetInt32(IDC_VARIABLE_TAG_FLOAT_PRECISION, defaultFloatPrecision, 0, 20);

	SetTreeLayout();

	_tree->SetRoot(&_tagData, &g_variableTagDataFunctable, this);
	_tree->Refresh();

	return true;
}

void VariableTagDataDialog::SetTreeLayout()
{
	BaseContainer layout;
	Int32 numColumns = 16;
	GetInt32(IDC_VARIABLE_TAG_NUM_COLS, numColumns);

	layout.SetInt32('tree', LV_TREE);
	for (Int32 c = 0; c < numColumns; ++c)
		layout.SetInt32(c, LV_USER);
	_tree->SetLayout(1 + numColumns, layout);
}

Bool VariableTagDataDialog::Command(Int32 id, const BaseContainer& msg)
{
	switch (id)
	{
		case IDC_VARIABLE_TAG_NUM_COLS:
			GetInt32(IDC_VARIABLE_TAG_NUM_COLS, _tagData.numColumns);
			SetTreeLayout();
			_tree->SetRoot(&_tagData, &g_variableTagDataFunctable, this);
			_tree->Refresh();
			break;

		case IDC_VARIABLE_TAG_NUM_ROWS:
			GetInt32(IDC_VARIABLE_TAG_NUM_ROWS, _tagData.numRows);
			_tree->Refresh();
			break;

		case IDC_VARIABLE_TAG_NUM_ROW_OFFSET:
			GetInt32(IDC_VARIABLE_TAG_NUM_ROW_OFFSET, _tagData.rowOffset);
			_tree->Refresh();
			break;

		case IDC_VARIABLE_TAG_DATA_FORMAT:
		{
			Int32 dataType = 0;
			GetInt32(IDC_VARIABLE_TAG_DATA_FORMAT, dataType);
			_tagData.dataType = (VariableTagDataTreeViewFunctions::DATA_TYPE)dataType;
			_tree->Refresh();
			break;
		}

		case IDC_VARIABLE_TAG_COLUMN_WIDTH:
			GetInt32(IDC_VARIABLE_TAG_COLUMN_WIDTH, _tagData.columnWidth);
			_tree->Refresh();
			break;

		case IDC_VARIABLE_TAG_FLOAT_PRECISION:
		{
			Int32 precision = 0;
			GetInt32(IDC_VARIABLE_TAG_FLOAT_PRECISION, precision);
			snprintf(_tagData.floatLayout, SIZEOF(_tagData.floatLayout), "@{.%d}", precision);
			_tree->Redraw();
			break;
		}
	}

	return true;
}

Int32 VariableTagDataDialog::Message(const BaseContainer& msg, BaseContainer& result)
{
	return GeDialog::Message(msg, result);
}

Bool VariableTagDataDialog::CoreMessage(Int32 id, const BaseContainer& msg)
{
	return GeDialog::CoreMessage(id, msg);
}

void ShowVariableTagDataDialog(const VariableTag* tag)
{
	VariableTagDataDialog dlg(tag);
	dlg.Open(DLG_TYPE::MODAL_RESIZEABLE, ID_VARIABLE_TAG_DIALOG);
}
