//
//  create_node_material.cpp
//  Copyright © 2022 MAXON Computer GmbH. All rights reserved.
//
// This example show how to create a node material for the standard node space.
// When creating a NodeMaterial, CreateDefaultGraph can be called to add a NodeGraph for a specific node space.
// CreateDefaultGraph will automatically called a function that will define a default state of the nodeGraph.
// In the case of the Standard NodeSpace, it will create a "bsdf node" and an "End node" and connect them.
// This example will create after that two color nodes and a mix node to mix them. The connections will be added.
// After that, the example will group the color nodes and the mix node and create an input port for the group.
// That input port will be connected to a new value node. The purpose is to be able to change the value of the mix node inside
// the group node without opening it.
// Along the example, there are also parts that show how to mute and unmute connections, select and unselect node, port, wires.
// There are also test for checking mute state of connections, or selection state of nodes, ports, wires.

#include "c4d_general.h"
#include "c4d_basedocument.h"
#include "c4d_basematerial.h"
#include "c4d_commanddata.h"

#include "maxon/apibase.h"
#include "maxon/graph.h"
#include "maxon/graph_basics.h"
#include "maxon/nodesgraph.h"
#include "maxon/nodesgraph_helpers.h"
#include "maxon/node_spaces.h"
#include "maxon/nodes_all.h"
#include "maxon/nodesgraph.h"
#include "maxon/nodes_corenodes_base.h"
#include "maxon/datadescription_nodes.h"
#include "maxon/definitions/nodes_utility.h"
#include "maxon/definitions/nodes_math.h"
#include "maxon/graph_helper.h"

using namespace cinema;

namespace maxonsdk
{

static const Int32 g_createcommandid =  1059218;

//----------------------------------------------------------------------------------------
/// Try to find a muted connection between two nodes.
/// @param[in] firstNode					The first node to find a muted connection from.
/// @param[in] secondNode					The second node to find a muted connection from.
/// @param[in] portDirection			The direction where to look for connection.
/// @return												True or false if a muted connection was founded.
//----------------------------------------------------------------------------------------
static	maxon::Result<Bool> FindMutedConnection(const maxon::GraphNode& firstNode, const maxon::GraphNode& secondNode, const maxon::PORT_DIR& portDirection)
	{
		iferr_scope;
		Bool foundMutedConnection = false;
		firstNode.GetConnections(portDirection, [&secondNode, &foundMutedConnection](const maxon::GraphConnection& connection) -> maxon::Result<Bool>
			{
				iferr_scope;
				if (connection.second[maxon::Wires::INHIBIT] == maxon::WIRE_MODE::NORMAL)
				{
					if (connection.first == secondNode)
					{
						foundMutedConnection = true;
					}
				}
				return true;
			}) iferr_return;
		return foundMutedConnection;
	}

//----------------------------------------------------------------------------------------
/// Create the material, insert it inside the document and also create some nodes and connect them.
/// @return												Tuple of the NodeMaterial pointer, and some GraphNode for the next function.
//----------------------------------------------------------------------------------------
static maxon::Result<maxon::Tuple<NodeMaterial*, maxon::HomogenousTupleType<5, maxon::GraphNode>> > CreateMaterial()
{
	
	iferr_scope;
	
	// Create a material and insert it in the doc. Define the node space id as the standard one.
	NodeMaterial* nodeMaterial = static_cast<NodeMaterial*>(BaseMaterial::Alloc(Mmaterial));
	CheckState(nodeMaterial != nullptr);
	const maxon::Id nodeSpaceID = maxon::nodes::MaterialNodeSpaces::Standard.GetId();
	nodeMaterial->CreateDefaultGraph(nodeSpaceID) iferr_return;
	BaseDocument* doc = GetActiveDocument();
	if (doc == nullptr)
		return maxon::NullptrError(MAXON_SOURCE_LOCATION);
	doc->InsertMaterial(nodeMaterial);
	
	// Retrieve the nodeGraph reference itself, this will be use for some helper functions.
	const maxon::nodes::NodesGraphModelRef& nodeGraph = nodeMaterial->GetGraph(nodeSpaceID) iferr_return;
	if (nodeGraph.IsReadOnly())
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "NodeGraph is read only"_s);
	
