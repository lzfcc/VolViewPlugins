/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVVPluginSelector - this class represents a plugin for VolView

#ifndef __vtkVVPluginSelector_h
#define __vtkVVPluginSelector_h

#include "vtkKWCompositeWidget.h"
#include "vtkKWVolViewConfigure.h" // KWVolView_PLUGINS_USE_SPLINE and such

class vtkImageData;
class vtkKWFrame;
class vtkKWMenuButtonWithLabel;
class vtkKWPushButton;
class vtkVVPlugin;
class vtkVVWindowBase;
class vtkVVPluginInterface;

//BTX
template<class DataType> class vtkVector;
template<class DataType> class vtkVectorIterator;
//ETX

class VTK_EXPORT vtkVVPluginSelector : public vtkKWCompositeWidget
{
public:
  static vtkVVPluginSelector* New();
  vtkTypeRevisionMacro(vtkVVPluginSelector,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the window (do not ref count it since the window will ref count
  // this widget).
  vtkGetObjectMacro(Window, vtkVVWindowBase);
  virtual void SetWindow(vtkVVWindowBase*);

  // Description:
  // Do not ref count it to avoid circular loops. The plugin interface is to 
  // be set by the PluginInterface itself on this plugin as 
  //   this->Plugin->SetPluginInterface(this);
  virtual void SetPluginInterface( vtkVVPluginInterface * );

  // Refresh the interface given the current value of the Window and its
  // views/composites/widgets.
  virtual void Update();

  // Description:
  // Check if we have a plugin given the plugin name (and optionally group).
  // Returns 0 on error, 1 on success.
  virtual int HasPlugin(const char *plugin_name, const char *group = 0);

  // Description:
  // Select a plugin given the plugin name (and optionally group).
  // Returns 0 on error, 1 on success.
  virtual int SelectPlugin(const char *plugin_name, const char *group = 0);
  virtual char* GetSelectedPluginName();
  virtual char* GetSelectedPluginGroup();

  // Description:
  // Apply a plugin given the plugin name (and optionally group), or apply
  // the currently selected plugin.
  // Returns 0 on error, 1 on success.
  virtual int ApplyPlugin(const char *plugin_name, const char *group = 0);
  virtual int ApplySelectedPlugin();

  // Description:
  // Cancel a plugin given the plugin name (and optionally group), or cancel
  // the currently selected plugin, or cancel all plugins.
  virtual void CancelPlugin(const char *plugin_name, const char *group = 0);
  virtual void CancelSelectedPlugin();
  virtual void CancelAllPlugins();

  // Description:
  // Remove (and unload) one or more plugins from the list of available plugin.
  // Note that a PluginFilterListRemovedEvent event is send with
  // the list of available plugins after the deletion.
  // RemovePlugins can be used to remove a whole list of plugins, then send
  // only one event (instead of sending an event for each removal).
  // Returns 0 on error, 1 on success.
  virtual int RemovePlugin(const char *plugin_name, const char *group = 0);
  virtual int RemovePlugins(
    int nb, const char *plugin_names[], const char *groups[]);

  // Description:
  // Allows a plugin to specify undo data
  void SetUndoData(vtkImageData *id);
  
  // Description
  // support for undo and redo
  virtual void Undo();
  virtual void Redo();
//BTX
#ifdef KWVolView_PLUGINS_USE_SPLINE
  virtual void RemoveMesh();
#endif
//ETX
  
  // Description:
  // Callbacks
  virtual void SelectPluginCallback(
    const char *plugin_name, const char *group);
  virtual void ApplyPluginCallback();
  virtual void CancelPluginCallback();
  virtual void UndoCallback();
  virtual void RedoCallback();
//BTX
#ifdef KWVolView_PLUGINS_USE_SPLINE
  virtual void RemoveMeshCallback();
#endif
//ETX

  // Description:
  // Get a plugin. Internal use.
  virtual int GetNumberOfPlugins();
  virtual int GetSelectedPluginIndex();
  virtual vtkVVPlugin* GetPlugin(int index);
  virtual vtkVVPlugin* GetPlugin(
    const char *plugin_name, const char *group = 0);
  virtual int GetPluginIndex(
    const char *plugin_name, const char *group = 0);

  // Description:
  // Write the current list of plugin name/group to a stream
  // Parse the same kind of list and remove all plugins not listed.
  virtual void WriteAvailablePluginsList(ostream &os);
  virtual void RemovePluginsNotInList(istream &is);
  virtual void SendAvailablePluginsListAsEvent(unsigned long event);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Try to find and load plugins. Can be called more than once to refresh
  // the list (add new plugins dynamically).
  // Note: it will generate a PluginFilterListAddedEvent if plugins have been
  // added to the list of available plugins.
  virtual void LoadPlugins();

protected:
  vtkVVPluginSelector();
  ~vtkVVPluginSelector();
  
  // Description:
  // Create the widget
  virtual void CreateWidget();

  vtkVVWindowBase          *Window;
  
  vtkKWMenuButtonWithLabel *PluginsMenu;
  vtkKWPushButton          *ReloadButton;
  vtkKWFrame               *PluginFrame;
  vtkKWPushButton          *ApplyButton;
  vtkKWPushButton          *UndoButton;
#ifdef KWVolView_PLUGINS_USE_SPLINE
  vtkKWPushButton          *RemoveMeshButton;
#endif

  //BTX
  typedef vtkVector<vtkVVPlugin*> PluginsContainer;
  typedef vtkVectorIterator<vtkVVPlugin*> PluginsContainerIterator;
  PluginsContainer *Plugins;
  //ETX

  int SelectedPlugin;

  // when redo or undo we need to propagate the meta info
  virtual void PushNewProperties();
  
  // Description:
  // Are the scalar components of this data independent of each other?
  vtkSetClampMacro(IndependentComponents, int, 0, 1);
  vtkGetMacro(IndependentComponents, int);
  vtkBooleanMacro(IndependentComponents, int);
  int IndependentComponents;

  // Description:
  // Set/Get the scalar units for this dataset (e.g. density, T1, T2, etc)
  virtual const char *GetScalarUnits(int i);
  virtual void SetScalarUnits(int i, const char *units);
  char *ScalarUnits[VTK_MAX_VRCOMP];

  // Description:
  // Set/Get the distance units that pixel sizes are measured in
  vtkSetStringMacro(DistanceUnits);
  vtkGetStringMacro(DistanceUnits);
  char *DistanceUnits;

  // Update the plugin menu
  virtual void UpdatePluginsMenu();
  virtual void UpdatePluginsMenuEnableState();

  // Description:
  // Updates the Undo and redo buttons. This checks the current data item volume,
  // checks if it can be undone or redone for this plugin and displays 
  // the appropriate text.
  virtual void UpdateUndoButton();

  // Update the selection
  virtual void UpdateSelectedPlugin();
  virtual void UpdateSelectedPluginInMenu();

  // Get the name+group string
  //BTX
  virtual void GetPluginPrettyName(
    ostream &os, 
    const char *plugin_name, const char *group = 0, size_t length_max = 0);
  //ETX

  // Remove a single plugin
  virtual int RemoveSinglePlugin(const char *plugin_name, const char *group = 0);

  vtkVVPluginInterface * PluginInterface;

private:
  vtkVVPluginSelector(const vtkVVPluginSelector&); // Not implemented
  void operator=(const vtkVVPluginSelector&); // Not Implemented
};

#endif
