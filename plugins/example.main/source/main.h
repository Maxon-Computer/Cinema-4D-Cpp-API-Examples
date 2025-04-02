#ifndef MAIN_H__
#define MAIN_H__

#include "ge_prepass.h"
#include "c4d_plugin.h"

namespace cinema
{

class BaseDocument;
class AtomArray;

} // namespace cinema

cinema::Bool RegisterBLWSimpleGenerator();
cinema::Bool RegisterBLWPluginLicenseDialog();
cinema::Bool RegisterCustomErrorExample();
cinema::Bool RegisterSN();
cinema::Bool RegisterGradient();
cinema::Bool RegisterBitmap();
cinema::Bool RegisterMandelbrot();
cinema::Bool RegisterSimpleMaterial();
cinema::Bool RegisterParticleVolume();
cinema::Bool RegisterProgressTest();
cinema::Bool RegisterMenuTest();
cinema::Bool RegisterAsyncTest();
cinema::Bool RegisterActiveObjectDlg();
cinema::Bool RegisterPGPTest();
cinema::Bool RegisterListView();
cinema::Bool RegisterSubDialog();
cinema::Bool RegisterSpherify();
cinema::Bool RegisterRoundedTube();
cinema::Bool RegisterAssetContainerObjectData();
cinema::Bool RegisterTriangulate();
cinema::Bool RegisterVPTest();
cinema::Bool RegisterVPInvertImage();
cinema::Bool RegisterBlinker();
cinema::Bool RegisterAtomObject();
cinema::Bool RegisterCircle();
cinema::Bool RegisterGLTestMaterial();
cinema::Bool RegisterSTL();
cinema::Bool RegisterLookAtCamera();
cinema::Bool RegisterGravitation();
cinema::Bool RegisterPrimitiveTool();
cinema::Bool RegisterMorphMixer();
cinema::Bool RegisterVPVisualizeNormals();
cinema::Bool RegisterVPVisualizeChannel();
cinema::Bool RegisterVPReconstruct();
cinema::Bool RegisterExampleDataType();
cinema::Bool RegisterMemoryStat();
cinema::Bool RegisterEdgeCutTool();
cinema::Bool RegisterPickObjectTool();
cinema::Bool RegisterReverseNormals();
cinema::Bool RegisterLayerShaderBrowser();
cinema::Bool RegisterPainterSaveTest();
cinema::Bool RegisterRandomFalloff();
cinema::Bool RegisterNoiseEffector();
cinema::Bool RegisterDropEffector();
cinema::Bool RegisterDeformerObject();
cinema::Bool RegisterForceObject();
cinema::Bool RegisterCollisionObject();
cinema::Bool RegisterConstraintObject();
cinema::Bool RegisterGrassObject();
cinema::Bool RegisterShader();
cinema::Bool RegisterVideopost();
cinema::Bool RegisterStylingTag();
cinema::Bool RegisterRenderingTag();
cinema::Bool RegisterGeneratorObject();
cinema::Bool RegisterExampleSNHook();
void MiscTest();
void MiscDelegateTest();
void MaxonArrayTest();
void MoveCopyConstructorSample();
cinema::Bool RegisterPythonRegexCommand();
cinema::Bool RegisterSculptingTool();
cinema::Bool RegisterSculpt();
cinema::Bool RegisterSculptPullBrush();
cinema::Bool RegisterSculptCubesBrush();
cinema::Bool RegisterSculptDrawPolyTool();
cinema::Bool RegisterSculptSelectionBrush();
cinema::Bool RegisterSculptGrabBrush();
cinema::Bool RegisterSculptDrawPolyBrush();
cinema::Bool AddUndo(cinema::BaseDocument* doc, cinema::AtomArray* arr, cinema::UNDOTYPE type);
cinema::Bool RegisterSnapTool();
cinema::Bool RegisterSculptDeformer();
cinema::Bool RegisterSculptModifiers();
cinema::Bool RegisterSculptBrushTwist();
cinema::Bool RegisterSculptBrushMultiStamp();
cinema::Bool RegisterSnapDataNullSnap();
cinema::Bool RegisterCustomGUIString();
cinema::Bool RegisterCustomDatatypeCustomGUI();
cinema::Bool RegisterExampleDialogCommand();
cinema::Bool RegisterObjectDynamicDescription();
cinema::Bool RegisterSculptBrushSpline();
cinema::Bool RegisterGetSetDParameterExample();
cinema::Bool RegisterTakeTestCommmands();
cinema::Bool RegisterPolygonReductionTest();
cinema::Bool RegisterObjectHyperFileExample();
cinema::Bool RegisterPlaneByPolygons();
cinema::Bool RegisterGreekTemple();
cinema::Bool RegisterHeartShape();
cinema::Bool RegisterLatticePlane();
cinema::Bool RegisterLoftedMesh();
cinema::Bool RegisterPorcupine();
cinema::Bool RegisterRevolvedMesh();
cinema::Bool RegisterRuledMesh();
cinema::Bool RegisterParticlesShuffling();
cinema::Bool RegisterVertexHandle();
cinema::Bool RegisterCustomDataTagDescription();
cinema::Bool RegisterCustomDataTagCommand();

cinema::Bool RegisterPaintTool();
cinema::Bool RegisterPaintObject();

cinema::Bool RegisterPolyExample();
void CommandLineRendering(cinema::C4DPL_CommandLineArgs* args);

#endif // MAIN_H__
