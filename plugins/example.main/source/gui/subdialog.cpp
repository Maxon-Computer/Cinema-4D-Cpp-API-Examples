// example code for a sub dialog

#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_SUBDIALOGTEST = 1000454;

class MySubDialog1 : public SubDialog
{
	virtual Bool CreateLayout()
	{
		GroupBegin(0, BFH_SCALEFIT | BFV_SCALEFIT, 2, 0, String(), BFV_GRIDGROUP_ALLOW_WEIGHTS);
		AddButton(1000, BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, "SubDialog1"_s);
		AddButton(1001, BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, "SubDialog1"_s);
		AddButton(1002, BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, "SubDialog1"_s);
		AddButton(1003, BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, "SubDialog1"_s);
		AddEditNumberArrows(1006, BFH_SCALEFIT);
		AddEditShortcut(1004, BFH_SCALEFIT | BFV_CENTER);

		BaseContainer fontchooserdata;
		fontchooserdata.SetBool(FONTCHOOSER_ENABLE_SIZE, true);
		AddCustomGui(1010, CUSTOMGUI_FONTCHOOSER, String(), BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, fontchooserdata);

		GroupEnd();
		return true;
	}
	virtual Bool InitValues()
	{
		SetInt32(1006, Int32(0), 0, 10);

		BaseContainer shortcut;
		shortcut.SetInt32(0, 0);			// qual
		shortcut.SetInt32(1, KEY_F1);	// key

		BaseContainer m(BFM_VALUECHNG);
		m.SetContainer(BFM_ACTION_VALUE, shortcut);

		SendMessage(1004, m);

		return true;
	}
	virtual Bool Command(Int32 id, const BaseContainer& msg)
	{
		switch (id)
		{
			case 1004:
			{
				const GeData& d = msg.GetData(BFM_ACTION_VALUE);
				if (d.GetType() == DA_CONTAINER)
				{
					const BaseContainer* bc = d.GetContainer();
					String shortcut;
					if (bc)
					{
						shortcut = Shortcut2String(*bc);
						ApplicationOutput("Shortcut received: " + shortcut);
					}
				}
				break;
			}
		}
		return true;
	}
};

class MySubDialog2 : public SubDialog
{
public:
	MySubDialog2()
	{
	}

	virtual Bool CreateLayout()
	{
		GroupBegin(999, BFH_SCALEFIT | BFV_SCALEFIT, 3, 0, String(), BFV_GRIDGROUP_ALLOW_WEIGHTS);

		AddStaticText(1000, BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, "SubDialog2"_s, BORDER_THIN_IN);
		AddStaticText(1001, BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, "SubDialog2"_s, BORDER_THIN_IN);
		AddStaticText(1001, BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, "SubDialog2"_s, BORDER_THIN_IN);

		AddStaticText(1002, BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, "SubDialog2"_s, BORDER_THIN_IN);
		AddStaticText(1003, BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, "SubDialog2"_s, BORDER_THIN_IN);
		AddStaticText(1003, BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, "SubDialog2"_s, BORDER_THIN_IN);

		AddStaticText(1004, BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, "SubDialog2"_s, BORDER_THIN_IN);
		AddStaticText(1005, BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, "SubDialog2"_s, BORDER_THIN_IN);
		AddStaticText(1005, BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, "SubDialog2"_s, BORDER_THIN_IN);

		AddEditNumberArrows(1006, BFH_SCALEFIT);

		if (!_weightsSaved)
		{
			// set the columns
			_weights.SetInt32(GROUPWEIGHTS_PERCENT_W_CNT, 3);				// number of rows - has to be equal to the given layout
			_weights.SetFloat(GROUPWEIGHTS_PERCENT_W_VAL + 0, 1);		// weight for col 1
			_weights.SetFloat(GROUPWEIGHTS_PERCENT_W_VAL + 1, -250);	// FIXED weight for col 2
			_weights.SetFloat(GROUPWEIGHTS_PERCENT_W_VAL + 2, 1);		// weight for col 1

			// set the rows
			_weights.SetInt32(GROUPWEIGHTS_PERCENT_H_CNT, 4);					// number of rows - has to be equal to the given layout
			_weights.SetFloat(GROUPWEIGHTS_PERCENT_H_VAL + 0, -1.0);		// weight for row 1
			_weights.SetFloat(GROUPWEIGHTS_PERCENT_H_VAL + 1, -150.0);	// FIXED weight for row 2
			_weights.SetFloat(GROUPWEIGHTS_PERCENT_H_VAL + 2, 60.0);		// weight for row 3
			_weights.SetFloat(GROUPWEIGHTS_PERCENT_H_VAL + 3, 0.0);		// weight for row 4
			_weightsSaved = true;
		}

		GroupWeightsLoad(999, _weights);

		GroupEnd();
		return true;
	}
	virtual Int32 Message(const BaseContainer& msg, BaseContainer& result)
	{
		switch (msg.GetId())
		{
			case BFM_WEIGHTS_CHANGED:
				// if the weights change because of user interaction you will get notified
				if (msg.GetInt32(BFM_WEIGHTS_CHANGED) == 999)
					GroupWeightsSave(999, _weights);
				break;
		}
		return SubDialog::Message(msg, result);
	}
	virtual Bool InitValues()
	{
		SetInt32(1006, Int32(0), 0, 10);
		return true;
	}
	virtual Bool Command(Int32 id, const BaseContainer& msg)
	{
		return true;
	}

private:
	BaseContainer _weights;
	Bool _weightsSaved = false;
};


