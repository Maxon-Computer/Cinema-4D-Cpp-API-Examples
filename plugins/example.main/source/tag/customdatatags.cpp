#include "customdatatags.h"
#include "c4d_basedocument.h"
#include "c4d_basedraw.h"
#include "c4d_commanddata.h"
#include "c4d_general.h"
#include "c4d_resource.h"
#include "c4d_tooldata.h"
#include "customgui_gradient.h"

using namespace cinema;

namespace maxon
{

MAXON_DATATYPE_REGISTER(VERTEXCOLOR_MESHATTRIBUTE);
MAXON_DATATYPE_REGISTER(FLOATTYPE_MESHATTRIBUTE);

class VertexColorDisplayImpl : public Component<VertexColorDisplayImpl, CustomDataTagDisplayInterface>
{
	MAXON_COMPONENT();

public:

	MAXON_METHOD Result<void> Init(BaseTag* tag)
	{
		return OK;
	}

	MAXON_METHOD Result<void> Draw(BaseTag* tag, BaseObject* op, BaseDraw* bd, BaseDrawHelp* bh)
	{
		if (!tag || !op || !bd || !bh)
			return IllegalArgumentError(MAXON_SOURCE_LOCATION);

		bd->SetMatrix_Screen();
		bd->SetPen(Vector(1.0, 0.0, 0.0), SET_PEN_USE_PROFILE_COLOR);
		bd->DrawLine(Vector(100.0, 100.0, 0.0), Vector(200.0, 200.0, 0.0), 0);
		bd->SetMatrix_Matrix(nullptr, Matrix());

		return OK;
	}

	MAXON_METHOD Result<void> DisplayControl(const BaseTag* tag, BaseDocument* doc, BaseObject* op, BaseDraw* bd, BaseDrawHelp* bh, ControlDisplayStruct& cds) const
	{
		iferr_scope;

		if (!tag || !bd || !bh || !op || !op->IsInstanceOf(Opoint))
			return OK;

		const CustomDataTag* customtag = static_cast<const CustomDataTag*>(tag);

		CUSTOMDATATAG_MODE mode = customtag->GetMode();
		cds.colorMode = mode == CUSTOMDATATAG_MODE::POLYVERTEX ? DISPLAYCONTROL_MODE::PER_POLYGON_PER_VERTEX : DISPLAYCONTROL_MODE::PER_VERTEX;
		switch (mode)
		{
			case CUSTOMDATATAG_MODE::VERTEX:
			{
				Int32 pointCount = ToPoint(op)->GetPointCount();
				maxon::Color32* vertexColor = NewMem(maxon::Color32, pointCount) iferr_return;
				cds.vertexColor = vertexColor;
				for (Int32 vertexIndex = 0; vertexIndex < pointCount; ++vertexIndex)
				{
					const ColorA32 col = customtag->GetVertexData<ColorA32>(vertexIndex);
					vertexColor[vertexIndex] = col.GetColor3();
				}
				break;
			}
			case CUSTOMDATATAG_MODE::POLYVERTEX:
			{
				Int32 polygonCount = ToPoly(op)->GetPolygonCount();
				maxon::Color32* vertexColor = NewMem(maxon::Color32, polygonCount * 4) iferr_return;
				cds.vertexColor = vertexColor;
				for (Int32 polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex)
				{
					for (Int32 i = 0; i < 4; ++i)
					{
						const ColorA32& color = customtag->GetPolyVertexData<ColorA32>(polygonIndex, i);
						vertexColor[4 * polygonIndex + i] = color.GetColor3();
					}
				}
				break;
			}
			default:
				break;
		}

		return OK;
	}

	MAXON_METHOD Result<void> InitDisplayControl(BaseTag* tag, BaseDocument* doc, BaseDraw* bd, const AtomArray* active)
	{
		return OK;
	}

	MAXON_METHOD void FreeDisplayControl(BaseTag* tag)
	{
	}
	
	MAXON_METHOD void Reset()
	{
		
	}

};

class VertexColorImpl : public Component<VertexColorImpl, CustomDataTagClassInterface>
{
	MAXON_COMPONENT();
	MAXON_CUSTOMDATATAG(ID_CUSTOMDATA_TAG_VC, "Vertex Color"_s, ""_s, Id("net.maxonexample.mesh_misc.customdatatagdisplay.vertexcolor"), 0, 4, TAG_VISIBLE | TAG_MULTIPLE, VERTEXCOLOR_MESHATTRIBUTE);

public:

