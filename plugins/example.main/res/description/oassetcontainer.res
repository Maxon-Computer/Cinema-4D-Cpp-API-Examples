CONTAINER Oassetcontainer
{
	NAME Oassetcontainer;
	INCLUDE Obase;

	GROUP ID_OBJECTPROPERTIES
	{
		// The selection for the asset the asset container will return as its cache - i.e., the output
		// of the asset container visible in the scene.
		LONG ASSETCONTAINER_SELECTION 
		{ 
			CYCLE
			{
				ASSETCONTAINER_SELECTION_NONE;
			}
		}

		// A link and a button to add new assets to the asset container which can then be selected by
		// the user with the ASSETCONTAINER_ASSET parameter.
		GROUP
		{
			COLUMNS 2;
			LINK ASSETCONTAINER_SOURCE { ACCEPT { Obase; } }
			BUTTON ASSETCONTAINER_ADD {}
		}
	}
}