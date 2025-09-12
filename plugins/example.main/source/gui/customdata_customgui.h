/*
	Dots CustomDataType & CustomGui Example
	(C) 2021 MAXON Computer GmbH

	Author: Sebastian Bach
	Date: 13/12/2021

	Showcases how to use custom data types in combination with custom GUIs.

  The type iCustomDataTypeDots is the actual data object that stores the custom data, where
	DotsCustomDataTypeClass is the plugin class that represents the new data type within Cinema 4D, 
	handling the underlying iCustomDataTypeDots data.

  SDKExampleCustomGUIDots is the plugin class that represents a new GUI element for that data
  type. It creates an iExampleCustomGUIDots dialog that realizes the GUI for that data type. The
  dialog includes the GeUserArea implementation DotsUserArea which handles the rendering of the
  custom data in a GUI.

  This example have been modified to be used a node inside the node editor.
*/
#ifndef CUSTOMDATA_CUSTOMGUI_H__
#define CUSTOMDATA_CUSTOMGUI_H__

#include "c4d_baselist.h"
#include "c4d_customdatatype.h"
#include "c4d_gui.h"

static constexpr const maxon::Int32 id_sdk_example_customgui_dots = 1035302;	///< The ID of the custom GUI
static constexpr const maxon::Int32 id_sdk_example_customdatatype_dots = 1035303; ///< The ID of the custom datatype

class DotsCustomDataTypeClass; // forward declaration

//---------------------------
/// the actual data
//---------------------------
class iCustomDataTypeDots : public cinema::iCustomDataType<iCustomDataTypeDots, id_sdk_example_customdatatype_dots>
{
	friend class DotsCustomDataTypeClass;

public:
	iCustomDataTypeDots() = default;
	explicit iCustomDataTypeDots(const maxon::BaseArray<cinema::Vector> &points);

	//---------------------------
	// The example "how to use a Cinema API UI and Datatype inside the node editor" 
	// need some extra code.
	// In order to use the Cinema API data type and its UI in the node editor, 
	// a copy constructor is mandatory, an assignment operator, and 
	// the DescribeIO function or the data will not be properly saved to the cinema4D's project file.
	
	iCustomDataTypeDots(const iCustomDataTypeDots& cdtd);
	
	void operator =(const iCustomDataTypeDots& b)
	{
		_points.Flush();
		_points.CopyFrom(b._points) iferr_ignore("Simply ignore copy from error"_s);
	}
		
	MAXON_OPERATOR_EQUALITY(iCustomDataTypeDots, _points);

	/// DescribeIO is mandatory otherwise, maxon Data cannot be saved.
	static maxon::Result<void> DescribeIO(const maxon::DataSerializeInterface& stream);
	//---------------------------
	
	// The accessors that will be used in several functions.
	maxon::BaseArray<cinema::Vector> GetValue() const;
	void SetValue(const maxon::BaseArray<cinema::Vector> &value);


	// an array of Vectors
	maxon::BaseArray<cinema::Vector> _points;

};
// For the Node API example, customDataTypeDot need to be registered as a Maxon Data.
MAXON_DATATYPE(iCustomDataTypeDots, "net.maxonsdk.datatype.iCustomDataTypeDots");


//---------------------------
/// The user area used to display the custom datatype
//---------------------------
class DotsUserArea : public cinema::GeUserArea
{
public:
	DotsUserArea();
	virtual ~DotsUserArea();

  virtual cinema::Bool Init();
  virtual cinema::Bool InitValues();
  virtual cinema::Bool GetMinSize(cinema::Int32& w, cinema::Int32& h);

  virtual void DrawMsg(cinema::Int32 x1, cinema::Int32 y1, cinema::Int32 x2, cinema::Int32 y2, const cinema::BaseContainer& msg);
  virtual cinema::Bool InputEvent(const cinema::BaseContainer& msg);

	iCustomDataTypeDots* _data = nullptr;
};

//------------
/// the cinema::iCustomGui dialog hosting the user area 
//------------
enum
{
	ID_USERAREA = 2000, ///< The ID of the user area GUI element.
};

class iExampleCustomGUIDots : public cinema::iCustomGui
{
	INSTANCEOF(iExampleCustomGUIDots, cinema::iCustomGui)
public:
	iExampleCustomGUIDots(const cinema::BaseContainer &settings, cinema::CUSTOMGUIPLUGIN *plugin);
	virtual cinema::Bool CreateLayout();
	virtual cinema::Bool InitValues();
	virtual cinema::Bool Command(cinema::Int32 id, const cinema::BaseContainer &msg);
	virtual cinema::Bool SetData(const cinema::TriState<cinema::GeData> &tristate);
	virtual cinema::TriState<cinema::GeData> GetData();

private:
	cinema::Bool _tristate;
	iCustomDataTypeDots _data;
	DotsUserArea _dotsUserArea;
};

//---------------------
/// custom GUI data
//---------------------
static cinema::Int32 g_stringtable[] = { id_sdk_example_customdatatype_dots };

class SDKExampleCustomGUIDots : public cinema::CustomGuiData
{
public:
	virtual cinema::Int32 GetId();
  virtual cinema::CDialog* Alloc(const cinema::BaseContainer& settings);
  virtual void Free(cinema::CDialog* dlg, void* userdata);
  virtual const cinema::Char* GetResourceSym();
  virtual cinema::CustomProperty* GetProperties();
  virtual cinema::Int32 GetResourceDataType(cinema::Int32*& table);
};

//---------------------------
/// The datatype class
//---------------------------
class DotsCustomDataTypeClass : public cinema::CustomDataTypeClass
{
  INSTANCEOF(DotsCustomDataTypeClass, cinema::CustomDataTypeClass)

public:
  virtual cinema::Int32 GetId();
  virtual cinema::CustomDataType* AllocData();
  virtual void FreeData(cinema::CustomDataType* data);
  virtual cinema::Bool CopyData(const cinema::CustomDataType* src, cinema::CustomDataType* dst, cinema::AliasTrans* aliastrans);
  virtual cinema::Int32 Compare(const cinema::CustomDataType* d1, const cinema::CustomDataType* d2);
  virtual cinema::Bool WriteData(const cinema::CustomDataType* t_d, cinema::HyperFile* hf);
  virtual cinema::Bool ReadData(cinema::CustomDataType* t_d, cinema::HyperFile* hf, cinema::Int32 level);
  virtual const cinema::Char* GetResourceSym();
  virtual void GetDefaultProperties(cinema::BaseContainer& data);
};

//---------------------------
/// Register
//---------------------------
cinema::Bool RegisterCustomDatatypeCustomGUI();

#include "customdata_customgui1.hxx"
#include "customdata_customgui2.hxx"

#endif // CUSTOMDATA_CUSTOMGUI_H__
