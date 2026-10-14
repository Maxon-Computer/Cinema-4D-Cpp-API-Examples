TOOLHINTS
{
	450000293 // The tool for which these hints are defined (ID_SAMPLE_TOOLHINT_TOOL)
	{
		// Syntax for a tool hint entry is as follows:
		//
		//   Keys , Mouse, String [, Axis] [, Condition];
		//
		// Keys      - The key which must be pressed to trigger the tool action. Supported values
		//             are NONE, ESC, ALT, SHIFT, CTRL. Key combinations are possible.
		// Mouse     - The mouse button which must be pressed alongside the specified keys to trigger the
		//             tool action. Supported values are NONE, LEFT, MIDDLE, RIGHT.
		// String    - The string identifier for the tool hint text. This must be a string symbol defined
		//             in c4d_symbols.h with matching string definitions in the /res/strings_xx_YY folders.
		// Axis      - Optional axis constraint for the tool action. Supported values include HORIZONTAL 
		//             and VERTICAL.
		// Condition - Optional condition of document modes which must be met for the tool hint 
		//             to be displayed. Supported values include RESTRICT and EXCLUDE, followed by 
		//             the relevant document mode symbol.
	
		{ NONE }, NONE, IDS_EXAMPLE_HINT_BASE;
		{ ESC }, NONE, IDS_EXAMPLE_HINT_ESCAPE;
		{ NONE }, LEFT, IDS_EXAMPLE_HINT_LEFT_MOUSE_BUTTON;
		{ NONE }, MIDDLE, IDS_EXAMPLE_HINT_MIDDLE_MOUSE_BUTTON;
		{ NONE }, RIGHT, IDS_EXAMPLE_HINT_RIGHT_MOUSE_BUTTON;
		{ ALT }, LEFT, 	IDS_EXAMPLE_HINT_ALT_LEFT_MOUSE_BUTTON;
		{ SHIFT }, LEFT, IDS_EXAMPLE_HINT_SHIFT_LEFT_MOUSE_BUTTON;
		{ SHIFT, CTRL }, RIGHT, IDS_EXAMPLE_HINT_SHIFT_CTRL_RIGHT_MOUSE_BUTTON;
		{ NONE }, LEFT, IDS_EXAMPLE_HINT_ONLY_POINTS, RESTRICT { Mpoints };
		{ NONE }, LEFT, IDS_EXAMPLE_HINT_ONLY_EDGES_AND_POLYGONS, RESTRICT { Medges, Mpolygons };
		{ NONE }, LEFT, IDS_EXAMPLE_HINT_NOT_IN_EDGE_MODE, EXCLUDE { Medges };
		{ NONE }, LEFT, IDS_EXAMPLE_HINT_LEFT_MOUSE_BUTTON_HORIZONTAL, HORIZONTAL;
		{ CTRL }, RIGHT, IDS_EXAMPLE_HINT_RIGHT_MOUSE_BUTTON_VERTICAL, VERTICAL;
	}

	// 450000294 (another tool for which hints can be defined)
	// { ... }

	// ...
}
