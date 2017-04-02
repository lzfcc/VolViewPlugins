/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVVPluginInterface - a user interface panel.
// .SECTION Description
// A concrete implementation of a user interface panel.
// See vtkKWUserInterfacePanel for a more detailed description.
// .SECTION See Also
// vtkKWUserInterfacePanel vtkKWUserInterfaceManager

#ifndef __vtkVVPluginInterface_h
#define __vtkVVPluginInterface_h

#include "vtkVVUserInterfacePanel.h"

class vtkKWFrameWithLabel;
class vtkVVPluginSelector;
class vtkVVWindowBase;
class vtkKWRenderWidget;

class VTK_EXPORT vtkVVPluginInterface : public vtkVVUserInterfacePanel
{
public:
  static vtkVVPluginInterface* New();
  vtkTypeRevisionMacro(vtkVVPluginInterface,vtkVVUserInterfacePanel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the interface objects.
  virtual void Create();

  // Description:
  // Refresh the interface given the current value of the Window and its
  // views/composites/widgets.
  virtual void Update();

  // Description:
  // Access to some sub-widgets.
  vtkGetObjectMacro(Plugins, vtkVVPluginSelector);
  
  // Description:
  // Get the id of the page in this user interface
  vtkGetMacro(PageId, int);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // UpdateAccordingToImageData() should be called once you know
  // that the vtkImageData has been modified(). An alternative is to call 
  // the Update() of the display interface, but we are assuming that the 
  // plugin operation did not modify the views/volume property etc. So
  // we can do faster than having to call Window->DisplayInterface->Update().
  // We will just update the volume and image widgets as per the changed data.
  virtual void UpdateAccordingToImageData();

  // Description:
  // Get some internal objects
  vtkGetObjectMacro(PluginsFrame, vtkKWFrameWithLabel);

protected:
  vtkVVPluginInterface();
  ~vtkVVPluginInterface();

  // The datatype of the underlying image may have changed when running the
  // plugin. This can wreck havoc with the paintbrush widgets, since these
  // do a blending of the paintbrush with the underlying data - Hence if we
  // switch the underlying data under the carpet, we need to do some magic.
  void ReinstallPaintbrushWidgets(vtkKWRenderWidget *rw);

  // Plugins
  vtkKWFrameWithLabel      *PluginsFrame;
  vtkVVPluginSelector      *Plugins;
  int PageId;

private:
  vtkVVPluginInterface(const vtkVVPluginInterface&); // Not implemented
  void operator=(const vtkVVPluginInterface&); // Not Implemented
};

#endif

