#include "customnode-command.h"
#include "customnode-customnodespace_descriptions.h"

#include "maxon/datadescription_ui.h"
#include "maxon/graph.h"
#include "maxon/datadescription_nodes.h"
#include "maxon/node_undo.h"

#include "maxon/threadsaferef.h"
#include "maxon/weakref.h"
#include "maxon/conditionvariable.h"

#include "c4d_basematerial.h"
#include "maxon/nodesgraph.h"
#include "maxon/thread.h"

using namespace cinema;

namespace maxonsdk
{

// We manage command activity in a simple static thread-safe data structure.
using CommandsInProcessing = maxon::HashSet<maxon::NodePath>;
using CommandsInProcessingRef = maxon::StrongRef<CommandsInProcessing>;
static maxon::ThreadSafeRef<CommandsInProcessingRef> g_commandsInProcess;

maxon::Result<maxon::DESCRIPTIONMESSAGECHECKFLAGS> CustomNodeCommand::CommandCheck(const maxon::DataDictionary& userData)
{
	iferr_scope;

	// We unpack the message and extract the instance of the node.
	maxon::GraphModelRef graphModel = userData.Get(maxon::ARGUMENTS::NODECORE::GRAPHMODEL) iferr_return;
	maxon::NodePath nodePath = userData.Get(maxon::ARGUMENTS::NODECORE::NODEPATH) iferr_return;
	maxon::GraphNode node = graphModel.GetNode(nodePath);

	MAXON_SCOPE // We check whether this node is being processed currently.
	{
		CommandsInProcessingRef inProcessingRef = g_commandsInProcess;
		CheckState(inProcessingRef != nullptr);

		const maxon::Bool isBeingProcessed = inProcessingRef->Contains(nodePath);
		if (isBeingProcessed == true)
		{
			return maxon::DESCRIPTIONMESSAGECHECKFLAGS::VISIBLE;
		}
	}

	// Non-processed nodes get an enabled button.
	return maxon::DESCRIPTIONMESSAGECHECKFLAGS::VISIBLE | maxon::DESCRIPTIONMESSAGECHECKFLAGS::ENABLED;
}

maxon::Result<void> CustomNodeCommand::CommandExecute(const maxon::DataDictionary& userData, maxon::DataDictionary& multiSelectionStorage)
{
	iferr_scope;
	
	// We unpack the message and extract the instance of the node.
	maxon::GraphModelRef graphModel = userData.Get(maxon::ARGUMENTS::NODECORE::GRAPHMODEL) iferr_return;
	maxon::NodePath nodePath = userData.Get(maxon::ARGUMENTS::NODECORE::NODEPATH) iferr_return;

	MAXON_SCOPE // We print some info to the console
	{
		maxon::nodes::NodesGraphModelRef nodeGraph = maxon::Cast<maxon::nodes::NodesGraphModelRef>(graphModel);
		BaseMaterial* baseMaterial = NodeMaterial::GetMaterial(nodeGraph);
		DiagnosticOutput("CustomNodeCommand::CommandExecute | Graph: '@' | Material '@'", nodeGraph, baseMaterial);
	}

	MAXON_SCOPE // We mark this node as in-process.
	{
		CommandsInProcessingRef inProcessingRef = g_commandsInProcess;
		CheckState(inProcessingRef != nullptr);
		inProcessingRef->Insert(nodePath) iferr_return;
	}

	MAXON_SCOPE // We simulate a certain computation time and update the node later.
	{
		maxon::WeakRef<maxon::GraphModelRef> weakGraphModel = graphModel;

		maxon::JobRef processingJob = maxon::JobRef::Create(
		[weakGraphModel, nodePath]() -> maxon::Result<void>
		{
			iferr_scope;

			// We block this thread for a longer amount of time.
			// In the real world, this may be replaced by file loading or baking, or else.
			maxon::ConditionVariableRef wait = maxon::ConditionVariableRef::Create() iferr_return;
			wait.Wait(maxon::Seconds(1), maxon::WAITMODE::DEFAULT);
					 
			// We're done processing and un-mark this node.
			CommandsInProcessingRef inProcessingRef = g_commandsInProcess;
			if (inProcessingRef != nullptr)
				inProcessingRef->Erase(nodePath);

			// For safety, we apply the change to the node graph inside the main thread.
			maxon::ExecuteOnMainThread(
			[weakGraphModel, nodePath]()
			{
				iferr_scope_handler
				{
					err.CritStop();
					return;
				};

				maxon::GraphModelRef graphModel = weakGraphModel;
				if (graphModel == nullptr)
					return;

				// We simply invalidate the node to trigger a preview computation.
				maxon::DataDictionary noUndo;
				noUndo.Set(maxon::nodes::UndoMode, maxon::nodes::UNDO_MODE::NONE) iferr_return;
				maxon::GraphTransaction transaction = graphModel.BeginTransaction(noUndo) iferr_return;
				maxon::GraphNode node = graphModel.GetNode(nodePath);
				maxon::GraphNode valuePort = node.GetInputs().FindChild(maxonexample::NODE::USERNODE::COLORA) iferr_return;
				valuePort.TouchValue(maxon::DESCRIPTION::DATA::BASE::DEFAULTVALUE.Get()) iferr_return;
				transaction.Commit() iferr_return;

			}, maxon::WAITMODE::DONT_WAIT);

			return maxon::OK;
		}) iferr_return;

		processingJob.Enqueue(maxon::JobQueueInterface::GetServiceIOQueue());
	}

	return maxon::OK;
}

maxon::Result<void> CustomNodeCommand::InitializeProcessing()
{
	iferr_scope;

	CommandsInProcessingRef inProcessingRef = NewObj(CommandsInProcessing) iferr_return;
	g_commandsInProcess = inProcessingRef;

	return maxon::OK;
}

void CustomNodeCommand::ShutdownProcessing()
{
	g_commandsInProcess = nullptr;
}

MAXON_INITIALIZATION([]() -> maxon::Result<void>
{
	iferr_scope;

	maxon::DataDescriptionDefinitionDatabaseInterface::RegisterMessage(maxonexample::NODE::USERNODE::GetId(), maxonexample::NODE::USERNODE::CUSTOMCOMMAND, maxon::Id(),
		maxon::DescriptionMessageFunction(maxon::DESCRIPTION::UI::BASE::COMMANDCONTEXT.ENUM_NODECORE, CustomNodeCommand::CommandCheck,
			maxon::DescriptionMessageBuildSubMenu(), CustomNodeCommand::CommandExecute)) iferr_return;

	return maxon::OK;
},
nullptr);

} // namespace maxonsdk
