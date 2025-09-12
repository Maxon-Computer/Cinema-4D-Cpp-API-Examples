#include "c4d.h"

#include "main.h"
#include "c4d_symbols.h"
#include "ohyperfile.h"

//----------------------------------------------------------------------------------------
/// This ObjectData example demonstrates how to work with HyperFiles and different data types.
///
/// The example consists of two small, simple helper functions and the actual ObjectData class.
//----------------------------------------------------------------------------------------

using namespace cinema;

// At first the two helper functions to print file errors to the console.

//----------------------------------------------------------------------------------------
/// Simple helper function to convert file errors to string.
/// @param[in] hf									Pointer to HyperFile, which current error will be decoded.
/// @return												A String describing the error.
//----------------------------------------------------------------------------------------
static String FileErrorToString(const HyperFile* const hf)
{
	String errString = "";
	if (!hf)
		return errString;

	switch (hf->GetError())
	{
		case FILEERROR::NONE:
			errString = "No error";
			break;
		case FILEERROR::OPEN:
			errString = "Problems opening the file";
			break;
		case FILEERROR::CLOSE:
			errString = "Problems closing the file";
			break;
		case FILEERROR::READ:
			errString = "Problems reading the file";
			break;
		case FILEERROR::WRITE:
			errString = "Problems writing the file";
			break;
		case FILEERROR::SEEK:
			errString = "Problems seeking the file";
			break;
		case FILEERROR::INVALID:
			errString = "Invalid parameter or operation (e.g. writing in read-mode)";
			break;
		case FILEERROR::OUTOFMEMORY:
			errString = "Not enough memory";
			break;
		case FILEERROR::USERBREAK:
			errString = "User break";
			break;
		case FILEERROR::WRONG_VALUE:
			errString = "Other value detected than expected";
			break;
		case FILEERROR::CHUNK_NUMBER:
			errString = "Wrong number of chunks or sub-chunks detected";
			break;
		case FILEERROR::VALUE_NO_CHUNK:
			errString = "There was a value without any enclosing START/STOP chunks";
			break;
		case FILEERROR::FILEEND:
			errString = "The file end was reached without finishing reading";
			break;
		case FILEERROR::UNKNOWN_VALUE:
			errString = "Unknown value detected";
			break;
		default:
			errString = "Unknown error";
			break;
	}
	return errString;
}

//----------------------------------------------------------------------------------------
/// Small helper function.
/// Depending on verbose parameter a file error (if any) will also be decoded to a human readable string.
/// Can also be used, when there's no HyperFile (e.g. HyperFile allocation failed), then only the filename will be used.
/// It returns true, if there was no error, otherwise false.
/// In this way it can be used in snippets to directly return from the calling function.
/// @param[in] fn									Filename, will be used as additional information or in case hf is nullptr.
/// @param[in] hf									Pointer to HyperFile, which current error will be decoded. If nullptr, only provided errText and filename fn will be printed.
/// @param[in] errText						Additional text to be printed with the error message.
/// @param[in] verbose						If true an additional line will be printed, containing the error number decoded to text.
/// @return												Returns false, if a file error occurred or hf is nullptr, otherwise true.
//----------------------------------------------------------------------------------------
static Bool PrintFileError(const Filename& fn, const HyperFile* const hf, const String& errText, Bool verbose = false)
{
	if (!hf)
	{
		ApplicationOutput("Error: " + errText + ": " + fn.GetString());
		return false;
	}

	const FILEERROR err = hf->GetError();
	// TODO: (Sebastian Bach): please adapt the print code for the R20 enum classes
	const Int32 ERROR_NAME = -1;
	if (err != FILEERROR::NONE)
		ApplicationOutput("Error (" + String::IntToString(ERROR_NAME) + "): " + errText + ": " + fn.GetString());
	else
		ApplicationOutput("No error: " + fn.GetString());

	if (verbose)
		ApplicationOutput("  " + String::IntToString(ERROR_NAME) + ": " + FileErrorToString(hf)); // FileErrorToString() is a custom function.
	return err == FILEERROR::NONE;
}


