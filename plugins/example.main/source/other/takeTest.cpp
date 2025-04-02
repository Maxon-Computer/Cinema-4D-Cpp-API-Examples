/////////////////////////////////////////////////////////////
// Cinema 4D SDK                                           //
/////////////////////////////////////////////////////////////
// (c) MAXON Computer GmbH, all rights reserved            //
/////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////
// Take API example
////////////////////////////////////////////////////////////////////////////////////

#include "c4d.h"
#include "lib_takesystem.h"
#include "main.h"

using namespace cinema;

static const Int32 ID_CREATETAKE = 1033812;
static const Int32 ID_EDITOVERIDE = 1033814;
static const Int32 ID_ANIMATEOVERRIDE = 1033815;
static const Int32 ID_AUTOTAKE = 1033816;

class TakeTestCommmand : public CommandData
{
public:
  explicit TakeTestCommmand(Int32 commandID) : _commandID(commandID) { }

  virtual Int32 GetState(BaseDocument* doc, GeDialog* parentManager);
  virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager);

private:
  Int32 _commandID;
};


Int32 TakeTestCommmand::GetState(BaseDocument* doc, GeDialog* parentManager)
{
  return CMD_ENABLED;
}

Bool TakeTestCommmand::Execute(BaseDocument* doc, GeDialog* parentManager)
{
  if (!doc)
    return false;

  switch (_commandID)
  {
    case ID_CREATETAKE:
    {
      // create a simple new document
      BaseDocument* newDoc = BaseDocument::Alloc();
      if (!newDoc)
        return false;

      // add it to the document list and make it active
      InsertBaseDocument(newDoc);
      SetActiveDocument(newDoc);

      // grab form the document the take context
      TakeData* takeData = newDoc->GetTakeData();
      if (!takeData)
        return false;

      // grab the current main take take just as example in this case
      // main take must be always !nullptr in a valid document
      BaseTake* cTake = takeData->GetMainTake();
      if (!cTake)
        return false;

      // create a new simple cube with default value
      BaseObject* cube = BaseObject::Alloc(Ocube);
      if (!cube)
        return false;

      // add it to the document
      newDoc->InsertObject(cube, nullptr, nullptr, true);

      // now we can geneate a new take
      BaseTake* newTake = takeData->AddTake(String("My New Take"), nullptr, nullptr);
      if (newTake)
      {
        DescID did = ConstDescID(DescLevel(PRIM_CUBE_SUBY, DA_LONG, 0));
        GeData newValue = GeData(11); // value for new take
        GeData mainValue = GeData(2); // value for main take

        BaseOverride* overrideNode = newTake->FindOrAddOverrideParam(takeData, cube, did, newValue, mainValue);
        if (overrideNode)
          overrideNode->UpdateSceneNode(takeData, ConstDescIDLevel(PRIM_CUBE_SUBY)); // update the scene node if necesaty, ALWAYS call it at least one time after override a node

        // set current the new take if you like
        takeData->SetCurrentTake(newTake);

        EventAdd();
      }
      break;
    }
		case ID_EDITOVERIDE:
		{
      // get an object in the document
			BaseObject* op = doc->GetFirstObject();
			if (!op)
				return true;

      // grab form the document the take context
			TakeData* takeData = doc->GetTakeData();
			if (!takeData)
				return false;

      // get the current take
			BaseTake* cTake = takeData->GetCurrentTake();
			if (!cTake)
				return false;

      // just if we are in an user defined take
			// in this example we work only with a simple cube
			if (!cTake->IsMain() && op->IsInstanceOf(Ocube))
			{
				BaseOverride* overrideNOde = cTake->FindOverride(takeData, op);
        if (overrideNOde && overrideNOde->IsOverriddenParam(ConstDescID(DescLevel(PRIM_CUBE_SUBY, DA_LONG, 0)))) // if we find the overridden param
				{
          overrideNOde->SetParameter(ConstDescID(DescLevel(PRIM_CUBE_SUBY, DA_LONG, 0)), GeData(20), DESCFLAGS_SET::NONE); // we can just change the value like in all other nodes with SetParameter
          overrideNOde->UpdateSceneNode(takeData, ConstDescID(DescLevel(PRIM_CUBE_SUBY, DA_LONG, 0))); // commit the chage to the scene
					BaseTake* resultTake = nullptr;
          BaseOverride* counterpartNode = takeData->FindOverrideCounterPart(overrideNOde, ConstDescID(DescLevel(PRIM_CUBE_SUBY, DA_LONG, 0)), resultTake); // now we search the node whenre the backup value is stored
					if (counterpartNode)
					{
            // resultTake culd be not the main take if the same parameter is overridden in also in a parent non main take.
            counterpartNode->SetParameter(ConstDescID(DescLevel(PRIM_CUBE_SUBY, DA_LONG, 0)), GeData(0), DESCFLAGS_SET::NONE); // change the value also here for example
						if (resultTake && resultTake->IsMain())
							DiagnosticOutput("Result Take is Main Take");
					}
				}

				EventAdd();
			}
			break;
		}
		case ID_AUTOTAKE:
		{
			// get an object in the document
			BaseObject* op = doc->GetFirstObject();
			if (!op)
				return true;

			// grab form the document the take context
			TakeData* takeData = doc->GetTakeData();
			if (!takeData)
				return false;

			// get the current take
			BaseTake* cTake = takeData->GetCurrentTake();
			if (!cTake)
				return false;

			// just if we are in an user defined take
			if (!cTake->IsMain() && op->IsInstanceOf(Ocube))
			{
				doc->StartUndo();
				doc->AddUndo(UNDOTYPE::CHANGE, op); // we store trhe previews state we could also use get clone if the undo step is not needed

				// we can modify some parameters
				op->SetAbsPos(Vector(100, 100, 100));
				op->SetParameter(ConstDescID(DescLevel(PRIM_CUBE_SUBY, DA_LONG, 0)), GeData(20), DESCFLAGS_SET::NONE);

				doc->EndUndo();

				// then if the undo clone exist we can generate automatic overrides
				BaseList2D* undo = doc->FindUndoPtr(op, UNDOTYPE::CHANGE);
				if (undo)
					cTake->AutoTake(takeData, op, undo);
				EventAdd();
			}
			break;
		}
		case ID_ANIMATEOVERRIDE:
		{
      // get an object in the document
      BaseObject* op = doc->GetFirstObject();
      if (!op)
        return true;

      // grab form the document the take context
      TakeData* takeData = doc->GetTakeData();
      if (!takeData)
        return false;

      // get the current take
      BaseTake* cTake = takeData->GetCurrentTake();
      if (!cTake)
        return false;

      // just if we are in an user defined take
			// in this example we work only with a simple cube
			if (!cTake->IsMain() && op->IsInstanceOf(Ocube))
      {
        DescID did = ConstDescID(DescLevel(PRIM_CUBE_SUBY, DA_LONG, 0));
        GeData newValue = GeData(11); // value for new take

        // anyway we must override before animate
        BaseOverride* overrideNode = cTake->FindOrAddOverrideParam(takeData, op, did, newValue);
        if (overrideNode)
        {
          CTrack* cTrack = overrideNode->FindCTrack(did);
          if (!cTrack)
          {
            cTrack = CTrack::Alloc(op, did); // always pass the original node to CTrack::Alloc
            if (!cTrack)
              return false;
            overrideNode->InsertTrackSorted(cTrack);
          }

          CCurve* seq = cTrack->GetCurve();
          // we remove existing animation if we need a completelly new version
          // otherwise you can just modify existing one (add, remove, edit key si allowed)
          if (seq->GetKeyCount() > 0)
            seq->FlushKeys();

          CKey* key = seq->AddKey(BaseTime(0));
          if (!key)
            return false;

          key->SetValue(seq, 10);
          key->ChangeNBit(NBIT::CKEY_AUTO, NBITCONTROL::SET);
          key->ChangeNBit(NBIT::CKEY_CLAMP, NBITCONTROL::SET);

          key = seq->AddKey(BaseTime(3));
          if (!key)
            return false;

          key->SetValue(seq, 50);
          key->ChangeNBit(NBIT::CKEY_AUTO, NBITCONTROL::SET);
          key->ChangeNBit(NBIT::CKEY_CLAMP, NBITCONTROL::SET);

          // we can add an additional override
          did = ConstDescID(DescLevel(ID_BASEOBJECT_REL_POSITION, DTYPE_VECTOR, 0), DescLevel(VECTOR_X, DTYPE_REAL, 0)); // descid have to be defined like in animations at subdescription level
          newValue = GeData(100.0); // value for new take
          overrideNode = cTake->FindOrAddOverrideParam(takeData, op, did, newValue);
          overrideNode->UpdateSceneNode(takeData, ConstDescIDLevel(ID_BASEOBJECT_REL_POSITION)); // update the scene node if necesaty, ALWAYS call it at least one time after override a node
          EventAdd();
        }
      }
			break;
		}
    default:
      break;
  }

  return true;
}

Bool RegisterTakeTestCommmands()
{
  Bool res = true;
  res = RegisterCommandPlugin(ID_CREATETAKE,  String("Take Test 1"), 0, nullptr, String("Create a take and a static override"), NewObjClear(TakeTestCommmand, ID_CREATETAKE));
	res = RegisterCommandPlugin(ID_EDITOVERIDE,  String("Take Test 2"), 0, nullptr, String("Edit existing overrides"), NewObjClear(TakeTestCommmand, ID_EDITOVERIDE));
	res = RegisterCommandPlugin(ID_AUTOTAKE,  String("Take Test 3"), 0, nullptr, String("AutoTake"), NewObjClear(TakeTestCommmand, ID_AUTOTAKE));
	res = RegisterCommandPlugin(ID_ANIMATEOVERRIDE,  String("Take Test 4"), 0, nullptr, String("Handle animated overrides"), NewObjClear(TakeTestCommmand, ID_ANIMATEOVERRIDE));
  return res;
}
