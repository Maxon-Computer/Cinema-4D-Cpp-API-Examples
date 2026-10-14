//
//  blw_simplegenerator.h
//  blw_simplegenerator
//
//  Created by Riccardo Gigante on 23/07/2019.
//  Copyright © 2019 MAXON Computer GmbH. All rights reserved.
//

#ifndef BLW_SIMPLEGENERATOR_H__
#define BLW_SIMPLEGENERATOR_H__

//------------------------------------------------------------------------------------------------
/// Basic ObjectData implementation responsible for generating a simple plane centered in the
/// origin and sized 200 x 200.
//------------------------------------------------------------------------------------------------
class BLW_SimpleGenerator : public cinema::ObjectData
{
	INSTANCEOF(BLW_SimpleGenerator, cinema::ObjectData)
	
public:
	static cinema::NodeData* Alloc()
	{
		iferr (cinema::NodeData * nodeData = NewObj(BLW_SimpleGenerator))
		{
			err.DiagOutput();
			err.DbgStop();
			return nullptr;
		}
		
		return nodeData;
	};
	virtual cinema::Bool Init(cinema::GeListNode* node, cinema::Bool isCloneInit);
	virtual void GetDimension(const cinema::BaseObject* op, cinema::Vector* mp, cinema::Vector* rad) const;
	virtual cinema::BaseObject* GetVirtualObjects(cinema::BaseObject* op, const cinema::HierarchyHelp* hh);
};

#endif // BLW_SIMPLEGENERATOR_H__