	MAXON_METHOD void InterpolateLinear(void* data1, const void* data2, Float blendValue) const
	{
		if (!data1 || !data2)
			return;

		ColorA32* 			v1 = static_cast<ColorA32*>(data1);
		const ColorA32* v2 = static_cast<const ColorA32*>(data2);
		*v1 = Blend(*v1, *v2, (Float32)blendValue);
	}

	MAXON_METHOD void InterpolateInOutline(void* data, const Block<void*>& outline, const Block<Float>& weights) const
	{
		if (!data)
			return;

		if (outline.GetCount() != weights.GetCount())
			return;

		ColorA32* currentValue = nullptr;
		ColorA32* v = static_cast<ColorA32*>(data);
		ColorA32 	numerator = ColorA32();
		Float			denominator = 0.0;

		for (auto it = Iterable::IndexIterator(outline); it; ++it)
		{
			currentValue = static_cast<ColorA32*>(*it);
			if (!currentValue)
				continue;

			numerator += ((*currentValue) * (Float32)weights[it.GetIndex()]);
			denominator += weights[it.GetIndex()];
		}

		denominator = Inverse(denominator);
		*v = numerator * (Float32)denominator;
	}

	MAXON_METHOD void GetDefaultValue(void* data) const
	{
		if (!data)
			return;

		ColorA32* v = static_cast<ColorA32*>(data);
		if (v)
			*v = ColorA32();
	}

	MAXON_METHOD Bool AtrLessThen(const void* data1, const void* data2) const
	{
		if (!data1 || !data2)
			return false;

		const ColorA32* v1 = static_cast<const ColorA32*>(data1);
		const ColorA32* v2 = static_cast<const ColorA32*>(data2);
		return LexicographicalCompare(v1->r, v2->r, v1->g, v2->g, v1->b, v2->b, v1->a, v2->a) == COMPARERESULT::LESS;
	}

	MAXON_METHOD Bool AtrIsEqual(const void* data1, const void* data2) const
	{
		if (!data1 || !data2)
			return false;

		const ColorA32* v1 = static_cast<const ColorA32*>(data1);
		const ColorA32* v2 = static_cast<const ColorA32*>(data2);
		return LexicographicalCompare(v1->r, v2->r, v1->g, v2->g, v1->b, v2->b, v1->a, v2->a) == COMPARERESULT::EQUAL;
	}

	MAXON_METHOD void AtrAdd(void* data1, const void* data2) const
	{
		if (!data1 || !data2)
			return;

		ColorA32* 			v1 = static_cast<ColorA32*>(data1);
		const ColorA32* v2 = static_cast<const ColorA32*>(data2);
		*v1 = *v1 + *v2;
	}

	MAXON_METHOD void AtrSubstract(void* data1, const void* data2) const
	{
		if (!data1 || !data2)
			return;

		ColorA32* 			v1 = static_cast<ColorA32*>(data1);
		const ColorA32* v2 = static_cast<const ColorA32*>(data2);
		*v1 = *v1 - *v2;
	}

	MAXON_METHOD void AtrMultiply(void* data1, const void* data2) const
	{
		if (!data1 || !data2)
			return;

		ColorA32* 			v1 = static_cast<ColorA32*>(data1);
		const ColorA32* v2 = static_cast<const ColorA32*>(data2);
		*v1 = *v1 * *v2;
	}

	MAXON_METHOD void AtrMultiply(void* data, Float value) const
	{
		if (!data)
			return;

		ColorA32* 			v1 = static_cast<ColorA32*>(data);
		*v1 = ColorA32(v1->r * (Float32)value, v1->g * (Float32)value, v1->b * (Float32)value, v1->a * (Float32)value);
	}

	MAXON_METHOD void AtrDivide(void* data1, const void* data2) const
	{
		if (!data1 || !data2)
			return;

		ColorA32* 			v1 = static_cast<ColorA32*>(data1);
		const ColorA32* v2 = static_cast<const ColorA32*>(data2);
		*v1 = ColorA32(v1->r * Inverse(v2->r), v1->g * Inverse(v2->g), v1->b * Inverse(v2->b), v1->a * Inverse(v2->a));
	}

	MAXON_METHOD void AtrDivide(void* data, Float value) const
	{
		if (!data)
			return;

		ColorA32* 			v1 = static_cast<ColorA32*>(data);
		Float32 				divisor = (Float32)Inverse(value);
		*v1 = ColorA32(v1->r * divisor, v1->g * divisor, v1->b * divisor, v1->a * divisor);
	}

