#ifndef CUSTOMNODE_CUSTOMNODESPACE_DESCRIPTIONS_H__
#define CUSTOMNODE_CUSTOMNODESPACE_DESCRIPTIONS_H__

#include "maxon/fid.h"
#include "maxon/url.h"
#include "maxon/uuid.h"
#include "maxon/vector.h"
#include "maxon/vector2d.h"
#include "maxon/vector4d.h"
#include "hybriddatatype.h"

//----------------------------------------------------------------------------------------
// BEGIN - auto generated code, do not edit
//----------------------------------------------------------------------------------------
namespace maxonexample
{
namespace DATATYPE
{
	namespace PORTBUNDLE
	{
		namespace GRADIENT
		{
			MAXON_RESOURCE_DATABASE_SCOPE("net.maxonexample.datatype.portbundle.gradient");

			MAXON_ATTRIBUTE(maxon::ColorA, COLOR, "color");

			MAXON_ATTRIBUTE(maxon::Float, POSITION, "position");

			MAXON_ATTRIBUTE(maxon::Float, BIAS, "bias");

			MAXON_ATTRIBUTE(maxon::InternedId, INTERPOLATION, "interpolation");
			MAXON_ATTRIBUTE(void, INTERPOLATION_ENUM_SMOOTHKNOT, "smoothknot");
			MAXON_ATTRIBUTE(void, INTERPOLATION_ENUM_CUBICKNOT, "cubicknot");
			MAXON_ATTRIBUTE(void, INTERPOLATION_ENUM_LINEARKNOT, "linearknot");
			MAXON_ATTRIBUTE(void, INTERPOLATION_ENUM_NONE, "none");
			MAXON_ATTRIBUTE(void, INTERPOLATION_ENUM_BLEND, "blend");
			MAXON_ATTRIBUTE(void, INTERPOLATION_ENUM_CUBICBIAS, "cubicbias");
		}

		namespace PAIR
		{
			MAXON_RESOURCE_DATABASE_SCOPE("net.maxonexample.datatype.portbundle.pair");

			MAXON_ATTRIBUTE(maxon::ColorA, FIRSTVALUE, "firstvalue");

			MAXON_ATTRIBUTE(maxon::Float, SECONDVALUE, "secondvalue");
		}
	}
}

namespace NODE
{
	namespace BUNDLENODE
	{
		MAXON_RESOURCE_DATABASE_SCOPE("net.maxonexample.node.bundlenode");

		// supports MAXON::NODE::BASE::FILTERTAGS
		// supports MAXON::NODE::BASE::NODEPREVIEWIMAGE
		// supports MAXON::NODE::BASE::NAME
		// supports MAXON::NODE::BASE::ASSETVERSION
		// supports MAXON::NODE::BASE::COLOR
		// supports MAXON::NODE::BASE::PORTDISPLAY
		// supports MAXON::NODE::BASE::DISPLAYPREVIEW
		// supports MAXON::NODE::BASE::DISPLAYCOMMENT
		// supports MAXON::NODE::BASE::COMMENT
		// supports MAXON::NODE::BASE::CATEGORY
		// supports MAXON::NODE::BASE::ICON
		// supports MAXON::NODE::BASE::UPDATEPOLICY
		// supports MAXON::ASSET::BASE::PROTECTED

		MAXON_ATTRIBUTE(void, FIRSTVARIADICBUNDLE, "firstvariadicbundle");

		MAXON_ATTRIBUTE(void, SECONDVARIADICBUNDLE, "secondvariadicbundle");

		MAXON_ATTRIBUTE(void, FIRSTNONVARIADICBUNDLE, "firstnonvariadicbundle");

		MAXON_ATTRIBUTE(void, SECONDNONVARIADICBUNDLE, "secondnonvariadicbundle");

		MAXON_ATTRIBUTE(maxon::ColorA, RESULT, "result");
	}

	namespace DYNAMICNODE
	{
		MAXON_RESOURCE_DATABASE_SCOPE("net.maxonexample.node.dynamicnode");

