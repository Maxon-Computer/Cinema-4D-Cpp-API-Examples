/// @brief Demonstrates the ItemTree and LongIcons custom GUIs in an object plugin.
///
/// @details ItemTree is a custom GUI that can display arbitrary hierarchical data in a tree view,
/// while LongIcons realizes a menu bar with icon buttons that can show popup menus. Both GUIs are
/// often used in conjunction with each other, for example in the Field List custom GUI or the
/// Volume Builder object. Both custom GUIs can be used both in descriptions and in dialogs.
///
/// The tree in this example displays hierarchies of Object Manager objects. The user can add
/// objects to the tree through the Long Icons GUI or by dragging them from the Object Manager.
/// When the user deletes an object from the document, the tree automatically removes the
/// corresponding element(s) from its hierarchy. The reverse is however not true, the tree does not
/// automatically delete the linked objects when a node is removed from the tree, and adding an
/// object in the Object Manager does not automatically add it to the tree. Finally, the user can
/// change the mode, angle, and enabled state of deformers in the tree, and those changes are
/// immediately reflected in the linked objects in the document.
///
/// Most functionality is implemented in the Message() and GetDDescription() methods of the example,
/// as both custom GUIs require in detail data handling to work. The tree view gets its column count
/// and other basic properties from the resource description, the long icon buttons are just a LONG
/// parameter in the description. Almost everything else is handled at runtime in code.
///
/// @note To adapt the example to a dialog, move the work done in GetDDescription() to
/// the dialog's ::CreateLayout(), there directly creating the settings container for the custom GUIs.
/// NodeData MSG_DESCRIPTION_POPUP and MSG_ITEMTREE_CALLBACK message types are replaced for dialogs
/// by a BFM_ACTION message. BFM_ACTION_ID identifies the custom GUI and BFM_ACTION_VALUE carries
/// the action data to GeDialog::Message, but otherwise the GUIs work identically.

#include "c4d_basedocument.h"
#include "c4d_basetag.h"
#include "c4d_baseobject.h"
#include "c4d_general.h"
#include "c4d_objectdata.h"
#include "customgui_bitmapbutton.h"
#include "customgui_itemtree_utils.h"
#include "customgui_itemtree.h"
#include "customgui_listview.h"
#include "customgui_longicons.h"
#include "lib_description.h"

#include "maxon/fixedsizearray.h"

#include "description/obase.h"
#include "description/obasedeform.h"
#include "description/ocone.h"
#include "description/ocube.h"
#include "description/tphong.h"
#include "description/oshear.h"
#include "description/ospline.h"
#include "description/osplinecircle.h"
#include "description/osplinerectangle.h"
#include "description/osweep.h"

#include "main.h"
#include "oobjecttree.h"

using namespace cinema;

/// Plugin ID of the object tree example. This ID must be obtained from developers.maxon.net.
static constexpr Int32 Oobjecttree = 1069667;

/// ItemTree column for the deformer mode drop-down control.
static constexpr Int32 g_tree_mode_column = ITEMTREE_USER_COL + 0;

/// ItemTree column for the deformer angle control.
static constexpr Int32 g_tree_angle_column = ITEMTREE_USER_COL + 1;

/// ItemTree column for the deformer "enabled" green checkmark control.
static constexpr Int32 g_tree_enabled_column = ITEMTREE_USER_COL + 2;


/// The default flags shared by tree nodes created by the example.
static constexpr UInt32 g_tree_node_flags =
	ITEMTREE_FLAG_BL_NAME |						// Use the linked object's name as the node name.
	ITEMTREE_FLAG_OPEN |							// Open the node by default.
	ITEMTREE_FLAG_INSERT_BEFORE |				// Allow nodes to be inserted before, after, or under this node.
	ITEMTREE_FLAG_INSERT_AFTER |
	ITEMTREE_FLAG_INSERT_UNDER;

/// @brief Realizes an object plugin that displays an item tree that can hold a hierarchy of objects.
//----------------------------------------------------------------------------------------
class ObjectTreeData : public ObjectData
{
  INSTANCEOF(ObjectTreeData, ObjectData)

  BaseContainer _objectPopup;
  BaseContainer _splinePopup;
  BaseContainer _deformerPopup;

public:
  /// @brief Allocates the ObjectData implementation.
  //----------------------------------------------------------------------------------------
  static NodeData* Alloc()
  {
    return NewObjClear(ObjectTreeData);
  }

