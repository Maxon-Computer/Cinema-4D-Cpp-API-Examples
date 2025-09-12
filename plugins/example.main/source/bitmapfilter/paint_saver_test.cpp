// this example source code saves out the selected BP texture
// in all combinations (bitdepth, rgb, grey) of all registered
// bitmap filters

#include "c4d_commanddata.h"
#include "c4d_general.h"
#include "c4d_painter.h"
#include "lib_description.h"
#include "c4d_includes.h"
#include "main.h"

using namespace cinema;

#ifdef MAXON_TARGET_DEBUG
/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_BITMAP_SAVER_TEST = 200000109;
#endif

class BitmapSaverTest : public CommandData
{
	INSTANCEOF(BitmapSaverTest, CommandData)

public:
	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager);

	static BitmapSaverTest* Alloc() { return NewObjClear(BitmapSaverTest); }
};


static void CreateImageSaverList(BasePlugin* bp, AtomArray* array)
{
	for (; bp; bp = bp->GetNext())
	{
		if (bp->GetPluginType() == PLUGINTYPE::BITMAPSAVER)
		{
			array->Append(bp);
		}
		if (bp->GetDown())
			CreateImageSaverList(bp->GetDown(), array);
	}
}

static void ExportImage(PaintTexture* tex, BitmapSaverPlugin* bp, COLORMODE colormode, Int32 needbits, SAVEBIT savebits, const String& append)
{
	if (!(bp->GetInfo() & needbits))
		return;

	Int32 saveformat = bp->GetID();

	String suffix;
	Int32	 alpha;
	bp->BmGetDetails(&alpha, &suffix);

	//	if (suffix!="tif") return;

	GeData d_filename;
	tex->GetParameter(ConstDescIDLevel(ID_PAINTTEXTURE_FILENAME), d_filename, DESCFLAGS_GET::NONE);

	Filename fn = d_filename.GetFilename();

	fn.ClearSuffix();

	if (bp->GetName().ToUpper().FindFirst("QUICKTIME", nullptr, 0))
		fn.SetFile(fn.GetFileString() + String("_QUICK_") + suffix + String("_") + append);
	else
		fn.SetFile(fn.GetFileString() + String("_C4D_") + suffix + String("_") + append);
	fn.SetSuffix(suffix);

	PaintTexture* copy = (PaintTexture*)tex->GetClone(COPYFLAGS::NONE, nullptr);
	if (!copy)
		return;

	copy->SetColorMode(colormode, false);

	switch (needbits)
	{
		case PLUGINFLAG_BITMAPSAVER_SUPPORT_8BIT:
		case PLUGINFLAG_BITMAPSAVER_SUPPORT_16BIT:
		case PLUGINFLAG_BITMAPSAVER_SUPPORT_32BIT:
		{
			AutoAlloc<BaseBitmap> merge;
			if (copy->ReCalc(nullptr, 0, 0, copy->GetBw(), copy->GetBh(), merge, RECALC_NOGRID | RECALC_INITBMP, 0))
			{
				BaseContainer data;
				if (merge->Save(fn, saveformat, &data, savebits | SAVEBIT::ALPHA) != IMAGERESULT::OK)
				{
					GeOutString("Error saving image \"" + append + "\"", GEMB::OK);
				}
			}
			break;
		}

		case PLUGINFLAG_BITMAPSAVER_SUPPORT_8BIT_LAYERS:
		case PLUGINFLAG_BITMAPSAVER_SUPPORT_16BIT_LAYERS:
		case PLUGINFLAG_BITMAPSAVER_SUPPORT_32BIT_LAYERS:
		{
			copy->SetParameter(ConstDescIDLevel(ID_PAINTTEXTURE_FILENAME), fn, DESCFLAGS_SET::NONE);
			copy->SetParameter(ConstDescIDLevel(ID_PAINTTEXTURE_SAVEFORMAT), saveformat, DESCFLAGS_SET::NONE);

			BaseContainer bc;
			bc.SetInt32(PAINTER_SAVETEXTURE_FLAGS, PAINTER_SAVEBIT_SAVECOPY);
			SendPainterCommand(PAINTER_SAVETEXTURE, nullptr, copy, &bc);
			break;
		}
	}
	SendPainterCommand(PAINTER_FORCECLOSETEXTURE, nullptr, copy, nullptr);
}

