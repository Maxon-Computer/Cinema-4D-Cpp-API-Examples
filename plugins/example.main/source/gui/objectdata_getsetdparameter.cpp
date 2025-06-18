// Example on how to handle Get-/SetDParameter for certain CustomGUIs
// This example has no use for the user.
// It simply creates CustomGUI descriptions dynamically and handles all subchannels in Get-/SetDParameter(),
// emulating the internal behavior of these CustomGUIs.

#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"
#include "customgui_texbox.h"

// NOTE: Be sure to use a unique ID obtained from developers.maxon.net!
#define ID_GETSETDPARAMETEREXAMPLE 1035580

#define ID_SPLINE   1000
#define ID_GRADIENT 1001
#define ID_LINK     1002

using namespace cinema;

static const Int32 SPLINE_SC_KNOT_POS_X = 0; // Float
static const Int32 SPLINE_SC_KNOT_POS_Y = 1; // Float
static const Int32 SPLINE_SC_KNOT_TANGENT_LEFT_X = 2; // Float
static const Int32 SPLINE_SC_KNOT_TANGENT_LEFT_Y = 3; // Float
static const Int32 SPLINE_SC_KNOT_TANGENT_RIGHT_X = 4; // Float
static const Int32 SPLINE_SC_KNOT_TANGENT_RIGHT_Y = 5; // Float
static const Int32 SPLINE_SC_KNOT_TANGENT_BREAK = 6; // Bool
static const Int32 SPLINE_SC_KNOT_INTERPOLATIOM = 7; // CustomSplineKnotInterpolation / Int32
static const Int32 SPLINE_SC_KNOT_LOCK_X = 8; // Bool
static const Int32 SPLINE_SC_KNOT_LOCK_Y = 9; // Bool
static const Int32 SPLINE_SC_KNOT_LOCK_TANGENT_ANGLE = 10; // Bool
static const Int32 SPLINE_SC_KNOT_LOCK_TANGENT_LENGTH = 11; // Bool

static const Int32 SPLINE_SC_TENSION = 1000; // Float
static const Int32 SPLINE_SC_KNOT_BASE = 10100; // if >=, decode subchannels

#define SPLINE_MAX_IDS_PER_KNOT 100
#define SPLINE_DECODE_KNOT_INDEX(id)       ((id - (SPLINE_SC_KNOT_BASE + SPLINE_SC_TENSION)) / SPLINE_MAX_IDS_PER_KNOT)
#define SPLINE_DECODE_KNOT_SUBCHANNEL(id)  (id % SPLINE_MAX_IDS_PER_KNOT)

static const Int32 GRADIENT_SC_KNOT_COLOR = 0; // Vector
static const Int32 GRADIENT_SC_KNOT_INTENSITY = 1; // Float
static const Int32 GRADIENT_SC_KNOT_POSITION = 2; //
static const Int32 GRADIENT_SC_KNOT_BIAS = 3; // Float
static const Int32 GRADIENT_SC_KNOT_INTERPOLATION = 4; // Float

// static const Int32 GRADIENT_SC_INTERPOLATION = 1000; // Int32
static const Int32 GRADIENT_SC_KNOT_BASE = 10000; // if >=, decode subchannels
static const Int32 GRADIENT_SC_ALPHAGRADIENT_OFFSET = 1000000000; // if id >=, it's a knot of an alpha gradient is addressed

#define GRADIENT_MAX_IDS_PER_KNOT 100
#define GRADIENT_DECODE_KNOT_INDEX(id)       ((id - GRADIENT_SC_KNOT_BASE) / GRADIENT_MAX_IDS_PER_KNOT)
#define GRADIENT_DECODE_KNOT_SUBCHANNEL(id)  (id % GRADIENT_MAX_IDS_PER_KNOT)

class GetSetDParameterExample : public ObjectData
{
	INSTANCEOF(GetSetDParameterExample, ObjectData)

public:
	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual Bool GetDDescription(const GeListNode* node, Description* description, DESCFLAGS_DESC& flags) const;
	virtual Bool GetDParameter(const GeListNode* node, const DescID& id, GeData& t_data, DESCFLAGS_GET& flags) const;
	virtual Bool SetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags);
	virtual BaseObject* GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh);

	static NodeData* Alloc() { return NewObjClear(GetSetDParameterExample); }