//----------------------------------------------------------------------------------------
/// ObjectData example to demonstrate HyperFile access in Read() and Write().
///
/// Alloc() -							The usual class allocator, just return a new object of the class. \n
/// Free() -							Upon Free() everything allocated by the plugin has to be destroyed, \n
///												in this example the images, that might have been added by the user. \n
/// Init() -							Initialize parameters and set any member variables in a defined state.
///
/// Read() -							Called when the object gets loaded from a HyperFile (e.g. upon scene load). \n
///												One needs to care for member variables, that can't be handled by Cinema 4D directly. \n
///												Take care, the order of reads has to match the order of writes in Write(). \n
/// Write() -							Called when the object gets stored in a HyperFile (e.g. when a scene is saved). \n
///												One needs to care for member variables, that can't be handled by Cinema 4D directly. \n
///												Take care, the order of writes has to match the order of reads in Read(). \n
/// CopyTo() -						Called when the object gets copied (e.g. when a scene gets rendered to Picture Viewer or \n
///												in Undo/Redo situations). \n
///												Make sure to copy data stored in member variables to the destination node.
///
/// GetVirtualObjects() -	Usually one of the main functions of an ObjectData (generator) plugin, \n
///												returning the generated geometry. \n
///												In this case just returning a cube, so there's something to see in the viewport. \n
/// Message() -						Reacts to the button presses of the user, either adding an image to the object or \n
///												showing the contained images in the Picture Viewer.
///
/// FreeImages() -				Just a custom helper function to free the images/BaseBitmaps pointed to by a BaseArray.
//----------------------------------------------------------------------------------------
class ObjectDataHyperFileExample : public ObjectData
{
	INSTANCEOF(ObjectDataHyperFileExample, ObjectData);

public:
	static NodeData* Alloc();
	virtual void Free(GeListNode* node);
	virtual Bool Init(GeListNode* node, Bool isCloneInit);

	virtual Bool Read(GeListNode* node, HyperFile* hf, Int32 level);
	virtual Bool Write(const GeListNode* node, HyperFile* hf) const;
	virtual Bool CopyTo(NodeData* dest, const GeListNode* snode, GeListNode* dnode, COPYFLAGS flags, AliasTrans* trn) const;

	virtual BaseObject* GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh);
	virtual Bool Message(GeListNode* node, Int32 type, void* data);

private:
	static void FreeImages(maxon::BaseArray<BaseBitmap*>& bmps);  // free all images contained in a BaseArray

	// Member variables are neither automatically handled,
	// when an object is written to (Write()) or read from (Read()) a HyperFile,
	// nor when the object gets copied (CopyTo()).
	Vector												_myProprietaryColor;		///< A random color, will be changed on every save.
	maxon::BaseArray<BaseBitmap*>	_myBitmaps;							///< Stores all images added by the user.
};

NodeData* ObjectDataHyperFileExample::Alloc()
{
	iferr (ObjectDataHyperFileExample* result = NewObj(ObjectDataHyperFileExample))
	{
		return nullptr;
	}
	return result;
}

void ObjectDataHyperFileExample::FreeImages(maxon::BaseArray<BaseBitmap*>& bmps)
{
	BaseBitmap* bmp = nullptr;

	// Free all BaseBitmap stored in the BaseArray (the BaseArray stores only the pointers and has to be free'd on its own).
	while (bmps.Pop(&bmp))
	{
		BaseBitmap::Free(bmp);
	}
}

void ObjectDataHyperFileExample::Free(GeListNode* node)
{
	FreeImages(_myBitmaps);  // The BaseArray itself will be free'd upon destruction of "this".
}

Bool ObjectDataHyperFileExample::Init(GeListNode* node, Bool isCloneInit)
{
	_myProprietaryColor = Vector(0.9, 0.7, 0.0);
	return true;
}