  /// @brief Initializes the node and prepares both custom GUIs.
  ///
  /// @details The tree stores links to document objects, so its owner must be set to the plugin
  /// object before the custom data is placed into the object description. The Long Icons popup
  /// menus are also built here once; ::Message returns them when Cinema 4D requests a popup.
  //----------------------------------------------------------------------------------------
  Bool Init(GeListNode* node, Bool isCloneInit) override
  {
    if (!node)
      return false;

    // Since this object has no parameters which take literals, this node does not need default
    // values. But #op still has to become the owner of its ItemTreeData so that linked objects can
    // be resolved through its document.
    BaseObject* op = static_cast<BaseObject*>(node);
    BaseContainer& data = op->GetDataInstanceRef();

    if (!isCloneInit)
    {
      GeData treeData(CUSTOMDATA_ITEMTREE, DEFAULTVALUE);
      ItemTreeData* tree = treeData.GetCustomDataTypeWritable<ItemTreeData>();
      if (!tree)
        return false;

      tree->SetOwner(op);
      data.SetData(OBJECTTREE_ITEMTREE, treeData);
    }
    else
    {
      ItemTreeData* tree = data.GetCustomDataTypeWritableObsolete<ItemTreeData>(OBJECTTREE_ITEMTREE);
      if (tree)
        tree->SetOwner(op);
    }

    // The Long Icons custom GUI has three popup menus: one for objects, one for splines, and one for
    // deformers. We build them once and store them in member variables instead of rebuilding them every
    // time Message() handles a popup request.
    LongIconsCustomGui::AddPopupMenuItem(_objectPopup, 0, "Objects"_s);
    LongIconsCustomGui::AddPopupMenuItem(_objectPopup, Ocube, "Cube"_s, Ocube);
    LongIconsCustomGui::AddPopupMenuItem(_objectPopup, Osphere, "Sphere"_s, Osphere);
    LongIconsCustomGui::AddPopupMenuItem(_objectPopup, Ocone, "Cone"_s, Ocone);
    LongIconsCustomGui::AddPopupMenuItem(_objectPopup, Ocylinder, "Cylinder"_s, Ocylinder);
    LongIconsCustomGui::AddPopupMenuItem(_splinePopup, 0, "Splines"_s);
    LongIconsCustomGui::AddPopupMenuItem(_splinePopup, Osplinecircle, "Circle"_s, Osplinecircle);
    LongIconsCustomGui::AddPopupMenuItem(_splinePopup, Osplinerectangle, "Rectangle"_s, Osplinerectangle);
    LongIconsCustomGui::AddPopupMenuItem(_splinePopup, Osplinestar, "Star"_s, Osplinestar);
    LongIconsCustomGui::AddPopupMenuItem(_splinePopup, Osplinehelix, "Helix"_s, Osplinehelix);
    LongIconsCustomGui::AddPopupMenuItem(_deformerPopup, 0, "Deformers"_s);
    LongIconsCustomGui::AddPopupMenuItem(_deformerPopup, Obend, "Bend"_s, Obend);
    LongIconsCustomGui::AddPopupMenuItem(_deformerPopup, Oshear, "Shear"_s, Oshear);
    LongIconsCustomGui::AddPopupMenuItem(_deformerPopup, Otwist, "Twist"_s, Otwist);
    LongIconsCustomGui::AddPopupMenuItem(_deformerPopup, Obulge, "Bulge"_s, Obulge);

    return SUPER::Init(node, isCloneInit);
  }

