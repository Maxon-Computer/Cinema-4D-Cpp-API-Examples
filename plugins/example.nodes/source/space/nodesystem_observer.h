#ifndef NODESYSTEM_OBSERVER_H__
#define NODESYSTEM_OBSERVER_H__

#include "maxon/apibase.h"
#include "maxon/errorbase.h"
#include "maxon/nodesgraph.h"
#include "maxon/spinlock.h"
#include "maxon/weakref.h"

namespace maxonsdk
{

// We could create way more complex handlers that for example allow subscribing to particular nodes or ports that change changed.
// We could then propagate these changes to other connected nodes, c.f. hierarchical dirtiness. A similar approach is taken
// for the node previews in the GUI.
class NodeSystemChangedHandler
{
public:

	virtual ~NodeSystemChangedHandler() {}

	virtual maxon::Result<void> HandleGraphChanged(const maxon::nodes::NodesGraphModelRef& graph) = 0;
};
using NodeSystemChangedHandlerRef = maxon::StrongRef<NodeSystemChangedHandler>;

class NodeSystemChangedTicket;

class NodeSystemChangedMonitor;
using NodeSystemChangedMonitorRef = maxon::StrongRef<NodeSystemChangedMonitor>;
using NodeSystemChangedMonitorWeakRef = maxon::WeakRef<NodeSystemChangedMonitorRef>;
class NodeSystemChangedMonitor
{
private:
	explicit NodeSystemChangedMonitor(maxon::UInt64 nodeSystemId);

public:

	~NodeSystemChangedMonitor();

	// We enforce heap allocation for weak referencing.
	static maxon::Result<NodeSystemChangedMonitorRef> Create(maxon::UInt64 nodeSystemId, const maxon::nodes::NodesGraphModelRef& graph);

	maxon::Result<void> RegisterHandler(maxon::UInt64 handlerId, NodeSystemChangedHandlerRef&& changeHandler);

	void RemoveHandler(maxon::UInt64 handlerId);

	// A more advanced system that does require immediate action would collect all changes in a queue 
	// and process some with some well-defined frequency in batch.
	void NotifySystemChanged();

	void NotifySystemDestroyed();

private:
	maxon::FunctionBaseRef _systemDestroyedTicket;
	maxon::FunctionBaseRef _systemChangedTicket;
	maxon::WeakRef<maxon::nodes::NodeSystemManagerRef> _weakManager;
	const maxon::UInt64 _nodeSystemId;

	maxon::Spinlock _lock;
	maxon::DataDictionary _handlers;
};

class NodeSystemObserverManager;
using NodeSystemObserverManagerRef = maxon::StrongRef<NodeSystemObserverManager>;
class NodeSystemObserverManager
{
	friend class NodeSystemChangedTicket;
	friend class NodeSystemChangedMonitor;
public:
	
	static maxon::Result<void> Initialize();

	static void Shutdown();
	
	static maxon::Result<maxon::GenericData> RegisterChangeHandler(const maxon::nodes::NodesGraphModelRef& graph, NodeSystemChangedHandlerRef&& changeHandler);

	protected:

	static void RemoveChangeHandler(maxon::UInt64 nodeSystemId, maxon::UInt64 handlerId);

	static void RemoveMonitor(maxon::UInt64 nodeSystemId);

private:
	
	maxon::Result<maxon::GenericData> _RegisterChangeHandler(const maxon::nodes::NodesGraphModelRef& graph, NodeSystemChangedHandlerRef&& changeHandler);

	void _RemoveChangeHandler(maxon::UInt64 nodeSystemId, maxon::UInt64 handlerId);

	void _RemoveMonitor(maxon::UInt64 nodeSystemId);

	static maxon::UInt64 GetNodeSystemId(const maxon::nodes::NodesGraphModelRef& graph);

	maxon::UInt64 GetHandlerId();

	maxon::AtomicUInt64 _handlerIdFactory { 0 };

	maxon::Spinlock _lock;
	maxon::HashMap<maxon::UInt64, maxon::HashSet<maxon::UInt64>> _changeHandlers; // monitor -> list of observers
	maxon::HashMap<maxon::UInt64, NodeSystemChangedMonitorRef> _changeMonitors;
};

} // namespace maxonsdk

#endif // NODESYSTEM_OBSERVER_H__
