/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkObjectFactory.h"

#include "vtkDirectory.h"
#include "vtkDynamicLoader.h"
#include "vtkImageData.h"

#include "vtkKWApplication.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMenuButtonWithLabel.h"
#include "vtkKWProgressGauge.h"
#include "vtkKWPushButton.h"
#include "vtkKWVolumeWidget.h"
#include "vtkKWOptions.h"

#include "vtkVVDataItemVolume.h"
#include "vtkVVPlugin.h"
#include "vtkVVPluginInterface.h"
#include "vtkVVPluginSelector.h"
#include "vtkVVWindowBase.h"
#include "vtkVVSelectionFrame.h"
#include "vtkVector.txx"
#include "vtkVectorIterator.txx"
#include "vtkVolumeProperty.h"
#include "vtkVVSelectionFrame.h"
#include "vtkVVSelectionFrameLayoutManager.h"

#define PLUGINS_DO_NOT_SUPPORT_XML_RW 1

#ifndef PLUGINS_DO_NOT_SUPPORT_XML_RW
#include "XML/vtkXMLVVFiltersReader.h"
#include "XML/vtkXMLVVFiltersWriter.h"
#include "XML/vtkXMLVVPluginWriter.h"
#endif

#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

#ifdef _MSC_VER
#include <direct.h>
#endif

#include <vtkstd/string>
#include <vtkstd/map>
#include <time.h>

#define VTK_VV_PLUGINS_FULLNAME_LENGTH_MAX 50
#define VTK_VV_PLUGINS_UNDO_TEXT "Undo Last Applied Plugin"
#define VTK_VV_PLUGINS_REDO_TEXT "Redo Last Applied Plugin"
#define VTK_VV_PLUGINS_NO_UNDO_TEXT "Undo Not Available"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkVVPluginSelector );
vtkCxxRevisionMacro(vtkVVPluginSelector, "$Revision: 1.17 $");

//----------------------------------------------------------------------------
vtkVVPluginSelector::vtkVVPluginSelector()
{
  this->Window = 0;
  
  this->Plugins = vtkVVPluginSelector::PluginsContainer::New();
  this->SelectedPlugin = -1;

  // UI

  this->PluginsMenu      = vtkKWMenuButtonWithLabel::New();
  this->ReloadButton     = vtkKWPushButton::New();
  this->PluginFrame      = vtkKWFrame::New();
  this->ApplyButton      = vtkKWPushButton::New();
  this->UndoButton       = vtkKWPushButton::New();
#ifdef KWVolView_PLUGINS_USE_SPLINE
  this->RemoveMeshButton = vtkKWPushButton::New();
#endif

  int i;
  for (i = 0; i < VTK_MAX_VRCOMP; i++)
    {
    this->ScalarUnits[i] = NULL;
    }
  this->DistanceUnits = 0;

  this->PluginInterface = NULL;
}

//----------------------------------------------------------------------------
vtkVVPluginSelector::~vtkVVPluginSelector()
{
  // Free all of the widgets

  if (this->PluginsMenu)
    {
    this->PluginsMenu->Delete();
    this->PluginsMenu = NULL;
    }

  if (this->ReloadButton)
    {
    this->ReloadButton->Delete();
    this->ReloadButton = NULL;
    }
    
  if (this->PluginFrame)
    {
    this->PluginFrame->Delete();
    this->PluginFrame = NULL;
    }

  if (this->ApplyButton)
    {
    this->ApplyButton->Delete();
    this->ApplyButton = NULL;
    }
    
  if (this->UndoButton)
    {
    this->UndoButton->Delete();
    this->UndoButton = NULL;
    }

#ifdef KWVolView_PLUGINS_USE_SPLINE
  if (this->RemoveMeshButton)
    {
    this->RemoveMeshButton->Delete();
    this->RemoveMeshButton = NULL;
    }
#endif

  // Delete all the plugins

  vtkVVPluginSelector::PluginsContainerIterator *it = this->Plugins->NewIterator();
  while (!it->IsDoneWithTraversal())
    {
    vtkVVPlugin *plugin = NULL;
    if (it->GetData(plugin) == VTK_OK && plugin)
      {
      plugin->Delete();
      }
    it->GoToNextItem();
    }
  it->Delete();
  this->Plugins->Delete();
  this->Plugins = NULL;

  int i;
  for (i = 0; i < VTK_MAX_VRCOMP; i++)
    {
    this->SetScalarUnits(i, 0);
    }
  this->SetDistanceUnits(0);
}