		// supports MAXON::NODE::BASE::FILTERTAGS
		// supports MAXON::NODE::BASE::NODEPREVIEWIMAGE
		// supports MAXON::NODE::BASE::NAME
		// supports MAXON::NODE::BASE::ASSETVERSION
		// supports MAXON::NODE::BASE::COLOR
		// supports MAXON::NODE::BASE::PORTDISPLAY
		// supports MAXON::NODE::BASE::DISPLAYPREVIEW
		// supports MAXON::NODE::BASE::DISPLAYCOMMENT
		// supports MAXON::NODE::BASE::COMMENT
		// supports MAXON::NODE::BASE::CATEGORY
		// supports MAXON::NODE::BASE::ICON
		// supports MAXON::NODE::BASE::UPDATEPOLICY
		// supports MAXON::ASSET::BASE::PROTECTED

		MAXON_ATTRIBUTE(maxon::String, CODE, "code");

		MAXON_ATTRIBUTE(maxon::ColorA, RESULT, "result");
	}

	namespace ENDNODE
	{
		MAXON_RESOURCE_DATABASE_SCOPE("net.maxonexample.node.endnode");

		// supports MAXON::NODE::BASE::FILTERTAGS
		// supports MAXON::NODE::BASE::NODEPREVIEWIMAGE
		// supports MAXON::NODE::BASE::NAME
		// supports MAXON::NODE::BASE::ASSETVERSION
		// supports MAXON::NODE::BASE::COLOR
		// supports MAXON::NODE::BASE::PORTDISPLAY
		// supports MAXON::NODE::BASE::DISPLAYPREVIEW
		// supports MAXON::NODE::BASE::DISPLAYCOMMENT
		// supports MAXON::NODE::BASE::COMMENT
		// supports MAXON::NODE::BASE::CATEGORY
		// supports MAXON::NODE::BASE::ICON
		// supports MAXON::NODE::BASE::UPDATEPOLICY
		// supports MAXON::ASSET::BASE::PROTECTED

		MAXON_ATTRIBUTE(maxon::InternedId, MATERIALPRESET, "materialpreset");
		MAXON_ATTRIBUTE(void, MATERIALPRESET_ENUM_NONE, "none");
		MAXON_ATTRIBUTE(void, MATERIALPRESET_ENUM_PLASTIC, "plastic");
		MAXON_ATTRIBUTE(void, MATERIALPRESET_ENUM_GLASS, "glass");
		MAXON_ATTRIBUTE(void, MATERIALPRESET_ENUM_METAL, "metal");
		MAXON_ATTRIBUTE(void, MATERIALPRESET_ENUM_LAVA, "lava");

		MAXON_ATTRIBUTE(maxon::Bool, METAL, "metal");

		MAXON_ATTRIBUTE(maxon::Color, SPECULARCOLOR, "specularColor");

		MAXON_ATTRIBUTE(maxon::Float, SPECULARCOLORINTENSITY, "specularColorIntensity");

		MAXON_ATTRIBUTE(maxon::Float, SPECULARROUGHNESS, "specularRoughness");

		MAXON_ATTRIBUTE(maxon::Float, SPECULARIOR, "specularIOR");

		MAXON_ATTRIBUTE(maxon::Color, BASECOLOR, "baseColor");

		MAXON_ATTRIBUTE(maxon::Float, BASECOLORINTENSITY, "baseColorIntensity");

		MAXON_ATTRIBUTE(maxon::Color, EMISSIONCOLOR, "emissionColor");

		MAXON_ATTRIBUTE(maxon::Float, EMISSIONCOLORINTENSITY, "emissionColorIntensity");

		MAXON_ATTRIBUTE(maxon::Float, REFRACTIONINTENSITY, "refractionIntensity");

		MAXON_ATTRIBUTE(maxon::Float, SURFACEALPHA, "surfaceAlpha");

		MAXON_ATTRIBUTE(maxon::ColorA, RESULT, "result");
	}

	namespace GRADIENTNODE
	{
		MAXON_RESOURCE_DATABASE_SCOPE("net.maxonexample.node.gradientnode");