Bool BitmapSaverTest::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	PaintTexture* tex = PaintTexture::GetSelectedTexture();
	if (!tex)
	{
		GeOutString("No Texture Selected"_s, GEMB::OK);
		return false;
	}

	BasePlugin* bpList = GetFirstPlugin();
	AutoAlloc<AtomArray> array;
	CreateImageSaverList(bpList, array);
	Int32 i;
	for (i = 0; i < array->GetCount(); i++)
	{
		BitmapSaverPlugin* bp = (BitmapSaverPlugin*)array->GetIndex(i);

		ExportImage(tex, bp, COLORMODE::RGB, PLUGINFLAG_BITMAPSAVER_SUPPORT_8BIT, SAVEBIT::NONE, "_8Bit_rgb_flat");
		ExportImage(tex, bp, COLORMODE::RGBw, PLUGINFLAG_BITMAPSAVER_SUPPORT_16BIT, SAVEBIT::USE16BITCHANNELS, "16Bit_rgb_flat");
		ExportImage(tex, bp, COLORMODE::RGBf, PLUGINFLAG_BITMAPSAVER_SUPPORT_32BIT, SAVEBIT::USE32BITCHANNELS, "32Bit_rgb_flat");
		ExportImage(tex, bp, COLORMODE::RGB, PLUGINFLAG_BITMAPSAVER_SUPPORT_8BIT_LAYERS, SAVEBIT::MULTILAYER, "_8Bit_rgb_layers");
		ExportImage(tex, bp, COLORMODE::RGBw, PLUGINFLAG_BITMAPSAVER_SUPPORT_16BIT_LAYERS, SAVEBIT::MULTILAYER | SAVEBIT::USE16BITCHANNELS, "16Bit_rgb_layers");
		ExportImage(tex, bp, COLORMODE::RGBf, PLUGINFLAG_BITMAPSAVER_SUPPORT_32BIT_LAYERS, SAVEBIT::MULTILAYER | SAVEBIT::USE32BITCHANNELS, "32Bit_rgb_layers");

		ExportImage(tex, bp, COLORMODE::GRAY, PLUGINFLAG_BITMAPSAVER_SUPPORT_8BIT, SAVEBIT::GREYSCALE, "_8Bit_grey_flat");
		ExportImage(tex, bp, COLORMODE::GRAYw, PLUGINFLAG_BITMAPSAVER_SUPPORT_16BIT, SAVEBIT::GREYSCALE | SAVEBIT::USE16BITCHANNELS, "16Bit_grey_flat");
		ExportImage(tex, bp, COLORMODE::GRAYf, PLUGINFLAG_BITMAPSAVER_SUPPORT_32BIT, SAVEBIT::GREYSCALE | SAVEBIT::USE32BITCHANNELS, "32Bit_grey_flat");
		ExportImage(tex, bp, COLORMODE::GRAY, PLUGINFLAG_BITMAPSAVER_SUPPORT_8BIT_LAYERS, SAVEBIT::GREYSCALE | SAVEBIT::MULTILAYER, "_8Bit_grey_layers");
		ExportImage(tex, bp, COLORMODE::GRAYw, PLUGINFLAG_BITMAPSAVER_SUPPORT_16BIT_LAYERS, SAVEBIT::GREYSCALE | SAVEBIT::MULTILAYER | SAVEBIT::USE16BITCHANNELS, "16Bit_grey_layers");
		ExportImage(tex, bp, COLORMODE::GRAYf, PLUGINFLAG_BITMAPSAVER_SUPPORT_32BIT_LAYERS, SAVEBIT::GREYSCALE | SAVEBIT::MULTILAYER | SAVEBIT::USE32BITCHANNELS, "32Bit_grey_layers");
	}

	return true;
}

Bool RegisterPainterSaveTest()
{
#ifdef MAXON_TARGET_DEBUG
	return RegisterCommandPlugin(ID_BITMAP_SAVER_TEST, String("C++ SDK - BitmapSaverTest"), 0, nullptr, String("BitmapSaverTest"), BitmapSaverTest::Alloc());
#else
	return true;
#endif
}
