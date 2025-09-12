#include "c4d.h"
#include "gradientuserarea.h"

using namespace cinema;

Vector CalcGradientMix(const Vector& g1, const Vector& g2, Float per, Int32 interpol)
{
	switch (interpol)
	{
		default:
		case 0: return g1; break;
		case 1: return g1 + (g2 - g1) * per; break;
		case 2: return g1 + (g2 - g1) * Ln(per * 1.7182819 + 1.0); break;
		case 3: return g1 + (g2 - g1) * (1.0 - Ln((1.0 - per) * 1.7182819 + 1.0)); break;
		case 4: return g1 + (g2 - g1) * Smoothstep(0.0_f, 1.0_f, per); break;
	}
}

static Vector CalcGradientPixel(Float y, const maxon::Block<SDKGradient>& g, Int32 interpol)
{
	Int32 i = 0;
	while (i + 1 < g.GetCount() && y >= g[i + 1].pos)
		i++;

	if (i + 1 < g.GetCount() && y >= g[i].pos)
	{
		Float delta = g[i + 1].pos - g[i].pos;
		if (delta == 0.0)
			return g[i].col;
		return CalcGradientMix(g[i].col, g[i + 1].col, (y - g[i].pos) / delta, interpol);
	}
	else if (i < g.GetCount())
	{
		return g[i].col;
	}
	else
	{
		return Vector(0.0);
	}
}

SDKGradientGadget::SDKGradientGadget()
{
	_col = BaseBitmap::Alloc();
}

SDKGradientGadget::~SDKGradientGadget()
{
}

static constexpr const Int32 BOXRAD = 4;

void SDKGradientGadget::Init(GeUserArea* a_ua, Int32 interpol, Int32 maxgrad)
{
	if (_g.Resize(2) == maxon::FAILED)
		return;

	_g[0].pos = 0.0;
	_g[0].id = 0;
	_g[0].col = Vector(1.0, 1.0, 0.0);
	_g[1].id = 1;
	_g[1].col = Vector(1.0, 0.0, 0.0);
	_g[1].pos = 1.0;

	_ua = a_ua;
	_maxgrad = maxgrad;
	_interpol = interpol;
	_active = Int32(_g.GetCount() - 1);
}

Bool SDKGradientGadget::InitDim(Int32 bw, Int32 bh)
{
	if (!_col)
		return false;
	_iw = bw; _ih = bh;
	_xmin = _iw - _ih * 3 / 2;
	if (_xmin <= 50)
		_xmin = 50;
	if (_col->Init(_iw, _ih, 24) != IMAGERESULT::OK)
		return false;
	CalcImage();
	return true;
}

void SDKGradientGadget::SetPosition(Float per)
{
	if (_active < 0 || _active >= _g.GetCount())
		return;
	per = Clamp01(per);

	SDKGradient gg = _g[_active];
	_g.Erase(_active) iferr_cannot_fail("num must be valid");
	_active = InsertBox(gg.col, per, gg.id);

	CalcImage();
}

Bool SDKGradientGadget::GetPosition(Float* per)
{
	if (_active < 0 || _active >= _g.GetCount())
		return false;
	*per = _g[_active].pos;
	return true;
}

Bool SDKGradientGadget::MouseDown(Int32 x, Int32 y, Bool dbl)
{
	if (x < 0 || y < 0 || x >= _iw || y >= _ih)
		return false;

	Int32 i = 0;
	for (i = 0; i < _g.GetCount(); i++)
	{
		Int32 xp, yp;
		GetBoxPosition(i, &xp, &yp);
		if (x >= xp - BOXRAD && x <= xp + BOXRAD && y >= yp - BOXRAD && y <= yp + BOXRAD)
			break;
	}

	Bool	move = false;
	if (i < _g.GetCount() || ((x >= _xmin && y >= BOXRAD && y < _ih - BOXRAD && _g.GetCount() + 1 <= _maxgrad) && !dbl))
	{
		move = i < _g.GetCount();

		Int32 num = 0;
		Int32 xp, yp;
		if (move || dbl)
		{
			_dragcol = _g[i].col;
			num	= i;
			GetBoxPosition(i, &xp, &yp);
		}
		else
		{
			Float per = YtoP(y);
			_dragcol = CalcGradientPixel(per, _g, _interpol);
			num	 = InsertBox(_dragcol, per, FindID());
			xp	 = x; yp = y;
			move = true;
		}
		
		_active = num;

		_dragx	 = xp - x;
		_dragy	 = yp - y;
		_dragid = _g[num].id;

		if (dbl)
		{
			GeChooseColor(&_g[num].col, 0);
			move = false;
		}

		CalcImage();
	}

	return move;
}

void SDKGradientGadget::MouseDrag(Int32 x, Int32 y)
{
	x += _dragx;
	y += _dragy;

	Float per;
	if (_active != -1 && Abs(YtoP(y) - _g[_active].pos) < 1.0 / Float(_ih - 2 * BOXRAD - 1))
		return;

	per = Truncate(YtoP(y) * 1000.0 + 0.5) * 0.001;
	per = Clamp01(per);
	if (_active != -1 && _g.GetCount() > 2)
		_g.Erase(_active) iferr_cannot_fail("num must be valid");

	if (x >= 0 && x < _iw && y >= -25 && y <= _ih + 25)
		_active = InsertBox(_dragcol, per, _dragid);
	else
		_active = -1;

	CalcImage();
}