	MAXON_METHOD String AtrToString(const void* data, const FormatStatement* formatStatement) const
	{
		if (!data)
			return String();

		const ColorA32* v = static_cast<const ColorA32*>(data);
		return FormatString("(@,@,@)", v->r, v->g, v->b);
	}

	MAXON_METHOD Result<void> Read(void* data, HyperFile* hf, Int32 level) const
	{
		if (!hf || !data)
			return IllegalArgumentError(MAXON_SOURCE_LOCATION);

		ColorA32* dataPtr = static_cast<ColorA32*>(data);
		Vector32 	color;
		if (!hf->ReadVector32(&color))
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		Float32 alpha = 0.0;
		if (!hf->ReadFloat32(&alpha))
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		*dataPtr = ColorA32(color.GetColor(), alpha);
		return OK;
	}

	MAXON_METHOD Result<void> Write(const void* data, HyperFile* hf) const
	{
		if (!data || !hf)
			return IllegalArgumentError(MAXON_SOURCE_LOCATION);

		const ColorA32* dataPtr = static_cast<const ColorA32*>(data);
		if (!dataPtr)
			return OutOfMemoryError(MAXON_SOURCE_LOCATION);

		Vector32 	color = dataPtr->GetVector().GetVector3();
		Float32 	alpha = dataPtr->a;

		if (!hf->WriteVector32(color))
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		if (!hf->WriteFloat32(alpha))
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		return OK;
	}

	MAXON_METHOD Int32 GetIcon(Bool perPolyVertex) const
	{
		if (perPolyVertex)
			return Ocone;
		return Osky;
	}
	

	MAXON_METHOD Bool Message(GeListNode* node, Int32 type, void* data) const
	{
		return true;
	}
	
};

class VertexFloatDisplayImpl : public Component<VertexFloatDisplayImpl, CustomDataTagDisplayInterface>
{
	MAXON_COMPONENT();

public:

	VertexFloatDisplayImpl() {	}
	~VertexFloatDisplayImpl() { Reset();	}

	// Since we define a constructor and a destructor, a copy constructor is created by the compiler.
	// However the default copy constructor would not work for this class, so we delete it.
	VertexFloatDisplayImpl& operator=(const VertexFloatDisplayImpl& other) = delete;

	MAXON_METHOD Result<void> Init(BaseTag* tag)
	{
		if (!_gradient)
		{
			_gradient = ::Gradient::Alloc();

			maxon::GradientKnot k1, k2;
			k1.col = maxon::Color(1.0_f, 1.0_f, 1.0_f);
			k1.pos = 0.0_f;

			k2.col = maxon::Color(0.0_f, 0.0_f, 0.0_f);
			k2.pos = 1.0_f;

			_gradient->InsertKnot(k1);
			_gradient->InsertKnot(k2);
		}

		return OK;
	}

	MAXON_METHOD Result<void> Draw(BaseTag* tag, BaseObject* op, BaseDraw* bd, BaseDrawHelp* bh)
	{
		if (!tag || !bd || !bh || !op)
			return IllegalArgumentError(MAXON_SOURCE_LOCATION);

		bd->SetMatrix_Screen();
		bd->SetPen(Vector(1.0, 1.0, 0.0), SET_PEN_USE_PROFILE_COLOR);
		bd->DrawLine(Vector(100.0, 100.0, 0.0), Vector(200.0, 200.0, 0.0), 0);
		bd->SetMatrix_Matrix(nullptr, Matrix());

		return OK;
	}

