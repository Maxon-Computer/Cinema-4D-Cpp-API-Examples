CONTAINER ToolDisplaycontrol
{
  NAME ToolDisplaycontrol;
	INCLUDE ToolBase;

  GROUP MDATA_MAINGROUP
  {
		GROUP
		{
			LONG DISPLAYCONTROLTOOL_MODE
			{
				CYCLE
				{
					PER_OBJECT;
					PER_VERTEX;
					PER_VERTEX_PER_POLYGON;
				}
			}
		}
	}
}
