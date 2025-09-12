#include "c4d.h"
#include "main.h"
#include "odescription.h"
#include "c4d_symbols.h"

//----------------------------------------------------------------------------------------
/// This ObjectData example shows how to create parameter descriptions dynamically and how to access parameters.
//----------------------------------------------------------------------------------------

#include "customgui_filename.h"
#include "customgui_texbox.h"
#include "customgui_inexclude.h"
#include "customgui_priority.h"
#include "customgui_datetime.h"

using namespace cinema;

enum class DESCRIPTIONELEMENTSOBJECTDATA
{
	BUTTON = 0,
	LONG,
	LONG_CYCLE,
	LONG_QUICKTAB,
	LONG_SLIDER,
	REAL,
	REAL_SLIDER,
	VECTOR,
	MATRIX,
	STRING,
	STRING_MULTILINE,
	STATIC_TEXT,
	FILENAME,
	BOOL,
	LINK,
	SHADERLINK,
	TIME,
	COLOR_PARAMETER,

	GRADIENT = 50,
	SPLINE,
	BITMAPBUTTON,
	INCLUDE_EXCLUDE,
	PRIORITY,
	COLORPROFILE,
	DATETIME,

	GROUP = 100,
	SEPARATOR = 150,

	DUMMY
} MAXON_ENUM_LIST(DESCRIPTIONELEMENTSOBJECTDATA);

class ObjectDynamicDescription: public ObjectData
{
	INSTANCEOF(ObjectDynamicDescription, ObjectData);

public:

	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual Bool GetDEnabling(const GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc) const;
	virtual Bool GetDDescription(const GeListNode* node, Description* description, DESCFLAGS_DESC& flags) const;
	virtual Bool 	Message (GeListNode *node, Int32 type, void *data);

	static NodeData* Alloc() { return NewObjPtr(ObjectDynamicDescription); }

private:

	void FillSelection(Description* const description) const;
	void CreateDynamicDescriptions(Description* const description) const;
	void CreateDynamicElement(Description* const description, const Int32 selection) const;

	void SetParameter(GeListNode* node);
	void GetParameter(GeListNode* node);
};