	MAXON_METHOD Result<void> DisplayControl(const BaseTag* tag, BaseDocument* doc, BaseObject* op, BaseDraw* bd, BaseDrawHelp* bh, ControlDisplayStruct& cds) const
	{
		iferr_scope;

		if (!tag || !bd || !bh || !op || !op->IsInstanceOf(Opoint))
			return OK;

		const CustomDataTag* customtag = static_cast<const CustomDataTag*>(tag);

		if (_gradient)
		{
			CUSTOMDATATAG_MODE mode = customtag->GetMode();
			cds.colorMode = mode == CUSTOMDATATAG_MODE::POLYVERTEX ? DISPLAYCONTROL_MODE::PER_POLYGON_PER_VERTEX : DISPLAYCONTROL_MODE::PER_VERTEX;
			switch (mode)
			{
				case CUSTOMDATATAG_MODE::VERTEX:
				{
					Int32 pointCount = ToPoint(op)->GetPointCount();
					maxon::Color32* vertexColor = NewMem(maxon::Color32, pointCount) iferr_return;
					cds.vertexColor = vertexColor;
					for (Int32 vertexIndex = 0; vertexIndex < pointCount; ++vertexIndex)
					{
						const Float w = customtag->GetVertexData<Float>(vertexIndex);
						vertexColor[vertexIndex] = (maxon::Color32)_gradientRenderData.CalcGradientPixel(w);
					}
					break;
				}
				case CUSTOMDATATAG_MODE::POLYVERTEX:
				{
					Int32 polygonCount = ToPoly(op)->GetPolygonCount();
					maxon::Color32* vertexColor = NewMem(maxon::Color32, polygonCount * 4) iferr_return;
					cds.vertexColor = vertexColor;
					for (Int32 polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex)
					{
						for (Int32 i = 0; i < 4; ++i)
						{
							const Float& v = customtag->GetPolyVertexData<Float>(polygonIndex, i);
							vertexColor[4 * polygonIndex + i] = (maxon::Color32)_gradientRenderData.CalcGradientPixel(v);
						}
					}
					break;
				}
				default:
					break;
			}

		}

		return OK;
	}

	MAXON_METHOD Result<void> InitDisplayControl(BaseTag* tag, BaseDocument* doc, BaseDraw* bd, const AtomArray* active)
	{
		if (_gradient)
		{
			InitRenderStruct irs(doc);
			iferr (_gradientRenderData = _gradient->PrepareRenderData(irs))
			{
				return err;
			}
		}
		return OK;
	}

	MAXON_METHOD void FreeDisplayControl(BaseTag* tag)
	{
		_gradientRenderData = nullptr;
	}

	MAXON_METHOD void Reset()
	{
		_gradientRenderData = nullptr;
		if (_gradient)
			::Gradient::Free(_gradient);
	}

private:
	::Gradient*								_gradient = nullptr;
	maxon::GradientRenderData _gradientRenderData;
};

class VertexFloatImpl : public Component<VertexFloatImpl, CustomDataTagClassInterface>
{
	MAXON_COMPONENT();
	MAXON_CUSTOMDATATAG(ID_CUSTOMDATA_TAG_FL, "Float Test"_s, "tfloatc"_s, Id("net.maxonexample.mesh_misc.customdatatagdisplay.float"), 0, 1, TAG_VISIBLE | TAG_MULTIPLE, FLOATTYPE_MESHATTRIBUTE);

public:

	MAXON_METHOD void InterpolateLinear(void* data1, const void* data2, Float blendValue) const
	{
		if (!data1 || !data2)
			return;

		Float* 				v1 = static_cast<Float*>(data1);
		const Float* 	v2 = static_cast<const Float*>(data2);
		*v1 = Blend(*v1, *v2, blendValue);
	}

	MAXON_METHOD void InterpolateInOutline(void* data, const Block<void*>& outline, const Block<Float>& weights) const
	{
		if (!data)
			return;

		if (outline.GetCount() != weights.GetCount())
			return;

		Float* 			currentValue = nullptr;
		Float* 			v = static_cast<Float*>(data);
		Float 			numerator = Float();
		Float				denominator = 0.0;

		for (auto it = Iterable::IndexIterator(outline); it; ++it)
		{
			currentValue = static_cast<Float*>(*it);
			if (!currentValue)
				continue;

			numerator += ((*currentValue) * weights[it.GetIndex()]);
			denominator += weights[it.GetIndex()];
		}

		denominator = Inverse(denominator);
		*v = numerator * denominator;
	}

	MAXON_METHOD void GetDefaultValue(void* data) const
	{
		if (!data)
			return;

		Float* v = static_cast<Float*>(data);
		if (v)
			*v = 0.0;
	}

	MAXON_METHOD Bool AtrLessThen(const void* data1, const void* data2) const
	{
		if (!data1 || !data2)
			return false;

		const Float* v1 = static_cast<const Float*>(data1);
		const Float* v2 = static_cast<const Float*>(data2);
		return LexicographicalCompare(*v1, *v2) == COMPARERESULT::LESS;
	}

	MAXON_METHOD Bool AtrIsEqual(const void* data1, const void* data2) const
	{
		if (!data1 || !data2)
			return false;

		const Float* v1 = static_cast<const Float*>(data1);
		const Float* v2 = static_cast<const Float*>(data2);
		return LexicographicalCompare(*v1, *v2) == COMPARERESULT::EQUAL;
	}

