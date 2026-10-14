/*
	Asset API Examples - Dots Preset Asset Type
	Copyright (C) 2022 MAXON Computer GmbH

	Author: Ferdinand Hoppe
	Date: 01/04/2022
	SDK: R26

	Contains the Asset API examples related to Dots preset asset implementation.
*/
#ifndef EXAMPLES_DOTS_H__
#define EXAMPLES_DOTS_H__

#include "c4d_basedocument.h"

#include "maxon/apibase.h"
#include "maxon/assets.h"

//----------------------------------------------------------------------------------------
/// Inserts a null-object containing a DotsData user data parameter into the passed document.
/// 
/// @param[in] doc								The document to insert the null object into.
//----------------------------------------------------------------------------------------
maxon::Result<void> InsertDotsDataNull(cinema::BaseDocument* doc);


//----------------------------------------------------------------------------------------
/// Demonstrates how to instantiate a preset asset with its data type.
/// 
/// How a preset asset can be instantiated from their targeted data type depends on the 
/// implementation of the preset asset. The pattern shown here applies to the dots preset asset 
/// example and some native preset asset types of Cinema 4D, but there is no guarantee that all 
/// preset asset types directly wrap their data type in the PresetSaveArgs.
/// 
/// @return												The asset description for the new Dots preset asset.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::AssetDescription> InstantiateDotsPresetAsset();


//----------------------------------------------------------------------------------------
/// Showcases how to access the implementation of an asset type from an asset description.
/// 
/// An asset thumbnail can be updated with AssetCreationInterface::AddPreviewRenderAsset in a more
/// convenient fashion. This example demonstrates how to load the asset interface from an asset
/// description and then invoke one of the implementations of its components.
/// 
/// The Dots Preset Asset Type makes its preview thumbnails dependent on a custom meta data
/// attribute called DOTS_SCALE. The example showcases how to write that attribute and how to 
/// manually update the preview image by calling the dots preset asset implementation.
/// 
/// @param[in,out] assetDescription	The asset to update the preview thumbnail for.
/// @param[in] dotScaleDelta			The difference in the dot scale to the old value. Pass in 
/// 															negative values for decreasing the scale and positive values
/// 															for increasing it.
//----------------------------------------------------------------------------------------
maxon::Result<void> UpdatePreviewThumbnail(
	maxon::AssetDescription& assetDescription, maxon::Float32 dotScaleDelta);

#endif // EXAMPLES_DOTS_H__
