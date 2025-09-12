// example code for usage of listview elements

#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"
#include <stdarg.h>
#include "maxon/utilities/sprintf_safe.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_LISTVIEWTEST = 1000452;

struct TestData
{
	Int32 id;
	Char	name[20];
};

TestData g_testdata[] =
{
	{ 10, "Sharpen" },
	{ 12, "LensFlares" },
	{ 11, "Emboss" },
	{ 14, "Depth of Field" },
	{ 214, "AA" },

	{ 0, "" }
};

class ListViewDialog : public GeDialog
{
public:
	ListViewDialog() { }
	virtual ~ListViewDialog() { }

	virtual Bool CreateLayout();
	virtual Bool InitValues();
	virtual Bool Command(Int32 id, const BaseContainer& msg);
	virtual Int32 Message(const BaseContainer& msg, BaseContainer& result);

private:
	void UpdateButtons();

private:
	SimpleListView _listview1;
	SimpleListView _listview2;
	AutoAlloc<BaseSelect>	_selection;
	Int32 _counter2 = 0;
};

enum
{
	IDC_OFFSET_LV = 1001,
	IDC_ACCESS_LV = 1002
};

Bool ListViewDialog::CreateLayout()
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();
	if (res)
		res = LoadDialogResource(DLG_LISTVIEW, nullptr, 0);

	if (res)
	{
		_listview1.AttachListView(this, GADGET_LISTVIEW1);
		_listview2.AttachListView(this, GADGET_LISTVIEW2);
	}

	return res;
}

void ListViewDialog::UpdateButtons()
{
	if (!_selection)
		return;

	Enable(GADGET_INSERT, _listview1.GetSelection(_selection) != 0);
	Enable(GADGET_REMOVE, _listview2.GetSelection(_selection) != 0);
}

Bool ListViewDialog::InitValues()
{
	// first call the parent instance
	if (!GeDialog::InitValues())
		return false;

	BaseContainer layout;
	BaseContainer data;

	layout.SetInt32('name', LV_COLUMN_TEXT);
	_listview1.SetLayout(1, layout);

	layout = BaseContainer();
	layout.SetInt32('chck', LV_COLUMN_CHECKBOX);
	layout.SetInt32('name', LV_COLUMN_TEXT);
	layout.SetInt32('bttn', LV_COLUMN_BUTTON);
	_listview2.SetLayout(3, layout);

	data = BaseContainer();

	for (Int32 i = 0; g_testdata[i].id; i++)
	{
		data.SetString('name', String(g_testdata[i].name));
		_listview1.SetItem(g_testdata[i].id, data);
	}

	_listview1.DataChanged();
	_listview2.DataChanged();

	UpdateButtons();

	return true;
}


Bool ListViewDialog::Command(Int32 id, const BaseContainer& msg)
{
	switch (id)
	{
		case GADGET_LISTVIEW1:
		case GADGET_LISTVIEW2:
		{
			switch (msg.GetInt32(BFM_ACTION_VALUE))
			{
				case LV_SIMPLE_SELECTIONCHANGED:
				{
					ApplicationOutput("Selection changed, id: @, val: @ ", msg.GetInt32(LV_SIMPLE_ITEM_ID), msg.GetVoid(LV_SIMPLE_DATA));
					break;
				}
				case LV_SIMPLE_CHECKBOXCHANGED:
				{
					ApplicationOutput("CheckBox changed, id: @, col: @, val: @", msg.GetInt32(LV_SIMPLE_ITEM_ID), msg.GetInt32(LV_SIMPLE_COL_ID), msg.GetVoid(LV_SIMPLE_DATA));
					break;
				}
				case LV_SIMPLE_FOCUSITEM:
				{
					ApplicationOutput("Focus set id: @, col: @", msg.GetInt32(LV_SIMPLE_ITEM_ID), msg.GetInt32(LV_SIMPLE_COL_ID));
					break;
				}
				case LV_SIMPLE_BUTTONCLICK:
				{
					ApplicationOutput("Button clicked id: @, col: @", msg.GetInt32(LV_SIMPLE_ITEM_ID), msg.GetInt32(LV_SIMPLE_COL_ID));
					break;
				}
			}
			UpdateButtons();
			break;
		}

		case GADGET_INSERT:
		{
			AutoAlloc<BaseSelect> s2;
			if (_selection && s2)
			{
				// TEST
				BaseContainer test;

				Int32	count = _listview1.GetItemCount();
				for (Int32 i = 0; i < count; i++)
				{
					Int32 id2;
					_listview1.GetItemLine(i, &id2, &test);
				}

				// TEST
				if (!_listview1.GetSelection(_selection))
				{
					ApplicationOutput("No Selection"_s);
				}
				else
				{
					Int32	 a, b;
					String str;
					for (Int32 i = 0; _selection->GetRange(i, LIMIT<Int32>::MAX, &a, &b); i++)
					{
						if (a == b)
							str += String::IntToString(a) + " ";
						else
							str += String::IntToString(a) + "-" + String::IntToString(b) + " ";
					}
					//				str.Delete(str.GetLength()-1,1);
					ApplicationOutput("Selection: " + str);

					BaseContainer data;
					for (Int32 i = 0; g_testdata[i].id; i++)
					{
						if (_selection->IsSelected(g_testdata[i].id))
						{
							data.SetInt32('chck', true);
							data.SetString('name', String(g_testdata[i].name));
							data.SetString('bttn', "..."_s);
							_selection->Select(_counter2);
							_listview2.SetItem(_counter2++, data);
						}
					}
					_listview2.SetSelection(_selection);
					_listview2.DataChanged();
				}
			}
			UpdateButtons();
			break;
		}

		case GADGET_REMOVE:
		{
			if (_selection && _listview2.GetSelection(_selection))
			{
				Int32 a = 0;
				Int32 b = 0;
				for (Int32 i = 0; _selection->GetRange(i, LIMIT<Int32>::MAX, &a, &b); i++)
				{
					for (; a <= b; a++)
					{
						_listview2.RemoveItem(a);
					}
				}
				_listview2.DataChanged();
			}
			UpdateButtons();
			break;
		}
	}
	return true;
}


Int32 ListViewDialog::Message(const BaseContainer& msg, BaseContainer& result)
{
	// nothing to do here
	return GeDialog::Message(msg, result);
}

class ListViewTest : public CommandData
{
public:
	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager);
	virtual Int32 GetState(BaseDocument* doc, GeDialog* parentManager);
	virtual Bool RestoreLayout(void* secret);

private:
	ListViewDialog _dlg;
};

Int32 ListViewTest::GetState(BaseDocument* doc, GeDialog* parentManager)
{
	return CMD_ENABLED;
}

Bool ListViewTest::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	return _dlg.Open(DLG_TYPE::ASYNC, ID_LISTVIEWTEST, -1, -1);
}

Bool ListViewTest::RestoreLayout(void* secret)
{
	return _dlg.RestoreLayout(ID_LISTVIEWTEST, 0, secret);
}

Bool RegisterListView()
{
	return RegisterCommandPlugin(ID_LISTVIEWTEST, GeLoadString(IDS_LISTVIEW), 0, nullptr, String(), NewObjClear(ListViewTest));
}