	MAXON_METHOD void AtrAdd(void* data1, const void* data2) const
	{
		if (!data1 || !data2)
			return;

		Float* 				v1 = static_cast<Float*>(data1);
		const Float* 	v2 = static_cast<const Float*>(data2);
		*v1 = *v1 + *v2;
	}

	MAXON_METHOD void AtrSubstract(void* data1, const void* data2) const
	{
		if (!data1 || !data2)
			return;

		Float* 				v1 = static_cast<Float*>(data1);
		const Float* 	v2 = static_cast<const Float*>(data2);
		*v1 = *v1 - *v2;
	}

	MAXON_METHOD void AtrMultiply(void* data1, const void* data2) const
	{
		if (!data1 || !data2)
			return;

		Float* 				v1 = static_cast<Float*>(data1);
		const Float* 	v2 = static_cast<const Float*>(data2);
		*v1 = *v1 * *v2;
	}

	MAXON_METHOD void AtrMultiply(void* data, Float value) const
	{
		if (!data)
			return;

		Float* v1 = static_cast<Float*>(data);
		*v1 = *v1 * value;
	}

	MAXON_METHOD void AtrDivide(void* data1, const void* data2) const
	{
		if (!data1 || !data2)
			return;

		Float* 				v1 = static_cast<Float*>(data1);
		const Float* 	v2 = static_cast<const Float*>(data2);
		*v1 = *v1 * Inverse(*v2);
	}

	MAXON_METHOD void AtrDivide(void* data, Float value) const
	{
		if (!data)
			return;

		Float* v1 = static_cast<Float*>(data);
		*v1 = *v1 * Inverse(value);
	}

	MAXON_METHOD String AtrToString(const void* data, const FormatStatement* formatStatement) const
	{
		const Float* v = static_cast<const Float*>(data);
		return FormatString("(@)", *v);
	}

	MAXON_METHOD Result<void> Read(void* data, HyperFile* hf, Int32 level) const
	{
		if (!hf || !data)
			return IllegalArgumentError(MAXON_SOURCE_LOCATION);

		Float* dataPtr = static_cast<Float*>(data);
		if (!dataPtr)
			return OutOfMemoryError(MAXON_SOURCE_LOCATION);

		if (!hf->ReadFloat(dataPtr))
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		return OK;
	}

	MAXON_METHOD Result<void> Write(const void* data, HyperFile* hf) const
	{
		if (!data || !hf)
			return IllegalArgumentError(MAXON_SOURCE_LOCATION);

		const Float* dataPtr = static_cast<const Float*>(data);
		if (!dataPtr)
			return OutOfMemoryError(MAXON_SOURCE_LOCATION);
		
		if (!hf->WriteFloat(*dataPtr))
			return IllegalStateError(MAXON_SOURCE_LOCATION);
		
		return OK;
	}
	
	MAXON_METHOD Int32 GetIcon(Bool perPolyVertex) const
	{
		if (perPolyVertex)
			return Ocube;
		return Olight;
	}
	
	MAXON_METHOD Bool Message(GeListNode* node, Int32 type, void* data) const
	{
		return true;
	}
};

MAXON_COMPONENT_OBJECT_REGISTER(VertexFloatImpl, CustomDataTagClasses::FLOAT)
MAXON_COMPONENT_CLASS_REGISTER(VertexFloatDisplayImpl, CustomDataTagDisplayClasses, "net.maxonexample.mesh_misc.customdatatagdisplay.float");

MAXON_COMPONENT_OBJECT_REGISTER(VertexColorImpl, CustomDataTagClasses::VC)
MAXON_COMPONENT_CLASS_REGISTER(VertexColorDisplayImpl, CustomDataTagDisplayClasses, "net.maxonexample.mesh_misc.customdatatagdisplay.vertexcolor");

// data types registration
MAXON_DATATYPE_REGISTER(PointIndex);
MAXON_DATATYPE_REGISTER(POINTINDEX_MESHATTRIBUTE);

// This is the custom data tag implementation, to emulate a point index map we don't need to implement interpolations and arithmetic
// In this implementation whenever a point is deleted from the object the last point in the object will take its place,
// if a point is added will be added at end with and the value will be NOTOK.
class PointIndexImpl : public Component<PointIndexImpl, CustomDataTagClassInterface>
{
	MAXON_COMPONENT();
	MAXON_CUSTOMDATATAG(ID_POINT_INDEX_TAG, "Point Index"_s, ""_s, CustomDataTagClasses::POINTINDEX.GetId(), 0, NOTOK, 0, POINTINDEX_MESHATTRIBUTE); // notice the last value is 0, this mean the tag is not VISIBLE and not multiple

public:

