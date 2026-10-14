/*
	example.2024_migration
	(C) MAXON Computer GmbH, 2023

	Author: Ferdinand Hoppe
	Date: 22/05/2023

	Contains boilerplate code to register and run the 2024 migration examples. 

	Does not have to be read or understood when seeking information about 2024 API changes.
*/
#ifndef _MIGRATION_EXAMPLE_PLUGIN_H__
#define _MIGRATION_EXAMPLE_PLUGIN_H__

#include "c4d_commandplugin.h"
#include "c4d_general.h"

#include "maxon/apibase.h"

/// @brief Provides the command to run the minor 2024 migration examples.
class ChangeExamplesCommand : public cinema::CommandData
{
	INSTANCEOF(ChangeExamplesCommand, cinema::CommandData)

public:
	static ChangeExamplesCommand* Alloc() { return NewObjClear(ChangeExamplesCommand); }

	/// @brief Runs all code examples defined in minor_examples.cpp.
	virtual cinema::Bool Execute(cinema::BaseDocument* doc, cinema::GeDialog* parentManager);
};

/// Called to register the minor examples command plugin and the Oboundingbox object.
cinema::Bool RegisterChangeExamplesCommand();
cinema::Bool RegisterBoundingBoxObject();

#endif // _MIGRATION_EXAMPLE_PLUGIN_H__