Bool ObjectDataHyperFileExample::Read(GeListNode* node, HyperFile* hf, Int32 level)
{
	// Try to get the filename of the HyperFile, in this case only used in error printing.
	Filename fn = Filename("");
	if (hf->GetDocument())
		fn = hf->GetDocument()->GetDocumentName();

	// Skip the first value (in Write() we wrote integer 42 as first value).
	HYPERFILEVALUE hfValueHeader;
	if (!hf->ReadValueHeader(&hfValueHeader))
		return PrintFileError(fn, hf, "Failed to read value header from");
	if (hfValueHeader == HYPERFILEVALUE::INT32)
	{
		// Note: SkipValue() only works in conjunction with ReadHeaderValue().
		if (!hf->SkipValue(HYPERFILEVALUE::INT32))
			return PrintFileError(fn, hf, "Failed to skip value in");
	}

	// Read color from the HyperFile (Note: For example purposes in Write() this value gets randomized on every save).
	if (!hf->ReadVector(&_myProprietaryColor))
		return PrintFileError(fn, hf, "Failed to read vector from");

	// Read a chunk of data.
	if (!hf->ReadValueHeader(&hfValueHeader))
		return PrintFileError(fn, hf, "Failed to read value header of chunk from");
	if (hfValueHeader == HYPERFILEVALUE::START)
	{
		Int32 chunkId;
		Int32 chunkLevel;
		// Note: ReadChunkStart() only works in conjunction with ReadHeaderValue().
		if (!hf->ReadChunkStart(&chunkId, &chunkLevel))
			return PrintFileError(fn, hf, "Failed to read start of chunk from");
		if (chunkId == 123 && chunkLevel == 0)
		{
			// It's the expected chunk (ID) with expected version (level).
			// Let's read only the first character from the chunk and then skip to the end.
			Char c;
			if (!hf->ReadChar(&c))
				return PrintFileError(fn, hf, "Failed to read Char from");
		}
	}
	// Make sure to skip to the chunk's end, even if it was not the expected one.
	// It is recommended (i.e. less error prone) even if all chunk values were read and ReadChunkEnd() could be used.
	if (!hf->SkipToEndChunk())
		return PrintFileError(fn, hf, "Failed to skip to end of chunk in");

	// Read the value after the chunk (the number of saved bitmaps).
	Int64 numBitmaps;
	if (!hf->ReadInt64(&numBitmaps))
		return PrintFileError(fn, hf, "Failed to read number of bitmaps after chunk from");
	// Then read all bitmaps from the HyperFile (if any).
	for (Int32 idxBitmap = 0; idxBitmap < numBitmaps; ++idxBitmap)
	{
		AutoAlloc<BaseBitmap> bmp;
		if (!bmp || !hf->ReadImage(bmp))
			return PrintFileError(fn, hf, "Failed to read image from");
		iferr (_myBitmaps.Append(bmp.Release())) // Note: Releasing bitmap from AutoAlloc, as the BaseArray owns the BaseBitmap from now on.
			return false;
	}

	return SUPER::Read(node, hf, level);
}

Bool ObjectDataHyperFileExample::Write(const GeListNode* node, HyperFile* hf) const
{
	// Try to get the filename of the HyperFile, in this case only used in error printing.
	Filename fn = Filename("");
	if (hf->GetDocument())
		fn = hf->GetDocument()->GetDocumentName();

	// Randomize our color.
	Random rnd;
	rnd.Init(1234);
	MAXON_REMOVE_CONST(this)->_myProprietaryColor = Vector(rnd.Get01(), rnd.Get01(), rnd.Get01());

	// Write an arbitrary value, so we have something to skip in Read().
	if (!hf->WriteInt32(42))
		return PrintFileError(fn, hf, "Failed to write Int32 to");

	// Store color in the HyperFile.
	if (!hf->WriteVector(_myProprietaryColor))
		return PrintFileError(fn, hf, "Failed to write Vector to");

	// Store a chunk of data
	const Int32 chunkId = 123; // Recommended to use for browsing through a file in a loop and then acting on chunk ID.
	const Int32 chunkLevel = 0; // Recommended to use for versioning of chunks.
	if (!hf->WriteChunkStart(chunkId, chunkLevel))
		return PrintFileError(fn, hf, "Failed to write start of chunk to");

	// Write a few bytes into the chunk.
	const Char* const chars = "Whatever";
	for (UInt32 idxByte = 0; idxByte < sizeof(chars); ++idxByte)
	{
		if (!hf->WriteChar(chars[idxByte]))
			return PrintFileError(fn, hf, "Failed to write Char into the chunk in");
	}
	if (!hf->WriteChunkEnd())
		return PrintFileError(fn, hf, "Failed to write end of chunk in");

	// Finally write the bitmaps (if any), so we have something to read after the chunk.
	// First write the number of bitmaps, so we know what to do in Read().
	if (!hf->WriteInt64(_myBitmaps.GetCount()))
		return PrintFileError(fn, hf, "Failed to write number of bitmaps after chunk in");
	// Then write all bitmaps to the HyperFile.
	for (Int32 idxBitmap = 0; idxBitmap < _myBitmaps.GetCount(); ++idxBitmap)
	{
		if (!hf->WriteImage(_myBitmaps[idxBitmap], FILTER_TIF, nullptr, SAVEBIT::NONE))
			return PrintFileError(fn, hf, "Failed to write image into");
	}

	return SUPER::Write(node, hf);
}