private:
	void SplineInit(BaseContainer* const data);
	Bool SplineGetDDescription(const GeListNode *node, Description *description, DESCFLAGS_DESC &flags, const DescID* const singleid) const;
	Bool SplineGetDParameter(const GeListNode* node, const DescID& id, GeData& t_data, DESCFLAGS_GET& flags, const BaseContainer* data) const;
	Bool SplineSetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags, BaseContainer* const data);

	void GradientInit(BaseContainer* const data);
	Bool GradientGetDDescription(const GeListNode *node, Description *description, DESCFLAGS_DESC &flags, const DescID* const singleid) const;
	Bool GradientGetDParameter(const GeListNode* node, const DescID& id, GeData& t_data, DESCFLAGS_GET& flags, const BaseContainer* data) const;
	Bool GradientSetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags, BaseContainer* const data);
	Bool GradientFindKnot(const Gradient* gradient, Int32 knotIdx, maxon::GradientKnot& knot, Int32& idx) const;

	void LinkInit(BaseContainer* const data);
	Bool LinkGetDDescription(const GeListNode *node, Description *description, DESCFLAGS_DESC &flags, const DescID* const singleid) const;
	Bool LinkGetDParameter(const GeListNode* node, const DescID& id, GeData& t_data, DESCFLAGS_GET& flags, const BaseContainer* data) const;
	Bool LinkSetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags, BaseContainer* const data);
	void LinkGetVirtualObjectsTest(BaseObject* const op);
};


//
// Spline
//
void GetSetDParameterExample::SplineInit(BaseContainer* const data)
{
	// Initialize spline description
	GeData spline(CUSTOMDATATYPE_SPLINE, DEFAULTVALUE);
	SplineData* const spd = spline.GetCustomDataTypeWritable<SplineData>();

	if (spd)
	{
		spd->MakeLinearSplineBezier(2);
		CustomSplineKnot* knot = spd->GetKnot(0);
		// move points, so spline fits optimal area configured in GetDDescription()
		knot->vPos = Vector(0.2, 0.2, 0.0);
		knot = spd->GetKnot(1);
		knot->vPos = Vector(0.8, 0.8, 0.0);
	}
	data->SetData(ID_SPLINE, spline);
}

Bool GetSetDParameterExample::SplineGetDDescription(const GeListNode *node, Description *description, DESCFLAGS_DESC &flags, const DescID* const singleid) const
{
	// Create a spline description
	const DescID cid = ConstDescID(DescLevel(ID_SPLINE, CUSTOMDATATYPE_SPLINE, 0));

	if (!singleid || cid.IsPartOf(*singleid, nullptr)) // important to check for speedup c4d!
	{
		BaseContainer bcSpline = GetCustomDataTypeDefault(CUSTOMDATATYPE_SPLINE);

		bcSpline.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_SPLINE); // Actually this is already set by GetCustomDataTypeDefault()
		bcSpline.SetString(DESC_SHORT_NAME, "Spline"_s); // This name is shown in the GUI
		bcSpline.SetString(DESC_NAME, "Spline"_s); // This name is visible in Xpresso
		bcSpline.SetInt32(DESC_ANIMATE, DESC_ANIMATE_ON); // Default is DESC_ANIMATE_ON
		// Spline GUI configuration
		// Set minimum size of spline GUI
		bcSpline.SetInt32(SPLINECONTROL_MINSIZE_H, 100); // Default is 120
		bcSpline.SetInt32(SPLINECONTROL_MINSIZE_V, 150); // Default is 160
		// Spline GUI area may be defined as square
		bcSpline.SetBool(SPLINECONTROL_SQUARE, false); // Default is false
		// Gridline painting
		bcSpline.SetBool(SPLINECONTROL_GRID_H, true); // Default is true
		bcSpline.SetBool(SPLINECONTROL_GRID_V, false); // Default is true
		// Axis labels, which will be shown, if GUI is extended (they are also shown on edit fields)
		bcSpline.SetString(SPLINECONTROL_X_TEXT, "A"_s); // Default is "X"
		bcSpline.SetString(SPLINECONTROL_Y_TEXT, "B"_s); // Default is "Y"
		// Edit fields for x and y values, true is default, if false respective component will be locked, default is true
		bcSpline.SetBool(SPLINECONTROL_VALUE_EDIT_H, true); // Default is true
		bcSpline.SetBool(SPLINECONTROL_VALUE_EDIT_V, true); // Default is true
		// Define increments of arrows on edit fields in extended GUI
		bcSpline.SetFloat(SPLINECONTROL_X_STEPS, 0.05); // Default is 0.1
		bcSpline.SetFloat(SPLINECONTROL_Y_STEPS, 0.1); // Default is 0.1
		// Allow scaling of the spline area
		bcSpline.SetBool(SPLINECONTROL_ALLOW_HORIZ_SCALE_MOVE, true); // Default is true
		bcSpline.SetBool(SPLINECONTROL_ALLOW_VERT_SCALE_MOVE, true); // Default is true
		// Hide "Show in separate window..." in popup menu, button in extended GUI is not influenced and will stay visible
		bcSpline.SetBool(SPLINECONTROL_NO_FLOATING_WINDOW, true); // Default is false
		// Hide the buttons to load and save presets in extended GUI
		bcSpline.SetBool(SPLINECONTROL_NO_PRESETS, true); // Default is false
		// Minimum and maximum values for spline points, on start area will be zoomed in on this range
		bcSpline.SetFloat(SPLINECONTROL_X_MIN, 0.1); // Default is 0.0
		bcSpline.SetFloat(SPLINECONTROL_Y_MIN, 0.1); // Default is 0.0
		bcSpline.SetFloat(SPLINECONTROL_X_MAX, 0.9); // Default is 1.0
		bcSpline.SetFloat(SPLINECONTROL_Y_MAX, 0.9); // Default is 1.0
		// Optimal area, shows a highlighted rectangle in background, indicating a zone for "optimal" values, area will be zoomed in to this zone on start
		bcSpline.SetBool(SPLINECONTROL_OPTIMAL, true); // Default is false
		bcSpline.SetFloat(SPLINECONTROL_OPTIMAL_X_MIN, 0.2);
		bcSpline.SetFloat(SPLINECONTROL_OPTIMAL_Y_MIN, 0.2);
		bcSpline.SetFloat(SPLINECONTROL_OPTIMAL_X_MAX, 0.8);
		bcSpline.SetFloat(SPLINECONTROL_OPTIMAL_Y_MAX, 0.8);
		// Custom color for spline
		bcSpline.SetBool(SPLINECONTROL_CUSTOMCOLOR_SET, true); // Default is false
		bcSpline.SetVector(SPLINECONTROL_CUSTOMCOLOR_COL, Vector(0.6, 1.0, 0.6)); // Value range 0.0 to 1.0

		if (!description->SetParameter(cid, bcSpline, ConstDescIDLevel(ID_OBJECTPROPERTIES)))
			return false;
	}
	return true;
}