	// Retrieve the repository from where we will instantiate the nodes.
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetBuiltinRepository();

	// Find the bsdf node so we can later connect nodes to it.
	const maxon::GraphNode root = nodeGraph.GetViewRoot();
	maxon::GraphNode bsdfGraphNode;
	maxon::GraphModelHelper::FindNodesByAssetId(nodeGraph, maxon::Id("net.maxon.render.node.bsdf"), true,
										[&bsdfGraphNode](const maxon::GraphNode& child) -> Bool
										{
										iferr_scope;
										bsdfGraphNode = child;
										return false;
	}) iferr_return;
	if (!bsdfGraphNode.IsValid())
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "could not find the bsdf node"_s);
	
	// Begin a transaction so every modification on the nodeGraph will be applied at the end.
	maxon::GraphTransaction graphTransaction = nodeGraph.BeginTransaction() iferr_return;
	
	// Create a color node, set the dataType of this node, set the value of the input port and select it.
	maxon::nodes::NodeTemplate valueNodeTemplate = maxon::nodes::NodesLib::LoadTemplate(repository, maxon::NODE::TYPE::GetId()) iferr_return;
	maxon::GraphNode colorGraphNode = nodeGraph.AddChild(maxon::Id("Color"), valueNodeTemplate) iferr_return;
	maxon::GraphNode datatypePort = colorGraphNode.GetInputs().FindChild(maxon::NODE::TYPE::DATATYPE) iferr_return;
	datatypePort.SetPortValue(maxon::Id("net.maxon.parametrictype.col<3,float>")) iferr_return;
	maxon::GraphNode valuePort = colorGraphNode.GetInputs().FindChild(maxon::NODE::TYPE::IN) iferr_return;
	valuePort.SetPortValue(maxon::Color(0.5, 0.0, 0.0)) iferr_return;
	maxon::GraphModelHelper::SelectNode(colorGraphNode) iferr_return;
	
	// Add another color node and define a different color.
	maxon::GraphNode colorGraphNode2 = nodeGraph.AddChild(maxon::Id("Color2"), valueNodeTemplate) iferr_return;
	maxon::GraphNode datatypePort2 = colorGraphNode2.GetInputs().FindChild(maxon::NODE::TYPE::DATATYPE) iferr_return;
	datatypePort2.SetPortValue(maxon::Id("net.maxon.parametrictype.col<3,float>")) iferr_return;
	maxon::GraphNode valuePort2 = colorGraphNode2.GetInputs().FindChild(maxon::NODE::TYPE::IN) iferr_return;
	valuePort2.SetPortValue(maxon::Color(0.0, 0.5, 0.0)) iferr_return;

	// Add a Blend node and define the weight of the blend.
	maxon::nodes::NodeTemplate blendNodeTemplate = maxon::nodes::NodesLib::LoadTemplate(repository, maxon::NODE::BLEND::GetId()) iferr_return;
	maxon::GraphNode blendGraphNode = nodeGraph.AddChild(maxon::Id("Blend"), blendNodeTemplate) iferr_return;
	maxon::GraphNode datatypeBlend = blendGraphNode.GetInputs().FindChild(maxon::NODE::BLEND::DATATYPE) iferr_return;
	datatypeBlend.SetPortValue(maxon::Id("net.maxon.parametrictype.col<3,float>")) iferr_return;
	maxon::GraphNode blendValuePort = blendGraphNode.GetInputs().FindChild(maxon::NODE::BLEND::IN3) iferr_return;
	blendValuePort.SetPortValue(Float(0.5)) iferr_return;

	// Wire Nodes, retrieve all the ports needed to connect the node between them.
	maxon::GraphNode colorOutPort = colorGraphNode.GetOutputs().FindChild(maxon::NODE::TYPE::OUT) iferr_return;
	maxon::GraphNode color2OutPort = colorGraphNode2.GetOutputs().FindChild(maxon::NODE::TYPE::OUT) iferr_return;
	maxon::GraphNode colorBlendIn1 = blendGraphNode.GetInputs().FindChild(maxon::NODE::BLEND::IN1) iferr_return;
	maxon::GraphNode colorBlendIn2 = blendGraphNode.GetInputs().FindChild(maxon::NODE::BLEND::IN2) iferr_return;
	maxon::GraphNode colorBlendOut = blendGraphNode.GetOutputs().FindChild(maxon::NODE::BLEND::OUT) iferr_return;
	maxon::GraphNode bsdfIn = bsdfGraphNode.GetInputs().FindChild(maxon::Id("color")) iferr_return;
	// Connect the founded ports.
	colorOutPort.Connect(colorBlendIn1) iferr_return;
	color2OutPort.Connect(colorBlendIn2) iferr_return;
	colorBlendOut.Connect(bsdfIn) iferr_return;

	// Commit the transaction so the modification are applied to the nodeGraph.
	graphTransaction.Commit() iferr_return;
	return maxon::Tuple<NodeMaterial*, maxon::HomogenousTupleType<5, maxon::GraphNode>> (nodeMaterial, {colorGraphNode, colorGraphNode2, colorOutPort, colorBlendIn1, blendGraphNode});
}