class MainDialog : public GeDialog
{
public:
	MainDialog() { }
	virtual ~MainDialog() { }

	virtual Bool CreateLayout();
	virtual Bool InitValues();
	virtual Bool Command(Int32 id, const BaseContainer& msg);
	virtual Int32 Message(const BaseContainer& msg, BaseContainer& result);

private:
	MySubDialog1 _subdialog1;
	MySubDialog2 _subdialog2;
	SubDialog* _lastdlg = nullptr;
};

Bool MainDialog::CreateLayout()
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	if (res)
		res = LoadDialogResource(DLG_SUBDIALOG, nullptr, 0);

	if (res)
	{
		AttachSubDialog(&_subdialog1, GADGET_SUBDIALOG);
		_lastdlg = &_subdialog1;
	}

	return res;
}

Bool MainDialog::InitValues()
{
	// first call the parent instance
	if (!GeDialog::InitValues())
		return false;

	return true;
}

Bool MainDialog::Command(Int32 id, const BaseContainer& msg)
{
	switch (id)
	{
		case GADGET_SUB1:
		{
			if (!_lastdlg || !_lastdlg->CheckClose())
			{
				AttachSubDialog(&_subdialog1, GADGET_SUBDIALOG);
				LayoutChanged(GADGET_SUBDIALOG);
				_lastdlg = &_subdialog1;
			}
			break;
		}
		case GADGET_SUB2:
		{
			if (!_lastdlg || !_lastdlg->CheckClose())
			{
				AttachSubDialog(&_subdialog2, GADGET_SUBDIALOG);
				LayoutChanged(GADGET_SUBDIALOG);
				_lastdlg = &_subdialog2;
			}
			break;
		}
	}
	return true;
}


Int32 MainDialog::Message(const BaseContainer& msg, BaseContainer& result)
{
	//	switch (msg.GetId())
	//	{
	//	}
	return GeDialog::Message(msg, result);
}

class SubDialogTest : public CommandData
{
public:
	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager);
	virtual Int32 GetState(BaseDocument* doc, GeDialog* parentManager);
	virtual Bool RestoreLayout(void* secret);

private:
	MainDialog _dlg;
};

Int32 SubDialogTest::GetState(BaseDocument* doc, GeDialog* parentManager)
{
	return CMD_ENABLED;
}

Bool SubDialogTest::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	return _dlg.Open(DLG_TYPE::ASYNC, ID_SUBDIALOGTEST, -1, -1);
}

Bool SubDialogTest::RestoreLayout(void* secret)
{
	return _dlg.RestoreLayout(ID_SUBDIALOGTEST, 0, secret);
}

Bool RegisterSubDialog()
{
	return RegisterCommandPlugin(ID_SUBDIALOGTEST, GeLoadString(IDS_SUBDIALOG), 0, nullptr, String(), NewObjClear(SubDialogTest));
}