Float SDKGradientGadget::YtoP(Int32 y)
{
	return 1.0 - (y - BOXRAD) / Float(_ih - 2 * BOXRAD - 1);
}

Int32 SDKGradientGadget::PtoY(Float pos)
{
	return BOXRAD + Int32((1.0 - pos) * Float(_ih - 2 * BOXRAD - 1) + 0.01);
}

void SDKGradientGadget::GetBoxPosition(Int32 num, Int32* x, Int32* y)
{
	iferr_scope_handler { err.CritStop(); return; };
	maxon::BaseArray<UChar> field;
	maxon::BaseArray<Int32> sort;

	if (_ih <= 0 || _maxgrad <= 0)
		return;
	
	field.Resize(_ih) iferr_return;
	sort.Resize(_maxgrad) iferr_return;

	*x = _xmin - BOXRAD - 6;
	*y = PtoY(_g[num].pos);

	for (Int32 i = 0; i < _g.GetCount(); i++)
		sort[i] = i;

	// bubble sort
	for (Int32 i = 0; i < _g.GetCount() - 1; i++)
	{
		for (Int32 j = i + 1; j < _g.GetCount(); j++)
		{
			if (_g[sort[i]].id > _g[sort[j]].id)
				Swap(sort[i], sort[j]);
		}
	}

	Int32	 pos = 0;
	for (Int32 i = 0; i < _g.GetCount(); i++)
	{
		Int32 j = sort[i];
		Int32 ys = PtoY(_g[j].pos);

		Int32 mask = 0;
		for (Int32 yi = ys - BOXRAD; yi <= ys + BOXRAD; yi++)
			mask |= field[yi];

		if (!(mask & 1))
			pos = 0;
		else if (!(mask & 2))
			pos = 1;
		else if (!(mask & 4))
			pos = 2;
		else if (!(mask & 8))
			pos = 3;
		else
			pos = 4;

		if (j == num)
			break;

		for (Int32 yi = ys - BOXRAD; yi <= ys + BOXRAD; yi++)
			field[yi] |= 1 << pos;
	}

	*x -= (2 * BOXRAD + 1) * pos;
}

Int32 SDKGradientGadget::FindID()
{
	iferr_scope_handler { err.CritStop(); return 0; };
	maxon::BaseArray<Char> used;
	
	used.Resize(_maxgrad) iferr_return;
	for (Int32 i = 0; i < _g.GetCount(); i++)
		used[_g[i].id] = true;

	for (Int32 id = 0; id < _maxgrad; id++)
	{
		if (!used[id])
			return id;
	}

	return 0;
}

Int32 SDKGradientGadget::InsertBox(Vector color, Float per, Int32 id)
{
	Int num = 0;
	while ((num < _g.GetCount()) && (per > _g[num].pos))
		num++;

	if (_g.Insert(num, color, per, id) == maxon::FAILED)
		return 0;

	return Int32(num);
}

void SDKGradientGadget::CalcImage()
{
	if (_col == nullptr || _ua == nullptr)
		return;

	_ua->DrawSetPen(COLOR_BG);
	_ua->FillBitmapBackground(_col, 0, 0);

	Int32 x = 0;
	Int32 y = 0;
	for (Int32 pass = 0; pass < 3; pass++)
	{
		for (Int32 i = 0; i < _g.GetCount(); i++)
		{
			if (pass == 0)
			{
				GetBoxPosition(i, &x, &y);
				_col->SetPen(0, 0, 0);
				if (x >= 0)
					_col->Line(x, y, _xmin - 1, y);
			}
			else
			{
				if ((pass == 2) != (i == _active))
					continue;

				GetBoxPosition(i, &x, &y);

				UInt16 rr = UInt16(_g[i].col.x * COLORTOINT_MULTIPLIER);
				UInt16 gg = UInt16(_g[i].col.y * COLORTOINT_MULTIPLIER);
				UInt16 bb = UInt16(_g[i].col.z * COLORTOINT_MULTIPLIER);

				if (pass == 2)
					_col->SetPen(255, 255, 255);
				else
					_col->SetPen(0, 0, 0);

				if (x - BOXRAD > 0)
				{
					_col->Line(x - BOXRAD, y + BOXRAD, x + BOXRAD, y + BOXRAD);
					_col->Line(x + BOXRAD, y - BOXRAD, x + BOXRAD, y + BOXRAD);
					_col->Line(x - BOXRAD, y - BOXRAD, x + BOXRAD, y - BOXRAD);
					_col->Line(x - BOXRAD, y - BOXRAD, x - BOXRAD, y + BOXRAD);

					_col->Clear(x - BOXRAD + 1, y - BOXRAD + 1, x + BOXRAD - 1, y + BOXRAD - 1, rr, gg, bb);
				}
			}
		}
	}

	Vector v;
	for (y = BOXRAD; y < _ih - BOXRAD; y++)
	{
		v	= CalcGradientPixel(YtoP(y), _g, _interpol);
		UInt16 rr = UInt16(v.x * COLORTOINT_MULTIPLIER);
		UInt16 gg = UInt16(v.y * COLORTOINT_MULTIPLIER);
		UInt16 bb = UInt16(v.z * COLORTOINT_MULTIPLIER);
		_col->SetPen(rr, gg, bb);
		_col->Line(_xmin, y, _iw - 1, y);
	}
}

