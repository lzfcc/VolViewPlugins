/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* perform a median filter */

#include <string.h>
#include <stdlib.h>
#include "vtkImageImport.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkImageShrink3D.h"
#include "vtkCallbackCommand.h"
#include "vtkAlgorithm.h"
#include "vtkDataArray.h"

#include "vtkVVPluginAPI.h"

extern "C" {  
  static void vvShrinkProgress(vtkObject *obj, unsigned long, void *inf, 
                               void *vtkNotUsed(prog))
  {
    vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
    vtkAlgorithm *src = vtkAlgorithm::SafeDownCast(obj);
    if (src)
      {
      info->UpdateProgress(info,src->GetProgress(),"Computing Shrinks..."); 
      /* check for abort */
      src->SetAbortExecute(atoi(info->GetProperty(info,VVP_ABORT_PROCESSING)));
      }
  }
}

static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
  int *dim = info->InputVolumeDimensions;
  int *odim = info->OutputVolumeDimensions;

  // create a Gaussian Filter 
  vtkImageShrink3D *ig = vtkImageShrink3D::New();
  // Set the parameters on it
  ig->SetShrinkFactors(
    atoi(info->GetGUIProperty(info,0,VVP_GUI_VALUE)),
    atoi(info->GetGUIProperty(info,1,VVP_GUI_VALUE)),
    atoi(info->GetGUIProperty(info,2,VVP_GUI_VALUE)));

  // set the operation
  const char *operation = info->GetGUIProperty(info, 3, VVP_GUI_VALUE);
  // default to subsample and turn on the appropriate method
  ig->MeanOff();
  if (!strcmp(operation,"Mean"))
    {
    ig->MeanOn();
    }
  if (!strcmp(operation,"Median"))
    {
    ig->MedianOn();
    }
  if (!strcmp(operation,"Minimum"))
    {
    ig->MinimumOn();
    }
  if (!strcmp(operation,"Maximum"))
    {
    ig->MaximumOn();
    }

  // setup progress
  vtkCallbackCommand *cc = vtkCallbackCommand::New();
  cc->SetCallback(vvShrinkProgress);
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
  int size = odim[0] * odim[1] * odim[2] * 
    info->InputVolumeNumberOfComponents;
  od->SetExtent(0, odim[0] - 1, 0, odim[1] - 1, 0, odim[2] - 1);
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

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "I Shrink Factor");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT, "2");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP, "What factor to shrink by");
  info->SetGUIProperty(info, 0, VVP_GUI_HINTS , "1 10 1");

  info->SetGUIProperty(info, 1, VVP_GUI_LABEL, "J Shrink Factor");
  info->SetGUIProperty(info, 1, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 1, VVP_GUI_DEFAULT, "2");
  info->SetGUIProperty(info, 1, VVP_GUI_HELP, "What factor to shrink by");
  info->SetGUIProperty(info, 1, VVP_GUI_HINTS , "1 10 1");

  info->SetGUIProperty(info, 2, VVP_GUI_LABEL, "K Shrink Factor");
  info->SetGUIProperty(info, 2, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 2, VVP_GUI_DEFAULT, "2");
  info->SetGUIProperty(info, 2, VVP_GUI_HELP, "What factor to shrink by");
  info->SetGUIProperty(info, 2, VVP_GUI_HINTS , "1 10 1");

  info->SetGUIProperty(info, 3, VVP_GUI_LABEL, "Shrink Technique");
  info->SetGUIProperty(info, 3, VVP_GUI_TYPE, VVP_GUI_CHOICE);
  info->SetGUIProperty(info, 3, VVP_GUI_DEFAULT , "Mean");
  info->SetGUIProperty(info, 3, VVP_GUI_HELP,
                       "Operation to perform");
  info->SetGUIProperty(info, 3, VVP_GUI_HINTS, 
                       "5\nMean\nMinimum\nMaximum\nMedian\nSubSample");

  const char *text;
  int factors[3];
  int i;
  for (i = 0; i < 3; ++i)
    {
    factors[i] = 1;
    text = info->GetGUIProperty(info,i,VVP_GUI_VALUE);
    if (text)
      {
      factors[i] = atoi(text);
      }
    }
  
  sprintf(tmp,"%d", factors[2] -1 ); 
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP, tmp);
  
  info->OutputVolumeScalarType = info->InputVolumeScalarType;
  info->OutputVolumeNumberOfComponents = 
    info->InputVolumeNumberOfComponents;
  
  for (i = 0; i < 3; ++i)
    {
    info->OutputVolumeDimensions[i] = 
      (int)(floor((float)(info->InputVolumeDimensions[i]-factors[i])
                  / (float)(factors[i]))) + 1;
    info->OutputVolumeSpacing[i]  = info->InputVolumeSpacing[i]*factors[i];
    info->OutputVolumeOrigin[i] = info->InputVolumeOrigin[i];
    }
  
  return 1;
}

extern "C" {
  
void VV_PLUGIN_EXPORT vvVTKShrinkInit(vtkVVPluginInfo *info)
{
  /* always check the version */
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Shrink Filter (VTK)");
  info->SetProperty(info, VVP_GROUP, "Utility");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
          "Produce a smaller volume (less voxels) by some integer factor.");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "Compute a reduced resolution volume. This filter will produce a reduce resolution version of the input volume using the technique specified. The Basic process is that a number of input voxels will go into producing just one output voxel. For example with a shrink factor of two on each axis eight input voxels will go towards creating one output voxel. The value of the output voxel is determined by the shrink technique specified. Subsample takes the physically closest input voxel to be the output voxel and ignores the rest. Mean, Median, Minimum, and Maximum perform their respective operation on all the pertinate input voxels (eight in this example) to compute the resulting ouotput voxel. This filter does not change the data type or number of components of the volume.");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "0");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "4");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}

}
