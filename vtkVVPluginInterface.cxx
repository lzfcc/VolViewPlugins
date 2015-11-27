/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVVPluginInterface.h"

#include "vtkKWFrameWithLabel.h"
#include "vtkObjectFactory.h"
#include "vtkVVPluginSelector.h"
#include "vtkKWEvent.h"
#include "vtkKWIcon.h"
#include "vtkVVWindow.h"
#include "vtkKWRenderWidgetPro.h"
#include "vtkKWVolumeWidget.h"
#include "vtkVVDataItemVolume.h"
#include "vtkKWImageWidget.h"
#include "vtkImageData.h" 
#include "vtkVVSelectionFrame.h"
#include "vtkVVSelectionFrameLayoutManager.h"
#include "vtkVVWidgetInterface.h"
#include "vtkVVPaintbrushWidgetEditor.h"
#include "vtkKWEPaintbrushWidget.h"
#include "vtkKWEPaintbrushDrawing.h"
#include "vtkKWEPaintbrushRepresentation2D.h"

#include <vtksys/SystemTools.hxx>

//---------------------------------------------------------------------------

#define VTK_VV_PLUGIN_UIP_LABEL        "Plugins"

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkVVPluginInterface);
vtkCxxRevisionMacro(vtkVVPluginInterface, "$Revision: 1.12 $");

//---------------------------------------------------------------------------
vtkVVPluginInterface::vtkVVPluginInterface()
{
  this->SetName(VTK_VV_PLUGIN_UIP_LABEL);

  // Plugins

  this->PluginsFrame = 0;

  // Some objects are instantiated here because observers will most likely be 
  // attached to them in the future.

  this->Plugins = vtkVVPluginSelector::New();
  this->PageId = -1;
}

//---------------------------------------------------------------------------
vtkVVPluginInterface::~vtkVVPluginInterface()
{
  // Plugins

  if (this->PluginsFrame)
    {
    this->PluginsFrame->Delete();
    this->PluginsFrame = NULL;
    }
  if (this->Plugins)
    {
    this->Plugins->Delete();
    this->Plugins = NULL;
    }
}


// --------------------------------------------------------------------------
void vtkVVPluginInterface::Create()
{
  if (this->IsCreated())
    {
    vtkErrorMacro("The panel is already created.");
    return;
    }

  // Create the superclass instance (and set the application)

  this->Superclass::Create();

  ostrstream tk_cmd;

  vtkKWWidget *page;
  vtkKWFrame *frame;

  // --------------------------------------------------------------
  // Add a "Plugin" page

  this->PageId = this->AddPage(NULL, this->GetName());
  this->SetPageIconToPredefinedIcon(
    this->PageId, vtkKWIcon::IconNuvola22x22ActionsMisc);

  page = this->GetPageWidget(this->PageId);

  // --------------------------------------------------------------
  // Plugins : frame

  if (!this->PluginsFrame)
    {
    this->PluginsFrame = vtkKWFrameWithLabel::New();
    }

  this->PluginsFrame->SetParent(this->GetPagesParentWidget());
  this->PluginsFrame->LimitedEditionModeIconVisibilityOn();
  this->PluginsFrame->Create();
  this->PluginsFrame->SetLabelText("Plugins");
    
  tk_cmd << "pack " << this->PluginsFrame->GetWidgetName()
         << " -side top -anchor nw -fill x -padx 2 -pady 2 " 
         << " -in " << page->GetWidgetName() << endl;
  
  frame = this->PluginsFrame->GetFrame();

  // --------------------------------------------------------------
  // Create the plugins UI

  this->Plugins->SetParent(frame);
  this->Plugins->SetWindow(this->Window);
  this->Plugins->SetPluginInterface(this);
  this->Plugins->Create();
    
  tk_cmd << "pack " << this->Plugins->GetWidgetName()
         << " -side top -anchor n -expand y -fill x -padx 2 -pady 2" << endl;

  // --------------------------------------------------------------
  // Pack 

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);

  // Update according to the current Window
  this->Update();
}