//----------------------------------------------------------------------------------------
/// Manipulate a GraphModel by adding nodes, connect them, group them, mute, unmute connections...
/// @param[in] parameter					The node where to create the output port.
/// @return												The error if any.
//----------------------------------------------------------------------------------------
static maxon::Result<void> ManipulatingNodeMaterial(maxon::Tuple<NodeMaterial*, maxon::HomogenousTupleType<5, maxon::GraphNode>>& parameter)
{
	iferr_scope;
	// Retrieve the NodeMaterial and the GraphNodes passed as parameter.
	
	NodeMaterial* nodeMaterial = parameter.GetFirst();
	CheckState(nodeMaterial != nullptr);
	
	maxon::GraphNode colorGraphNode;
	maxon::GraphNode colorGraphNode2;
	maxon::GraphNode colorOutPort;
	maxon::GraphNode colorBlendIn1;
	maxon::GraphNode blendGraphNode;
	Tie(colorGraphNode, colorGraphNode2, colorOutPort, colorBlendIn1, blendGraphNode) = parameter.GetSecond();
	if (! (colorGraphNode.IsValid() && colorGraphNode2.IsValid() && colorOutPort.IsValid() && colorBlendIn1.IsValid() && blendGraphNode.IsValid()))
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Some node are not valid"_s);
	
	const maxon::Id nodeSpaceID = maxon::nodes::MaterialNodeSpaces::Standard.GetId();
	const maxon::nodes::NodesGraphModelRef& nodeGraph = nodeMaterial->GetGraph(nodeSpaceID) iferr_return;
	if (nodeGraph.IsReadOnly())
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "NodeGraph is read only"_s);
	
	// Select some elements so later, some code can be ran to test if those elements are selected.
	maxon::GraphTransaction graphTransaction = nodeGraph.BeginTransaction() iferr_return;
	maxon::GraphModelHelper::SelectNode(colorGraphNode) iferr_return;
	maxon::GraphModelHelper::SelectNode(colorOutPort) iferr_return;
	maxon::GraphModelHelper::SelectConnection(colorOutPort, colorBlendIn1) iferr_return;
	graphTransaction.Commit() iferr_return;

	// Test the Node selection.
	Bool trueNodeSelected = maxon::GraphModelHelper::IsNodeSelected(colorGraphNode);
	if (!trueNodeSelected)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "The True  Node isn't selected"_s);

	graphTransaction = nodeGraph.BeginTransaction() iferr_return;
	maxon::GraphModelHelper::DeselectNode(colorGraphNode) iferr_return;
	graphTransaction.Commit() iferr_return;

	// Test the Connection Selection.
	Bool wireSelected = maxon::GraphModelHelper::IsConnectionSelected(colorOutPort, colorBlendIn1);
	if (!wireSelected)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "The wire isn't selected"_s);

	// Retrieve Selected Nodes (true nodes and port).
	maxon::GraphModelHelper::GetSelectedNodes(nodeGraph, maxon::NODE_KIND::ALL_MASK, [](const maxon::GraphNode& selectedNode) -> maxon::Result<Bool>
		{
			iferr_scope;
			ApplicationOutput("this node is selected @", selectedNode);
			return true;
		}) iferr_return;
	
	// Create an output and input port on Root.
	maxon::GraphNode root = nodeGraph.GetViewRoot();
	graphTransaction = nodeGraph.BeginTransaction() iferr_return;
	maxon::GraphNode rootOutputPort = maxon::GraphModelHelper::CreateOutputPort(root, "mymaterial"_cs, "My Output Port"_s) iferr_return;
	maxon::GraphNode rootInputPort = maxon::GraphModelHelper::CreateInputPort(root, "this is my inputport"_cs, "My Input Port"_s) iferr_return;
	graphTransaction.Commit() iferr_return;

	// Check if we can found a successor of the colorGraphNode. The node blendGraphNode is connected to an output of colorGraphNode so it
	// should be founded by the function as a successor of colorGraphNode.
	Bool foundsuccessor = false;
	maxon::GraphModelHelper::GetDirectSuccessors(colorGraphNode, maxon::NODE_KIND::NODE, [&blendGraphNode, &foundsuccessor](const maxon::GraphNode& next) -> maxon::Result<Bool>
		{
			iferr_scope;
			if (next == blendGraphNode)
				foundsuccessor = true;
			return true;
		}) iferr_return;
	if (!foundsuccessor)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Cannot find the direct successor of Color Node"_s);

	// Retrieve all successors of colorGraphNode will return all nodes connected directly to colorGraphNode and all nodes connected to them.
	maxon::GraphModelHelper::GetAllSuccessors(colorGraphNode, maxon::NODE_KIND::NODE, [&colorGraphNode](const maxon::GraphNode& next) -> maxon::Result<Bool>
		{
			iferr_scope;
			ApplicationOutput("we found successor to @ : @", colorGraphNode, next);
			return true;
		}) iferr_return;
	
	// Check if a node is connected to a port.
	Bool colorConnectedToColorBlendIn1 = maxon::GraphModelHelper::IsConnected(colorGraphNode, colorBlendIn1);
	if (!colorConnectedToColorBlendIn1)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Color node isn't connected to BSDIn port (with object)"_s);

	// Save blend  and color path for later, some function need a path as parameter.
	const maxon::NodePath blendPath = blendGraphNode.GetPath();
	const maxon::NodePath colorPath = colorGraphNode.GetPath();

	// Group the two color nodes and the blend node. An array will be created to store the nodes we want to group.
	maxon::BaseArray<maxon::GraphNode> selection;
	selection.Append(colorGraphNode) iferr_return;
	selection.Append(colorGraphNode2) iferr_return;
	selection.Append(blendGraphNode) iferr_return;
	graphTransaction = nodeGraph.BeginTransaction() iferr_return;
	String groupName = "My nodes grouped"_s;
	maxon::GraphNode myGroup = nodeGraph.MoveToGroup(maxon::nodes::NodeSystem(), maxon::Id("thisismygroup"), selection) iferr_return;
	myGroup.SetValue(maxon::NODE::BASE::NAME, groupName) iferr_return;
	graphTransaction.Commit() iferr_return;
	
	// Add an input port to the group and connect that input port to the factor port of the blend node.
	graphTransaction = nodeGraph.BeginTransaction() iferr_return;
	const maxon::GraphNode groupInputPort = maxon::GraphModelHelper::CreateInputPort(myGroup, "factor"_cs, "Blend Factor"_s) iferr_return;
	blendGraphNode = myGroup.FindInnerNode(blendPath) iferr_return;
	maxon::GraphNode colorBlendIn3 = blendGraphNode.GetInputs().FindChild(maxon::NODE::BLEND::IN3) iferr_return;
	groupInputPort.Connect(colorBlendIn3) iferr_return;
	graphTransaction.Commit() iferr_return;
	
	// Creates a node to be able to manipulate the the blend factor outside the group and sets the dataType of this node.
	graphTransaction = nodeGraph.BeginTransaction() iferr_return;
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetBuiltinRepository();
	maxon::nodes::NodeTemplate blendFactorNodeTemplate = maxon::nodes::NodesLib::LoadTemplate(repository, maxon::NODE::TYPE::GetId()) iferr_return;
	maxon::GraphNode blendFactorGraphNode = nodeGraph.AddChild(maxon::Id("BlendFactor"), blendFactorNodeTemplate) iferr_return;
	maxon::GraphNode blendDataTypePort = blendFactorGraphNode.GetInputs().FindChild(maxon::NODE::TYPE::DATATYPE) iferr_return;
	blendDataTypePort.SetPortValue(maxon::Id("float")) iferr_return;
	maxon::GraphNode blendFactorValuePort = blendFactorGraphNode.GetInputs().FindChild(maxon::NODE::TYPE::IN) iferr_return;
	blendFactorValuePort.SetPortValue(Float(0.5)) iferr_return;
	maxon::GraphNode blendFactorResult = blendFactorGraphNode.GetOutputs().FindChild(maxon::NODE::TYPE::OUT) iferr_return;
	blendFactorResult.Connect(groupInputPort) iferr_return;
	graphTransaction.Commit() iferr_return;

	// Mute connection between blendFactorResult and groupInputPort.
	graphTransaction = nodeGraph.BeginTransaction() iferr_return;
	maxon::GraphModelHelper::MuteConnection(blendFactorResult, groupInputPort) iferr_return;
	graphTransaction.Commit() iferr_return;
	
	// Test if the connection have been muted.
	Bool colorOutPortFoundMutedConnection = FindMutedConnection(blendFactorResult, groupInputPort, maxon::PORT_DIR::OUTPUT) iferr_return;
	if (!colorOutPortFoundMutedConnection)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "No connection muted could be founded"_s);
	
	// Unmuted the connection.
	graphTransaction = nodeGraph.BeginTransaction() iferr_return;
	maxon::GraphModelHelper::UnmuteConnection(blendFactorResult, groupInputPort) iferr_return;
	graphTransaction.Commit() iferr_return;

	// Mute All connections for blendFactorResult in the output direction.
	graphTransaction = nodeGraph.BeginTransaction() iferr_return;
	maxon::GraphModelHelper::MuteAllConnections(blendFactorResult, maxon::PORT_DIR::OUTPUT) iferr_return;
	graphTransaction.Commit() iferr_return;
	
	// Finding node by name can return a lot of nodes. Name are not unique in a nodeGraph.
	maxon::GraphModelHelper::FindNodesByName(nodeGraph, "Norm"_s, maxon::NODE_KIND::PORT_MASK, maxon::PORT_DIR::INPUT, false, [](const maxon::GraphNode& child) -> Bool
		{
			ApplicationOutput("found a Normal port @", child);
			return true;
		}) iferr_return;


	return maxon::OK;
}

// This command can be execute to lanch the example.
class CreateMaterialCommandData : public CommandData
{

	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager)
	{
		iferr_scope_handler
		{
			return false;
		};
		
		maxon::Tuple<NodeMaterial*, maxon::HomogenousTupleType<5, maxon::GraphNode>> parameters = CreateMaterial() iferr_return;
		ManipulatingNodeMaterial(parameters) iferr_return;
		
		return true;
	}
	
};

Bool RegisterCreateMaterialExample();
Bool RegisterCreateMaterialExample()
{
	return RegisterCommandPlugin(g_createcommandid, "C++ SDK - Create Node Material"_s, 0, nullptr, "Create a node material and several nodes"_s, NewObjClear(CreateMaterialCommandData));
	
}
} // namespace maxonsdk