Bool GetSetDParameterExample::SplineGetDParameter(const GeListNode* node, const DescID& id, GeData& t_data, DESCFLAGS_GET& flags, const BaseContainer* data) const
{
	if (id[1].id == 0)  // if second level id is zero, the complete custom datatype is requested
	{
		const GeData d = data->GetData(id[0].id);
		const SplineData* const spd = d.GetCustomDataType<SplineData>();

		t_data.SetCustomDataType(*spd);
		flags |= DESCFLAGS_GET::PARAM_GET;
	}
	else if (id[1].id == SPLINE_SC_TENSION)
	{
		// Can not be read directly, so pass this request on to parent
		Bool result = SUPER::GetDParameter(node, id, t_data, flags);

		// Then you could do something with read t_data, here...
		return result;
	}
	else if (id[1].id > SPLINE_SC_KNOT_BASE)
	{
		// First decode ID to get the correct knot and subchannel
		const Int32 knotIdx = SPLINE_DECODE_KNOT_INDEX(id[1].id);
		const Int32 subchannelIdx = SPLINE_DECODE_KNOT_SUBCHANNEL(id[1].id);

		// Now, get the SplineData, in order to access it directly.
		// As mentioned before, there's no real need to do so, but you could...
		const GeData d = data->GetData(id[0].id);
		const SplineData* const spd = d.GetCustomDataType<SplineData>();

		DebugAssert((knotIdx >= 0) && (knotIdx < spd->GetKnotCount()), "WRONG KNOT INDEX");

		const CustomSplineKnot* knot = spd->GetKnot(knotIdx);
		if (knot)
		{
			switch (subchannelIdx)
			{
				case SPLINE_SC_KNOT_POS_X:
					t_data.SetFloat(knot->vPos.x);
					break;
				case SPLINE_SC_KNOT_POS_Y:
					t_data.SetFloat(knot->vPos.y);
					break;
				case SPLINE_SC_KNOT_TANGENT_LEFT_X:
					t_data.SetFloat(knot->vTangentLeft.x);
					break;
				case SPLINE_SC_KNOT_TANGENT_LEFT_Y:
					t_data.SetFloat(knot->vTangentLeft.y);
					break;
				case SPLINE_SC_KNOT_TANGENT_RIGHT_X:
					t_data.SetFloat(knot->vTangentRight.x);
					break;
				case SPLINE_SC_KNOT_TANGENT_RIGHT_Y:
					t_data.SetFloat(knot->vTangentRight.y);
					break;
				case SPLINE_SC_KNOT_TANGENT_BREAK:
					t_data.SetInt32(knot->lFlagsSettings & FLAG_KNOT_T_BREAK);
					break;
				case SPLINE_SC_KNOT_INTERPOLATIOM:
					t_data.SetInt32(knot->interpol);
					break;
				case SPLINE_SC_KNOT_LOCK_X:
					t_data.SetInt32(knot->lFlagsSettings & FLAG_KNOT_LOCK_X);
					break;
				case SPLINE_SC_KNOT_LOCK_Y:
					t_data.SetInt32(knot->lFlagsSettings & FLAG_KNOT_LOCK_Y);
					break;
				case SPLINE_SC_KNOT_LOCK_TANGENT_ANGLE:
					t_data.SetInt32(knot->lFlagsSettings & FLAG_KNOT_T_LOCK_A);
					break;
				case SPLINE_SC_KNOT_LOCK_TANGENT_LENGTH:
					t_data.SetInt32(knot->lFlagsSettings & FLAG_KNOT_T_LOCK_L);
					break;
			}
		}
		flags |= DESCFLAGS_GET::PARAM_GET;
	}
	return SUPER::GetDParameter(node, id, t_data, flags);
}

