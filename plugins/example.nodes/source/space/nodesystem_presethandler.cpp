#include "nodesystem_presethandler.h"

#include "maxon/datadescription_nodes.h"
#include "maxon/derived_nodeattributes.h"
#include "customnode-customnodespace_descriptions.h"

namespace maxonsdk
{

NodeSystemPresetChangedHandler::NodeSystemPresetChangedHandler()
{

}

NodeSystemPresetChangedHandler::~NodeSystemPresetChangedHandler()
{

}

maxon::Result<void> NodeSystemPresetChangedHandler::HandleGraphChanged(const maxon::nodes::NodesGraphModelRef& graph)
{
	iferr_scope_handler
	{
		err.CritStop();
		return err;
	};

	maxon::TimeStamp currentTimeStamp = graph.GetModificationStamp();
	finally
	{
		_lastTimeStamp = currentTimeStamp;
	};

	maxon::BaseArray<maxon::NodePath> endNodesToMutate;

	// We can do very advanced checks here, even asynchronously if we properly clone the graph.
	maxon::nodes::NodeSystemManagerRef manager = graph.GetManager();
	manager.GetMainView().GetModificationsSince(_lastTimeStamp,
	[&endNodesToMutate](const maxon::GraphNode& node, maxon::GraphModelInterface::MODIFIED mod) -> maxon::Result<maxon::Bool>
	{
		iferr_scope;

		// The filter we apply here specifically ignores everything but the material preset input to the end node.

		const maxon::Bool isDataAttribute = (mod & (maxon::GraphModelInterface::MODIFIED::DATA_ATTRIBUTE_MASK)) != maxon::GraphModelInterface::MODIFIED::NONE;
		if (isDataAttribute == false)
			return true;

		const maxon::IdAndVersion assetId = node.GetValue(maxon::nodes::AssetId).GetValueOrDefault() iferr_return;
		const maxon::Id& nodeId = assetId.Get<0>();
		if (nodeId != maxonexample::NODE::ENDNODE::GetId())
			return true;

		iferr (maxon::nodes::Port port = maxon::nodes::ToPort(node))
		{
			return true;
		}

		const maxon::InternedId portId = port.GetId();
		if (portId != maxonexample::NODE::ENDNODE::MATERIALPRESET)
			return true;

		const maxon::InternedId presetType = node.GetEffectivePortValue<maxon::InternedId>().GetValueOrDefault() iferr_return;
		if (presetType.IsEmpty() == true || presetType == maxonexample::NODE::ENDNODE::MATERIALPRESET_ENUM_NONE)
			return true;

		endNodesToMutate.Append(node.GetPath()) iferr_return;

		return true;
	}) iferr_return;

	if (endNodesToMutate.IsEmpty() == true)
		return maxon::OK;

	MAXON_SCOPE // Mutate the node graph on the main thread.
	{
		maxon::WeakRef<maxon::nodes::NodeSystemManagerRef> weakManager = manager;
		maxon::JobRef::Enqueue([weakManager, endNodesToMutate{ std::move(endNodesToMutate) }]()
		{
			iferr_scope_handler
			{
				err.CritStop();
				return;
			};
			maxon::nodes::NodeSystemManagerRef manager = weakManager;
			if (manager == nullptr)
				return;
			const maxon::nodes::NodesGraphModelRef& graph = manager.GetMainView();

			maxon::GraphTransaction gt = graph.BeginTransaction() iferr_return;

			// Apply the preset to the nodes.
			for (const maxon::NodePath& nodePath : endNodesToMutate)
			{
				maxon::GraphNode presetInput = graph.GetNode(nodePath);
				ApplyMaterialPreset(presetInput) iferr_return;
			}
			gt.Commit() iferr_return;
		}, maxon::JobQueueInterface::GetMainThreadQueue()) iferr_return;
	}
	
	return maxon::OK;
}

maxon::Result<void> NodeSystemPresetChangedHandler::ApplyMaterialPreset(maxon::GraphNode& presetInput)
{
	iferr_scope;

	// Resetting the preset value ensures that we do not consume this change event again.
	// Alternatively, we could attach some further meta data to mark the change to be consumed.
	// This would allow us to early-out in the change handler above, avoiding to re-consume one change event.

	maxon::GraphNode inputs = presetInput.GetParent() iferr_return;
	const maxon::InternedId presetType = presetInput.GetEffectivePortValue<maxon::InternedId>().GetValueOrDefault() iferr_return;

	// Note that if this preset port is propagated through a group or an asset, this value assigment will not suffice.
	// We would have to follow back the inputs for the presetInput port and mutate there.
	presetInput.SetPortValue(maxon::InternedId(maxonexample::NODE::ENDNODE::MATERIALPRESET_ENUM_NONE)) iferr_return;

	switch (ID_SWITCH(presetType))
	{
		case ID_CASE(maxonexample::NODE::ENDNODE::MATERIALPRESET_ENUM_GLASS):
		{
			ApplyGlassPreset(inputs) iferr_return;
			break;
		}
		case ID_CASE(maxonexample::NODE::ENDNODE::MATERIALPRESET_ENUM_PLASTIC):
		{
			ApplyPlasticPreset(inputs) iferr_return;
			break;
		}
		case ID_CASE(maxonexample::NODE::ENDNODE::MATERIALPRESET_ENUM_METAL):
		{
			ApplyMetalPreset(inputs) iferr_return;
			break;
		}
		case ID_CASE(maxonexample::NODE::ENDNODE::MATERIALPRESET_ENUM_LAVA):
		{
			ApplyLavaPreset(inputs) iferr_return;
			break;
		}
		case ID_CASE(maxonexample::NODE::ENDNODE::MATERIALPRESET_ENUM_NONE):
		{
			// There's nothing we need to do.
			break;
		}
		default:
		{
			CriticalStop("Unsupported material preset type '@' in node '@'", presetType, presetInput);
			break;
		}
	}
	return maxon::OK;
}


maxon::Result<void> NodeSystemPresetChangedHandler::ApplyPlasticPreset(maxon::GraphNode& inputs)
{
	iferr_scope;

	inputs.FindChild(maxonexample::NODE::ENDNODE::METAL).SetPortValue(maxon::Bool(false)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SPECULARCOLOR).SetPortValue(maxon::Color(1.0, 1.0, 1.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SPECULARCOLORINTENSITY).SetPortValue(maxon::Float(1.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SPECULARROUGHNESS).SetPortValue(maxon::Float(0.15)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SPECULARIOR).SetPortValue(maxon::Float(1.46)) iferr_return;

	inputs.FindChild(maxonexample::NODE::ENDNODE::BASECOLOR).SetPortValue(maxon::Color(0.4, 1.0, 0.1)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::BASECOLORINTENSITY).SetPortValue(maxon::Float(1.0)) iferr_return;

	inputs.FindChild(maxonexample::NODE::ENDNODE::EMISSIONCOLOR).SetPortValue(maxon::Color(0.0, 0.0, 0.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::EMISSIONCOLORINTENSITY).SetPortValue(maxon::Float(0.0)) iferr_return;

	inputs.FindChild(maxonexample::NODE::ENDNODE::REFRACTIONINTENSITY).SetPortValue(maxon::Float(0.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SURFACEALPHA).SetPortValue(maxon::Float(1.0)) iferr_return;

	return maxon::OK;
}

maxon::Result<void> NodeSystemPresetChangedHandler::ApplyGlassPreset(maxon::GraphNode& inputs)
{
	iferr_scope;

	inputs.FindChild(maxonexample::NODE::ENDNODE::METAL).SetPortValue(maxon::Bool(false)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SPECULARCOLOR).SetPortValue(maxon::Color(1.0, 1.0, 1.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SPECULARCOLORINTENSITY).SetPortValue(maxon::Float(1.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SPECULARROUGHNESS).SetPortValue(maxon::Float(0.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SPECULARIOR).SetPortValue(maxon::Float(1.52)) iferr_return;

	inputs.FindChild(maxonexample::NODE::ENDNODE::BASECOLOR).SetPortValue(maxon::Color(1.0, 1.0, 1.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::BASECOLORINTENSITY).SetPortValue(maxon::Float(1.0)) iferr_return;

	inputs.FindChild(maxonexample::NODE::ENDNODE::EMISSIONCOLOR).SetPortValue(maxon::Color(0.0, 0.0, 0.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::EMISSIONCOLORINTENSITY).SetPortValue(maxon::Float(0.0)) iferr_return;

	inputs.FindChild(maxonexample::NODE::ENDNODE::REFRACTIONINTENSITY).SetPortValue(maxon::Float(1.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SURFACEALPHA).SetPortValue(maxon::Float(1.0)) iferr_return;

	return maxon::OK;
}

maxon::Result<void> NodeSystemPresetChangedHandler::ApplyMetalPreset(maxon::GraphNode& inputs)
{
	iferr_scope;

	inputs.FindChild(maxonexample::NODE::ENDNODE::METAL).SetPortValue(maxon::Bool(true)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SPECULARCOLOR).SetPortValue(maxon::Color(1.0, 1.0, 1.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SPECULARCOLORINTENSITY).SetPortValue(maxon::Float(1.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SPECULARROUGHNESS).SetPortValue(maxon::Float(0.02)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SPECULARIOR).SetPortValue(maxon::Float(2.5)) iferr_return;

	inputs.FindChild(maxonexample::NODE::ENDNODE::BASECOLOR).SetPortValue(maxon::Color(0.4, 1.0, 0.1)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::BASECOLORINTENSITY).SetPortValue(maxon::Float(1.0)) iferr_return;

	inputs.FindChild(maxonexample::NODE::ENDNODE::EMISSIONCOLOR).SetPortValue(maxon::Color(0.0, 0.0, 0.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::EMISSIONCOLORINTENSITY).SetPortValue(maxon::Float(0.0)) iferr_return;

	inputs.FindChild(maxonexample::NODE::ENDNODE::REFRACTIONINTENSITY).SetPortValue(maxon::Float(0.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SURFACEALPHA).SetPortValue(maxon::Float(1.0)) iferr_return;

	return maxon::OK;
}

maxon::Result<void> NodeSystemPresetChangedHandler::ApplyLavaPreset(maxon::GraphNode& inputs)
{
	iferr_scope;

	inputs.FindChild(maxonexample::NODE::ENDNODE::METAL).SetPortValue(maxon::Bool(false)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SPECULARCOLOR).SetPortValue(maxon::Color(0.0, 0.0, 0.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SPECULARCOLORINTENSITY).SetPortValue(maxon::Float(0.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SPECULARROUGHNESS).SetPortValue(maxon::Float(0.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SPECULARIOR).SetPortValue(maxon::Float(1.0)) iferr_return;

	inputs.FindChild(maxonexample::NODE::ENDNODE::BASECOLOR).SetPortValue(maxon::Color(0.16, 0.14, 0.11)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::BASECOLORINTENSITY).SetPortValue(maxon::Float(1.0)) iferr_return;

	inputs.FindChild(maxonexample::NODE::ENDNODE::EMISSIONCOLOR).SetPortValue(maxon::Color(1.0, 0.51, 0.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::EMISSIONCOLORINTENSITY).SetPortValue(maxon::Float(1.0)) iferr_return;

	inputs.FindChild(maxonexample::NODE::ENDNODE::REFRACTIONINTENSITY).SetPortValue(maxon::Float(0.0)) iferr_return;
	inputs.FindChild(maxonexample::NODE::ENDNODE::SURFACEALPHA).SetPortValue(maxon::Float(1.0)) iferr_return;

	return maxon::OK;
}

} // namespace maxonsdk
