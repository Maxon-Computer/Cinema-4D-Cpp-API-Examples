CONTAINER Oobjecttree
{
	NAME Oobjecttree;
	INCLUDE Obase;

	GROUP ID_OBJECTPROPERTIES
	{
		// A four column tree view with headers, icons, that allows objects to be dragged into it.
		ITEMTREE OBJECTTREE_ITEMTREE
		{
			SCALE_H; FOLDERS; ICON; HEADERS; COLUMNS 4; DRAGDROP; ACCEPT { Obase; }
		}

		GROUP
		{
			COLUMNS 3;

			// The menu icon bar. Under the hood it is just a long parameter, where we do not even
			// set a custom GUI. All the work happens at runtime in GetDDescription.
			LONG OBJECTTREE_MENU_ITEMS { ANIM OFF; }
			BUTTON OBJECTTREE_ADD_EXAMPLE { }
		}
	}
}