//---------------------------------------------------------------------------
void vtkVVPluginInterface::Update()
{
  this->Superclass::Update();
  if( !this->IsCreated() )
    {
    return;
    }
  
  // Check if we have data.
  bool has_data = false;
  if( this->Window )
    {
    vtkVVDataItemVolume *volume_data = vtkVVDataItemVolume::SafeDownCast(
                                      this->Window->GetSelectedDataItem());
    if( volume_data )
      {
      vtkImageData *image_data = volume_data->GetImageData();
      if( image_data ) // Only for image data for now
        {
        has_data = true;
        }
      }
    }

  // Update the plugins based on new info.. This will set the "Undo" button
  // as enabled/disabled or the 'Redo' button status etc..
  if( has_data && this->Plugins )
    {
    this->Plugins->Update();
    }
  
  // If there is no input data, let's disable everything

  if (!has_data && this->Plugins)
    {
    this->Plugins->SetEnabled(0);
    }
}


//---------------------------------------------------------------------------
void vtkVVPluginInterface::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // Plugins

  if (this->PluginsFrame)
    {
    this->PluginsFrame->SetEnabled(this->GetEnabled());
    }

  if (this->Plugins)
    {
    this->Plugins->SetEnabled(this->GetEnabled());
    }
}


//----------------------------------------------------------------------------
void vtkVVPluginInterface::UpdateAccordingToImageData()
{
  int nb_rw = this->Window->GetNumberOfRenderWidgetsUsingSelectedDataItem();
  for (int i = 0; i < nb_rw; i++)
    {
    vtkKWRenderWidgetPro *rwp = vtkKWRenderWidgetPro::SafeDownCast(
      this->Window->GetNthRenderWidgetUsingSelectedDataItem(i));
    if (!rwp)
      {
      continue;
      }
    int mode = rwp->GetRenderMode();
    rwp->SetRenderModeToDisabled();
    
    // image_data ivar remains the same, but its contents have changed after 
    // the plugin is applied. Manually force the update.
    rwp->UpdateAccordingToInput();
    
    rwp->SetRenderMode(mode);

    // Re-install any paintbrush widgets. We are screwed at the moment if the
    // extents of the underlying image change.. Let's handle this later.
    this->ReinstallPaintbrushWidgets(rwp);

    rwp->Render();
    }

  this->Window->Update();
}

//----------------------------------------------------------------------------
void vtkVVPluginInterface
::ReinstallPaintbrushWidgets(vtkKWRenderWidget *rw)
{
  // The datatype of the underlying image may have changed when running the
  // plugin. This can wreck havoc with the paintbrush widgets, since these
  // do a blending of the paintbrush with the underlying data - Hence if we
  // switch the underlying data under the carpet, we need to do some magic.
  //   We will re-install the paintbrush pipeline.
  vtkVVSelectionFrameLayoutManager *layoutMgr
    = this->Window->GetDataSetWidgetLayoutManager();
  if (vtkVVSelectionFrame *selFrame
        = vtkVVSelectionFrame::SafeDownCast(
            layoutMgr->GetContainingSelectionFrame(rw)))
    {
    const int nWidgets = selFrame->GetNumberOfInteractorWidgets();
    for (int j = 0; j < nWidgets; j++)
      {
      if (vtkKWEPaintbrushWidget *w = vtkKWEPaintbrushWidget::SafeDownCast(
                                      selFrame->GetNthInteractorWidget(j)))
        {
        if (w->GetEnabled())
          {
          if (vtkKWEPaintbrushRepresentation2D *rep2D = 
              vtkKWEPaintbrushRepresentation2D::SafeDownCast(w->GetRepresentation()))
            {
            rep2D->UnInstallPipeline();
            rep2D->InstallPipeline();

            // Repopulate the sketches since we may have modified the number of
            // labels etc. Also collapse the history of all sketches in the
            // drawing since the plugin could have modified the paintbrush data
            // under the hood without going through the Paintbrush's Undo/Redo
            // stack mechanism
            rep2D->GetPaintbrushDrawing()->CreateSketches();
            rep2D->GetPaintbrushDrawing()->CollapseHistory();

            vtkVVWindow *win = vtkVVWindow::SafeDownCast(this->Window);
            vtkVVWidgetInterface *wi = win->GetWidgetInterface();
            if (!wi)
              {
              std::cout << "wi: " << wi << std::endl;
              }
            else if (!wi->GetPaintbrushWidgetEditor())
              {
              std::cout << "Paintbrush widget editor: " << wi->GetPaintbrushWidgetEditor() << std::endl;
              }
            else
              {
              wi->GetPaintbrushWidgetEditor()->PopulateSketchList();
              }
            }
          }
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkVVPluginInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