  /// @brief Called by Cinema 4D to send messages from various sources to the node.
  ///
  /// @details We use this to handle four things: commands being invoked, popup menu selections,
  /// ItemTree callbacks, and notifications from linked objects.
  //----------------------------------------------------------------------------------------
  Bool Message(GeListNode* node, Int32 type, void* messageData) override
  {
    BaseObject* op = static_cast<BaseObject*>(node);
    if (!op)
      return false;

    BaseContainer& data = op->GetDataInstanceRef();
    ItemTreeData* tree = data.GetCustomDataTypeWritableObsolete<ItemTreeData>(OBJECTTREE_ITEMTREE);
    if (!tree)
      return false;

    // We only use the ItemTreeData convenience method AddHierarchy() to add objects (and their children)
    // to the tree. AddHierarchy() takes a receiver to manipulate each node after it has been added; this
    // lambda is what we use as the receiver.
    const auto initializeTreeNode = [op, tree](ItemTreeNode* node) -> maxon::Result<Bool>
    {
      ItemTreeNodeData* treeNode = tree ? tree->GetNodeData(node) : nullptr;
      if (!treeNode)
        return true;

      // The BaseObject for which #node was just created and added to the tree.
      BaseObject* object = GetLinkedObject(op, treeNode);
      if (!object)
        return true;

      // Cinema will not automatically remove elements from the tree when their linked elements are
      // deleted from the document. So we register for removal notifications here, so that we can
      // remove the tree node when the linked object is deleted.
      object->AddEventNotification(op, NOTIFY_EVENT::REMOVE, NOTIFY_EVENT_FLAG::NONE, nullptr);

      // Now we are going to configure the tree node's data container, depending on the type of scene
      // element for which it was added. We made the arbitrary choice that we want to display extra
      // controls for all deformers, in our tree.
      BaseContainer bc;
      if (object->GetInfo() & OBJECT_MODIFIER) // This object is a deformer.
      {
        // Copy the deformer angle and mode parameters into the tree node's data container.
        GeData angle;
        if (object->GetParameter(ConstDescID(DescLevel(DEFORMOBJECT_ANGLE)), angle, DESCFLAGS_GET::NONE))
          bc.SetFloat(g_tree_angle_column, RadToDeg(angle.GetFloat()));

        GeData generatorFlag;
        if (object->GetParameter(ConstDescID(DescLevel(ID_BASEOBJECT_GENERATOR_FLAG)),
                                 generatorFlag, DESCFLAGS_GET::NONE))
          bc.SetBool(g_tree_enabled_column, generatorFlag.GetBool());

        GeData mode;
        if (object->GetParameter(ConstDescID(DescLevel(DEFORMOBJECT_MODE)), mode, DESCFLAGS_GET::NONE))
        {
          BaseContainer modeData;
          BaseContainer modeItems;
          modeItems.SetString(DEFORMOBJECT_MODE_BOX, "Box"_s);
          modeItems.SetString(DEFORMOBJECT_MODE_LIMIT, "Limited"_s);
          modeItems.SetString(DEFORMOBJECT_MODE_UNLIMITED, "Unlimited"_s);
          modeData.SetContainer(DESC_CYCLE, modeItems);
          modeData.SetInt32(g_tree_mode_column, mode.GetInt32());
          bc.SetContainer(g_tree_mode_column, modeData);
        }
      }

      // Finally, we copy the data container into the tree node's data. This will cause the tree to
      // update the node's controls to reflect the values we just set.
      if (treeNode && !bc.IsEmpty())
        treeNode->m_Data = GeData(bc);

      return true;
    };

    // --- Message handling starts here

    switch (type)
    {
      // MSG_DESCRIPTION_COMMAND is sent when the user clicks a button in the node UI,including the
      // Long Icons buttons.
      case MSG_DESCRIPTION_COMMAND:
      {
        DescriptionCommand* command = static_cast<DescriptionCommand*>(messageData);

        // The user pressed the "Add Example Hierarchy" button.
        if (command && command->_descId[0].id == OBJECTTREE_ADD_EXAMPLE)
        {
          // We create the example hierarchy and add it to the tree with AddHierarchy(). Making ourself
          // dirty is not really needed in this example, since the example has a static generator output,
          // but in a real world scenario you might want to rebuild your cache when the tree changes.
          BaseObject* root = CreateHierarchy(op);
          if (root)
          {
            tree->AddHierarchy(root, nullptr, initializeTreeNode, g_tree_node_flags);
            op->SetDirty(DIRTYFLAGS::DATA);
          }
          return true;
        }

        // The user clicked one of the Long Icons buttons or one of their popup menu items.
        else if (command && command->_descId[0].id == OBJECTTREE_MENU_ITEMS && command->_msg)
        {
          // The actual command to invoke is stored in the BFM_ACTION_VALUE of the message. Since
          // we setup our long icon command IDs so that they are object type IDs, we can just pass
          // #cmdId to CreateObject() to create the object that the user requested. We then call
          // AddHierarchy() which will add the new object to the tree and call our receiver to
          // initialize the node data. In this case we could also use AddNode() to add a single
          // object, but we use AddHierarchy() for consistency and because we have already the
          // lambda to initialize the node data.
          const Int32 cmdId = command->_msg->GetInt32(BFM_ACTION_VALUE, NOTOK);
          BaseObject* created = CreateObject(op, cmdId);
          if (created)
          {
            tree->AddHierarchy(created, tree->GetFirstSelected(), initializeTreeNode, g_tree_node_flags);
            op->SetDirty(DIRTYFLAGS::DATA);
          }
          return true;
        }
        break;
      }

      // This message is sent when Cinema 4D wants to know the popup menus for buttons in the Long
      // Icons custom GUI. We return the appropriate menu we previously built in ::Init() for the
      // button that was clicked.
      case MSG_DESCRIPTION_POPUP:
      {
        DescriptionPopup* popup = static_cast<DescriptionPopup*>(messageData);
        if (popup && popup->_descId[0].id == OBJECTTREE_MENU_ITEMS)
        {
          if (popup->_chosen == Ocube) // The popup menu for the top-level Ocube button.
            return LongIconsCustomGui::HandlePopup(_objectPopup, popup);
          if (popup->_chosen == Osplinecircle) // The popup menu for the top-level Osplinecircle button.
            return LongIconsCustomGui::HandlePopup(_splinePopup, popup);
          if (popup->_chosen == Obend) // The popup menu for the top-level Obend button.
            return LongIconsCustomGui::HandlePopup(_deformerPopup, popup);
        }
        break;
      }

      // MSG_ITEMTREE_CALLBACK is the central message with which we operate the tree view. There are
      // many more subtypes of this message than shown here. See the headers for details.
      case MSG_ITEMTREE_CALLBACK:
      {

        // Get the callback data from the message that holds information about the type of callback
        // and the affected node.
        ItemTreeCallbackData* callback = static_cast<ItemTreeCallbackData*>(messageData);
        if (!callback)
          return SUPER::Message(node, type, messageData);

        // ITEMTREE_CALLBACK_VALUECHANGED reports a changed slider or checkbox value. The tree is
        // calling us because the user changed a value in one of the columns. We react to that
        // change by synchronizing the linked object's parameter with the new value.
        if (callback->type == ITEMTREE_CALLBACK_VALUECHANGED && callback->item && callback->nodedata)
        {
          // The angle column has changed, we synchronize the angle parameter of the linked deformer
          // object with the new value.
          if (callback->id == g_tree_angle_column)
          {
            SyncDeformerAngle(op, callback->nodedata);
          }

          // The enable column has changed, we synchronize the generator flag of the linked deformer
          // object with the new value.
          else if (callback->id == g_tree_enabled_column)
          {
            BaseObject* deformer = GetLinkedObject(op, callback->nodedata);
            if (deformer && (deformer->GetInfo() & OBJECT_MODIFIER) && callback->data)
            {
              deformer->SetParameter(ConstDescID(DescLevel(ID_BASEOBJECT_GENERATOR_FLAG)),
                GeData(callback->data->GetBool(ITEMTREE_VALUECHANGED_VALUE)), DESCFLAGS_SET::NONE);
              deformer->SetDirty(DIRTYFLAGS::DATA);
            }
          }
          // We handled the value change, so we return 1 to indicate that we have processed the callback.
          callback->ret = 1;
        }

        // ITEMTREE_CALLBACK_SETDROPDOWNMENU reports the user's selected entry in the Mode menu; we
        // use it to propagate MODE column changes to the mode of the linked deformer object.
        else if (callback->type == ITEMTREE_CALLBACK_SETDROPDOWNMENU && callback->item &&
                 callback->nodedata && callback->data)
        {
          if (callback->data->GetInt32(ITEMTREE_SETDROPDOWNMENU_COLUMN, NOTOK) == g_tree_mode_column)
          {
            BaseObject* deformer = GetLinkedObject(op, callback->nodedata);
            const Int32 entry = callback->data->GetInt32(ITEMTREE_SETDROPDOWNMENU_ENTRY, NOTOK);
            if (deformer && (deformer->GetInfo() & OBJECT_MODIFIER) && entry != NOTOK)
            {
              deformer->SetParameter(ConstDescID(DescLevel(DEFORMOBJECT_MODE)),
                                     GeData(entry), DESCFLAGS_SET::NONE);
              deformer->SetDirty(DIRTYFLAGS::DATA);
            }
          }
        }

        // ITEMTREE_CALLBACK_GETDROPDOWNMENU asks for the contents of a drop-down menu. We provide
        // the menu items for the Mode column.
        else if (callback->type == ITEMTREE_CALLBACK_GETDROPDOWNMENU && callback->nodedata && callback->doc)
        {
          const BaseContainer* nodeData = callback->nodedata->m_Data.GetContainer();
          const BaseContainer* modeData = nodeData ? nodeData->GetContainerInstance(g_tree_mode_column) : nullptr;
          if (!modeData || !modeData->GetContainerInstance(DESC_CYCLE))
            callback->ret = LV_CHECKBOX_HIDE;
        }

        // ITEMTREE_CALLBACK_GETHEADERCOLUMNWIDTH and ITEMTREE_CALLBACK_GETCOLWIDTH ask for a
        // column width. We use it to customize the width of the Angle column, which is a slider and
        // needs more space than the default.
        else if ((callback->type == ITEMTREE_CALLBACK_GETHEADERCOLUMNWIDTH ||
          callback->type == ITEMTREE_CALLBACK_GETCOLWIDTH) && callback->id == g_tree_angle_column)
        {
          callback->ret = 120;
        }

        // ITEMTREE_CALLBACK_SELECTIONCHG reports a changed tree selection. The selection in the tree
        // has changed, so we use this to set the temporary manipulator object in the document. The
        // "temporary manipulator" is the quasi-selection highlighting in the Object Manager that is
        // not an actual selection. It is, for example, also used by the Field List custom GUI to
        // highlight the object that is currently being edited in the field list. Doing this is not
        // necessary for the tree to work, but can make for a better user experience, as the user
        // can see which Object Manager object corresponds to the selected tree node.
        else if (callback->type == ITEMTREE_CALLBACK_SELECTIONCHG)
        {
          BaseDocument* doc = op->GetDocument();
          if (!doc || !callback->item)
            break;

          ItemTreeNodeData* treeNode = callback->item->GetNodeData(callback->item->GetFirstSelected());
          if (!treeNode || !treeNode->m_pLink)
            break;

          BaseList2D* selectedObject = treeNode->m_pLink->GetLink(doc);
          if (selectedObject && selectedObject->GetClassification() == Obase)
            doc->SetTemporaryManipulatorObject(static_cast<BaseObject*>(selectedObject));

          callback->ret = 1;
        }

        // ITEMTREE_CALLBACK_DRAGACCEPT asks whether the current drag can be inserted into the tree.
        // The user has dragged something over the tree, for example an object, and we have to tell
        // Cinema 4D whether we accept the drop or not.
        else if (callback->type == ITEMTREE_CALLBACK_DRAGACCEPT && callback->data)
        {
          // The user is not dragging an AtomArray, which is the type of data we expect for a dragged
          // object, so we do not accept the drop.
          if (callback->data->GetInt32(ITEMTREE_DRAG_TYPE) != DRAGTYPE_ATOMARRAY)
            break;

          // The user is dragging a list of atoms, which at least holds one atom. We accept the drop
          // and set the drag mask to indicate that where it is allowed to drop the dragged object.
          // This drives where the little plus icon is shown when the user drags over the tree.
          AtomArray* objects = static_cast<AtomArray*>(callback->data->GetVoid(ITEMTREE_DRAG_OBJECT));
          if (objects && objects->GetCount() > 0)
            callback->data->SetInt32(ITEMTREE_DRAG_MASK, ITEMTREE_FLAG_INSERT_BEFORE |
                                     ITEMTREE_FLAG_INSERT_AFTER | ITEMTREE_FLAG_INSERT_UNDER);

          callback->ret = 1;
        }

        // ITEMTREE_CALLBACK_INSERTNODE reports that the user has dropped an accepted object onto
        // the tree. We add it to the tree and register for remove notifications so that the tree
        // can be updated when the object is deleted (via the lambda we defined above which is
        // called by AddHierarchy()).
        else if (callback->type == ITEMTREE_CALLBACK_INSERTNODE && callback->nodedata)
        {
          BaseObject* dropOp = static_cast<BaseObject*>(callback->typedata);
          if (!dropOp || !callback->node)
            break;

          callback->item->AddHierarchy(dropOp, callback->node, initializeTreeNode, g_tree_node_flags);
          callback->nodedata->m_Flags = g_tree_node_flags | ITEMTREE_FLAG_OPEN;

          callback->ret = 1;
        }
        break;
      }

      // MSG_NOTIFY_EVENT is the incoming message for all the event notifications we registered for
      // the objects linked in our tree. There are many more event types than the one we handle here,
      // but we only care about REMOVE events.
      case MSG_NOTIFY_EVENT:
      {
        // When the event notification is a REMOVE event, we validate the tree to remove any nodes
        // that link to the removed object.
        NotifyEventData* notifyData = static_cast<NotifyEventData*>(messageData);
        if (notifyData && notifyData->eventid == NOTIFY_EVENT::REMOVE)
        {
          tree->Validate(notifyData->doc);
          op->SetDirty(DIRTYFLAGS::DATA);
          return true;
        }
        break;
      }
    }

    return SUPER::Message(node, type, messageData);
  }

