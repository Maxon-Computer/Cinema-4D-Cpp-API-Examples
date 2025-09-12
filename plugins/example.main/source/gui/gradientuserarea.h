#ifndef GRADIENTUSERAREA_H__
#define GRADIENTUSERAREA_H__

#include "ge_math.h"

#define MAXGRADIENT	20

namespace cinema
{

class BaseBitmap;
class GeUserArea;

} // namespace cinema

struct SDKGradient
{
	cinema::Vector col;
	cinema::Float	 pos;
	cinema::Int32	 id;
};

class SDKGradientGadget
{
public:
	SDKGradientGadget();
	~SDKGradientGadget();

	void Init(cinema::GeUserArea* ua, cinema::Int32 interpol, cinema::Int32 maxgrad);
	cinema::Bool InitDim(cinema::Int32 x, cinema::Int32 y);

	cinema::Bool MouseDown(cinema::Int32 x, cinema::Int32 y, cinema::Bool dbl);
	void MouseDrag(cinema::Int32 x, cinema::Int32 y);

	void SetPosition(cinema::Float per);
	cinema::Bool GetPosition(cinema::Float* per);

	void CalcImage();

private:
	cinema::Float YtoP(cinema::Int32 y);
	cinema::Int32 PtoY(cinema::Float pos);
	void GetBoxPosition(cinema::Int32 num, cinema::Int32* x, cinema::Int32* y);
	cinema::Int32 InsertBox(cinema::Vector col, cinema::Float per, cinema::Int32 id);
	cinema::Int32 FindID();

public:
	maxon::UniqueRef<cinema::BaseBitmap> _col;
	cinema::Int32 _interpol = 0;

private:
	cinema::Int32 _iw = 0, _ih = 0, _xmin = 0, _active = 0;
	cinema::Int32 _maxgrad = 0;
	cinema::Int32 _dragx, _dragy, _dragid;
	cinema::Vector _dragcol;
	maxon::BaseArray<SDKGradient> _g;
	cinema::GeUserArea* _ua = nullptr;

};

cinema::Vector CalcGradientMix(const cinema::Vector& g1, const cinema::Vector& g2, cinema::Float per, cinema::Int32 interpol);

#endif // GRADIENTUSERAREA_H__
