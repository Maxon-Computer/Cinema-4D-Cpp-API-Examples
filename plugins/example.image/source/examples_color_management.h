/*
	Image API Examples - Color Management
	2022, (C) MAXON Computer GmbH

	Author: Ferdinand Hoppe
	Date: 02/11/2022
	SDK: 2023.100

	Demonstrates how to handle color spaces, profiles, and formats in the Image API.
*/

#ifndef EXAMPLES_COLOR_MANAGEMENT_H__
#define EXAMPLES_COLOR_MANAGEMENT_H__

#include "c4d_basedocument.h"
#include "maxon/apibase.h"
#include "c4d_symbols.h"

/// @brief Instantiate color profiles from the builtin color spaces and pixel formats.
maxon::Result<void> GetColorProfilesFromColorSpaces(
	maxon::HashMap<maxon::String, maxon::ColorProfile>& profiles);

/// @brief Instantiate color profiles from ICC color profile files and the builtin pixel formats.
maxon::Result<void> GetColorProfilesFromFile(
	maxon::HashMap<maxon::String, maxon::ColorProfile>& collection);

/// @brief Read color profile metadata such as description strings and supported pixel formats.
maxon::Result<void> GetColorProfileMetadata(
	const maxon::HashMap<maxon::String, maxon::ColorProfile>& profileCollection);

/// @brief Write a color profile object to an ICC color profile file on disk.
/// @note This can also be done with the builtin profiles or any maxon::ColorProfileInterface
/// reference exposed in the APIs. With Cinema 4D 2023.2, this will make it for example possible
/// to serialize an OCIO Display + View transform to disk.
maxon::Result<void> WriteColorProfileToFile();

/// @brief Access builtin pixel formats and their associated metadata, such as formatting groups, 
/// number of channels, and memory layout per channel.
maxon::Result<void> GetPixelFormats();

/// @brief Convert color data pixel by pixel with color profiles and/or pixel formats.
/// @details This pixel-by-pixel approach is not (considerably) less efficient than converting data
/// in blocks and in fact the dominant form of usage within our own APIs. The advantage is that 
/// with this approach one can be less verbose in setting up the conversion handlers.
maxon::Result<void> ConvertSinglePixelWithColorProfile(
	const maxon::HashMap<maxon::String, maxon::ColorProfile>& collection);

/// @brief Convert color data in chunks of pixels with color profiles and/or pixel formats.
/// @details This example also picks up the linear to non-linear space conversions example 
/// `[0.1, 0.2, 0.5, 0.6]` of the Color Management Manual.
maxon::Result<void> ConvertManyPixelWithColorProfile();

/// @brief Read color data from a bitmap to a buffer and convert this buffer with color profiles
/// and/or pixel formats.
maxon::Result<void> ConvertTextureWithColorProfile(
	const maxon::HashMap<maxon::String, maxon::ColorProfile>& collection);

/// @brief Convert colors between common color representations such as RGB, HSL, and CMYK.
maxon::Result<void> ConvertColorWithUtils();

#endif // EXAMPLES_COLOR_MANAGEMENT_H__