  /// @brief Adds the ItemTree columns and Long Icons creation buttons to the node description.
  ///
  /// @details The resource description provides the base controls, while this method supplies the
  /// runtime column types, headers, ranges, and menu contents needed by this example.
  //----------------------------------------------------------------------------------------
  Bool GetDDescription(const GeListNode* node, Description* description,
                       DESCFLAGS_DESC& flags) const override
  {
    if (!description->LoadDescription(node->GetType()))
      return false;

    // GetDDescription() is being called very often and it is important that we only do changes we
    // are asked to in each call. With #queryId Cinema 4D tells us which part of the description it
    // wants to update. We should not update the whole description every time, as that
    // would be very inefficient.


    const DescID* queryId = description->GetSingleDescID(); // The thing we should update.
    const DescID treeId = ConstDescID(DescLevel(OBJECTTREE_ITEMTREE)); // Our tree Id.
    const DescID menuId = ConstDescID(DescLevel(OBJECTTREE_MENU_ITEMS)); // Our menu Id.

    // Check if this update call is for any of these two description elements.
    const Bool updateMenu = queryId ? menuId.IsPartOf(*queryId, nullptr) : true;
    const Bool updateTree = queryId ? treeId.IsPartOf(*queryId, nullptr) : true;
    if (!updateMenu && !updateTree)
      return SUPER::GetDDescription(node, description, flags);

    // We must update the settings for the tree view custom GUI.
    if (updateTree)
    {
      AutoAlloc<AtomArray> descriptionArray;
      BaseContainer* treeSettings = description->GetParameterI(treeId, descriptionArray);
      if (treeSettings)
      {
        BaseContainer columnTypes, columnHeaders, columnMin, columnMax, columnActive;

        // In the description we only say that there are four columns. Here we actually set the
        // type of each column, which controls what kind of control is displayed in the tree. In a
        // real world scenario you would want to load localized strings for the column headers
        // instead of hard-coding them here.

        // Set the control type for each column.
        columnTypes.SetInt32(g_tree_mode_column, LV_DROPDOWN);
        columnTypes.SetInt32(g_tree_angle_column, LV_SLIDER);
        columnTypes.SetInt32(g_tree_enabled_column, LV_CHECKBOX);

        // Enable columns and set their headers.
        columnActive.SetBool(g_tree_mode_column, true);
        columnActive.SetBool(g_tree_angle_column, true);
        columnActive.SetBool(g_tree_enabled_column, true);

        columnHeaders.SetString(ITEMTREE_OBJECT_COL, "Object"_s);
        columnHeaders.SetString(g_tree_mode_column, "Mode"_s);
        columnHeaders.SetString(g_tree_angle_column, "Angle"_s);
        columnHeaders.SetString(g_tree_enabled_column, "Enabled"_s);

        // Set the column types, active columns, and column headers in the tree settings container
        // and configure the min/max range for the angle slider column.
        treeSettings->SetContainer(ITEMTREE_USER_COL_TYPES, columnTypes);
        treeSettings->SetContainer(ITEMTREE_USER_COL_ACTIVE, columnActive);
        treeSettings->SetContainer(ITEMTREE_ID_HEADERS_DATA, columnHeaders);

        columnMin.SetFloat(g_tree_angle_column, 0.0_f);
        columnMax.SetFloat(g_tree_angle_column, 360.0_f);
        treeSettings->SetContainer(ITEMTREE_USER_COL_MIN, columnMin);
        treeSettings->SetContainer(ITEMTREE_USER_COL_MAX, columnMax);
      }
    }

    // We must update the settings for the Long Icons custom GUI.
    if (updateMenu)
    {
      BaseContainer* settings = description->GetParameterI(menuId, nullptr);
      if (settings)
      {
        // Define the size of the icons and whether to show the names below them.
        settings->SetInt32((Int32)LONGICONS::HEIGHT, 20);
        settings->SetInt32((Int32)LONGICONS::WIDTH, 20);
        settings->SetBool((Int32)LONGICONS::SHOW_NAMES, true);

        // The Long Icons custom GUI follows a somewhat unusual paradigm. We only statically define the
        // top-level buttons in the description container, and their popups are built at runtime via
        // the MSG_DESCRIPTION_POPUP message. These buttons follow the Cinema logic of long and short
        // clicks. Briefly clicking them will invoke their command ID, while long clicking them will
        // show their popup menu. So, the Ocube button can both add a cube and show the object popup
        // menu, depending on how the user clicks it.
        LongIconsCustomGui::AddMenuItem(*settings,
                                        Ocube,                 // ID of the button.
                                        "Objects"_s,           // Its label.
                                        Ocube,                 // The ID of its icon (or NOTOK)
                                        "Create a cube."_s,    // Its tooltip.
                                        true);                  // Whether this button has a popup menu.

        LongIconsCustomGui::AddMenuItem(*settings, Osplinecircle, "Splines"_s, Osplinecircle,
                                        "Create a circle spline."_s, true);
        LongIconsCustomGui::AddMenuItem(*settings, Obend, "Deformers"_s, Obend,
                "Create a bend deformer."_s, true);
      }
    }

    flags |= DESCFLAGS_DESC::LOADED;
    return SUPER::GetDDescription(node, description, flags);
  }

