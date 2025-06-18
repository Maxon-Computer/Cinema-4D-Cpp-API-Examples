// example code for usage of listview elements

// be sure to use a unique ID obtained from developers.maxon.net
#define ID_LISTVIEWTEST 1000452

#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"
#include <stdarg.h>
#include "maxon/utilities/sprintf_safe.h"

using namespace cinema;

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
private:
	SimpleListView				listview1;
	SimpleListView				listview2;
	AutoAlloc<BaseSelect>	selection;
	Int32									counter2;

	void UpdateButtons();

public:
	ListViewDialog();
	virtual ~ListViewDialog();

	virtual Bool CreateLayout();
	virtual Bool InitValues();
	virtual Bool Command(Int32 id, const BaseContainer& msg);
	virtual Int32 Message(const BaseContainer& msg, BaseContainer& result);

};

enum
{
	IDC_OFFSET = 1001,
	IDC_ACCESS = 1002
};

ListViewDialog::ListViewDialog()
{
	counter2 = 0;
}

ListViewDialog::~ListViewDialog()
{
}

Bool ListViewDialog::CreateLayout()
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	res = LoadDialogResource(DLG_LISTVIEW, nullptr, 0);

	if (res)
	{
		listview1.AttachListView(this, GADGET_LISTVIEW1);
		listview2.AttachListView(this, GADGET_LISTVIEW2);
	}

	return res;
}

void ListViewDialog::UpdateButtons()
{
	if (!selection)
		return;

	Enable(GADGET_INSERT, listview1.GetSelection(selection) != 0);
	Enable(GADGET_REMOVE, listview2.GetSelection(selection) != 0);
}

Bool ListViewDialog::InitValues()
{
	// first call the parent instance
	if (!GeDialog::InitValues())
		return false;

	BaseContainer layout;
	BaseContainer data;
	Int32					i = 0;

	layout.SetInt32('name', LV_COLUMN_TEXT);
	//	layout.SetInt32('used',LV_COLUMN_CHECKBOX);
	listview1.SetLayout(1, layout);

	layout = BaseContainer();
	layout.SetInt32('chck', LV_COLUMN_CHECKBOX);
	layout.SetInt32('name', LV_COLUMN_TEXT);
	layout.SetInt32('bttn', LV_COLUMN_BUTTON);
	listview2.SetLayout(3, layout);

	data = BaseContainer();

	for (i = 0; g_testdata[i].id; i++)
	{
		data.SetString('name', String(g_testdata[i].name));
		//data.SetInt32('used',false);
		listview1.SetItem(g_testdata[i].id, data);
	}

	//data = BaseContainer();
	//for (i=0;testdata[i].id;i++)
	//{
	//	data.SetInt32('chck',true);
	//	data.SetString('name',testdata[i].name);
	//	data.SetString('bttn',"...");
	//	listview2.SetItem(testdata[i].id,data);
	//}

	listview1.DataChanged();
	listview2.DataChanged();

	UpdateButtons();

	return true;
}


static void ApplicationOutputF(const Char* format, ...)
{
	va_list arp;
	Char		buf[1024];

	va_start(arp, format);
	vsprintf_safe(buf, sizeof(buf), format, arp);
	ApplicationOutput(String(buf));
	va_end(arp);
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
					ApplicationOutputF("Selection changed, id: %d, val: %p ", msg.GetInt32(LV_SIMPLE_ITEM_ID), msg.GetVoid(LV_SIMPLE_DATA));
					break;

				case LV_SIMPLE_CHECKBOXCHANGED:
					ApplicationOutputF("CheckBox changed, id: %d, col: %d, val: %p", msg.GetInt32(LV_SIMPLE_ITEM_ID), msg.GetInt32(LV_SIMPLE_COL_ID), msg.GetVoid(LV_SIMPLE_DATA));
					break;

				case LV_SIMPLE_FOCUSITEM:
					ApplicationOutputF("Focus set id: %d, col: %d", msg.GetInt32(LV_SIMPLE_ITEM_ID), msg.GetInt32(LV_SIMPLE_COL_ID));
					break;

				case LV_SIMPLE_BUTTONCLICK:
					ApplicationOutputF("Button clicked id: %d, col: %d", msg.GetInt32(LV_SIMPLE_ITEM_ID), msg.GetInt32(LV_SIMPLE_COL_ID));
					break;
			}
			UpdateButtons();
			break;
		}

		//		case GADGET_LISTVIEW2:
		//			break;

		case GADGET_INSERT:
		{
			AutoAlloc<BaseSelect> s2;
			if (selection && s2)
			{
				// TEST
				Int32					i, id2, count = listview1.GetItemCount();
				BaseContainer test;

				for (i = 0; i < count; i++)
				{
					listview1.GetItemLine(i, &id2, &test);
				}
				// TEST

				if (!listview1.GetSelection(selection))
				{
					ApplicationOutput("No Selection"_s);
				}
				else
				{
					Int32	 a, b;
					String str;
					for (i = 0; selection->GetRange(i, LIMIT<Int32>::MAX, &a, &b); i++)
					{
						if (a == b)
							str += String::IntToString(a) + " ";
						else
							str += String::IntToString(a) + "-" + String::IntToString(b) + " ";
					}
					//				str.Delete(str.GetLength()-1,1);
					ApplicationOutput("Selection: " + str);

					BaseContainer data;
					for (i = 0; g_testdata[i].id; i++)
					{
						if (selection->IsSelected(g_testdata[i].id))
						{
							data.SetInt32('chck', true);
							data.SetString('name', String(g_testdata[i].name));
							data.SetString('bttn', "..."_s);
							selection->Select(counter2);
							listview2.SetItem(counter2++, data);
						}
					}
					listview2.SetSelection(selection);
					listview2.DataChanged();
				}
			}
			UpdateButtons();
			break;
		}

		case GADGET_REMOVE:
		{
			if (selection && listview2.GetSelection(selection))
			{
				Int32 i, a, b;
				for (i = 0; selection->GetRange(i, LIMIT<Int32>::MAX, &a, &b); i++)
				{
					for (; a <= b; a++)
					{
						listview2.RemoveItem(a);
					}
				}
				listview2.DataChanged();
			}
			UpdateButtons();
			break;
		}
	}
	return true;
}


Int32 ListViewDialog::Message(const BaseContainer& msg, BaseContainer& result)
{
	//	switch (msg.GetId())
	{
	}
	return GeDialog::Message(msg, result);
}

class ListViewTest : public CommandData
{
private:
	ListViewDialog dlg;

public:
	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager);
	virtual Int32 GetState(BaseDocument* doc, GeDialog* parentManager);
	virtual Bool RestoreLayout(void* secret);
};

Int32 ListViewTest::GetState(BaseDocument* doc, GeDialog* parentManager)
{
	return CMD_ENABLED;
}

Bool ListViewTest::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	return dlg.Open(DLG_TYPE::ASYNC, ID_LISTVIEWTEST, -1, -1);
}

Bool ListViewTest::RestoreLayout(void* secret)
{
	return dlg.RestoreLayout(ID_LISTVIEWTEST, 0, secret);
}

Bool RegisterListView()
{
	return RegisterCommandPlugin(ID_LISTVIEWTEST, GeLoadString(IDS_LISTVIEW), 0, nullptr, String(), NewObjClear(ListViewTest));
}
