// LayerShaderBrowser.h
//////////////////////////////////////////////////////////////////////

#ifndef LAYERSHADERBROWSER_H__
#define LAYERSHADERBROWSER_H__

#include "c4d_gui.h"

class LayerShaderBrowser : public cinema::GeDialog
{
public:
	LayerShaderBrowser() { }
	virtual ~LayerShaderBrowser() { }

	virtual cinema::Bool CreateLayout();
	virtual cinema::Bool InitValues();
	virtual cinema::Bool Command(cinema::Int32 id, const cinema::BaseContainer& msg);
	virtual cinema::Int32 Message(const cinema::BaseContainer& msg, cinema::BaseContainer& result);
	virtual cinema::Bool CoreMessage  (cinema::Int32 id, const cinema::BaseContainer& msg);

	void UpdateAll(cinema::Bool msg);
	void ShowInfo(cinema::LayerShaderLayer* l);

	void* lastselected;

private:
	cinema::LinkBoxGui*	_linkbox = nullptr;
	cinema::TreeViewCustomGui* _tree = nullptr;
	cinema::Int32 _lastdirty = -1;
};

#endif // LAYERSHADERBROWSER_H__