Bool GetSetDParameterExample::SplineSetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags, BaseContainer* const data)
{
	if (id[1].id == 0)
	{
		// Updated spline data
		const SplineData* const spd = t_data.GetCustomDataType<SplineData>();
		GeData d;

		d.SetCustomDataType(*spd);
		data->SetData(id[0].id, d);  // store in container
		flags |= DESCFLAGS_SET::PARAM_SET;
	}
	else if (id[1].id == SPLINE_SC_TENSION)
	{
		// Can not be set directly.
		// Do something with new spline tension in t_data...
		// Then pass this parameter on to parent, to get it correctly set in spline description
		return SUPER::SetDParameter(node, id, t_data, flags);
	}
	else if (id[1].id > SPLINE_SC_KNOT_BASE)
	{
		// First decode ID to get the correct knot and subchannel
		const Int32 knotIdx = SPLINE_DECODE_KNOT_INDEX(id[1].id);
		const Int32 subchannelIdx = SPLINE_DECODE_KNOT_SUBCHANNEL(id[1].id);

		// Now, get the SplineData, in order to access it directly.
		// As mentioned before, there's no real need to do so, but you could...
		GeData d = data->GetData(id[0].id);
		SplineData* const spd = d.GetCustomDataTypeWritable<SplineData>();

		DebugAssert((knotIdx >= 0) && (knotIdx < spd->GetKnotCount()), "WRONG KNOT INDEX");

		CustomSplineKnot* knot = spd->GetKnot(knotIdx);

		switch (subchannelIdx)
		{
			case SPLINE_SC_KNOT_POS_X:
				knot->vPos.x = t_data.GetFloat();
				break;
			case SPLINE_SC_KNOT_POS_Y:
				knot->vPos.y = t_data.GetFloat();
				break;
			case SPLINE_SC_KNOT_TANGENT_LEFT_X:
				knot->vTangentLeft.x = t_data.GetFloat();
				break;
			case SPLINE_SC_KNOT_TANGENT_LEFT_Y:
				knot->vTangentLeft.y = t_data.GetFloat();
				break;
			case SPLINE_SC_KNOT_TANGENT_RIGHT_X:
				knot->vTangentRight.x = t_data.GetFloat();
				break;
			case SPLINE_SC_KNOT_TANGENT_RIGHT_Y:
				knot->vTangentRight.y = t_data.GetFloat();
				break;
			case SPLINE_SC_KNOT_TANGENT_BREAK:
				if (t_data.GetBool())
					knot->lFlagsSettings |= FLAG_KNOT_T_BREAK;
				else
					knot->lFlagsSettings &= ~FLAG_KNOT_T_BREAK;
				break;
			case SPLINE_SC_KNOT_INTERPOLATIOM:
				knot->interpol = (CustomSplineKnotInterpolation)t_data.GetInt32();
				break;
			case SPLINE_SC_KNOT_LOCK_X:
				if (t_data.GetBool())
					knot->lFlagsSettings |= FLAG_KNOT_LOCK_X;
				else
					knot->lFlagsSettings &= ~FLAG_KNOT_LOCK_X;
				break;
			case SPLINE_SC_KNOT_LOCK_Y:
				if (t_data.GetBool())
					knot->lFlagsSettings |= FLAG_KNOT_LOCK_Y;
				else
					knot->lFlagsSettings &= ~FLAG_KNOT_LOCK_Y;
				break;
			case SPLINE_SC_KNOT_LOCK_TANGENT_ANGLE:
				if (t_data.GetBool())
					knot->lFlagsSettings |= FLAG_KNOT_T_LOCK_A;
				else
					knot->lFlagsSettings &= ~FLAG_KNOT_T_LOCK_A;
				break;
			case SPLINE_SC_KNOT_LOCK_TANGENT_LENGTH:
				if (t_data.GetBool())
					knot->lFlagsSettings |= FLAG_KNOT_T_LOCK_L;
				else
					knot->lFlagsSettings &= ~FLAG_KNOT_T_LOCK_L;
				break;
		}
		data->SetData(id[0].id, d);  // store changed spline in container
		flags |= DESCFLAGS_SET::PARAM_SET;
	}
	return SUPER::SetDParameter(node, id, t_data, flags);
}