Bool ObjectDynamicDescription::Init(GeListNode* node, Bool isCloneInit)
{
	BaseContainer& bc = static_cast<BaseList2D*>(node)->GetDataInstanceRef();
	if (!isCloneInit)
	{
		// set the default value for these custom or complex datatypes
		bc.SetData((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LINK, GeData(DTYPE_BASELISTLINK, DEFAULTVALUE));
		bc.SetData((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::SPLINE, GeData(CUSTOMDATATYPE_SPLINE, DEFAULTVALUE));
		bc.SetData((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::GRADIENT, GeData(CUSTOMDATATYPE_GRADIENT, DEFAULTVALUE));
		bc.SetData((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::INCLUDE_EXCLUDE, GeData(CUSTOMDATATYPE_INEXCLUDE_LIST, DEFAULTVALUE));
		bc.SetData((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::PRIORITY, GeData(CUSTOMGUI_PRIORITY_DATA, DEFAULTVALUE));
		bc.SetData((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::COLORPROFILE, GeData(CUSTOMDATATYPE_COLORPROFILE, DEFAULTVALUE));
	}

	return true;
}


Bool ObjectDynamicDescription::GetDEnabling(const GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc) const
{
	if (id[0].id == ID_SET_PARAMETER || id[0].id == ID_GET_PARAMETER)
	{
		const BaseContainer& data = static_cast<const BaseObject*>(node)->GetDataInstanceRef();
		const Int32 selection = data.GetInt32(ID_SELECT_DESCRIPTION);

		switch (selection)
		{
			case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::BUTTON:
			case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::STATIC_TEXT:
			case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::BITMAPBUTTON:
			case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::GROUP:
			case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::SEPARATOR:
			{
				return false;
			}
		}
	}
	return true;
}

Bool ObjectDynamicDescription::GetDDescription(const GeListNode* node, Description* description, DESCFLAGS_DESC& flags) const
{
	if (!description->LoadDescription(node->GetType()))
		return false;

	// fills the combo box defined in the resource file
	this->FillSelection(description);

	// create dynamic element
	this->CreateDynamicDescriptions(description);

	flags |= DESCFLAGS_DESC::LOADED;

	return SUPER::GetDDescription(node, description, flags);
}

Bool ObjectDynamicDescription::Message(GeListNode *node, Int32 type, void *data)
{
	switch (type)
	{
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand* dc = (DescriptionCommand*) data;

			const Int32 id = dc->_descId[0].id;

			switch (id)
			{
				case ID_SET_PARAMETER:
				{
					this->SetParameter(node);
					return true;
					break;
				}
				case ID_GET_PARAMETER:
				{
					this->GetParameter(node);
					return true;
					break;
				}
			}
			break;
		}
	}

	return SUPER::Message(node, type, data);
}

void ObjectDynamicDescription::FillSelection(Description* const description) const
{
	const DescID* singleid = description->GetSingleDescID();
	const DescID cid = ConstDescID(DescLevel(ID_SELECT_DESCRIPTION, DTYPE_LONG, 0));

	if (!singleid || cid.IsPartOf(*singleid, nullptr))
	{
		AutoAlloc<AtomArray> arr;
		BaseContainer* selectionParameter = description->GetParameterI(ConstDescID(DescLevel(ID_SELECT_DESCRIPTION, DTYPE_LONG, 0)), arr);

		if (selectionParameter != nullptr)
		{
			// fill the "cycle" of the combo box
			BaseContainer* items = selectionParameter->GetContainerInstanceWritable(DESC_CYCLE);

			if (items != nullptr)
			{
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::BUTTON, String("Button"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG, String("Long"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG_CYCLE, String("Long Cycle"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG_QUICKTAB, String("Long Quicktab"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG_SLIDER, String("Long Slider"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::REAL, String("Real"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::REAL_SLIDER, String("Real Slider"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::VECTOR, String("Vector"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::MATRIX, String("Matrix"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::STRING, String("String"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::STRING_MULTILINE, String("String Multiline"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::STATIC_TEXT, String("Static Text"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::FILENAME, String("Filename"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::BOOL, String("Bool"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::LINK, String("Link"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::SHADERLINK, String("Shaderlink"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::TIME, String("Time"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::COLOR_PARAMETER, String("Color"));

				items->SetString(-1, String(""));

				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::GRADIENT, String("Gradient"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::SPLINE, String("Spline"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::BITMAPBUTTON, String("Bitmap Button"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::INCLUDE_EXCLUDE, String("Include Exclude"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::PRIORITY, String("Priority"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::COLORPROFILE, String("Colorprofile"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::DATETIME, String("DateTime"));

				items->SetString(-2, String(""));

				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::GROUP, String("Group"));
				items->SetString((Int32)DESCRIPTIONELEMENTSOBJECTDATA::SEPARATOR, String("Separator"));
			}
		}
	}
}


void ObjectDynamicDescription::CreateDynamicDescriptions(Description* const description) const
{
	// get current selection

	GeData data;
	this->Get()->GetParameter(ConstDescIDLevel(ID_SELECT_DESCRIPTION), data, DESCFLAGS_GET::NONE);
	const Int32 selection = data.GetInt32();

	// create dynamic element
	this->CreateDynamicElement(description, selection);
}


MAXON_WARN_MUTE_FUNCTION_LENGTH void ObjectDynamicDescription::CreateDynamicElement(Description* const description, const Int32 selection) const
{
	const DescID* singleid = description->GetSingleDescID();

	switch (selection)
	{
		// DTYPE_BUTTON / CUSTOMGUI_BUTTON
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::BUTTON:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::BUTTON;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_BUTTON, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_BUTTON);

				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_BUTTON);
				bc.SetString(DESC_NAME, "Button"_s);
				bc.SetInt32(DESC_ANIMATE, DESC_ANIMATE_OFF);
				bc.SetInt32(DESC_SCALEH, 1);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_LONG
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_LONG, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_LONG);

				bc.SetString(DESC_NAME, "Long"_s);
				bc.SetInt32(DESC_SCALEH, 1);
				bc.SetInt32(DESC_MIN, 0);
				bc.SetInt32(DESC_MAX, 1000);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_LONG / CUSTOMGUI_CYCLE
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG_CYCLE:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG_CYCLE;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_LONG, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_LONG);

				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_CYCLE);
				bc.SetString(DESC_NAME, "Long Cycle"_s);
				bc.SetInt32(DESC_SCALEH, 1);

				// cycle elements
				BaseContainer items;
				items.SetString(0, String("Item 0"));
				items.SetString(1, String("Item 1"));
				items.SetString(2, String("Item 2"));

				bc.SetContainer(DESC_CYCLE, items);

				// icons
				BaseContainer icons;
				icons.SetInt32(0, IDM_COPY);
				icons.SetInt32(1, IDM_CUT);
				icons.SetInt32(2, IDM_DELETE);

				bc.SetContainer(DESC_CYCLEICONS, icons);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_LONG / QUICKTABSRADIO_GADGET
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG_QUICKTAB:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG_QUICKTAB;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_LONG, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_LONG);

				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_RADIOBUTTONS);
				bc.SetString(DESC_NAME, "Long Quicktab"_s);
				bc.SetInt32(DESC_SCALEH, 1);

				// quicktab elements
				BaseContainer items;
				items.SetString(0, String("Tab 0"));
				items.SetString(1, String("Tab 1"));
				items.SetString(2, String("Tab 2"));

				bc.SetContainer(DESC_CYCLE, items);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_LONG / CUSTOMGUI_LONGSLIDER
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG_SLIDER:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG_SLIDER;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_LONG, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_LONG);

				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_LONGSLIDER);
				bc.SetString(DESC_NAME, "Long Slider"_s);
				bc.SetInt32(DESC_SCALEH, 1);
				bc.SetInt32(DESC_MIN, 0);
				bc.SetInt32(DESC_MAX, 1000);
				bc.SetInt32(DESC_STEP, 1);

				// slider settings
				bc.SetInt32(DESC_MINSLIDER, 250);
				bc.SetInt32(DESC_MAXSLIDER, 750);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_REAL
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::REAL:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::REAL;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_REAL, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_REAL);

				bc.SetString(DESC_NAME, "Real"_s);
				bc.SetInt32(DESC_SCALEH, 1);
				bc.SetInt32(DESC_MIN, 0);
				bc.SetInt32(DESC_MAX, 1000);
				bc.SetInt32(DESC_STEP, 1);
				bc.SetInt32(DESC_UNIT, DESC_UNIT_METER);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_REAL / CUSTOMGUI_REALSLIDER
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::REAL_SLIDER:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::REAL_SLIDER;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_REAL, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_REAL);

				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_REALSLIDER);
				bc.SetString(DESC_NAME, "Real Slider"_s);
				bc.SetInt32(DESC_SCALEH, 1);
				bc.SetInt32(DESC_MIN, 0);
				bc.SetInt32(DESC_MAX, 1000);
				bc.SetInt32(DESC_STEP, 1);
				bc.SetInt32(DESC_UNIT, DESC_UNIT_METER);

				// slider settings
				bc.SetFloat(DESC_MINSLIDER, 250);
				bc.SetFloat(DESC_MAXSLIDER, 750);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_VECTOR / CUSTOMGUI_VECTOR
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::VECTOR:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::VECTOR;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_VECTOR, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_VECTOR);

				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_VECTOR);
				bc.SetString(DESC_NAME, "Vector"_s);
				bc.SetInt32(DESC_SCALEH, 1);
				bc.SetInt32(DESC_UNIT, DESC_UNIT_METER);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_MATRIX
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::MATRIX:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::MATRIX;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_MATRIX, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_MATRIX);
				bc.SetString(DESC_NAME, "Matrix"_s);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_STRING
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::STRING:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::STRING;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_STRING, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_STRING);
				bc.SetString(DESC_NAME, "String"_s);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_STRING / CUSTOMGUI_STRINGMULTI
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::STRING_MULTILINE:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::STRING_MULTILINE;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_STRING, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_STRING);

				bc.SetString(DESC_NAME, "Multiline"_s);
				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_STRINGMULTI);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_STATICTEXT / CUSTOMGUI_STATICTEXT
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::STATIC_TEXT:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::STATIC_TEXT;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_STATICTEXT, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_STATICTEXT);

				bc.SetString(DESC_NAME, "Static Text"_s);
				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_STATICTEXT);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_FILENAME / CUSTOMGUI_FILENAME
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::FILENAME:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::FILENAME;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_FILENAME, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_FILENAME);

				bc.SetString(DESC_NAME, "Filename"_s);
				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_FILENAME);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_BOOL
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::BOOL:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::BOOL;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_BOOL, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_BOOL);

				bc.SetString(DESC_NAME, "Boole"_s);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_BASELISTLINK / CUSTOMGUI_LINKBOX
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LINK:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LINK;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_BASELISTLINK, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);

				bc.SetString(DESC_NAME, "Link"_s);
				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_LINKBOX);

				BaseContainer ac;
				ac.SetInt32(Obase, 1);
				bc.SetContainer(DESC_ACCEPT, ac);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_BASELISTLINK / CUSTOMGUI_TEXBOX
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::SHADERLINK:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::SHADERLINK;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_BASELISTLINK, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);

				bc.SetString(DESC_NAME, "Shader"_s);
				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_TEXBOX);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_TIME
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::TIME:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::TIME;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_TIME, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_TIME);

				bc.SetString(DESC_NAME, "Time"_s);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_COLOR
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::COLOR_PARAMETER:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::COLOR_PARAMETER;
			const DescID cid = ConstDescID(DescLevel(ID, DTYPE_COLOR, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_COLOR);

				bc.SetString(DESC_NAME, "Color"_s);
				bc.SetData(DESC_GUIOPEN, true);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// CUSTOMDATATYPE_GRADIENT / CUSTOMGUI_GRADIENT
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::GRADIENT:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::GRADIENT;
			const DescID cid = ConstDescID(DescLevel(ID, CUSTOMDATATYPE_GRADIENT, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(CUSTOMDATATYPE_GRADIENT);

				bc.SetString(DESC_NAME, "Gradient"_s);
				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_GRADIENT);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// CUSTOMDATATYPE_SPLINE / CUSTOMGUI_SPLINE
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::SPLINE:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::SPLINE;
			const DescID cid = ConstDescID(DescLevel(ID, CUSTOMDATATYPE_SPLINE, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(CUSTOMDATATYPE_SPLINE);

				bc.SetString(DESC_NAME, "Spline"_s);
				bc.SetBool(SPLINECONTROL_GRID_H, false);
				bc.SetBool(SPLINECONTROL_GRID_V, true);
				bc.SetBool(SPLINECONTROL_VALUE_EDIT_H, true);
				bc.SetBool(SPLINECONTROL_VALUE_EDIT_V, true);
				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_SPLINE);
				bc.SetFloat(SPLINECONTROL_X_MIN, 0);
				bc.SetFloat(SPLINECONTROL_X_MAX, 100);
				bc.SetFloat(SPLINECONTROL_Y_MIN, 0);
				bc.SetFloat(SPLINECONTROL_Y_MAX, 100);


				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// CUSTOMDATATYPE_BITMAPBUTTON / CUSTOMGUI_BITMAPBUTTON
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::BITMAPBUTTON:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::BITMAPBUTTON;
			const DescID cid = ConstDescID(DescLevel(ID, CUSTOMDATATYPE_BITMAPBUTTON, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(CUSTOMDATATYPE_BITMAPBUTTON);

				bc.SetString(DESC_NAME, "Button"_s);
				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_BITMAPBUTTON);
				bc.SetBool(BITMAPBUTTON_BUTTON, true);
				bc.SetInt32(BITMAPBUTTON_BORDER, BORDER_BLACK);
				bc.SetInt32(BITMAPBUTTON_ICONID1, RESOURCEIMAGE_BROWSER_PLAY);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// CUSTOMDATATYPE_INEXCLUDE_LIST / CUSTOMGUI_INEXCLUDE_LIST
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::INCLUDE_EXCLUDE:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::INCLUDE_EXCLUDE;
			const DescID cid = ConstDescID(DescLevel(ID, CUSTOMDATATYPE_INEXCLUDE_LIST, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(CUSTOMDATATYPE_INEXCLUDE_LIST);

				bc.SetString(DESC_NAME, "In/Exclude"_s);
				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_INEXCLUDE_LIST);

				BaseContainer accept;
				accept.InsData(Obase, String());

				bc.SetData(DESC_ACCEPT, accept);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// CUSTOMGUI_PRIORITY_DATA / CUSTOMGUI_PRIORITY
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::PRIORITY:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::PRIORITY;
			const DescID cid = ConstDescID(DescLevel(ID, CUSTOMGUI_PRIORITY_DATA, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(CUSTOMGUI_PRIORITY_DATA);

				bc.SetString(DESC_NAME, "Priority"_s);
				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_PRIORITY);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// CUSTOMDATATYPE_COLORPROFILE
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::COLORPROFILE:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::COLORPROFILE;
			const DescID cid = ConstDescID(DescLevel(ID, CUSTOMDATATYPE_COLORPROFILE, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(CUSTOMDATATYPE_COLORPROFILE);

				bc.SetString(DESC_NAME, "Colorprofile"_s);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DATETIME_DATA
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::DATETIME:
		{
			const Int32 ID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::DATETIME;
			const DescID cid = ConstDescID(DescLevel(ID, DATETIME_DATA, 0));

			if (!singleid || cid.IsPartOf(*singleid, nullptr))
			{
				BaseContainer bc = GetCustomDataTypeDefault(DATETIME_DATA);

				bc.SetString(DESC_NAME, "DateTime"_s);

				description->SetParameter(cid, bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_SEPARATOR / CUSTOMGUI_SEPARATOR
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::SEPARATOR:
		{
			const Int32 separatorID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::SEPARATOR;

			// create two dummy string elements so that the separator appears

			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_STRING);
				bc.SetString(DESC_NAME, "Text"_s);

				description->SetParameter(CreateDescID(DescLevel(separatorID, DTYPE_STRING, 0)), bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}

			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_SEPARATOR);

				bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_SEPARATOR);
				bc.SetBool(DESC_SEPARATORLINE, true);

				description->SetParameter(CreateDescID(DescLevel(separatorID+1, DTYPE_SEPARATOR, 0)), bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}

			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_STRING);
				bc.SetString(DESC_NAME, "Text"_s);

				description->SetParameter(CreateDescID(DescLevel(separatorID+2, DTYPE_STRING, 0)), bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}
			break;
		}

		// DTYPE_GROUP
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::GROUP:
		{
			const Int32 groupID = (Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::GROUP;

			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_GROUP);

				bc.SetString(DESC_NAME, "Group"_s);
				bc.SetInt32(DESC_COLUMNS, 2);
				bc.SetInt32(DESC_DEFAULT, 1);
				bc.SetInt32(DESC_SCALEH, 1);

				description->SetParameter(CreateDescID(DescLevel(groupID, DTYPE_GROUP, 0)), bc, ConstDescIDLevel(ID_GROUP_DYNAMIC));
			}

			// dummy content

			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_STRING);
				bc.SetString(DESC_NAME, "Text 1"_s);
				bc.SetInt32(DESC_SCALEH, 1);

				description->SetParameter(CreateDescID(DescLevel(groupID+1, DTYPE_STRING, 0)), bc, ConstDescIDLevel(groupID));
			}

			{
				BaseContainer bc = GetCustomDataTypeDefault(DTYPE_STRING);
				bc.SetString(DESC_NAME, "Text 2"_s);
				bc.SetInt32(DESC_SCALEH, 1);

				description->SetParameter(CreateDescID(DescLevel(groupID+2, DTYPE_STRING, 0)), bc, ConstDescIDLevel(groupID));
			}
			break;
		}
	}
}

void ObjectDynamicDescription::SetParameter(GeListNode* node)
{
	if (node == nullptr)
		return;

	// get the document of this node
	BaseDocument* doc = node->GetDocument();

	if (doc == nullptr)
		return;

	// get the current selection
	GeData data;
	node->GetParameter(ConstDescIDLevel(ID_SELECT_DESCRIPTION), data, DESCFLAGS_GET::NONE);

	const Int32 selection = data.GetInt32();

	switch (selection)
	{
		// DTYPE_LONG
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG:
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG_SLIDER:
		{
			const Int32 value = 123;

			node->SetParameter(CreateDescID((Int32)ID_DYNAMIC_ELEMENT + selection), value, DESCFLAGS_SET::NONE);
			break;
		}

		// DTYPE_LONG
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG_CYCLE:
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG_QUICKTAB:
		{
			const Int32 value = 1;

			node->SetParameter(CreateDescID((Int32)ID_DYNAMIC_ELEMENT + selection), value, DESCFLAGS_SET::NONE);
			break;
		}

		// DTYPE_REAL
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::REAL:
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::REAL_SLIDER:
		{
			const Float32 value = 123.45f;

			node->SetParameter(CreateDescID((Int32)ID_DYNAMIC_ELEMENT + selection), value, DESCFLAGS_SET::NONE);
			break;
		}

		// DTYPE_VECTOR
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::VECTOR:
		{
			Vector value = Vector(1, 1, 1);

			if (node->IsInstanceOf(Obase))
			{
				BaseObject* object = static_cast<BaseObject*>(node);

				if (object != nullptr)
					value = object->GetMg().off;
			}

			node->SetParameter(CreateDescID((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::VECTOR), value, DESCFLAGS_SET::NONE);
			break;
		}

		// DTYPE_MATRIX
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::MATRIX:
		{
			Matrix value;

			if (node->IsInstanceOf(Obase))
			{
				BaseObject* object = static_cast<BaseObject*>(node);

				if (object != nullptr)
					value = object->GetMg();
			}

			node->SetParameter(CreateDescID((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::MATRIX), value, DESCFLAGS_SET::NONE);
			break;
		}

		// DTYPE_STRING
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::STRING:
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::STRING_MULTILINE:
		{
			String value = "Some string";

			if (node->IsInstanceOf(Tbaselist2d))
			{
				BaseList2D* entity = static_cast<BaseList2D*>(node);

				if (entity != nullptr)
					value = entity->GetName();
			}
			if (selection == (Int32)DESCRIPTIONELEMENTSOBJECTDATA::STRING_MULTILINE)
			{
				// Use newline (\n) to add more lines to a multi-line string element.
				// Note: In a string resource file the pipe symbol (|) is used instead.
				value += "\n2nd line: Some more string.";
			}

			node->SetParameter(CreateDescID((Int32)ID_DYNAMIC_ELEMENT + selection), value, DESCFLAGS_SET::NONE);
			break;
		}

		// DTYPE_FILENAME
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::FILENAME:
		{
			Filename filename;

			if (filename.FileSelect(FILESELECTTYPE::ANYTHING, FILESELECT::LOAD, "Select a C4D file"_s))
			{
				// set parameter
				node->SetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::FILENAME)), filename, DESCFLAGS_SET::NONE);
			}
			break;
		}

		// DTYPE_BOOL
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::BOOL:
		{
			const Bool value = true;

			node->SetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::BOOL)), value, DESCFLAGS_SET::NONE);
			break;
		}

		// DTYPE_BASELISTLINK
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LINK:
		{
			node->GetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LINK)), data, DESCFLAGS_GET::NONE);

			if (data.GetType() == DTYPE_BASELISTLINK)
			{
				AutoAlloc<BaseLink> link;
				if (link != nullptr)
				{
					link->SetLink(doc->GetActiveObject());

					// set parameter
					node->SetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LINK)), GeData(link), DESCFLAGS_SET::NONE);
				}
			}
			break;
		}

		// DTYPE_BASELISTLINK
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::SHADERLINK:
		{
			AutoAlloc<BaseLink> link;

			if (link != nullptr && node->IsInstanceOf(Tbaselist2d))
			{
				BaseShader* shader = BaseShader::Alloc(Xbrick);

				if (shader != nullptr)
				{
					BaseList2D* entity = static_cast<BaseList2D*>(node);

					link->SetLink(shader);

					// the entity takes ownership
					entity->InsertShader(shader);

					// set parameter
					node->SetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::SHADERLINK)), GeData(link), DESCFLAGS_SET::NONE);
				}
			}
			break;
		}

		// DTYPE_TIME
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::TIME:
		{
			BaseTime value;

			// get current document time
			value = doc->GetTime();

			// set parameter
			node->SetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::TIME)), value, DESCFLAGS_SET::NONE);
			break;
		}

		// DTYPE_COLOR
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::COLOR_PARAMETER:
		{
			const Vector value = Vector(0, 0, 1);

			// set parameter
			node->SetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::COLOR_PARAMETER)), value, DESCFLAGS_SET::NONE);
			break;
		}

		// CUSTOMDATATYPE_GRADIENT / CUSTOMGUI_GRADIENT
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::GRADIENT:
		{
			GeData value(CUSTOMDATATYPE_GRADIENT, DEFAULTVALUE);
			Gradient* gradient = value.GetCustomDataTypeWritable<Gradient>();

			if (gradient != nullptr)
			{
				{
					maxon::GradientKnot knot;
					knot.pos = 0.1;
					knot.col = maxon::Color(1, 0, 0);
					gradient->InsertKnot(knot);
				}

				{
					maxon::GradientKnot knot;
					knot.pos = 0.9;
					knot.col = maxon::Color(0, 1, 0);
					gradient->InsertKnot(knot);
				}
			}

			node->SetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::GRADIENT)), value, DESCFLAGS_SET::NONE);
			break;
		}

		// CUSTOMDATATYPE_SPLINE
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::SPLINE:
		{
			node->GetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::SPLINE)), data, DESCFLAGS_GET::NONE);

			if (data.GetType() == CUSTOMDATATYPE_SPLINE)
			{
				SplineData* splineData = data.GetCustomDataTypeWritable<SplineData>();

				if (splineData != nullptr)
				{

					splineData->SetRange(0.0, 100.0, 0.1, 0.0, 100, 0.0);

					splineData->DeleteAllPoints();
					splineData->InsertKnot(0, 0);
					splineData->InsertKnot(100, 100);

					// set parameter
					node->SetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::SPLINE)), data, DESCFLAGS_SET::NONE);
				}
			}
			break;
		}

		// CUSTOMDATATYPE_INEXCLUDE_LIST
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::INCLUDE_EXCLUDE:
		{
			node->GetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::INCLUDE_EXCLUDE)), data, DESCFLAGS_GET::NONE);

			if (data.GetType() == CUSTOMDATATYPE_INEXCLUDE_LIST)
			{
				InExcludeData* ieData = data.GetCustomDataTypeWritable<InExcludeData>();

				if (ieData != nullptr)
				{
					// add the active object to the list
					ieData->InsertObject(doc->GetActiveObject(), 0);

					// set parameter
					node->SetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::INCLUDE_EXCLUDE)), data, DESCFLAGS_SET::NONE);
				}
			}
			break;
		}

		// CUSTOMGUI_PRIORITY_DATA / CUSTOMGUI_PRIORITY
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::PRIORITY:
		{
			GeData priorityData = GeData(CUSTOMGUI_PRIORITY_DATA, DEFAULTVALUE);
			PriorityData *value = priorityData.GetCustomDataTypeWritable<PriorityData>();

			if (value != nullptr)
			{
				value->SetPriorityValue(PRIORITYVALUE_MODE, CYCLE_EXPRESSION);
				value->SetPriorityValue(PRIORITYVALUE_PRIORITY, 1);
			}

			// set parameter
			node->SetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::PRIORITY)), priorityData, DESCFLAGS_SET::NONE);
			break;
		}

		// CUSTOMDATATYPE_COLORPROFILE
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::COLORPROFILE:
		{
			GeData colorProfileData = GeData(CUSTOMDATATYPE_COLORPROFILE, DEFAULTVALUE);

			ColorProfile* value = colorProfileData.GetCustomDataTypeWritable<ColorProfile>();

			if (value != nullptr)
			{
				value->SetMonitorProfileMode(true);
			}

			// set parameter
			node->SetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::COLORPROFILE)), colorProfileData, DESCFLAGS_SET::NONE);
			break;
		}

		// DATETIME_DATA
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::DATETIME:
		{
			GeData dateTimeData = GeData(DATETIME_DATA, DEFAULTVALUE);

			DateTimeData* value = dateTimeData.GetCustomDataTypeWritable<DateTimeData>();

			if (value != nullptr)
			{
				DateTime time;
				GetDateTimeNow(time);

				value->SetDateTime(time);
			}

			// set parameter
			node->SetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::DATETIME)), dateTimeData, DESCFLAGS_SET::NONE);
			break;
		}
	}
}

