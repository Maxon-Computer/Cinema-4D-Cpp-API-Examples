// This is the main file of the Cinema 4D SDK
//
// When you create your own projects much less code is needed (this file is rather long as it tries to show all kinds of different uses).
//
// An empty project simply looks like this:
//
// #include "c4d.h"
//
// Bool PluginStart()
// {
//   ...do or register something...
//   return true;
// }
//
// void PluginEnd()
// {
// }
//
// Bool PluginMessage(Int32 id, void *data)
// {
//   return false;
// }
//

#include "main.h"
#include "registeradvancedpaint.h"
#include "microsdk/business_logic.h"
#include "microsdk/user_interface.h"
#include "customerror/customerror_interface.h"
#include "blw_crypt.h"
#include "blw_checklicense.h"

// #define DO_LICENSING_CHECK

CheckLicense *g_checkLic = nullptr;

namespace cinema
{

Bool PluginStart()
{
	#ifdef DO_LICENSING_CHECK
	// check for validity of CheckLicense global instance 
	if (nullptr == g_checkLic)
	{
		CriticalOutput("BLW >> Failed to allocate CheckLicense instance");
		return false;
	}
	
	// check for license validity
	iferr (g_checkLic->AnalyzeLicense())
	{
		return false;
	}
	
	// if license is valid than register the plugin
	if (g_checkLic->IsLicensed())
	{
		if (!RegisterBLWSimpleGenerator())
			return false;
	}
	#endif

	microsdk::RegisterCubeCommand();

	if (!RegisterCustomErrorExample())
		return false;

	// register the CommandData to generate the plugin license dialog
	if (!RegisterBLWPluginLicenseDialog())
		return false;

	// shader plugin examples
	if (!RegisterGradient())
		return false;
	if (!RegisterBitmap())
		return false;
	if (!RegisterMandelbrot())
		return false;
	if (!RegisterSimpleMaterial())
		return false;
	if (!RegisterParticleVolume())
		return false;

	// menu plugin examples
	if (!RegisterProgressTest())
		return false;	
	if (!RegisterMenuTest())
		return false;
	if (!RegisterAsyncTest())
		return false;
	if (!RegisterActiveObjectDlg())
		return false;
	if (!RegisterListView())
		return false;
	if (!RegisterSubDialog())
		return false;
	if (!RegisterLayerShaderBrowser())
		return false;
	if (!RegisterPGPTest())
		return false;

	// filter plugin examples
	if (!RegisterSTL())
		return false;
	if (!RegisterSculpt())
		return false;

	// object plugin examples
	if (!RegisterSpherify())
		return false;
	if (!RegisterRoundedTube())
		return false;
	if (!RegisterGravitation())
		return false;
	if (!RegisterAtomObject())
		return false;
	if (!RegisterCircle())
		return false;
	if (!RegisterTriangulate())
		return false;
	if (!RegisterMorphMixer())
		return false;

	// tool plugin examples
	if (!RegisterPrimitiveTool())
		return false;
	if (!RegisterEdgeCutTool())
		return false;
	if (!RegisterPickObjectTool())
		return false;
	if (!RegisterReverseNormals())
		return false;
	if (!RegisterDisplayControlTool())
		return false;
	if (!RegisterSnapTool())
		return false;

	// sculpting tool examples
	if (HasFullFeatureSet())
	{
		if (!RegisterSculptDrawPolyTool())
			return false;
		if (!RegisterSculptPullBrush())
			return false;
		if (!RegisterSculptSelectionBrush())
			return false;
		if (!RegisterSculptCubesBrush())
			return false;
		if (!RegisterSculptGrabBrush())
			return false;
		if (!RegisterSculptDrawPolyBrush())
			return false;
		if (!RegisterSculptDeformer())
			return false;
		if (!RegisterSculptModifiers())
			return false;
		if (!RegisterSculptBrushTwist())
			return false;
		if (!RegisterSculptBrushMultiStamp())
			return false;
		if (!RegisterPaintAdvanced())
			return false;
		if (!RegisterSculptBrushSpline())
			return false;
	}

	// animation plugin example
	if (!RegisterBlinker())
		return false;

	// tag / expression plugin examples
	if (!RegisterLookAtCamera())
		return false;

	// video post examples
	if (!RegisterVPTest())
		return false;
	if (!RegisterVPInvertImage())
		return false;
	if (!RegisterVPVisualizeNormals())
		return false;
	if (!RegisterVPReconstruct())
		return false;

	if (!RegisterMemoryStat())
		return false;
	if (!RegisterPainterSaveTest())
		return false;

	// falloff type examples
	if (!RegisterRandomFalloff())
		return false;

	// effector plugin examples, can only be loaded if MoGfx is installed
	if (HasFullFeatureSet())
	{
		if (!RegisterNoiseEffector())
			return false;
		if (!RegisterDropEffector())
			return false;
	}

	// hair examples
	if (!RegisterDeformerObject())
		return false;
	if (!RegisterForceObject())
		return false;
	if (!RegisterCollisionObject())
		return false;
	if (!RegisterConstraintObject())
		return false;
	if (!RegisterGrassObject())
		return false;
	if (!RegisterShader())
		return false;
	if (!RegisterVideopost())
		return false;
	if (!RegisterStylingTag())
		return false;
	if (!RegisterRenderingTag())
		return false;
	if (!RegisterGeneratorObject())
		return false;

	// SnapData example
	if (!RegisterSnapDataNullSnap())
		return false;

	// take system example
	if (!RegisterTakeTestCommmands())
		return false;

	// String custom GUI example
	if (!RegisterCustomGUIString())
		return false;

	// CustomDataType and CustomGUI example
	if (!RegisterCustomDatatypeCustomGUI())
		return false;

	// GeDialog example
	if (!RegisterExampleDialogCommand())
		return false;

	// ObjectData example showing the use of GetDDescription()
	if (!RegisterObjectDynamicDescription())
		return false;

	// ObjectData example showing the use of Get-/SetDParameter() with certain CustomGUIs
	if (!RegisterGetSetDParameterExample())
		return false;

	// Polygon Reduction example
	if (!RegisterPolygonReductionTest())
		return false;

	// Python Regex example
	if (!RegisterPythonRegexCommand())
		return false;

	// ObjectData example showing how to create an approximated circle spline
	if (!RegisterCircleShape())
		return false;

	// ObjectData example demonstrating the use of HyperFile class in Read()/Write()
	if (!RegisterObjectHyperFileExample())
		return false;
		
	// ObjectData example showing how to create a Greek temple starting from basic shapes
	if (!RegisterGreekTemple())
		return false;

	// ObjectData example showing how to create a heart shape spline
	if (!RegisterHeartShape())
		return false;

	// ObjectData example showing how to create a lattice plane modifier
	if (!RegisterLatticePlane())
		return false;

	// ObjectData example showing how to create a lofted mesh
	if (!RegisterLoftedMesh())
		return false;

	// ObjectData example showing how to create a shuffling particles modifier
	if (!RegisterParticlesShuffling())
		return false;

	// ObjectData example showing how to create a simple plane made by polygons
	if (!RegisterPlaneByPolygons())
		return false;

	// ObjectData example showing how to create a "porcupine-like" modifier
	if (!RegisterPorcupine())
		return false;

	// ObjectData example showing how to create a revolved mesh
	if (!RegisterRevolvedMesh())
		return false;

	// ObjectData example showing how to create a ruled mesh
	if (!RegisterRuledMesh())
		return false;

	// ObjectData example showing how to create handles and drag vertex of a triangular polygon
	if (!RegisterVertexHandle())
		return false;

	if (!RegisterAssetContainerObjectData())
		return false;

	// register a custom description for float custom data tag
	if (!RegisterCustomDataTagDescription())
		return false;

	// conversion command for custom data tag
	if (!RegisterCustomDataTagCommand())
		return false;

	if (!RegisterPaintTool())
		return false;
	
	if (!RegisterPaintObject())
		return false;

	if (!RegisterPolyExample())
		return false;
	
	return true;
}

void PluginEnd()
{
	FreePaintAdvanced();

	DeleteObj(g_checkLic);
	g_checkLic = nullptr;
}

Bool PluginMessage(Int32 id, void* data)
{
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if (!g_resource.Init())
				return false;		// don't start plugin without resource

			// register example datatype. This is happening at the earliest possible time
			if (!RegisterExampleDataType())
				return false;

			#ifdef DO_LICENSING_CHECK
			// allocate the global instance to check license validity
			iferr (g_checkLic = NewObj(CheckLicense))
			{
				CriticalOutput("BLW >> Failed to allocate CheckLicense instance [@]", err);
				return false;
			}
			#endif

			return true;

		case C4DMSG_PRIORITY:
			// react to this message to set a plugin priority (to determine in which order plugins are initialized or loaded
			// SetPluginPriority(data, mypriority);
			return true;

		case C4DPL_BUILDMENU:
			// react to this message to dynamically enhance the menu
			// EnhanceMainMenu();
			break;

		case C4DPL_COMMANDLINEARGS:
			// sample implementation of command line rendering:
			// CommandLineRendering(static_cast<C4DPL_CommandLineArgs*>(data));

			// react to this message to react to command line arguments on startup
			/*
			{
				C4DPL_CommandLineArgs *args = (C4DPL_CommandLineArgs*)data;
				Int32 i;

				for (i = 0; i<args->argc; i++)
				{
					if (!args->argv[i]) continue;

					if (!strcmp(args->argv[i],"--help") || !strcmp(args->argv[i],"-help"))
					{
						// do not clear the entry so that other plugins can make their output!!!
						ApplicationOutput("\x01-SDK is here :-)");
					}
					else if (!strcmp(args->argv[i],"-SDK"))
					{
						args->argv[i] = nullptr;
						ApplicationOutput("\x01-SDK executed:-)");
					}
					else if (!strcmp(args->argv[i],"-plugincrash"))
					{
						args->argv[i] = nullptr;
						*(static_cast<Int32*>(0)) = 1234;
					}
				}
			}
			*/
			break;

		case C4DPL_PROGRAM_STARTED:
		{
			// perform some action after the program has started
			microsdk::ExecuteMicroExampleCode();
			
			#ifdef DO_LICENSING_CHECK
			// check for validity of CheckLicense global instance
			if (nullptr == g_checkLic)
			{
				CriticalOutput("BLW >> Failed to allocate CheckLicense instance");
				return false;
			}
			
			if (!g_checkLic->IsLicensed())
			{
				WarningOutput("BLW >> SimpleGenerator license is invalid or not found");
				maxon::Bool generateC4DLicReport = QuestionDialog("No valid license has been found for SimpleGenerator.\n\nExport your Cinema 4D License Information on disk to share with plugin vendor and order a valid license?"_s);
				if (generateC4DLicReport)
				{
					iferr (StoreC4DLicenseReportOnDisk())
					{
						WarningOutput("BLW >> Failed to save Cinema 4D license report on disk");
						return false;
					}
				}
			}
			
			// check for demo mode
			if (g_checkLic->IsDemo())
				MessageDialog("SimpleGenerator is running in demo mode."_s);
			
			// check for demo period
			if (g_checkLic->IsExpired())
				MessageDialog("SimpleGenerator trial period has ended."_s);
			#endif
			
			break;
		}

		case C4DPL_EDITIMAGE:
			/*{
				C4DPL_EditImage *editimage = (C4DPL_EditImage*)data;
				if (!data) break;
				if (editimage->return_processed) break;
				ApplicationOutput("C4DSDK - Edit Image Hook: "+editimage->imagefn->GetString());
				// editimage->return_processed = true; if image was processed
			}*/
			return false;
	}

	return false;
}

} // namespace cinema