//
// Gradient
//
void GetSetDParameterExample::GradientInit(BaseContainer* const data)
{
	// Initialize color gradient description with a simple black/white gradient
	GeData gradientData(CUSTOMDATATYPE_GRADIENT, DEFAULTVALUE);
	Gradient* gradient = gradientData.GetCustomDataTypeWritable<Gradient>();

	if (gradient)
	{
		maxon::GradientKnot gradientKnot;

		gradientKnot.col = maxon::Color(0.0);
		gradientKnot.pos = 0.0;
		gradientKnot.index = 0;
		gradient->InsertKnot(gradientKnot);
		gradientKnot.col = maxon::Color(1.0);
		gradientKnot.pos = 1.0;
		gradientKnot.index = 1;
		gradient->InsertKnot(gradientKnot);
	}
	data->SetData(ID_GRADIENT, gradientData);
}

Bool GetSetDParameterExample::GradientGetDDescription(const GeListNode *node, Description *description, DESCFLAGS_DESC &flags, const DescID* const singleid) const
{
	// Create a color gradient description
	const DescID cid = ConstDescID(DescLevel(ID_GRADIENT, CUSTOMDATATYPE_GRADIENT, 0));

	if (!singleid || cid.IsPartOf(*singleid, nullptr)) // important to check for speedup c4d!
	{
		BaseContainer bcGradient = GetCustomDataTypeDefault(CUSTOMDATATYPE_GRADIENT);

		bcGradient.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_GRADIENT);
		bcGradient.SetString(DESC_SHORT_NAME, "Gradient"_s); // This name is shown in the GUI
		bcGradient.SetString(DESC_NAME, "Gradient"_s); // This name is visible in Xpresso
		bcGradient.SetInt32(DESC_ANIMATE, DESC_ANIMATE_ON);
		// Gradient GUI configuration
		// Set gradient mode, defaults to GRADIENTMODE_COLOR if both are false, GRADIENTMODE_COLORALPHA if both are true
		// GRADIENTMODE_ALPHA implies GRADIENTPROPERTY_NOEDITCOLOR true
		// Here GRADIENTMODE_COLORALPHA is used
		bcGradient.SetBool(GRADIENTPROPERTY_ALPHA_WITH_COLOR, true); // Default is false
		bcGradient.SetBool(GRADIENTPROPERTY_ALPHA, true); // Default is false
		// Hide color sliders in extended GUI (color of knots can still be edited via double click)
		bcGradient.SetBool(GRADIENTPROPERTY_NOEDITCOLOR, false); // Default is false
		// Hide the buttons to load and save presets in extended GUI
		bcGradient.SetBool(GRADIENTPROPERTY_NOPRESETS, true); // Default is false

		if (!description->SetParameter(cid, bcGradient, ConstDescIDLevel(ID_OBJECTPROPERTIES)))
			return false;
	}
	return true;
}

