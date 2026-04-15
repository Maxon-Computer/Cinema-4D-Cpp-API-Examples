/*
	Image API Examples - OCIO
	2022, (C) MAXON Computer GmbH

	Author: Ferdinand Hoppe
	Date: 02/11/2022
	SDK: 2023.100

	Demonstrates how to interact with the OCIO settings and color spaces in an OCIO document.
*/

#ifndef EXAMPLES_OCIO_H__
#define EXAMPLES_OCIO_H__

#include "c4d_basedocument.h"
#include "c4d_objectdata.h"
#include "maxon/apibase.h"
#include "c4d_symbols.h"

/// @brief Convert all colors contained in a document or contained in a set of elements in a 
/// document to a new OCIO Render space.
maxon::Result<void> ConvertSceneOrElements(cinema::BaseDocument* doc);

/// @brief Copy all color management settings from one document to another.
/// @note This does not entail any scene element color space conversions, one must instead use 
/// #SceneColorConverter as demonstrated in #ConvertSceneOrElements for this.
maxon::Result<void> CopyColorManagementSettings(cinema::BaseDocument* doc);

/// @brief Read and write the OCIO color management settings stored in a document.
maxon::Result<void> GetSetColorManagementSettings(cinema::BaseDocument* doc);

/// @brief Convert colors along OCIO conversion paths defined by the OCIO color spaces associated 
/// with a document.
maxon::Result<void> ConvertOcioColors(cinema::BaseDocument* doc);

/// @brief Convert colors from and to OCIO color spaces using arbitrary in- and output spaces.
maxon::Result<void> ConvertOcioColorsArbitrarily(cinema::BaseDocument* doc);

/// @brief Read and write color parameters in an OCIO document either in the native Render space of
/// the document or other spaces.
maxon::Result<void> GetSetColorValuesInOcioDocuments(cinema::BaseDocument* doc);

/// @brief Read and write the OCIO color spaces associated with #cinema::BaseBitmap instances to change or 
/// nullify parts of an OCIO conversion path for them.
/// @details This makes it for example possible to disable the Display and View transform for a 
/// singular bitmap when displayed in the Picture Viewer.
maxon::Result<void> GetSetBitmapOcioProfiles(cinema::BaseDocument* doc);

/// @brief Demonstrates how to render an (OCIO) document to disk and the picture viewer while controlling all
/// aspects of the rendering manually.
maxon::Result<void> ComplexRenderDocument(cinema::BaseDocument* doc);

maxon::Result<void> SimpleRenderDocument(cinema::BaseDocument* doc);

/// @brief Realizes a renderer which manipulates the OCIO profiles of an upcoming rendering.
class OcioAwareRenderer : public cinema::VideoPostData
{
	INSTANCEOF(OcioAwareRenderer, cinema::VideoPostData)

public:
	static NodeData* Alloc() { return NewObjClear(OcioAwareRenderer); }
	virtual cinema::Bool Init(cinema::GeListNode* node, cinema::Bool isCloneInit);

	/// @brief Called by Cinema 4D to let a renderer modify the OCIO profiles of an upcoming rendering.
	virtual void GetColorProfileInfo(cinema::BaseVideoPost* node, cinema::VideoPostStruct* vps, cinema::ColorProfileInfo& info);
	/// @brief Called by Cinema 4D to execute the renderer.
	virtual cinema::RENDERRESULT Execute(cinema::BaseVideoPost* node, cinema::VideoPostStruct* vps);
};

/// @brief Demonstrates how to handle OCIO colors in a scene element at the example of a generator 
/// object, specifically in the context of the Cinema 4D 2025.0.0 OCIO changes.
///
/// @details In 2025 and onwards, colors are always interpreted as sRGB-2.2 in NodeData::Init, even 
/// in OCIO color managed scenes which is now the default. This example highlights how to handle this
/// change and how to handle OCIO colors in other places such as a drawing function.
class OcioNode2025 : public cinema::ObjectData
{
	INSTANCEOF(OcioNode2025, cinema::ObjectData)

private:
	const cinema::BaseBitmap* _bitmap = nullptr;

public:
	/// @brief Called by Cinema 4D to initialize the node.
	virtual cinema::Bool Init(cinema::GeListNode* node, cinema::Bool isCloneInit);

	/// @brief Called by Cinema 4D when the node is being destroyed.
	virtual void Free(cinema::GeListNode* node);

	/// @brief Called by Cinema 4D to let the node handle messages.
	virtual cinema::Bool Message(cinema::GeListNode* node, cinema::Int32 type, void* pData);

	/// @brief Called by Cinema 4D to let the node draw into viewports.
	virtual cinema::DRAWRESULT Draw(cinema::BaseObject* op, cinema::DRAWPASS drawpass, cinema::BaseDraw* bd, cinema::BaseDrawHelp* bh);

	/// @brief Called by Cinema 4D to let the node create its cache.
	virtual cinema::BaseObject* GetVirtualObjects(cinema::BaseObject* op, const cinema::HierarchyHelp* hh);

	/// @brief Preloads textures used by the drawing function.
	/// @details This is a custom function which is not part of the ObjectData interface.
	maxon::Result<void> PreloadTextures();

	/// @brief Returns a new instance of this class.
	static NodeData* Alloc() { return NewObjClear(OcioNode2025); }
};

#endif // EXAMPLES_OCIO_H__