		// supports MAXON::NODE::BASE::FILTERTAGS
		// supports MAXON::NODE::BASE::NODEPREVIEWIMAGE
		// supports MAXON::NODE::BASE::NAME
		// supports MAXON::NODE::BASE::ASSETVERSION
		// supports MAXON::NODE::BASE::COLOR
		// supports MAXON::NODE::BASE::PORTDISPLAY
		// supports MAXON::NODE::BASE::DISPLAYPREVIEW
		// supports MAXON::NODE::BASE::DISPLAYCOMMENT
		// supports MAXON::NODE::BASE::COMMENT
		// supports MAXON::NODE::BASE::CATEGORY
		// supports MAXON::NODE::BASE::ICON
		// supports MAXON::NODE::BASE::UPDATEPOLICY
		// supports MAXON::ASSET::BASE::PROTECTED

		MAXON_ATTRIBUTE(void, FIRSTBUNDLE, "firstbundle");

		MAXON_ATTRIBUTE(maxon::ColorA, COLORA, "colora");

		MAXON_ATTRIBUTE(void, SECONDBUNDLE, "secondbundle");

		MAXON_ATTRIBUTE(maxon::ColorA, RESULT, "result");
	}

	namespace INVERTNODE
	{
		MAXON_RESOURCE_DATABASE_SCOPE("net.maxonexample.node.invertnode");

		// supports MAXON::NODE::BASE::FILTERTAGS
		// supports MAXON::NODE::BASE::NODEPREVIEWIMAGE
		// supports MAXON::NODE::BASE::NAME
		// supports MAXON::NODE::BASE::ASSETVERSION
		// supports MAXON::NODE::BASE::COLOR
		// supports MAXON::NODE::BASE::PORTDISPLAY
		// supports MAXON::NODE::BASE::DISPLAYPREVIEW
		// supports MAXON::NODE::BASE::DISPLAYCOMMENT
		// supports MAXON::NODE::BASE::COMMENT
		// supports MAXON::NODE::BASE::CATEGORY
		// supports MAXON::NODE::BASE::ICON
		// supports MAXON::NODE::BASE::UPDATEPOLICY
		// supports MAXON::ASSET::BASE::PROTECTED

		MAXON_ATTRIBUTE(maxon::Color, IN, "in");

		MAXON_ATTRIBUTE(maxon::Color, OUT, "out");
	}

	namespace USERNODE
	{
		MAXON_RESOURCE_DATABASE_SCOPE("net.maxonexample.node.usernode");

		// supports MAXON::NODE::BASE::FILTERTAGS
		// supports MAXON::NODE::BASE::NODEPREVIEWIMAGE
		// supports MAXON::NODE::BASE::NAME
		// supports MAXON::NODE::BASE::ASSETVERSION
		// supports MAXON::NODE::BASE::COLOR
		// supports MAXON::NODE::BASE::PORTDISPLAY
		// supports MAXON::NODE::BASE::DISPLAYPREVIEW
		// supports MAXON::NODE::BASE::DISPLAYCOMMENT
		// supports MAXON::NODE::BASE::COMMENT
		// supports MAXON::NODE::BASE::CATEGORY
		// supports MAXON::NODE::BASE::ICON
		// supports MAXON::NODE::BASE::UPDATEPOLICY
		// supports MAXON::ASSET::BASE::PROTECTED

		MAXON_ATTRIBUTE(maxon::ColorA, COLORA, "colora");

		MAXON_ATTRIBUTE(maxonsdk::HybridDataType, HYBRID, "hybrid");

		MAXON_ATTRIBUTE(maxon::String, STRING, "string");

		MAXON_ATTRIBUTE(maxon::ColorA, RESULT, "result");

		MAXON_ATTRIBUTE(void, CUSTOMCOMMAND, "customcommand");
	}
}

namespace NODESPACE
{
	namespace EXAMPLE
	{
		MAXON_RESOURCE_DATABASE_SCOPE("net.maxonexample.nodespace.example");

	}
}
}
//----------------------------------------------------------------------------------------
// END - auto generated code, do not edit
//----------------------------------------------------------------------------------------


#include "customnode-customnodespace_descriptions1.hxx"
#include "customnode-customnodespace_descriptions2.hxx"


#endif // CUSTOMNODE_CUSTOMNODESPACE_DESCRIPTIONS_H__