Bool GetSetDParameterExample::GradientGetDParameter(const GeListNode* node, const DescID& id, GeData& t_data, DESCFLAGS_GET& flags, const BaseContainer* data) const
{
	// Get the Gradient, in order to access it directly.
	// As mentioned before, there's no real need to do so, but you could...
	GeData d = data->GetData(id[0].id);
	const Gradient* gradient = d.GetCustomDataType<Gradient>();

	if (id[1].id == 0)  // if second level id is zero, the complete custom datatype is requested
	{
		t_data.SetCustomDataType(*gradient);
		flags |= DESCFLAGS_GET::PARAM_GET;
	}
	else if (id[1].id >= GRADIENT_SC_KNOT_BASE)
	{
		// First check, if a knot of an alpha gradient is addressed
		Int32 idDecoded = id[1].id;

		if (idDecoded >= GRADIENT_SC_ALPHAGRADIENT_OFFSET)
		{
			idDecoded -= GRADIENT_SC_ALPHAGRADIENT_OFFSET;
			gradient = gradient->GetAlphaGradient();
			if (!gradient)
				return SUPER::GetDParameter(node, id, t_data, flags);
		}

		// Then decode ID to get the correct knot and subchannel
		const Int32 knotIdx = GRADIENT_DECODE_KNOT_INDEX(idDecoded);
		const Int32 subchannelIdx = GRADIENT_DECODE_KNOT_SUBCHANNEL(idDecoded);

		// Now, find the addressed knot
		// Note:
		// Gradient is different than spline data, as the decoded knotIdx is not the index
		// to directly access the knot. Instead on every knot an index is stored.
		maxon::GradientKnot knot;
		Int32 idx;

		if (!GradientFindKnot(gradient, knotIdx, knot, idx))
			return SUPER::GetDParameter(node, id, t_data, flags);

		switch (subchannelIdx)
		{
			case GRADIENT_SC_KNOT_COLOR:
				switch (id[2].id) // this switch does the same as HandleDescGetVector, just one description level deeper
				{
					case 0:
						t_data.SetVector(Vector(knot.col));
						break;
					case 1000:
						t_data.SetFloat(knot.col.r);
						break;
					case 1001:
						t_data.SetFloat(knot.col.g);
						break;
					case 1002:
						t_data.SetFloat(knot.col.b);
						break;
				}
				break;
			case GRADIENT_SC_KNOT_INTENSITY:
				t_data.SetFloat(knot.brightness);
				break;
			case GRADIENT_SC_KNOT_POSITION:
				t_data.SetFloat(knot.pos);
				break;
			case GRADIENT_SC_KNOT_BIAS:
				t_data.SetFloat(knot.bias);
				break;
			case GRADIENT_SC_KNOT_INTERPOLATION:
				t_data.SetInt32(Int32(knot.interpolation));
				break;
		}
		flags |= DESCFLAGS_GET::PARAM_GET;
	}
	return SUPER::GetDParameter(node, id, t_data, flags);
}

Bool GetSetDParameterExample::GradientSetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags, BaseContainer* const data)
{
	if (id[1].id == 0)
	{
		// Updated gradient data
		const Gradient* const gradient = t_data.GetCustomDataType<Gradient>();
		GeData d;

		d.SetCustomDataType(*gradient);
		data->SetData(id[0].id, d);  // store in container
		flags |= DESCFLAGS_SET::PARAM_SET;
	}
	else if (id[1].id >= GRADIENT_SC_KNOT_BASE)
	{
		// Get the Gradient, in order to access it directly.
		// As mentioned before, there's no real need to do so, but you could...
		GeData d = data->GetData(id[0].id);
		Gradient* gradient = d.GetCustomDataTypeWritable<Gradient>();

		// First check, if a knot of an alpha gradient is addressed
		Int32 idDecoded = id[1].id;

		if (idDecoded >= GRADIENT_SC_ALPHAGRADIENT_OFFSET)
		{
			idDecoded -= GRADIENT_SC_ALPHAGRADIENT_OFFSET;
			gradient = gradient->GetAlphaGradient();
			if (!gradient)
				return SUPER::SetDParameter(node, id, t_data, flags);
		}

		// Then decode ID to get the correct knot and subchannel
		const Int32 knotIdx = GRADIENT_DECODE_KNOT_INDEX(idDecoded);
		const Int32 subchannelIdx = GRADIENT_DECODE_KNOT_SUBCHANNEL(idDecoded);

		// Now, find the addressed knot
		// Note:
		// Gradient is different than spline data, as the decoded knotIdx is not the index
		// to directly access the knot. Instead on every knot an index is stored.
		maxon::GradientKnot knot;
		Int32 idx;

		if (!GradientFindKnot(gradient, knotIdx, knot, idx))
			return SUPER::SetDParameter(node, id, t_data, flags);

		switch (subchannelIdx)
		{
			case GRADIENT_SC_KNOT_COLOR:
				switch (id[2].id) // this switch does the same as HandleDescSetVector, just one description level deeper
				{
					case 0:
						knot.col = maxon::Color(t_data.GetVector());
						break;
					case 1000:
						knot.col = maxon::Color(t_data.GetFloat(), knot.col.g, knot.col.b);
						break;
					case 1001:
						knot.col = maxon::Color(knot.col.r, t_data.GetFloat(), knot.col.b);
						break;
					case 1002:
						knot.col = maxon::Color(knot.col.r, knot.col.g, t_data.GetFloat());
						break;
				}
				break;
			case GRADIENT_SC_KNOT_INTENSITY:
				knot.brightness = t_data.GetFloat();
				break;
			case GRADIENT_SC_KNOT_POSITION:
				knot.pos = t_data.GetFloat();
				break;
			case GRADIENT_SC_KNOT_BIAS:
				knot.bias = t_data.GetFloat();
				break;
			case GRADIENT_SC_KNOT_INTERPOLATION:
				knot.interpolation = maxon::GRADIENT_INTERPOLATION_TYPE(t_data.GetInt32());
				break;
				
		}
		gradient->SetKnot(idx, knot);
		data->SetData(id[0].id, d);  // store changed gradient in container
		flags |= DESCFLAGS_SET::PARAM_SET;
	}
	return SUPER::SetDParameter(node, id, t_data, flags);
}

