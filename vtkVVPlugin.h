/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVVPlugin - this class represents a plugin for VolView

#ifndef __vtkVVPlugin_h
#define __vtkVVPlugin_h

#include "vtkKWCompositeWidget.h"
#include "vtkVVPluginAPI.h"

#define VTK_VV_PLUGIN_DEFAULT_GROUP "Miscelaneous"

class vtkImageData;
class vtkKWLabel;
class vtkKWLabelWithLabel;
class vtkKWPushButton;
class vtkVVWindowBase;
class vtkVVPluginSelector;
class vtkVVGUIItem;
class vtkKWOpenWizard;
class vtkVV4DOpenWizard;
class vtkKWEPaintbrushDrawing;

class VTK_EXPORT vtkVVPlugin : public vtkKWCompositeWidget
{
public:
  static vtkVVPlugin* New();
  vtkTypeRevisionMacro(vtkVVPlugin,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the window (do not ref count it since the window will ref count
  // this widget).
  vtkGetObjectMacro(Window, vtkVVWindowBase);
  virtual void SetWindow(vtkVVWindowBase*);

  // Description:
  // Refresh the interface given the current value of the Window or Ivars
  virtual void Update();

  // Description:
  // Apply the plugin onto the window's data
  virtual int CanBeExecuted(vtkVVPluginSelector *);
  virtual void Execute(vtkVVPluginSelector *);
  virtual void Cancel(vtkVVPluginSelector *);

  // Description:
  // Load this plugin and return success or failure. Success is zero.
  virtual int Load(const char *pluginDir, vtkKWApplication *app);

  // Used internally for then a plugin is executed in pieces
  float ProgressMinimum;
  float ProgressMaximum;
  
  // Description:
  // Set/Get some properties of the plugin
  vtkGetStringMacro(Name);
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Group);
  vtkSetStringMacro(Group);
  vtkGetStringMacro(TerseDocumentation);
  vtkSetStringMacro(TerseDocumentation);
  vtkGetStringMacro(FullDocumentation);
  vtkSetStringMacro(FullDocumentation);

  vtkGetStringMacro(PlottingXAxisTitle);
  vtkSetStringMacro(PlottingXAxisTitle);
  vtkGetStringMacro(PlottingYAxisTitle);
  vtkSetStringMacro(PlottingYAxisTitle);

  vtkGetStringMacro(ResultingDistanceUnits);
  vtkSetStringMacro(ResultingDistanceUnits);
  vtkGetStringMacro(ResultingComponent1Units);
  vtkSetStringMacro(ResultingComponent1Units);
  vtkGetStringMacro(ResultingComponent2Units);
  vtkSetStringMacro(ResultingComponent2Units);
  vtkGetStringMacro(ResultingComponent3Units);
  vtkSetStringMacro(ResultingComponent3Units);
  vtkGetStringMacro(ResultingComponent4Units);
  vtkSetStringMacro(ResultingComponent4Units);

  vtkGetMacro(NumberOfGUIItems, int);
  vtkGetMacro(RequiresSecondInput, int);
  vtkGetMacro(SecondInputIsUnstructuredGrid, int);
  vtkGetMacro(SecondInputOptional, int);  
  vtkGetMacro(RequiresLabelInput, int);
//BTX
#ifdef KWVolView_PLUGINS_USE_SPLINE
  vtkGetMacro(RequiresSplineSurfaces, int);
#endif
//ETX
//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
  vtkGetMacro(RequiresSeriesInput, int);
  vtkGetMacro(ProducesSeriesOutput, int);
#endif
//ETX
  vtkGetMacro(ProducesPlottingOutput, int);

  // Description:
  // Set/Get the second input filename
  virtual const char* GetSecondInputFileName();
  virtual void SetSecondInputFileName(const char *);
  vtkGetObjectMacro(SecondInputOpenWizard, vtkKWOpenWizard);

//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
  // Description:
  // Set/Get the series input filename
  virtual void GetSeriesInputFileNames(const char **pattern,int & min, int & max);
  virtual void SetSeriesInputFileNames(const char *pattern,int min, int max);
  vtkGetObjectMacro(SeriesInputOpenWizard, vtkVV4DOpenWizard);
  virtual void SetSeriesInputFileName(const char *pattern,int index);
#endif
//ETX

  // Description:
  // Set/Get some properties from the plugin point-of-view
  void SetProperty(int param, const char *value);
  const char *GetProperty(int param);
  void SetGUIProperty(int num, int param, const char *value);
  const char *GetGUIProperty(int num, int param);

  // Description:
  // Convenience method to Set/Get some properties from the plugin 
  // point-of-view using the label name of the property, not its index.
  // The VVP_GUI_LABEL parameter of the property is compared to 'label'
  void SetGUIProperty(const char *label, int param, const char *value);
  const char *GetGUIProperty(const char *label, int param);

  // Description:
  // Convenience method to Set/Get the VVP_GUI_VALUE parameter of a property
  // from the plugin point-of-view, using the label name of the property.
  // The VVP_GUI_LABEL parameter of the property is compared to 'label'
  void SetGUIPropertyValue(const char *label, const char *value);
  const char *GetGUIPropertyValue(const char *label);

  // Description:
  // Set the report and stop/watch text
  virtual void SetReportText(const char *text);
  virtual char* GetReportText();
  virtual void SetStopWatchText(const char *text);
  virtual char* GetStopWatchText();

  // Description:
  // Callback invoked when the users selects a second input
  virtual void SecondInputCallback();

//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
  // Description:
  // Callback invoked when the users selects a series input
  virtual void SeriesInputCallback();
#endif
//ETX

  // Description:
  // Verify paramters and load second and series input if required
  int PreparePlugin(vtkVVPluginSelector *);

//BTX
#ifdef KWVolView_PLUGINS_USE_SPLINE
  // Description:
  // Convert the spline surfaces into a list of points to be used
  // by the plugin. This prevents the plugin from having to know
  // about the mathematical details of the splines used in VolView.
  int PrepareSplineSurfaces();
#endif
//ETX
  
  // Description:
  // Synchronize the GUI and the plugin structure
  void SetGUIValues();
  void GetGUIValues();
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
  // Description:
  // Save volume series that may be produced by a plugin.
  virtual int SaveVolumeSeries(vtkVVProcessDataStruct *pds);
  virtual int SaveVolumeSeries(vtkVVProcessDataStruct *pds, const char *fname);
#endif
//ETX

  // Description:
  // Display the XY plot that may be the output of a plugin
  virtual void DisplayPlot(vtkVVProcessDataStruct *pds);

protected:
  vtkVVPlugin();
  ~vtkVVPlugin();
  
  // Description:
  // Create the widget
  virtual void CreateWidget();

  vtkVVPluginInfo PluginInfo;

  vtkKWWidget **Widgets;
  vtkKWLabel *DocLabel;
  vtkKWLabelWithLabel *DocText;
  vtkKWLabelWithLabel *ReportText;
  vtkKWLabelWithLabel *StopWatchText;
  vtkVVWindowBase *Window;

  virtual void UpdateGUI();

  // push new properties (not ImageData) to the widgets
  virtual void PushNewProperties();
  
  // widget for setting the second input if required
  vtkKWPushButton *SecondInputButton;
  vtkKWOpenWizard *SecondInputOpenWizard;
  virtual void UpdateAccordingToSecondInput();
  
//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
  // widget for setting the series input if required
  vtkKWPushButton *SeriesInputButton;
  vtkVV4DOpenWizard *SeriesInputOpenWizard;
  virtual void UpdateAccordingToSeriesInput();
#endif
//ETX
 
  void ExecuteData(vtkImageData *, vtkVVPluginSelector *);
  void UpdateData(vtkImageData *);
  void ProcessInOnePiece(vtkImageData *input, int memCheck,
                         vtkVVProcessDataStruct *, vtkVVPluginSelector *);
  void ProcessInPieces(vtkImageData *input, int memCheck, 
                       vtkVVProcessDataStruct *);
//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
  void ProcessSeriesByVolumes(vtkImageData *input, int memCheck, 
                       vtkVVProcessDataStruct *, vtkVVPluginSelector *);
#endif
//ETX
  
  // handle memory issues for this plugin, warn if to big
  int CheckMemory(vtkImageData *);

  // these members must be set by the plugin 
  char *Name;
  char *Group;
  char *TerseDocumentation;
  char *FullDocumentation;
  int SupportProcessingPieces;
  int SupportInPlaceProcessing;
//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
  int SupportProcessingSeriesByVolumes;
#endif
//ETX
  int NumberOfGUIItems;
//BTX
#ifdef KWVolView_PLUGINS_USE_SPLINE
  int ProducesMeshOnly;
#endif
//ETX
  int RequiredZOverlap;
  float PerVoxelMemoryRequired;
  int AbortProcessing;
  int RequiresSecondInput;
  int SecondInputIsUnstructuredGrid;
  int SecondInputOptional;  
  int RequiresLabelInput;
//BTX
#ifdef KWVolView_PLUGINS_USE_SPLINE
  int RequiresSplineSurfaces;
#endif
#ifdef KWVolView_PLUGINS_USE_SERIES
  int RequiresSeriesInput;
  int ProducesSeriesOutput;
#endif
//ETX
  int ProducesPlottingOutput;
  char *PlottingXAxisTitle;
  char *PlottingYAxisTitle;

  // why am I not using a fricking map here
  int ResultingComponentsAreIndependent;
  char *ResultingDistanceUnits;
  char *ResultingComponent1Units;
  char *ResultingComponent2Units;
  char *ResultingComponent3Units;
  char *ResultingComponent4Units;
  
  // where the GUI elements are stored
  vtkVVGUIItem *GUIItems;
  
  // Get the label image of the paintbrush widget that's selected. If there
  // is no paintbrush widget selected, this returns NULL
  vtkImageData * GetInputLabelImage();

  // Convenience helper method to get the paintbrush drawing from the
  // selected preset, if any.
  vtkKWEPaintbrushDrawing * GetPaintbrushDrawing();

private:
  vtkVVPlugin(const vtkVVPlugin&); // Not implemented
  void operator=(const vtkVVPlugin&); // Not Implemented
};


#endif



