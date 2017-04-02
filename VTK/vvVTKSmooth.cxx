/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* perform a simple threshold filter */

#include <string.h>
#include <stdlib.h>
#include "vtkImageImport.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkImageGaussianSmooth.h"
#include "vtkCallbackCommand.h"
#include "vtkAlgorithm.h"
#include "vtkDataArray.h"

#include "vtkVVPluginAPI.h"

extern "C" {  
  static void vvSmoothProgress(vtkObject *obj, unsigned long, void *inf, 
                               void *vtkNotUsed(prog))
  {
    vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
    vtkAlgorithm *src = vtkAlgorithm::SafeDownCast(obj);
    if (src)
      {
      info->UpdateProgress(info,src->GetProgress(),"Smoothing..."); 
      /* check for abort */
      src->SetAbortExecute(atoi(info->GetProperty(info,VVP_ABORT_PROCESSING)));
      }
  }
}

static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
  int *dim = info->InputVolumeDimensions;

  // create a Gaussian Filter 
  vtkImageGaussianSmooth *ig = vtkImageGaussianSmooth::New();
  // Set the parameters on it
  ig->SetStandardDeviations(
    atof(info->GetGUIProperty(info,0,VVP_GUI_VALUE)),
    atof(info->GetGUIProperty(info,1,VVP_GUI_VALUE)),
    atof(info->GetGUIProperty(info,2,VVP_GUI_VALUE)));
  ig->SetRadiusFactors(3,3,3);

  // setup progress
  vtkCallbackCommand *cc = vtkCallbackCommand::New();
  cc->SetCallback(vvSmoothProgress);
  cc->SetClientData(inf);
  ig->AddObserver(vtkCommand::ProgressEvent,cc);
  cc->Delete();
  
  // setup the input
  vtkImageImport *ii = vtkImageImport::New();
  ii->SetDataExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
  ii->SetWholeExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
  ii->SetDataScalarType(info->InputVolumeScalarType);
  ii->SetNumberOfScalarComponents(
    info->InputVolumeNumberOfComponents);
  ii->SetDataOrigin(info->InputVolumeOrigin[0], 
                    info->InputVolumeOrigin[1],
                    info->InputVolumeOrigin[2]);
  ii->SetDataSpacing(info->InputVolumeSpacing[0],
                     info->InputVolumeSpacing[1],
                     info->InputVolumeSpacing[2]);
  ii->SetImportVoidPointer(pds->inData);
  ig->SetInput(ii->GetOutput());
  
  // get the output, would be nice to have VTK write directly 
  // into the output buffer but... VTK is often broken in that regard
  // so we will try, but check afterwards to see if it worked
  vtkImageData *od = ig->GetOutput();
  od->UpdateInformation();
  od->SetExtent(0,0,0,0,0,0);
  od->AllocateScalars();
  int size = dim[0] * dim[1] * pds->NumberOfSlicesToProcess * 
    info->InputVolumeNumberOfComponents;
  od->SetExtent(0, dim[0] - 1, 0, dim[1] - 1,
                pds->StartSlice, 
                pds->StartSlice + pds->NumberOfSlicesToProcess - 1);
  od->GetPointData()->GetScalars()->SetVoidArray(pds->outData,size,1);

  // run the filter
  od->SetUpdateExtent(od->GetExtent());
  od->Update();

  // did VTK not use our memory?
  if (od->GetScalarPointer() != pds->outData)
    {
    memcpy(pds->outData, od->GetScalarPointer(), 
           (od->GetPointData()->GetScalars()->GetMaxId() + 1)*
           od->GetPointData()->GetScalars()->GetDataTypeSize());
    }
  
  // clean up
  ii->Delete();
  ig->Delete();
  return 0;
}

static int UpdateGUI(void *inf)
{
  char tmp[1024];
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "X Standard Deviation");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT, "1");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP, "The standard deviation in pixels");
  info->SetGUIProperty(info, 0, VVP_GUI_HINTS , "0 4 0.1");

  info->SetGUIProperty(info, 1, VVP_GUI_LABEL, "Y Standard Deviation");
  info->SetGUIProperty(info, 1, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 1, VVP_GUI_DEFAULT, "1");
  info->SetGUIProperty(info, 1, VVP_GUI_HELP, "The standard deviation in pixels");
  info->SetGUIProperty(info, 1, VVP_GUI_HINTS , "0 4 0.1");

  info->SetGUIProperty(info, 2, VVP_GUI_LABEL, "Z Standard Deviation");
  info->SetGUIProperty(info, 2, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 2, VVP_GUI_DEFAULT, "1");
  info->SetGUIProperty(info, 2, VVP_GUI_HELP, "The standard deviation in pixels");
  info->SetGUIProperty(info, 2, VVP_GUI_HINTS , "0 4 0.1");

  
  const char * text = info->GetGUIProperty(info,2,VVP_GUI_VALUE);
  if( text )
    {
    sprintf(tmp,"%f", 3.0 * atof( text ) + 0.5);
    info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP, tmp);
    }
  else 
    {
    info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP, "0");
    }

  
  info->OutputVolumeScalarType = info->InputVolumeScalarType;
  info->OutputVolumeNumberOfComponents = info->InputVolumeNumberOfComponents;
  int i;
  for (i = 0; i < 3; i++)
    {
    info->OutputVolumeDimensions[i] = info->InputVolumeDimensions[i];
    info->OutputVolumeSpacing[i] = info->InputVolumeSpacing[i];
    info->OutputVolumeOrigin[i] = info->InputVolumeOrigin[i];
    }


  // provide accurate estimate of memory required
  // always requires 2 copies of the input volume 
  int sizeReq = 2*info->InputVolumeNumberOfComponents*
    info->InputVolumeScalarSize;
  char tmps[500];
  sprintf(tmps,"%i",sizeReq);
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED, tmps); 

  return 1;
}

extern "C" {
  
void VV_PLUGIN_EXPORT vvVTKSmoothInit(vtkVVPluginInfo *info)
{
  /* always check the version */
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Gaussian Smooth (VTK)");
  info->SetProperty(info, VVP_GROUP, "Noise Suppression");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
           "Smooth the volume by convolving with a Gaussian kernel");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "This filter smooths a volume by convolving it with a Gaussian kernel with standard deviations as specified by the user. This filter operates in pieces, and does not change the dimensions, data type, or spacing of the volume.");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "1");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "3");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}

}