// GradientFindKnot returns true, if knot is found. knot and idx are set in this case. idx is to be used with GetKnot().
Bool GetSetDParameterExample::GradientFindKnot(const Gradient* gradient, Int32 knotIdx, maxon::GradientKnot& knot, Int32& idx) const
{
	const Int32 knotCount = gradient->GetKnotCount();

	for (idx = 0; idx < knotCount; ++idx)
	{
		knot = gradient->GetKnot(idx); // this idx is different from knotIdx!!!
		if (knot.index == knotIdx)
			break;
	}
	if (idx == knotCount)
		return false; // Knot not found. This can happen for example, if the user animated the gradient and then removes a knot
	return true;
}

//
// Link
//
void GetSetDParameterExample::LinkInit(BaseContainer* const data)
{
	// Initialize link
	data->SetLink(ID_LINK, nullptr);
}

Bool GetSetDParameterExample::LinkGetDDescription(const GeListNode *node, Description *description, DESCFLAGS_DESC &flags, const DescID* const singleid) const
{
	// Create a link description
	const DescID cid = ConstDescID(DescLevel(ID_LINK, DTYPE_BASELISTLINK, 0));

	if (!singleid || cid.IsPartOf(*singleid, nullptr))// important to check for speedup c4d!
	{
		BaseContainer bcLink = GetCustomDataTypeDefault(DTYPE_BASELISTLINK);

		bcLink.SetString(DESC_SHORT_NAME, "Link"_s);
		bcLink.SetString(DESC_NAME, "Link"_s);
		bcLink.SetInt32(DESC_ANIMATE, DESC_ANIMATE_ON);
		bcLink.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_TEXBOX);
		bcLink.SetInt32(DESC_SHADERLINKFLAG, true);

		if (!description->SetParameter(cid, bcLink, ConstDescIDLevel(ID_OBJECTPROPERTIES)))
			return false;
	}
	return true;
}

Bool GetSetDParameterExample::LinkGetDParameter(const GeListNode* node, const DescID& id, GeData& t_data, DESCFLAGS_GET& flags, const BaseContainer* data) const
{
	const GeData d = data->GetData(id[0].id);

	if (d.GetType() == DA_NIL) // If the link does not get properly initialized, you need to take care for this situation
	{
		AutoAlloc<BaseLink> bl;
		
		// check the BaseLink returned pointer
		if (!bl)
		{
			flags |= DESCFLAGS_GET::PARAM_GET;
			return SUPER::GetDParameter(node, id, t_data, flags);
		}
		
		bl->SetLink(nullptr);
		t_data.SetBaseLink(bl);
	}
	else
	{
		const BaseLink* bl = d.GetBaseLink();
		
		// check the BaseLink returned pointer
		if (!bl)
		{
			flags |= DESCFLAGS_GET::PARAM_GET;
			return SUPER::GetDParameter(node, id, t_data, flags);
		}
		
		t_data.SetBaseLink(*bl);
	}
	flags |= DESCFLAGS_GET::PARAM_GET;
	return SUPER::GetDParameter(node, id, t_data, flags);
}

Bool GetSetDParameterExample::LinkSetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags, BaseContainer* const data)
{
	const BaseLink* bl = t_data.GetBaseLink();
	GeData d;

	d.SetBaseLink(*bl);
	data->SetData(id[0].id, d);  // store in container
	flags |= DESCFLAGS_SET::PARAM_SET;
	return SUPER::SetDParameter(node, id, t_data, flags);
}