Bool ObjectDataHyperFileExample::CopyTo(NodeData* dest, const GeListNode* snode, GeListNode* dnode, COPYFLAGS flags, AliasTrans* trn) const
{
	// Not actually related to the HyperFile, CopyTo() got implemented for completeness, only.
	// The data stored in member variables needs to be handled here.
	if (!SUPER::CopyTo(dest, snode, dnode, flags, trn))
		return false;
	ObjectDataHyperFileExample* const destODHFE = static_cast<ObjectDataHyperFileExample*>(dest);

	// Copy color vector.
	destODHFE->_myProprietaryColor = _myProprietaryColor;

	// Just in case destination already contains images, throw them away.
	FreeImages(destODHFE->_myBitmaps);

	// Copy bitmaps.
	for (const auto& iterBmp : _myBitmaps)
	{
		BaseBitmap* const bmp = iterBmp->GetClone();
		if (!bmp)
			continue;
		iferr (destODHFE->_myBitmaps.Append(bmp))
			return false;
	}
	return true;
}

// In this example Message() has no further relevance.
// We do only process the two buttons, so user can add and show the images stored in the object (and saved and loaded from the HyperFile).
Bool ObjectDataHyperFileExample::Message(GeListNode* node, Int32 type, void* data)
{
	switch (type)
	{
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand* const dc = (DescriptionCommand*)data;
			const Int32 id = dc->_descId[0].id;

			switch (id)
			{
				case ID_BUTTON_ADD_IMAGE:
				{
					Filename fn;
					if (!fn.FileSelect(FILESELECTTYPE::IMAGES, FILESELECT::LOAD, "Open an image"_s))
						return true;
					AutoAlloc<BaseBitmap> bmp;
					if (!bmp || bmp->Init(fn) != IMAGERESULT::OK)
						return false;
					iferr (_myBitmaps.Append(bmp.Release()))
						return false;
					return true;
				}

				case ID_BUTTON_SHOW_IMAGES:
				{
					for (const auto& iterBmp : _myBitmaps)
					{
						ShowBitmap(iterBmp);
					}
					return true;
				}
			}
			break;
		}
	}

	return SUPER::Message(node, type, data);
}

// In this example GetVirtualObjects has no further relevance.
// We just create a cube and set its view color to the color randomized on every save,
// so there's something to see in the viewport.
BaseObject* ObjectDataHyperFileExample::GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh)
{
	Bool dirty = op->CheckCache(hh) || op->IsDirty(DIRTYFLAGS::DATA);
	if (!dirty)
		return op->GetCache();

	BaseObject* const cube = BaseObject::Alloc(Ocube);
	if (!cube)
		return nullptr;

	// Set display color to the random color stored in the member variable.
	ObjectColorProperties prop;
	cube->GetColorProperties(&prop);
	prop.usecolor = ID_BASEOBJECT_USECOLOR_ALWAYS;
	prop.color = _myProprietaryColor;
	cube->SetColorProperties(&prop);
	return cube;
}


/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_SDK_OBJECTDATA_HYPERFILEEXAMPLE = 1039637;  // Always request a unique Plugin ID from Plugin Cafe: developers.maxon.net/forum/developer.asp

Bool RegisterObjectHyperFileExample()
{
	return RegisterObjectPlugin(ID_SDK_OBJECTDATA_HYPERFILEEXAMPLE, GeLoadString(IDS_OBJECTDATA_HYPERFILEEXAMPLE), OBJECT_GENERATOR | OBJECT_USECACHECOLOR, ObjectDataHyperFileExample::Alloc, "Ohyperfile"_s, nullptr, 0);
};