	MAXON_METHOD void InterpolateLinear(void* data1, const void* data2, Float blendValue) const {	}

	MAXON_METHOD void InterpolateInOutline(void* data, const Block<void*>& outline, const Block<Float>& weights) const { }

	MAXON_METHOD void GetDefaultValue(void* data) const
	{
		if (!data)
			return;

		PointIndex* v = static_cast<PointIndex*>(data);
		if (v)
		{
			v->_index = NOTOK;
			v->_privateBuffer = 0;
		}
	}

	MAXON_METHOD Bool AtrLessThen(const void* data1, const void* data2) const
	{
		const PointIndex* a = static_cast<const PointIndex*>(data1);
		const PointIndex* b = static_cast<const PointIndex*>(data2);
		return a < b;
	}

	MAXON_METHOD Bool AtrIsEqual(const void* data1, const void* data2) const
	{
		const PointIndex* a = static_cast<const PointIndex*>(data1);
		const PointIndex* b = static_cast<const PointIndex*>(data2);
		return *a == *b;
	}

	MAXON_METHOD void AtrAdd(void* data1, const void* data2) const { }

	MAXON_METHOD void AtrSubstract(void* data1, const void* data2) const { }

	MAXON_METHOD void AtrMultiply(void* data1, const void* data2) const { }

	MAXON_METHOD void AtrMultiply(void* data, Float value) const { }

	MAXON_METHOD void AtrDivide(void* data1, const void* data2) const { }

	MAXON_METHOD void AtrDivide(void* data, Float value) const { }

	MAXON_METHOD String AtrToString(const void* data, const FormatStatement* formatStatement) const
	{
		const PointIndex* v = static_cast<const PointIndex*>(data);
		return FormatString("(@)", v->_index);
	}

	MAXON_METHOD Result<void> Read(void* data, HyperFile* hf, Int32 level) const
	{
		if (!hf || !data)
			return IllegalArgumentError(MAXON_SOURCE_LOCATION);

		PointIndex* dataPtr = static_cast<PointIndex*>(data);
		if (!dataPtr)
			return OutOfMemoryError(MAXON_SOURCE_LOCATION);

		if (!hf->ReadInt32(&dataPtr->_index))
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		if (!hf->ReadInt32(&dataPtr->_privateBuffer))
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		return OK;
	}

	MAXON_METHOD Result<void> Write(const void* data, HyperFile* hf) const
	{
		if (!data || !hf)
			return IllegalArgumentError(MAXON_SOURCE_LOCATION);

		const PointIndex* dataPtr = static_cast<const PointIndex*>(data);
		if (!dataPtr)
			return OutOfMemoryError(MAXON_SOURCE_LOCATION);

		if (!hf->WriteInt32(dataPtr->_index))
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		if (!hf->WriteInt32(dataPtr->_privateBuffer))
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		return OK;
	}

	MAXON_METHOD Int32 GetIcon(Bool perPolyVertex) const
	{
		return Olight;
	}
	
	MAXON_METHOD Bool Message(GeListNode* node, Int32 type, void* data) const
	{
		return true;
	}
	
};

// Register the custom data tag implementation
MAXON_COMPONENT_OBJECT_REGISTER(PointIndexImpl, CustomDataTagClasses::POINTINDEX);

} // namespace maxon

#include "c4d_objectdata.h"

// PointIndexImpl usage example in a simple polygon object to track the point changes
class MyPolyObject : public ObjectData
{
	INSTANCEOF(MyPolyObject, ObjectData);

public:

	MyPolyObject() 	{ }
	~MyPolyObject() 	{ }