  /// @brief Returns the existing cache or an empty null object.
  ///
  /// @details This example does not generate any meaningful geometry.
  //----------------------------------------------------------------------------------------
  BaseObject* GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh) override
  {
    BaseObject* cache = op->GetCache();
    return cache ? cache : BaseObject::Alloc(Onull);
  }

  // --- Custom helper methods which are not part of the ObjectData interface --------------

private:

  /// @brief Returns the scene element linked to the tree node, or nullptr if there is no link.
  ///
  /// @details This could be inlined, but we also check that the linked object is a BaseObject.
  /// This is more of a sanity check than a requirement because the tree accepts only Obase objects.
  /// the tree only accepts Obase objects).
  //----------------------------------------------------------------------------------------
  static BaseObject* GetLinkedObject(BaseObject* owner, const ItemTreeNodeData* data)
  {
    if (!owner || !data || !data->m_pLink)
      return nullptr;

    BaseList2D* linkedObject = data->m_pLink->GetLink(owner->GetDocument());
    return linkedObject && linkedObject->IsInstanceOf(Obase) ? static_cast<BaseObject*>(linkedObject) : nullptr;
  }

  /// @brief Synchronizes the angle parameter of a linked deformer object with its corresponding
  /// tree node value.
  //----------------------------------------------------------------------------------------
  static void SyncDeformerAngle(BaseObject* owner, const ItemTreeNodeData* data)
  {
    if (!owner || !data || data->m_Data.GetType() != DA_CONTAINER)
      return;

    // Get the linked object, its data container and make sure it is a deformer.
    BaseObject* object = GetLinkedObject(owner, data);
    const BaseContainer* angleData = data->m_Data.GetContainer();
    if (!object || !(object->GetInfo() & OBJECT_MODIFIER) || !angleData)
      return;

    GeData angle;
    const Float angleInRadians = DegToRad(angleData->GetFloat(g_tree_angle_column));
    if (!object->GetParameter(ConstDescID(DescLevel(DEFORMOBJECT_ANGLE)), angle, DESCFLAGS_GET::NONE) ||
      angle.GetFloat() == angleInRadians)
      return;

    // Update the deformer angle parameter with the new value from the tree node.
    object->SetParameter(ConstDescID(DescLevel(DEFORMOBJECT_ANGLE)), GeData(angleInRadians), DESCFLAGS_SET::NONE);
    object->SetDirty(DIRTYFLAGS::DATA);

    // Since angle value is set from a slider, Cinema's viewport will not update on its own while we
    // are dragging the slider. When we are okay with only updating the viewport after the user releases
    // the slider, we can just call EventAdd() here. For anything else, e.g., a checkbox column, this
    // is never necessary.
    DrawViews(DRAWFLAGS::FORCEFULLREDRAW | DRAWFLAGS::NO_ANIMATION | DRAWFLAGS::SKIP_IF_THREADED_DRAW_ACTIVE);
    EventAdd();
  }

  /// @brief Creates an object of the given #type below the currently selected object in the tree.
  /// @details Used by the Long Icons buttons and their popup menus to create new object.
  //----------------------------------------------------------------------------------------
  static BaseObject* CreateObject(BaseObject* owner, Int32 type)
  {
    // Create the new object and add a Phong tag to it.
    BaseDocument* doc = owner ? owner->GetDocument() : nullptr;
    BaseObject* created = BaseObject::Alloc(type);
    if (!doc || !created)
    {
      BaseObject::Free(created);
      return nullptr;
    }
    created->MakeTag(Tphong);

    // Get the selected node in the tree and its linked object, if any. We will insert the new object
    // below that linked object in the document hierarchy.
    BaseContainer& data = owner->GetDataInstanceRef();
    ItemTreeData* tree = data.GetCustomDataTypeWritableObsolete<ItemTreeData>(OBJECTTREE_ITEMTREE);
    ItemTreeNode* selectedNode = tree ? tree->GetFirstSelected() : nullptr;
    ItemTreeNodeData* selectedData = tree ? tree->GetNodeData(selectedNode) : nullptr;
    BaseObject* selectedObject = nullptr;
    if (selectedData && selectedData->m_pLink)
    {
      BaseList2D* linkedObject = selectedData->m_pLink->GetLink(doc);
      if (linkedObject && linkedObject->IsInstanceOf(Obase))
        selectedObject = static_cast<BaseObject*>(linkedObject);
    }

    doc->InsertObject(created, selectedObject, nullptr);
    return created;
  }

  /// @brief Builds and inserts the sample hierarchy used by the example command.
  ///
  /// @details This is a slightly more complex version of CreateObject() that creates a small
  /// hierarchy of objects and inserts it into the document.
  //----------------------------------------------------------------------------------------
  static BaseObject* CreateHierarchy(BaseObject* owner)
  {
    if (!owner)
      return nullptr;

    BaseDocument* doc = owner->GetDocument();
    if (!doc)
      return nullptr;

    // Allocate the root, primitive objects, deformers, spline objects, and an additional group used
    // to make the sample hierarchy visibly demonstrate several ItemTree nesting levels.
    BaseObject* root = BaseObject::Alloc(Onull);
    BaseObject* cube = BaseObject::Alloc(Ocube);
    BaseObject* shear = BaseObject::Alloc(Oshear);
    BaseObject* cone = BaseObject::Alloc(Ocone);
    BaseObject* bend = BaseObject::Alloc(Obend);
    BaseObject* group = BaseObject::Alloc(Onull);
    BaseObject* sweep = BaseObject::Alloc(Osweep);
    BaseObject* circle = BaseObject::Alloc(Osplinecircle);
    BaseObject* rectangle = BaseObject::Alloc(Osplinerectangle);
    BaseObject* twist = BaseObject::Alloc(Otwist);
    BaseTag* cubeTag = cube ? cube->MakeTag(Tphong) : nullptr;
    BaseTag* coneTag = cone ? cone->MakeTag(Tphong) : nullptr;

    if (!root || !cube || !shear || !cone || !bend || !group || !sweep || !circle || !rectangle ||
      !twist || !cubeTag || !coneTag)
    {
      BaseObject::Free(root);
      BaseObject::Free(cube);
      BaseObject::Free(shear);
      BaseObject::Free(cone);
      BaseObject::Free(bend);
      BaseObject::Free(group);
      BaseObject::Free(sweep);
      BaseObject::Free(circle);
      BaseObject::Free(rectangle);
      BaseObject::Free(twist);
      BaseTag::Free(cubeTag);
      BaseTag::Free(coneTag);

      return nullptr;
    }

    // Give the primitive objects useful settings and positions, then configure each deformer with
    // an angle that can be edited through the ItemTree columns.
    cubeTag->SetParameter(ConstDescID(DescLevel(PHONGTAG_PHONG_ANGLELIMIT)),
                          GeData(true), DESCFLAGS_SET::NONE);
    coneTag->SetParameter(ConstDescID(DescLevel(PHONGTAG_PHONG_ANGLELIMIT)),
                          GeData(true), DESCFLAGS_SET::NONE);

    cube->SetParameter(ConstDescID(DescLevel(PRIM_CUBE_LEN)),
                       GeData(Vector(200.0, 500.0, 200.0)), DESCFLAGS_SET::NONE);
    cube->SetParameter(ConstDescID(DescLevel(PRIM_CUBE_SUBX)),
                       GeData(Int32(24)), DESCFLAGS_SET::NONE);
    cube->SetParameter(ConstDescID(DescLevel(PRIM_CUBE_SUBY)),
                       GeData(Int32(24)), DESCFLAGS_SET::NONE);
    cube->SetParameter(ConstDescID(DescLevel(PRIM_CUBE_SUBZ)),
                       GeData(Int32(24)), DESCFLAGS_SET::NONE);
    cube->SetRelPos(Vector(-300.0, 0.0, 0.0));

    shear->SetParameter(ConstDescID(DescLevel(DEFORMOBJECT_STRENGTH)),
                        GeData(1.0), DESCFLAGS_SET::NONE);
    shear->SetParameter(ConstDescID(DescLevel(DEFORMOBJECT_ANGLE)),
                        GeData(DegToRad(20.0)), DESCFLAGS_SET::NONE);
    shear->SetParameter(ConstDescID(DescLevel(DEFORMOBJECT_MODE)),
                        GeData(DEFORMOBJECT_MODE_UNLIMITED), DESCFLAGS_SET::NONE);

    cone->SetParameter(ConstDescID(DescLevel(PRIM_CONE_HEIGHT)),
                       GeData(280.0), DESCFLAGS_SET::NONE);
    cone->SetParameter(ConstDescID(DescLevel(PRIM_CONE_HSUB)),
                       GeData(Int32(16)), DESCFLAGS_SET::NONE);
    cone->SetParameter(ConstDescID(DescLevel(PRIM_CONE_SEG)),
                       GeData(Int32(32)), DESCFLAGS_SET::NONE);
    cone->SetRelPos(Vector(300.0, 0.0, 0.0));

    bend->SetParameter(ConstDescID(DescLevel(DEFORMOBJECT_STRENGTH)),
                       GeData(1.0), DESCFLAGS_SET::NONE);
    bend->SetParameter(ConstDescID(DescLevel(DEFORMOBJECT_ANGLE)),
                       GeData(DegToRad(35.0)), DESCFLAGS_SET::NONE);
    bend->SetRelRot(Vector(0.0, 0.0, DegToRad(90.0)));

    // Do the same for the splines.
    rectangle->SetParameter(ConstDescID(DescLevel(PRIM_RECTANGLE_WIDTH)),
                            GeData(200.0), DESCFLAGS_SET::NONE);
    rectangle->SetParameter(ConstDescID(DescLevel(PRIM_RECTANGLE_HEIGHT)),
                            GeData(500.0), DESCFLAGS_SET::NONE);
    rectangle->SetParameter(ConstDescID(DescLevel(SPLINEOBJECT_INTERPOLATION)),
                            GeData(SPLINEOBJECT_INTERPOLATION_UNIFORM), DESCFLAGS_SET::NONE);
    rectangle->SetParameter(ConstDescID(DescLevel(SPLINEOBJECT_SUB)),
                            GeData(Int32(24)), DESCFLAGS_SET::NONE);
    rectangle->SetParameter(ConstDescID(DescLevel(PRIM_RECTANGLE_ROUNDING)),
                            GeData(true), DESCFLAGS_SET::NONE);
    rectangle->SetRelPos(Vector(300.0, 0.0, 400.0));

    circle->SetParameter(ConstDescID(DescLevel(PRIM_CIRCLE_RADIUS)),
                         GeData(10.0), DESCFLAGS_SET::NONE);
    circle->SetParameter(ConstDescID(DescLevel(SPLINEOBJECT_INTERPOLATION)),
                         GeData(SPLINEOBJECT_INTERPOLATION_UNIFORM), DESCFLAGS_SET::NONE);
    circle->SetParameter(ConstDescID(DescLevel(SPLINEOBJECT_SUB)),
                         GeData(Int32(24)), DESCFLAGS_SET::NONE);
    circle->SetRelPos(Vector(0.0));

    twist->SetParameter(ConstDescID(DescLevel(DEFORMOBJECT_STRENGTH)),
                        GeData(1.0), DESCFLAGS_SET::NONE);
    twist->SetParameter(ConstDescID(DescLevel(DEFORMOBJECT_ANGLE)),
                        GeData(DegToRad(25.0)), DESCFLAGS_SET::NONE);
    twist->SetParameter(ConstDescID(DescLevel(DEFORMOBJECT_MODE)),
                        GeData(DEFORMOBJECT_MODE_UNLIMITED), DESCFLAGS_SET::NONE);

    // Assemble the document hierarchy before inserting its root into the document.
    cube->InsertUnderLast(root);
    shear->InsertUnderLast(cube);
    cone->InsertUnderLast(root);
    bend->InsertUnderLast(cone);
    group->InsertUnderLast(root);
    sweep->InsertUnderLast(group);
    circle->InsertUnderLast(sweep);
    rectangle->InsertUnderLast(sweep);
    twist->InsertUnderLast(rectangle);

    // Unfold all objects in all Object Managers (Cinema has up to four object managers).
    const maxon::FixedSizeArray<BaseObject*, 10> objects =
    {
      root, cube, shear, cone, bend, group, sweep, circle, rectangle, twist
    };

    for (BaseObject* object : objects)
    {
      object->ChangeNBit(NBIT::OM1_FOLD, NBITCONTROL::SET);
      object->ChangeNBit(NBIT::OM2_FOLD, NBITCONTROL::SET);
      object->ChangeNBit(NBIT::OM3_FOLD, NBITCONTROL::SET);
      object->ChangeNBit(NBIT::OM4_FOLD, NBITCONTROL::SET);
    }

    doc->InsertObject(root, nullptr, nullptr);
    return root;
  }
};

/// Registers the object that hosts the ItemTree and Long Icons example.
Bool RegisterObjectTreeData()
{
  return RegisterObjectPlugin(Oobjecttree, "C++ SDK - Object Tree"_s, 0, ObjectTreeData::Alloc,
    "oobjecttree"_s, nullptr, 0);
}