void ObjectDynamicDescription::GetParameter(GeListNode* node)
{
	iferr_scope_handler
	{
		return;
	};

	if (node == nullptr)
		return;

	// get the document of this node
	BaseDocument* doc = node->GetDocument();

	if (doc == nullptr)
		return;

	// get the current selection
	GeData data;
	node->GetParameter(ConstDescIDLevel(ID_SELECT_DESCRIPTION), data, DESCFLAGS_GET::NONE);

	const Int32 selection = data.GetInt32();

	// print the value of the currently displayed dynamic parameter to the ID_RESULT_TEXT parameter
	switch (selection)
	{
		// DTYPE_LONG
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG:
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG_CYCLE:
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG_QUICKTAB:
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LONG_SLIDER:
		{
			const Bool success = node->GetParameter(CreateDescID((Int32)ID_DYNAMIC_ELEMENT + selection), data, DESCFLAGS_GET::NONE);

			// check type
			if (success && data.GetType() == DA_LONG)
			{
				const Int32 value = data.GetInt32();

				node->SetParameter(ConstDescIDLevel(ID_RESULT_TEXT), String::IntToString(value), DESCFLAGS_SET::NONE);
			}
			break;
		}

		// DTYPE_REAL
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::REAL:
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::REAL_SLIDER:
		{
			const Bool success = node->GetParameter(CreateDescID((Int32)ID_DYNAMIC_ELEMENT + selection), data, DESCFLAGS_GET::NONE);

			// check type
			if (success && data.GetType() == DA_REAL)
			{
				const Float value = data.GetFloat();

				node->SetParameter(ConstDescIDLevel(ID_RESULT_TEXT), String::FloatToString(value), DESCFLAGS_SET::NONE);
			}
			break;
		}

		// DTYPE_VECTOR / CUSTOMGUI_VECTOR
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::VECTOR:
		{
			const Bool success = node->GetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::VECTOR)), data, DESCFLAGS_GET::NONE);

			// check type
			if (success && data.GetType() == DA_VECTOR)
			{
				const Vector value = data.GetVector();

				node->SetParameter(ConstDescIDLevel(ID_RESULT_TEXT), String::VectorToString(value), DESCFLAGS_SET::NONE);
			}
			break;
		}

		// DTYPE_MATRIX
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::MATRIX:
		{
			const Bool success = node->GetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::MATRIX)), data, DESCFLAGS_GET::NONE);

			// check type
			if (success && data.GetType() == DA_MATRIX)
			{
				const Matrix value = data.GetMatrix();

				String result;

				result += String::VectorToString(value.sqmat.v1) + "\n";
				result += String::VectorToString(value.sqmat.v2) + "\n";
				result += String::VectorToString(value.sqmat.v3) + "\n";
				result += String::VectorToString(value.off) + "\n";

				node->SetParameter(ConstDescIDLevel(ID_RESULT_TEXT), result, DESCFLAGS_SET::NONE);
			}
			break;
		}

		// DTYPE_STRING
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::STRING:
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::STRING_MULTILINE:
		{
			const Bool success = node->GetParameter(CreateDescID((Int32)ID_DYNAMIC_ELEMENT + selection), data, DESCFLAGS_GET::NONE);

			// check type
			if (success && data.GetType() == DA_STRING)
			{
				const String result = data.GetString();
				node->SetParameter(ConstDescIDLevel(ID_RESULT_TEXT), result, DESCFLAGS_SET::NONE);
			}
			break;
		}

		// DTYPE_FILENAME
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::FILENAME:
		{
			const Bool success = node->GetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::FILENAME)), data, DESCFLAGS_GET::NONE);

			// check type
			if (success && data.GetType() == DA_FILENAME)
			{
				const Filename filename = data.GetFilename();
				const String result = filename.GetString();

				node->SetParameter(ConstDescIDLevel(ID_RESULT_TEXT), result, DESCFLAGS_SET::NONE);
			}
			break;
		}

		// DTYPE_BOOL
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::BOOL:
		{
			const Bool success = node->GetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::BOOL)), data, DESCFLAGS_GET::NONE);

			// check type
			if (success && data.GetType() == DA_LONG)
			{
				const Bool value = data.GetBool();
				String result;

				if (value)
					result = "Checked";
				else
					result = "Not Checked";

				node->SetParameter(ConstDescIDLevel(ID_RESULT_TEXT), result, DESCFLAGS_SET::NONE);
			}
			break;
		}

		// DTYPE_BASELISTLINK
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::LINK:
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::SHADERLINK:
		{
			const Bool success = node->GetParameter(CreateDescID((Int32)ID_DYNAMIC_ELEMENT + selection), data, DESCFLAGS_GET::NONE);

			// check type
			if (success && data.GetType() == DA_ALIASLINK)
			{
				BaseList2D* linkedEntity = data.GetLink(doc);

				if (linkedEntity != nullptr)
				{
					const String result = linkedEntity->GetName();

					node->SetParameter(ConstDescIDLevel(ID_RESULT_TEXT), result, DESCFLAGS_SET::NONE);
				}
			}
			break;
		}

		// DTYPE_TIME
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::TIME:
		{
			const Bool success = node->GetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::TIME)), data, DESCFLAGS_GET::NONE);

			// check type
			if (success && data.GetType() == DA_TIME)
			{
				const BaseTime time = data.GetTime();
				const Int32 frame = time.GetFrame(doc->GetFps());

				const String result = String::IntToString(frame);

				node->SetParameter(ConstDescIDLevel(ID_RESULT_TEXT), result, DESCFLAGS_SET::NONE);
			}
			break;
		}

		// DTYPE_COLOR
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::COLOR_PARAMETER:
		{
			const Bool success = node->GetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::COLOR_PARAMETER)), data, DESCFLAGS_GET::NONE);

			// check type
			if (success && data.GetType() == DA_VECTOR)
			{
				const Vector value = data.GetVector();

				node->SetParameter(ConstDescIDLevel(ID_RESULT_TEXT), String::VectorToString(value), DESCFLAGS_SET::NONE);
			}
			break;
		}

		// CUSTOMDATATYPE_GRADIENT / CUSTOMGUI_GRADIENT
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::GRADIENT:
		{
			const Bool success = node->GetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::GRADIENT)), data, DESCFLAGS_GET::NONE);

			// check type
			if (success && data.GetType() == CUSTOMDATATYPE_GRADIENT)
			{
				const Gradient* gradientData = data.GetCustomDataType<Gradient>();

				if (gradientData != nullptr)
				{
					String result;

					// init gradient
					InitRenderStruct irs(doc);
					maxon::GradientRenderData gradientRenderData = gradientData->PrepareRenderData(irs) iferr_return;

					for (Int32 i = 0; i < 10; i++)
					{
							const Vector color = Vector(gradientRenderData.CalcGradientPixel(Float(i) / 10.0));

							result += String::VectorToString(color) + "\n";
					}

					node->SetParameter(ConstDescIDLevel(ID_RESULT_TEXT), result, DESCFLAGS_SET::NONE);
				}
			}
			break;
		}

		// CUSTOMDATATYPE_SPLINE
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::SPLINE:
		{
			const Bool success = node->GetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::SPLINE)), data, DESCFLAGS_GET::NONE);

			// check type
			if (success && data.GetType() == CUSTOMDATATYPE_SPLINE)
			{
				const SplineData* splineData = data.GetCustomDataType<SplineData>();

				if (splineData != nullptr)
				{
					String result;

					// get range
					Float minX, maxX, dummy;
					splineData->GetRange(&minX, &maxX, &dummy, &dummy, &dummy, &dummy);

					const Float stepSize = (maxX - minX) / 10.0;

					// sample the curve
					for (Float i = minX; i <= maxX; i = i + stepSize)
					{
						const Vector point = splineData->GetPoint(i);

						result += String::FloatToString(point.y) + "\n";
					}

					node->SetParameter(ConstDescIDLevel(ID_RESULT_TEXT), result, DESCFLAGS_SET::NONE);
				}
			}
			break;
		}

		// CUSTOMDATATYPE_INEXCLUDE_LIST
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::INCLUDE_EXCLUDE:
		{
			const Bool success = node->GetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::INCLUDE_EXCLUDE)), data, DESCFLAGS_GET::NONE);

			// check type
			if (success && data.GetType() == CUSTOMDATATYPE_INEXCLUDE_LIST)
			{
				const InExcludeData* inexData = data.GetCustomDataType<InExcludeData>();

				if (inexData != nullptr)
				{
					String result;

					// loop through objects
					const Int32 count = inexData->GetObjectCount();

					for (Int32 i = 0; i < count; ++i)
					{
						BaseList2D* baseList = inexData->ObjectFromIndex(doc, i);

						// add object name to output
						if (baseList != nullptr)
							result += baseList->GetName()+"\n";
					}

					node->SetParameter(ConstDescIDLevel(ID_RESULT_TEXT), result, DESCFLAGS_SET::NONE);
				}
			}
			break;
		}

		// CUSTOMGUI_PRIORITY_DATA
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::PRIORITY:
		{
			const Bool success = node->GetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::PRIORITY)), data, DESCFLAGS_GET::NONE);

			// check type
			if (success && data.GetType() == CUSTOMGUI_PRIORITY_DATA)
			{
				const PriorityData* value = data.GetCustomDataType<PriorityData>();

				if (value != nullptr)
				{
					GeData priorityData = value->GetPriorityValue(PRIORITYVALUE_MODE);

					const String result = String::IntToString(priorityData.GetInt32());

					node->SetParameter(ConstDescIDLevel(ID_RESULT_TEXT), result, DESCFLAGS_SET::NONE);
				}
			}
			break;
		}

		// CUSTOMDATATYPE_COLORPROFILE
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::COLORPROFILE:
		{
			const Bool success = node->GetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::COLORPROFILE)), data, DESCFLAGS_GET::NONE);

			// check type
			if (success && data.GetType() == CUSTOMDATATYPE_COLORPROFILE)
			{
				const ColorProfile* value = data.GetCustomDataType<ColorProfile>();

				if (value != nullptr)
				{
					const String result = value->GetInfo(ColorProfile::COLORPROFILEINFO_DESCRIPTION);
					node->SetParameter(ConstDescIDLevel(ID_RESULT_TEXT), result, DESCFLAGS_SET::NONE);
				}
			}
			break;
		}

		// DATETIME_DATA
		case (Int32)DESCRIPTIONELEMENTSOBJECTDATA::DATETIME:
		{
			const Bool success = node->GetParameter(ConstDescID(DescLevel((Int32)ID_DYNAMIC_ELEMENT + (Int32)DESCRIPTIONELEMENTSOBJECTDATA::DATETIME)), data, DESCFLAGS_GET::NONE);

			// check type
			if (success && data.GetType() == DATETIME_DATA)
			{
				const DateTimeData* value = data.GetCustomDataType<DateTimeData>();

				if (value != nullptr)
				{
					DateTime time = value->GetDateTime();

					String result;
					result += String::IntToString(time.year) + ":";
					result += String::IntToString(time.month) + ":";
					result += String::IntToString(time.day) + " - ";
					result += String::IntToString(time.hour) + ":";
					result += String::IntToString(time.minute) + ":";
					result += String::IntToString(time.second);

					node->SetParameter(ConstDescIDLevel(ID_RESULT_TEXT), result, DESCFLAGS_SET::NONE);
				}
			}
			break;
		}
	}
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_SDK_OBJECTDATA_DESCRIPTION = 1035371;

Bool RegisterObjectDynamicDescription()
{
	return RegisterObjectPlugin(ID_SDK_OBJECTDATA_DESCRIPTION, GeLoadString(IDS_OBJECTDATA_DESCRIPTION), 0, ObjectDynamicDescription::Alloc, "Odescription"_s, nullptr, 0);
};