//----------------------------------------------------------------------------
int vtkVVPluginSelector::GetNumberOfPlugins()
{
  if (this->Plugins)
    {
    return this->Plugins->GetNumberOfItems();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::SetWindow(vtkVVWindowBase *arg)
{
  if (this->Window == arg)
    {
    return;
    }
  this->Window = arg;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::SetPluginInterface(vtkVVPluginInterface *interf)
{
  if (this->PluginInterface == interf)
    {
    return;
    }
  this->PluginInterface = interf;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::GetPluginPrettyName(
  ostream &os, const char *plugin_name, const char *_group, size_t length_max)
{
  if (!plugin_name)
    {
    return;
    }

  const char *sep = " : ";

  const char *group = 
    (_group && *_group) ? _group : VTK_VV_PLUGIN_DEFAULT_GROUP;

  if (length_max > 0)
    {
    size_t group_length = strlen(group);
    size_t sep_length = strlen(sep);
    size_t plugin_name_length = strlen(plugin_name);

    if (length_max > (group_length + sep_length + plugin_name_length))
      {
      os << group << sep << plugin_name;
      }
    else if (length_max > 5 + sep_length + 5)
      {
      length_max -= sep_length;

      size_t group_length_max = (size_t)
        ((float)length_max * 
         (float)group_length / (float)(group_length + plugin_name_length));

      vtksys_stl::string group_crop = 
        vtksys::SystemTools::CropString(group, group_length_max);
      vtksys_stl::string plugin_name_crop = 
        vtksys::SystemTools::CropString(plugin_name, length_max - group_length_max);

      os << group_crop.c_str() << sep << plugin_name_crop.c_str();
      }
    }
  else
    {
    os << group << sep << plugin_name;
    }
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  ostrstream tk_cmd;

  // --------------------------------------------------------------
  // Plugins menu

  this->PluginsMenu->SetParent(this);
  this->PluginsMenu->Create();
  this->PluginsMenu->GetLabel()->SetText("Plugin:");
  this->PluginsMenu->ExpandWidgetOn();

  tk_cmd << "pack " << this->PluginsMenu->GetWidgetName()
         << " -side top -padx 0 -pady 2 -anchor w -fill x -expand y" << endl;

  // --------------------------------------------------------------
  // Reload plugins

  this->ReloadButton->SetParent(this->PluginsMenu);
  this->ReloadButton->Create();
  this->ReloadButton->SetImageToPredefinedIcon(
    vtkKWIcon::IconCrystalProject16x16ActionsRotate);
  this->ReloadButton->SetCommand(this, "LoadPlugins");
  this->ReloadButton->SetBalloonHelpString(
    "Refresh the list of plugins/plugins.");

  tk_cmd << "pack " << this->ReloadButton->GetWidgetName()
         << " -side right -padx 2 -pady 0 -fill y" << endl;

  // --------------------------------------------------------------
  // Plugin frame

  this->PluginFrame->SetParent(this);
  this->PluginFrame->Create();

  tk_cmd << "pack " << this->PluginFrame->GetWidgetName()
         << " -side top -padx 0 -pady 2 -fill both -expand y" << endl;

  this->LoadPlugins();

  // --------------------------------------------------------------
  // Apply plugin

  this->ApplyButton->SetParent(this);
  this->ApplyButton->Create();
  this->ApplyButton->SetText("Apply Plugin");
  this->ApplyButton->SetCommand(this, "ApplyPluginCallback");

  tk_cmd << "pack " << this->ApplyButton->GetWidgetName()
         << " -side left -padx 2 -pady 2 -fill x -expand y" << endl;

  // --------------------------------------------------------------
  // Undo plugin

  this->UndoButton->SetParent(this);
  this->UndoButton->Create();
  this->UndoButton->SetText(VTK_VV_PLUGINS_UNDO_TEXT);
  this->UndoButton->SetCommand(this, "UndoCallback");

  tk_cmd << "pack " << this->UndoButton->GetWidgetName()
         << " -side left -padx 2 -pady 2 -fill x -expand y" << endl;

  // --------------------------------------------------------------
  // Remove mesh

#ifdef KWVolView_PLUGINS_USE_SPLINE
  this->RemoveMeshButton->SetParent(this);
  this->RemoveMeshButton->Create();
  this->RemoveMeshButton->SetText("Remove Surface");
  this->RemoveMeshButton->SetCommand(this, "RemoveMeshCallback");

  tk_cmd << "pack " << this->RemoveMeshButton->GetWidgetName()
         << " -side left -padx 2 -pady 2 -fill x -expand y" << endl;
#endif

  // --------------------------------------------------------------
  // Pack

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);

  // Update according to the current Window

  this->Update();

  // Select one plugin

  vtkVVPlugin *plugin = this->GetPlugin(0);
  if (plugin)
    {
    this->SelectPlugin(plugin->GetName(), plugin->GetGroup());
    }
}

//----------------------------------------------------------------------------
int vtkVVPluginSelectorSortPlugins(const void *arg1, const void *arg2)
{
  vtkVVPlugin *plugin1 = *(vtkVVPlugin **)arg1;
  const char *name1 = plugin1->GetName();
  const char *group1 = plugin1->GetGroup();
  if (!group1 || !*group1)
    {
    group1 = VTK_VV_PLUGIN_DEFAULT_GROUP;
    }

  vtkVVPlugin *plugin2 = *(vtkVVPlugin **)arg2;
  const char *name2 = plugin2->GetName();
  const char *group2 = plugin2->GetGroup();
  if (!group2 || !*group2)
    {
    group2 = VTK_VV_PLUGIN_DEFAULT_GROUP;
    }

  return (10 * strcmp(group1, group2) + strcmp(name1, name2));
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::LoadPlugins()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Find all the plugins, they start with vv and end with .dll
  // or .do .sl etc

  char plugpath[1024];
  plugpath[0] = '\0';

  const char *app_path = this->GetApplication()->GetInstallationDirectory();
  if (app_path)
    {
    strcat(plugpath, app_path);
    strcat(plugpath, "/Plugins");
    }

  vtkDirectory *dir = vtkDirectory::New();
  dir->Open(plugpath);

#ifdef _MSC_VER  
  // cd to the plug directory to catch and wiwndows dlls
  char cwd[2048];
  _getcwd(cwd, 2048);
 _chdir(plugpath);
#endif
  
  int old_nb_plugins = this->GetNumberOfPlugins();
  
  // Collect all the plugins and install those that have not been 
  // installed yet

  int i, numFiles = dir->GetNumberOfFiles(), loaded = 0;
  clock_t start_clock = clock();
  for (i = 0; i < numFiles; ++i)
    {
    const char *fname = dir->GetFile(i);
    if (strlen(fname) > 2 && fname[0] == 'v' && fname[1] == 'v')
      {
      char fullPath[2000];
      sprintf(fullPath, "%s/%s", plugpath, fname);

      vtkVVPlugin *plugin = vtkVVPlugin::New();
      if (!plugin->Load(fullPath, this->GetApplication()) &&
          !this->HasPlugin(plugin->GetName(), plugin->GetGroup()))
        {
        if (this->Window)
          {
          ostrstream msg;
          msg << "Loading plugins";
          if (plugin->GetName() && *plugin->GetName())
            {
            msg << " (" << plugin->GetName() << ")";
            }
          msg << ends;
          this->Window->SetStatusText(msg.str());
          msg.rdbuf()->freeze(0);
          this->Window->GetProgressGauge()->SetValue(
            (int)(100.0 * (float)i / (float)numFiles));
          }
        loaded++;
        this->Plugins->AppendItem(plugin);

        // Create the plugin

        plugin->SetParent(this->PluginFrame);
        plugin->SetWindow(this->Window);
        plugin->Create();
        plugin->Register(this);
        }
      plugin->Delete();
      }
    }

#ifdef _MSC_VER  
  // restore directory
 _chdir(cwd);
#endif

 if (this->Window && this->GetNumberOfPlugins())
    {
    clock_t end_clock = clock();
    this->Window->GetProgressGauge()->SetValue(0);
    char buffer[256];
    sprintf(buffer, 
            "Loading plugins (%d new, %d total) -- Done (in %0.2f s.)",
            loaded, 
            this->GetNumberOfPlugins(), 
            (double)(end_clock - start_clock) / (double)CLOCKS_PER_SEC);
    this->Window->SetStatusText(buffer);
    }

  dir->Delete();

  // Did we add new plugins ? Yes, then send the list as an event
  // Also update the selection menu

  if (old_nb_plugins != this->GetNumberOfPlugins())
    {
    this->Plugins->Sort(vtkVVPluginSelectorSortPlugins);
    this->UpdatePluginsMenu();
    this->SendAvailablePluginsListAsEvent(
      vtkKWEvent::PluginFilterListAddedEvent);
    }
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::SendAvailablePluginsListAsEvent(unsigned long event)
{
  ostrstream event_str;
  this->WriteAvailablePluginsList(event_str);
  event_str << ends;
  this->InvokeEvent(event, event_str.str());
  event_str.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::UpdatePluginsMenu()
{
  if (!this->IsCreated())
    {
    return;
    }

  vtkKWMenuButton *omenu = this->PluginsMenu->GetWidget();
  if (!omenu)
    {
    return;
    }

  vtkKWMenu *menu = omenu->GetMenu();
  if (!menu)
    {
    return;
    }

  menu->DeleteAllItems();
  menu->RemoveAllChildren();

  typedef vtkstd::map<vtksys_stl::string, vtkKWMenu*> MenuCache;
  MenuCache menu_cache;
 
  vtkVVPluginSelector::PluginsContainerIterator *it = 
    this->Plugins->NewIterator();
  while (!it->IsDoneWithTraversal())
    {
    vtkVVPlugin *plugin = NULL;
    if (it->GetData(plugin) == VTK_OK && plugin)
      {
      const char *group = plugin->GetGroup();
      if (!group || !*group)
        {
        group = VTK_VV_PLUGIN_DEFAULT_GROUP;
        }
      vtkKWMenu *group_menu = NULL;
      MenuCache::iterator found = menu_cache.find(group);
      if (found != menu_cache.end())
        {
        group_menu = (*found).second;
        }
      else
        {
        group_menu = vtkKWMenu::SafeDownCast(
          menu->GetChildWidgetWithName(
            menu->GetItemOption(menu->GetIndexOfItem(group), "-menu")));
        if (!group_menu)
          {
          group_menu = vtkKWMenu::New();
          group_menu->SetParent(menu);
          group_menu->Create();
          menu->AddCascade(group, group_menu);
          group_menu->Delete();
          }
        menu_cache[group] = group_menu;
        }
      if (group_menu)
        {
        char command[1024];
        sprintf(command, "SelectPluginCallback {%s} {%s}", 
                plugin->GetName(), plugin->GetGroup());
        group_menu->AddRadioButton(plugin->GetName(), this, command);
        }
      }
    it->GoToNextItem();
    }
  it->Delete();

  this->UpdateSelectedPluginInMenu();
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::UpdatePluginsMenuEnableState()
{
  if (!this->IsCreated())
    {
    return;
    }

  vtkKWMenuButton *omenu = this->PluginsMenu->GetWidget();
  if (!omenu)
    {
    return;
    }

  vtkKWMenu *menu = omenu->GetMenu();
  if (!menu)
    {
    return;
    }

  vtkVVSelectionFrame *sel_frame = 
    this->Window->GetSelectedSelectionFrame();
  int paintbrush_supported = 
    sel_frame && sel_frame->PaintbrushWidgetIsSupported();
  int menu_state = menu->GetEnabled() & paintbrush_supported 
    ? vtkKWOptions::StateNormal : vtkKWOptions::StateDisabled;

  typedef vtkstd::map<vtksys_stl::string, vtkKWMenu*> MenuCache;
  MenuCache menu_cache;

  vtkVVPluginSelector::PluginsContainerIterator *it = 
    this->Plugins->NewIterator();
  while (!it->IsDoneWithTraversal())
    {
    vtkVVPlugin *plugin = NULL;
    if (it->GetData(plugin) == VTK_OK && plugin && 
        plugin->GetRequiresLabelInput())
      {
      const char *group = plugin->GetGroup();
      if (!group || !*group)
        {
        group = VTK_VV_PLUGIN_DEFAULT_GROUP;
        }
      vtkKWMenu *group_menu = NULL;
      MenuCache::iterator found = menu_cache.find(group);
      if (found != menu_cache.end())
        {
        group_menu = (*found).second;
        }
      else
        {
        group_menu = vtkKWMenu::SafeDownCast(
          menu->GetChildWidgetWithName(
            menu->GetItemOption(menu->GetIndexOfItem(group), "-menu")));
        if (group_menu)
          {
          menu_cache[group] = group_menu;
          }
        }
      if (group_menu)
        {
        group_menu->SetItemState(plugin->GetName(), menu_state);
        }
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::UpdateSelectedPluginInMenu()
{
  if (!this->IsCreated())
    {
    return;
    }

  vtkKWMenuButton *omenu = this->PluginsMenu->GetWidget();
  if (!omenu)
    {
    return;
    }

  // No selection, set current entry to empty

  if (this->SelectedPlugin < 0)
    {
    omenu->SetValue("");
    }
  else
    {
    vtkVVPlugin *plugin = this->GetPlugin(this->SelectedPlugin);
    if (plugin)
      {
#if 0
      vtksys_ios::ostringstream full_name;
      this->GetPluginPrettyName(
        full_name, plugin->GetName(), plugin->GetGroup(), 
        VTK_VV_PLUGINS_FULLNAME_LENGTH_MAX);
      omenu->SetValue(full_name.str().c_str());
#else
      omenu->SetValue(plugin->GetName());
#endif
      }
    
    // Given that the plugin might have changed, the possibility of undo on 
    // this dataset may have changed.. 
    this->UpdateEnableState();
    this->UpdateUndoButton();
    }
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::Update()
{
  // Update enable state

  this->UpdateEnableState();

  // Update all plugins
  // Nope, costs too much, just update the selected one (below)
#if 0
  vtkVVPluginSelector::PluginsContainerIterator *it = this->Plugins->NewIterator();
  while (!it->IsDoneWithTraversal())
    {
    vtkVVPlugin *plugin = NULL;
    if (it->GetData(plugin) == VTK_OK && plugin)
      {
      plugin->SetWindow(this->Window);
      plugin->Update();
      }
    it->GoToNextItem();
    }
  it->Delete();
#endif

  // Update the selected plugin

  this->UpdateSelectedPlugin();
  
  this->UpdateUndoButton();
  
#ifdef KWVolView_PLUGINS_USE_SPLINE
  // Should the remove mesh button be enabled?
  if (this->IsCreated())
    {
    // And I have no mesh
    if (!this->Window->GetPolyData())
      {
      this->RemoveMeshButton->SetEnabled(0);
      }
    }
#endif
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::UpdateUndoButton()
{
  // Should the undo button be enabled?

  if (!this->IsCreated())
    {
    return;
    }

  // Check if I have undo data for the selected data item and the selected
  // plugin.
  vtkVVPlugin *plugin = this->GetPlugin(this->GetSelectedPluginIndex());
  bool can_not_undo = 
    !plugin || !this->Window || !this->Window->GetSelectedDataItem();
  if (!can_not_undo) 
    {
    vtkVVDataItemVolume *volume_data = 
      vtkVVDataItemVolume::SafeDownCast(this->Window->GetSelectedDataItem());
    // Check if the operation can be redone/undone.
    if (volume_data &&
        volume_data->GetUndoRedoPluginName() && 
        (volume_data->GetUndoRedoImageDataType() == vtkVVDataItemVolume::Undo ||
         volume_data->GetUndoRedoImageDataType() == vtkVVDataItemVolume::Redo) &&
        volume_data->GetUndoRedoImageData())
      {
      // Now check if it is an Undo or a Redo and set the appropriate text 
      // and callback
      if (volume_data->GetUndoRedoImageDataType() == vtkVVDataItemVolume::Undo)
        {
        this->UndoButton->SetText(VTK_VV_PLUGINS_UNDO_TEXT);
        this->UndoButton->SetBalloonHelpString(VTK_VV_PLUGINS_UNDO_TEXT);
        this->UndoButton->SetCommand(this, "UndoCallback");
        }
      if (volume_data->GetUndoRedoImageDataType() == vtkVVDataItemVolume::Redo)
        {
        this->UndoButton->SetText(VTK_VV_PLUGINS_REDO_TEXT);
        this->UndoButton->SetBalloonHelpString(VTK_VV_PLUGINS_REDO_TEXT);
        this->UndoButton->SetCommand(this, "RedoCallback");
        }
      }
    else
      {
      can_not_undo = true; // Undo data does not exist
      }
    }
  
  if (can_not_undo)
    {
    this->UndoButton->SetText(VTK_VV_PLUGINS_NO_UNDO_TEXT);
    this->UndoButton->SetBalloonHelpString(VTK_VV_PLUGINS_NO_UNDO_TEXT);
    this->UndoButton->SetEnabled(0);
    }
}

//----------------------------------------------------------------------------
int vtkVVPluginSelector::GetPluginIndex(const char *plugin_name, const char *group)
{
  if (!plugin_name || !*plugin_name)
    {
    return -1;
    }

  vtkIdType found = -1;
  vtkVVPluginSelector::PluginsContainerIterator *it = this->Plugins->NewIterator();
  while (!it->IsDoneWithTraversal())
    {
    vtkVVPlugin *plugin = NULL;
    if (it->GetData(plugin) == VTK_OK && plugin && 
        !strcmp(plugin_name, plugin->GetName()) &&
        (!group || !strcmp(group, plugin->GetGroup())))
      {
      it->GetKey(found);
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found;
}

//----------------------------------------------------------------------------
int vtkVVPluginSelector::HasPlugin(const char *plugin_name, const char *group)
{
  return this->GetPlugin(plugin_name, group) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkVVPluginSelector::SelectPlugin(const char *plugin_name, const char *group)
{
  this->SelectedPlugin = this->GetPluginIndex(plugin_name, group);

  if (!this->IsCreated())
    {
    return 0;
    }

  if (this->SelectedPlugin < 0)
    {
    vtksys_ios::ostringstream full_name;
    this->GetPluginPrettyName(full_name, plugin_name, group);
    vtkWarningMacro(
      "The plugin to select (" << full_name.str().c_str() << ") was not found "
      "in the list of plugin plugins available on this system.");
    return 0;
    }

  // Update the menu to reflect the choice

  this->UpdateSelectedPluginInMenu();

  this->UpdateSelectedPlugin();

  return 1;
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::UpdateSelectedPlugin()
{
  // Update the selected plugin, if any

  vtkVVPlugin *plugin = this->GetPlugin(this->SelectedPlugin);
  if (plugin)
    {
    plugin->Update();
    }

  // Unpack all plugins, pack the selected one (if any)

  if (this->PluginFrame && this->PluginFrame->IsCreated())
    {
    this->PluginFrame->UnpackChildren();
    if (plugin)
      {
      this->Script("pack %s -side top -fill both -expand t", 
                   plugin->GetWidgetName());
      }
    }
}

//----------------------------------------------------------------------------
char* vtkVVPluginSelector::GetSelectedPluginName()
{
  vtkVVPlugin *plugin = this->GetPlugin(this->SelectedPlugin);
  if (plugin)
    {
    return plugin->GetName();
    }
  return NULL;
}

//----------------------------------------------------------------------------
char* vtkVVPluginSelector::GetSelectedPluginGroup()
{
  vtkVVPlugin *plugin = this->GetPlugin(this->SelectedPlugin);
  if (plugin)
    {
    return plugin->GetGroup();
    }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkVVPluginSelector::GetSelectedPluginIndex()
{
  return this->SelectedPlugin;
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::SelectPluginCallback(const char *plugin_name, 
                                               const char *group)
{
  vtkVVPlugin *plugin = this->GetPlugin(plugin_name, group);
  if (plugin)
    {
    char *cargs[2];
    cargs[0] = plugin->GetName();
    cargs[1] = plugin->GetGroup();
    
    if (cargs[0] && cargs[1])
      {
      this->SelectPlugin(cargs[0], cargs[1]);
      }
    }
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::CancelPluginCallback()
{
  vtkVVPlugin *plugin = this->GetPlugin(this->SelectedPlugin);
  if (plugin)
    {
    char *cargs[2];
    cargs[0] = plugin->GetName();
    cargs[1] = plugin->GetGroup();
    
    if (cargs[0] && cargs[1])
      {
      this->CancelPlugin(cargs[0], cargs[1]);
      }
    }
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::CancelPlugin(const char *plugin_name, const char *group)
{
  vtkVVPlugin *plugin = this->GetPlugin(plugin_name, group);
  if (plugin)
    {
    plugin->Cancel(this);
    }
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::CancelSelectedPlugin()
{
  vtkVVPlugin *plugin = this->GetPlugin(this->SelectedPlugin);
  if (plugin)
    {
    this->CancelPlugin(plugin->GetName(), plugin->GetGroup());
    }
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::CancelAllPlugins()
{
  vtkVVPluginSelector::PluginsContainerIterator *it = this->Plugins->NewIterator();
  while (!it->IsDoneWithTraversal())
    {
    vtkVVPlugin *plugin = NULL;
    if (it->GetData(plugin) == VTK_OK && plugin)
      {
      this->CancelPlugin(plugin->GetName(), plugin->GetGroup());
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
int vtkVVPluginSelector::RemoveSinglePlugin(const char *plugin_name, const char *group)
{
  int idx = this->GetPluginIndex(plugin_name, group);
  vtkVVPlugin *plugin = this->GetPlugin(idx);
  if (!plugin)
    {
    return 0;
    }

  // Remove the plugin from the container

  this->Plugins->RemoveItem(idx);

  // Free the plugin to remove

  plugin->Delete();

  // Did we lose the selection
  
  if (this->SelectedPlugin == idx)
    {
    this->SelectedPlugin = (this->GetNumberOfPlugins() ? 0 : -1);
    }
  else if (this->SelectedPlugin > idx)
    {
    this->SelectedPlugin--;
    }

  return 1;
}


//----------------------------------------------------------------------------
int vtkVVPluginSelector::RemovePlugin(const char *plugin_name, const char *group)
{
  if (!this->RemoveSinglePlugin(plugin_name, group))
    {
    return 0;
    }

  // Update (including the UI + option menu)
  
  this->UpdatePluginsMenu();
  this->Update();
  
  // List has changed, signal it
  
  this->SendAvailablePluginsListAsEvent(
    vtkKWEvent::PluginFilterListRemovedEvent);

  return 1;
}

//----------------------------------------------------------------------------
int vtkVVPluginSelector::RemovePlugins(
  int nb, const char *plugin_names[], const char *groups[])
{
  if (nb <= 0 || !plugin_names || !groups)
    {
    return 0;
    }

  int i, nb_deleted = 0;
  for (i = 0; i < nb; i++)
    {
    nb_deleted += this->RemoveSinglePlugin(plugin_names[i], groups[i]);
    }

  if (nb_deleted)
    {
    // Update (including the UI + option menu)
  
    this->UpdatePluginsMenu();
    this->Update();
    
    // List has changed, signal it
    
    this->SendAvailablePluginsListAsEvent(
      vtkKWEvent::PluginFilterListRemovedEvent);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkVVPluginSelector::ApplyPlugin(const char *plugin_name, const char *group)
{
  vtkVVPlugin *plugin = this->GetPlugin(plugin_name, group);
  if (!plugin)
    {
    vtksys_ios::ostringstream full_name;
    this->GetPluginPrettyName(full_name, plugin_name, group);
    vtkWarningMacro(
      "The plugin to apply (" << full_name.str().c_str() << ") was not found "
      "in the list of plugin plugins available on this system. No "
      "modification will be performed on the data.");
    return 0;
    }

  // Make sure it is up-to-date

  plugin->Update();
  
  // It seems the grab has no impact on the menubar, so try to disable
  // it manually

  if (this->Window)
    {
    this->Window->GetMenu()->SetEnabled(0);
    }

  // Perform a grab and change the text

  this->ApplyButton->SetText("Cancel");
  this->ApplyButton->SetCommand(this, "CancelPluginCallback");
  this->ApplyButton->Grab();

  // Execute the plugin

  plugin->Execute(this);

  // Release the grab

  this->ApplyButton->ReleaseGrab();
  this->ApplyButton->SetText("Apply Plugin");
  this->ApplyButton->SetCommand(this, "ApplyPluginCallback");

  // Reenable the menubar

  if (this->Window)
    {
    this->Window->UpdateMenuState();
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkVVPluginSelector::ApplySelectedPlugin()
{
  vtkVVPlugin *plugin = this->GetPlugin(this->SelectedPlugin);
  if (plugin)
    {
    return this->ApplyPlugin(plugin->GetName(), plugin->GetGroup());
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::SetUndoData(vtkImageData *undoImageData)
{
  // Get the plugin name

  vtkVVPlugin *plugin = this->GetPlugin(this->GetSelectedPluginIndex());

  vtksys_ios::ostringstream full_name;
  this->GetPluginPrettyName(full_name, plugin->GetName(), plugin->GetGroup());
  
  vtkVVDataItemVolume *volume_data = vtkVVDataItemVolume::SafeDownCast(
    this->Window->GetSelectedDataItem());
  if (!volume_data)
    { 
    return;
    }
  
  if (!volume_data->SetUndoRedoImageData(undoImageData))
    {
    return; // Was the same undo data before.. 
    }
  
  volume_data->SetUndoRedoImageDataTypeToUndo();
  volume_data->SetUndoRedoPluginName(full_name.str().c_str());
 
  if (undoImageData)
    {
    // also store the current settings of the widgets, this meta information
    // should really be in the ImageData object itself
    this->SetDistanceUnits(volume_data->GetDistanceUnits());
    this->SetScalarUnits(0,volume_data->GetScalarUnits(0));
    this->SetScalarUnits(1,volume_data->GetScalarUnits(1));
    this->SetScalarUnits(2,volume_data->GetScalarUnits(2));
    this->SetScalarUnits(3,volume_data->GetScalarUnits(3));
    this->SetIndependentComponents(
      volume_data->GetVolumeProperty()->GetIndependentComponents());
    }
  else
    {
    int i;
    for (i=0; i< VTK_MAX_VRCOMP; i++)
      {
      this->SetScalarUnits(i,0);
      }
    }
  
  //this->Modified();

  // Update the undo button
  this->UpdateUndoButton();
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::PushNewProperties()
{
  unsigned short j;
  
  // if any properties were set then push them out
  int nb_rw = this->Window->GetNumberOfRenderWidgetsUsingSelectedDataItem();
  int ic = 0;
  char *su[4];
  char *du = 0;
  int i;
  for (i = 0; i < nb_rw; i++)
    {
    vtkKWRenderWidgetPro *rwp = vtkKWRenderWidgetPro::SafeDownCast(
      this->Window->GetNthRenderWidgetUsingSelectedDataItem(i));
    if (!rwp)
      {
      continue;
      }

    // first save the old values
    if (rwp->GetDistanceUnits())
      {
      du = new char [strlen(rwp->GetDistanceUnits())+1];
      strcpy(du,rwp->GetDistanceUnits());
      }
    ic = rwp->GetIndependentComponents();
    for (j = 0; j < 4; ++j)
      {
      su[j] = 0;
      if (rwp->GetScalarUnits(j))
        {
        su[j] = new char [strlen(rwp->GetScalarUnits(j))+1];
        strcpy(su[j],rwp->GetScalarUnits(j));
        }
      }
      break;
    }
    
  // set the new values
  for (i = 0; i < nb_rw; i++)
    {
    vtkKWRenderWidgetPro *rwp = vtkKWRenderWidgetPro::SafeDownCast(
      this->Window->GetNthRenderWidgetUsingSelectedDataItem(i));
    if (!rwp)
      {
      continue;
      }
    rwp->SetIndependentComponents(this->IndependentComponents);      
    rwp->SetDistanceUnits(this->DistanceUnits);
    rwp->SetScalarUnits(0,this->ScalarUnits[0]);
    rwp->SetScalarUnits(1,this->ScalarUnits[1]);
    rwp->SetScalarUnits(2,this->ScalarUnits[2]);
    rwp->SetScalarUnits(3,this->ScalarUnits[3]);
    }  
  
  // and store the old value for undo
  this->SetDistanceUnits(du);
  if (du)
    {
    delete [] du;
    }
  for (j = 0; j < 4; ++j)
    {
    this->SetScalarUnits(j,su[j]);
    if (su[j])
      {
      delete [] su[j];
      }
    }
  this->SetIndependentComponents(ic);
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::Undo()
{
  // Replace the input data with the undo data using shallow copies
  vtkVVDataItemVolume *volume_data = vtkVVDataItemVolume::SafeDownCast(
                              this->Window->GetSelectedDataItem());
  if (!volume_data)
    {
    return;
    }
  vtkImageData *image_data = volume_data->GetImageData();
  vtkImageData *undo_data = volume_data->GetUndoRedoImageData();

  // Check if we have the image and undo datas and that that the data is really
  // an UNDO data.
  if (!image_data || !undo_data || !this->Window ||
      volume_data->GetUndoRedoImageDataType() != vtkVVDataItemVolume::Undo)
    {
    return;
    }
  
  vtkImageData *tempID = vtkImageData::New();
  tempID->ShallowCopy(undo_data);
  undo_data->ShallowCopy(image_data);
  image_data->ShallowCopy(tempID);
  
  // push out new properties
  this->PushNewProperties();

  // Always leave the undo data as more recent
  //undo_data->Modified();

  if (this->IsCreated())
    {
    // Now that we just did a Undo, the next operation we can do is Redo
    this->UndoButton->SetText(VTK_VV_PLUGINS_REDO_TEXT);
    this->UndoButton->SetBalloonHelpString(VTK_VV_PLUGINS_REDO_TEXT);
    this->UndoButton->SetCommand(this, "RedoCallback");
    volume_data->SetUndoRedoImageDataTypeToRedo();
    }
  
  tempID->Delete();
}

//BTX
#ifdef KWVolView_PLUGINS_USE_SPLINE
//----------------------------------------------------------------------------
void vtkVVPluginSelector::RemoveMesh()
{
  // Replace the input data with the undo data using shallow copies

  if (this->Window)
    {
    this->Window->SetPolyData(0);
    }
}
#endif
//ETX

//----------------------------------------------------------------------------
void vtkVVPluginSelector::Redo()
{
  // Replace the input data with the redo data using shallow copies
  vtkVVDataItemVolume *volume_data = vtkVVDataItemVolume::SafeDownCast(
                              this->Window->GetSelectedDataItem());
  if (!volume_data)
    {
    return;
    }
  vtkImageData *image_data = volume_data->GetImageData();
  vtkImageData *redo_data = volume_data->GetUndoRedoImageData();
  if (!image_data || !redo_data || !this->Window ||
      volume_data->GetUndoRedoImageDataType() != vtkVVDataItemVolume::Redo)
    {
    return;
    }
  
  vtkImageData *tempID = vtkImageData::New();
  tempID->ShallowCopy(redo_data);
  redo_data->ShallowCopy(image_data);
  image_data->ShallowCopy(tempID);
  
  // push out new properties
  this->PushNewProperties();

  // Always leave the redo data as more recent
  //redo_data->Modified();

  if (this->IsCreated())
    {
    // Now that we just did a Redo, the next operation we can do is Undo
    this->UndoButton->SetText(VTK_VV_PLUGINS_UNDO_TEXT);
    this->UndoButton->SetBalloonHelpString(VTK_VV_PLUGINS_UNDO_TEXT);
    this->UndoButton->SetCommand(this, "UndoCallback");
    volume_data->SetUndoRedoImageDataTypeToUndo();
    }
  
  tempID->Delete();
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::UndoCallback()
{
  this->Undo();

  // Image data has changed.
  this->PluginInterface->UpdateAccordingToImageData();
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::RedoCallback()
{
  this->Redo();

  // Image data has changed.
  this->PluginInterface->UpdateAccordingToImageData();
}

//BTX
#ifdef KWVolView_PLUGINS_USE_SPLINE
//----------------------------------------------------------------------------
void vtkVVPluginSelector::RemoveMeshCallback()
{
  this->InvokeEvent(vtkKWEvent::PluginFilterRemoveMeshEvent, (void *)0);
}
#endif
//ETX

//----------------------------------------------------------------------------
void vtkVVPluginSelector::ApplyPluginCallback()
{
  vtkVVPlugin *plugin = this->GetPlugin(this->SelectedPlugin);
  if (!plugin)
    {
    return;
    }

  // Apply the plugin 

  char *cargs[3];
  cargs[0] = plugin->GetName();
  cargs[1] = plugin->GetGroup();

  ostrstream event_str;

#ifndef PLUGINS_DO_NOT_SUPPORT_XML_RW
  vtkXMLVVPluginWriter *xmlw = vtkXMLVVPluginWriter::New();
  xmlw->SetObject(plugin);
  xmlw->WriteToStream(event_str);
  xmlw->Delete();
#endif

  event_str << ends;

  cargs[2] = event_str.str();
  event_str.rdbuf()->freeze(0);
  
  this->PluginInterface->Update();
  this->ApplyPlugin(cargs[0], cargs[1]);
  this->PluginInterface->UpdateAccordingToImageData();
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // Enable/Disable widgets

  this->PropagateEnableState(this->ReloadButton);
  this->PropagateEnableState(this->PluginsMenu);
  this->UpdatePluginsMenuEnableState();

  vtkVVPlugin *plugin = this->GetPlugin(this->SelectedPlugin);
  vtkVVSelectionFrame *sel_frame = this->Window->GetSelectedSelectionFrame();

  int enabled = this->GetEnabled();
  if (plugin && plugin->GetRequiresLabelInput() && 
      sel_frame && !sel_frame->PaintbrushWidgetIsSupported())
    {
    enabled = 0;
    }

  if (plugin)
    {
    plugin->SetEnabled(enabled);
    }
  if (this->ApplyButton)
    {
    this->ApplyButton->SetEnabled(enabled);
    }

  this->PropagateEnableState(this->UndoButton);
#ifdef KWVolView_PLUGINS_USE_SPLINE
  this->PropagateEnableState(this->RemoveMeshButton);
#endif
}

//----------------------------------------------------------------------------
vtkVVPlugin* vtkVVPluginSelector::GetPlugin(int idx)
{
  vtkVVPlugin *plugin = NULL;
  if (this->Plugins->GetItem(idx, plugin) == VTK_OK)
    {
    return plugin;
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkVVPlugin* vtkVVPluginSelector::GetPlugin(
  const char *plugin_name, const char *group)
{
  return this->GetPlugin(this->GetPluginIndex(plugin_name, group));
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::WriteAvailablePluginsList(ostream &os)
{
#ifndef PLUGINS_DO_NOT_SUPPORT_XML_RW
  vtkXMLVVPluginsWriter *xmlw = vtkXMLVVPluginsWriter::New();
  xmlw->SetObject(this);
  xmlw->OutputPluginNameOnlyOn();
  xmlw->WriteToStream(os);
  xmlw->Delete();
#else
  (void)os;
#endif
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::RemovePluginsNotInList(istream &is)
{
#ifndef PLUGINS_DO_NOT_SUPPORT_XML_RW
  vtkXMLVVPluginsReader *xmlr = vtkXMLVVPluginsReader::New();
  xmlr->SetObject(this);
  xmlr->RemovePluginsNotInListOn();
  xmlr->ParseStream(is);
  xmlr->Delete();
#else
  (void)is;
#endif
}

//----------------------------------------------------------------------------
const char *vtkVVPluginSelector::GetScalarUnits(int i)
{
  if (i < 0 || i >= VTK_MAX_VRCOMP)
    {
    return 0;
    }
  return this->ScalarUnits[i];
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::SetScalarUnits(int i, const char *_arg)
{
  if (i < 0 || i >= VTK_MAX_VRCOMP)
    {
    return;
    }

  vtkDebugMacro(<< this->GetClassName() << " (" << this 
                << "): setting ScalarUnits to " << _arg); 

  if (this->ScalarUnits[i] == NULL && _arg == NULL) 
    { 
    return;
    } 

  if (this->ScalarUnits[i] && _arg && (!strcmp(this->ScalarUnits[i], _arg))) 
    { 
    return;
    } 

  if (this->ScalarUnits[i]) 
    { 
    delete [] this->ScalarUnits[i]; 
    } 

  if (_arg) 
    { 
    this->ScalarUnits[i] = new char[strlen(_arg) + 1]; 
    strcpy(this->ScalarUnits[i], _arg); 
    } 
  else 
    { 
    this->ScalarUnits[i] = NULL; 
    } 

  this->Modified(); 
}

//----------------------------------------------------------------------------
void vtkVVPluginSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Window: " << this->Window << endl;
  os << indent << "SelectedPlugin: " << this->SelectedPlugin << endl;
  os << indent << "Image metadata: " << endl;
  os << indent << "Independent Components: " << this->IndependentComponents << endl;
  if (this->DistanceUnits)
    {
    os << indent << "DistanceUnits: " << this->DistanceUnits << endl;
    }
  else
    {
    os << indent << "None" << endl;
    }
  int i;
  for (i = 0; i < VTK_MAX_VRCOMP; i++)
    {
    if (this->ScalarUnits[i])
      {
      os << indent << "ScalarUnits[" << i << "]: " << this->ScalarUnits[i] << endl;
      }
    else
      {
      os << indent << "ScalarUnits[" << i << "]: None" << endl;
      }
    }
}