void GetSetDParameterExample::LinkGetVirtualObjectsTest(BaseObject* const op)
{
	BaseContainer* bc = op->GetDataInstance();

	// For demonstration purposes, the link is accessed in two different ways:

	// a) Using the BaseLink
	const BaseLink* bl = bc->GetBaseLink(ID_LINK);

	if (bl)
	{
		BaseList2D* bl2d = bl->GetLink(op->GetDocument());

		if (bl2d)
			ApplicationOutput("Via GetBaseLink(): " + bl2d->GetName());
		else
			ApplicationOutput("Via GetBaseLink(): No Link"_s);
	}

	// b) Using directly GetLink()
	BaseList2D* bl2d = bc->GetLink(ID_LINK, op->GetDocument());

	if (bl2d)
		ApplicationOutput("Via GetLink(): " + bl2d->GetName());
	else
		ApplicationOutput("Via GetLink(): No Link"_s);
}

//
// ObjectData function overrides
//
Bool GetSetDParameterExample::Init(GeListNode* node, Bool isCloneInit)
{
	BaseContainer* const data = static_cast<BaseMaterial*>(node)->GetDataInstance();
	if (!isCloneInit)
	{
		SplineInit(data);
		GradientInit(data);
		LinkInit(data);
	}
	return true;
}

Bool GetSetDParameterExample::GetDDescription(const GeListNode *node, Description *description, DESCFLAGS_DESC &flags) const
{
	if (!description->LoadDescription(node->GetType()))
		return false;

	const DescID* const singleid = description->GetSingleDescID();

	if (!SplineGetDDescription(node, description, flags, singleid))
		return false;

	if (!GradientGetDDescription(node, description, flags, singleid))
		return false;

	if (!LinkGetDDescription(node, description, flags, singleid))
		return false;

	flags |= DESCFLAGS_DESC::LOADED;
	return true;
}

Bool GetSetDParameterExample::GetDParameter(const GeListNode* node, const DescID& id, GeData& t_data, DESCFLAGS_GET& flags) const
{
	const BaseContainer* const data = static_cast<const BaseObject*>(node)->GetDataInstance();

	// This implementation is rather useless and is for demonstration purposes, only.
	// Instead of reading the parameters manually, you could always call
	//   SUPER::GetDParameter() without setting DESCFLAGS_GET::PARAM_GET.
	switch (id[0].id)
	{
		case ID_SPLINE:
		{
			return SplineGetDParameter(node, id, t_data, flags, data);
			break;
		}
		case ID_GRADIENT:
		{
			return GradientGetDParameter(node, id, t_data, flags, data);
			break;
		}
		case ID_LINK:
		{
			return LinkGetDParameter(node, id, t_data, flags, data);
			break;
		}
		default:
			CriticalOutput("Unknown description ID (@)!", id[0].id);
			break;
	}
	return SUPER::GetDParameter(node, id, t_data, flags);
}

Bool GetSetDParameterExample::SetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags)
{
	BaseContainer* const data = static_cast<BaseObject*>(node)->GetDataInstance();

	// This implementation is rather useless and is for demonstration purposes, only.
	// Instead of setting the parameters manually, you could always call
	//   SUPER::SetDParameter() without setting DESCFLAGS_SET::PARAM_SET and would be done.
	// In a real world plugin, you probably would want to set some internal variables here.
	switch (id[0].id)
	{
		case ID_SPLINE:
		{
			return SplineSetDParameter(node, id, t_data, flags, data);
			break;
		}
		case ID_GRADIENT:
		{
			return GradientSetDParameter(node, id, t_data, flags, data);
			break;
		}
		case ID_LINK:
		{
			return LinkSetDParameter(node, id, t_data, flags, data);
			break;
		}
		default:
			CriticalOutput("Unknown description ID (@)!", id[0].id);
			break;
	}
	return SUPER::SetDParameter(node, id, t_data, flags);
}

BaseObject* GetSetDParameterExample::GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh)
{
	LinkGetVirtualObjectsTest(op);
	// Just return a Null object, the generated object is useless in this example
	return BaseObject::Alloc(Onull);
}

Bool RegisterGetSetDParameterExample()
{
	return RegisterObjectPlugin(ID_GETSETDPARAMETEREXAMPLE,
															GeLoadString(IDS_OBJECTDATA_GETSETDPARAMETEREXAMPLE),
															OBJECT_GENERATOR,
															GetSetDParameterExample::Alloc,
															"Ogetsetdparameterexample"_s,
															nullptr,
															0);
}