	virtual Bool Init(GeListNode* node, Bool isCloneInit)
	{
		if (!node)
			return false;
		PolygonObject* polyOp = static_cast<PolygonObject*>(node);

		if (!polyOp->GetTag(Tpolygon) && !polyOp->GetTag(Tpoint))
		{
			polyOp->MakeTag(Tpolygon);
			polyOp->MakeTag(Tpoint);

			if (!polyOp->ResizeObject(8, 0))
				return false;

			Vector* 	points = polyOp->GetPointW();

			if (!points)
				return false;

			points[0] = Vector(0.0);
			points[1] = Vector(0.0, 10.0, 0.0);
			points[2] = Vector(0.0, 20.0, 0.0);
			points[3] = Vector(0.0, 30.0, 0.0);
			points[4] = Vector(0.0, 40.0, 0.0);
			points[5] = Vector(0.0, 50.0, 0.0);
			points[6] = Vector(0.0, 60.0, 0.0);
			points[7] = Vector(0.0, 70.0, 0.0);

			// we create the new point map tag
			BaseTag* newDataTag = CustomDataTag::Alloc(ID_POINT_INDEX_TAG);
			if (!newDataTag)
				return false;

			polyOp->InsertTag(newDataTag);

			CustomDataTag* customDataTag = static_cast<CustomDataTag*>(newDataTag);

			iferr (customDataTag->InitData())
			{
				err.DbgStop();
				return false;
			}

			// set the map mode to vertex mode
			customDataTag->SetMode(CUSTOMDATATAG_MODE::VERTEX);

			Int32 vertexCount = ToPoly(polyOp)->GetPointCount();
			iferr (customDataTag->Resize(vertexCount))
			{
				err.DbgStop();
				return false;
			}

			for (Int32 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
			{
				customDataTag->SetVertexData<maxon::PointIndex>(vertexIndex, vertexIndex);
			}

			// check the vertex index and the value in the map are in sync
			// the output will be:
			//			[0], 0
			//			[1], 1
			//			[2], 2
			//			[3], 3
			//			[4], 4
			//			[5], 5
			//			[6], 6
			//			[7], 7

			for (Int32 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
			{
				const maxon::PointIndex index = customDataTag->GetVertexData<maxon::PointIndex>(vertexIndex);
				DiagnosticOutput("[@], @", vertexIndex, index);
			}

			_lastVertexCount = vertexCount;
		}

		return SUPER::Init(node, isCloneInit);
	}

	virtual Bool Message(GeListNode* node, Int32 type, void* data)
	{
		if (type == MSG_UPDATE)
		{
			SUPER::Message(node, type, data);
			PolygonObject* polyOp = static_cast<PolygonObject*>(node);
			if (!polyOp)
				return false;

			Int32 currentVertexCount = polyOp->GetPointCount();
			if (currentVertexCount == _lastVertexCount) // nothing to do here.
				return true;

			// here you will find your map
			BaseTag* pointMapTag = polyOp->GetTag(ID_POINT_INDEX_TAG);
			if (pointMapTag)
			{
				CustomDataTag* customDataTag = static_cast<CustomDataTag*>(pointMapTag);
				if (!customDataTag)
					return false;

				Int32 valueCount = (Int32)customDataTag->GetComponentCount(); // must be equal to the new object point count

				if (valueCount != polyOp->GetPointCount())
				{
					CriticalStop();
					return false;
				}

				// Something changed in the point count so the content of the custom data tag show how to map data
				// if we delete point 4 the output will be:
				//			[0], 0
				//			[1], 1
				//			[2], 2
				//			[3], 3
				//			[4], 7
				//			[5], 5
				//			[6], 6

				// else if we add a new point the output will be:
				//			[0], 0
				//			[1], 1
				//			[2], 2
				//			[3], 3
				//			[4], 4
				//			[5], 5
				//			[6], 6
				//			[7], 7
				//			[8], -1 (NOTOK)
				for (Int32 vertexIndex = 0; vertexIndex < valueCount; ++vertexIndex)
				{
					const maxon::PointIndex& index = customDataTag->GetVertexData<maxon::PointIndex>(vertexIndex);

					// edit your own data here
					DiagnosticOutput("[@], @", vertexIndex, index._index);

					// reallign indices in the point map for the next edit
					customDataTag->SetVertexData<maxon::PointIndex>(vertexIndex, vertexIndex);
				}

				_lastVertexCount = valueCount;
			}
			return true;
		}
		return SUPER::Message(node, type, data);
	}

	DRAWRESULT Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh)
	{
		if (bd->TestBreak())
			return DRAWRESULT::SKIP;

		bd->SetMatrix_Matrix(op, op->GetMg());

		return bd->DrawPolygonObject(bh, ToPoly(op), DRAWOBJECT::FORCEBASE);
	}

	static NodeData* Alloc()
	{
		return NewObjClear(MyPolyObject);
	}

private:
	Int32 _lastVertexCount = 0; // stores the last point count.
};

Bool RegisterPolyExample()
{
	String name = "Poly test"_s;
	return RegisterObjectPlugin(ID_POINT_INDEX_OBJECT_EXAMPLE, name, OBJECT_POLYGONOBJECT | OBJECT_POINTOBJECT, MyPolyObject::Alloc, ""_s, nullptr, 0);
}

class ConvertToCustomdataTagCommand : public CommandData
{
public:

	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager)
	{
		if (!doc)
			return false;

		AutoAlloc<AtomArray> activeTags;
		if (!activeTags)
			return false;

		doc->GetActiveTags(*activeTags);

		BaseObject* 		op = nullptr;
		BaseTag* 				currentTag = nullptr;
		BaseTag* 				newDataTag = nullptr;
		CustomDataTag* 	customDataTag = nullptr;
		for (Int32 tagIndex = 0; tagIndex < activeTags->GetCount(); ++tagIndex)
		{
			currentTag = static_cast<BaseTag*>(activeTags->GetIndex(tagIndex));
			if (!currentTag)
				continue;

			op = currentTag->GetObject();
			if (!op)
				return false;

			if (currentTag->IsInstanceOf(Tvertexmap))
			{
				newDataTag = CustomDataTag::Alloc(ID_CUSTOMDATA_TAG_FL);
				if (!newDataTag)
					return false;

				op->InsertTag(newDataTag);

				customDataTag = static_cast<CustomDataTag*>(newDataTag);

				iferr (customDataTag->InitData())
				{
					err.DbgStop();
					return false;
				}

				customDataTag->SetMode(CUSTOMDATATAG_MODE::VERTEX);

				Int32 vertexCount = ToPoly(op)->GetPointCount();
				iferr (customDataTag->Resize(vertexCount))
				{
					err.DbgStop();
					return false;
				}

				const Float32* values = static_cast<VertexMapTag*>(currentTag)->GetDataAddressR();
				if (!values)
					return false;
				for (Int32 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
				{
					Float32 value = values[vertexIndex];
					customDataTag->SetVertexData<Float>(vertexIndex, std::move(value));
				}
			}
			else if (currentTag->IsInstanceOf(Tvertexcolor))
			{
				newDataTag = CustomDataTag::Alloc(ID_CUSTOMDATA_TAG_VC);
				if (!newDataTag)
					return false;

				op->InsertTag(newDataTag);

				Int32 count = 0;
				VertexColorTag* vcTag = static_cast<VertexColorTag*>(currentTag);
				customDataTag = static_cast<CustomDataTag*>(newDataTag);
				iferr (customDataTag->InitData())
				{
					err.DbgStop();
					return false;
				}

				if (vcTag->IsPerPointColor())
				{
					customDataTag->SetMode(CUSTOMDATATAG_MODE::VERTEX);
					count = ToPoly(op)->GetPointCount();

					iferr (customDataTag->Resize(count))
					{
						err.DbgStop();
						return false;
					}

					ConstVertexColorHandle values = vcTag->GetDataAddressR();
					if (!values)
						return false;
					for (Int32 vertexIndex = 0; vertexIndex < count; ++vertexIndex)
					{
						maxon::ColorA32 value = VertexColorTag::Get(values, nullptr, nullptr, vertexIndex);
						customDataTag->SetVertexData<maxon::ColorA32>(vertexIndex, std::move(value));
					}
				}
				else
				{
					customDataTag->SetMode(CUSTOMDATATAG_MODE::POLYVERTEX);
					count = ToPoly(op)->GetPolygonCount();

					iferr (customDataTag->Resize(count))
					{
						err.DbgStop();
						return false;
					}

					ConstVertexColorHandle values = vcTag->GetDataAddressR();
					if (!values)
						return false;

					VertexColorStruct vcs;
					for (Int32 polygonIndex = 0; polygonIndex < count; ++polygonIndex)
					{
						VertexColorTag::Get(values, polygonIndex, vcs);
						for (Int32 polyvertexIndex = 0; polyvertexIndex < 4; ++polyvertexIndex)
						{
							maxon::ColorA32 value = vcs[polyvertexIndex];
							customDataTag->SetPolyVertexData<maxon::ColorA32>(polygonIndex, polyvertexIndex, std::move(value));
						}
					}
				}
			}
		}
		EventAdd();
		return true;
	}
};

Bool RegisterCustomDataTagDescription()
{
	return RegisterDescription(ID_CUSTOMDATA_TAG_FL, "tfloat"_s);
}

Bool RegisterCustomDataTagCommand()
{
	return RegisterCommandPlugin(ID_CUSTOMDATA_TAG_COMMAND, "Convert To CustomDataTag"_s, 0, nullptr, "Convert To CustomDataTag"_s, NewObjClear(ConvertToCustomdataTagCommand));
}


