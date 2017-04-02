/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* perform a resample filter */

#include <string.h>
#include <stdlib.h>
#include "vtkImageImport.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkImageResample.h"
#include "vtkCallbackCommand.h"
#include "vtkAlgorithm.h"
#include "vtkDataArray.h"

#include "vtkVVPluginAPI.h"

extern "C" {  
  static void vvResampleProgress(vtkObject *obj, unsigned long, void *inf, 
                                 void *vtkNotUsed(prog))
  {
    vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
    vtkAlgorithm *src = vtkAlgorithm::SafeDownCast(obj);
    if (src)
      {
      info->UpdateProgress(info,src->GetProgress(),"Computing Resamples..."); 
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

  // Create a Resample Filter 

  vtkImageResample *ig = vtkImageResample::New();

  // Set the parameters on it

  ig->SetAxisMagnificationFactor(
    0, atof(info->GetGUIProperty(info, 0, VVP_GUI_VALUE)));
  ig->SetAxisMagnificationFactor(
    1, atof(info->GetGUIProperty(info, 1, VVP_GUI_VALUE)));
  ig->SetAxisMagnificationFactor(
    2, atof(info->GetGUIProperty(info, 2, VVP_GUI_VALUE)));

  // Set the interpolation type

  const char *interpolation = info->GetGUIProperty(info, 3, VVP_GUI_VALUE);

  ig->SetInterpolationModeToCubic();
  if (!strcmp(interpolation, "Nearest Neighbor"))
    {
    ig->SetInterpolationModeToNearestNeighbor();
    }
  else if (!strcmp(interpolation, "Linear"))
    {
    ig->SetInterpolationModeToLinear();
    }
  else if (!strcmp(interpolation, "Cubic"))
    {
    ig->SetInterpolationModeToCubic();
    }

  // Setup progress

  vtkCallbackCommand *cc = vtkCallbackCommand::New();
  cc->SetCallback(vvResampleProgress);
  cc->SetClientData(inf);
  ig->AddObserver(vtkCommand::ProgressEvent,cc);
  cc->Delete();
  
  // Setup the input

  vtkImageImport *ii = vtkImageImport::New();
  ii->SetDataExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
  ii->SetWholeExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
  ii->SetDataScalarType(info->InputVolumeScalarType);
  ii->SetNumberOfScalarComponents(info->InputVolumeNumberOfComponents);
  ii->SetDataOrigin(info->InputVolumeOrigin[0], 
                    info->InputVolumeOrigin[1],
                    info->InputVolumeOrigin[2]);
  ii->SetDataSpacing(info->InputVolumeSpacing[0],
                     info->InputVolumeSpacing[1],
                     info->InputVolumeSpacing[2]);
  ii->SetImportVoidPointer(pds->inData);
  ig->SetInput(ii->GetOutput());
  
  // Get the output, would be nice to have VTK write directly 
  // into the output buffer but... VTK is often broken in that regard
  // so we will try, but check afterwards to see if it worked

  vtkImageData *od = ig->GetOutput();
  od->UpdateInformation();
  od->SetExtent(0, 0, 0, 0, 0, 0);
  od->AllocateScalars();
  unsigned long size = 
    odim[0] * odim[1] * odim[2] * info->InputVolumeNumberOfComponents;
  od->SetExtent(0, odim[0] - 1, 0, odim[1] - 1, 0, odim[2] - 1);
  od->GetPointData()->GetScalars()->SetVoidArray(pds->outData, size, 1);

  // Run the filter

  od->SetUpdateExtent(od->GetExtent());
  od->Update();

  // Did VTK not use our memory?

  if (od->GetScalarPointer() != pds->outData)
    {
    memcpy(pds->outData, od->GetScalarPointer(), 
           (od->GetPointData()->GetScalars()->GetMaxId() + 1)*
           od->GetPointData()->GetScalars()->GetDataTypeSize());
    }
  
  // Clean up

  ii->Delete();
  ig->Delete();

  return 0;
}

static int UpdateGUI(void *inf)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "X Axis Magnification Factor");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT, "2.0");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP, "What factor to resample by");
  info->SetGUIProperty(info, 0, VVP_GUI_HINTS , "0.05 10.0 0.05");

  info->SetGUIProperty(info, 1, VVP_GUI_LABEL, "Y Axis Magnification Factor");
  info->SetGUIProperty(info, 1, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 1, VVP_GUI_DEFAULT, "2.0");
  info->SetGUIProperty(info, 1, VVP_GUI_HELP, "What factor to resample by");
  info->SetGUIProperty(info, 1, VVP_GUI_HINTS , "0.05 10.0 0.05");

  info->SetGUIProperty(info, 2, VVP_GUI_LABEL, "Z Axis Magnification Factor");
  info->SetGUIProperty(info, 2, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 2, VVP_GUI_DEFAULT, "2.0");
  info->SetGUIProperty(info, 2, VVP_GUI_HELP, "What factor to resample by");
  info->SetGUIProperty(info, 2, VVP_GUI_HINTS , "0.05 10.0 0.05");

  info->SetGUIProperty(info, 3, VVP_GUI_LABEL, "Interpolation Technique");
  info->SetGUIProperty(info, 3, VVP_GUI_TYPE, VVP_GUI_CHOICE);
  info->SetGUIProperty(info, 3, VVP_GUI_DEFAULT , "Cubic");
  info->SetGUIProperty(info, 3, VVP_GUI_HELP,
                       "Interpolation technique used to perform resampling");
  info->SetGUIProperty(info, 3, VVP_GUI_HINTS, 
                       "3\nNearest Neighbor\nLinear\nCubic");

  info->SetGUIProperty(info, 4, VVP_GUI_LABEL, "Output Spacing");
  info->SetGUIProperty(info, 4, VVP_GUI_TYPE, VVP_GUI_CHOICE);
  info->SetGUIProperty(info, 4, VVP_GUI_DEFAULT , "Divide By Factor");
  info->SetGUIProperty(info, 4, VVP_GUI_HELP,
                       "Specify how the output spacing should be computed from the input spacing");
  info->SetGUIProperty(info, 4, VVP_GUI_HINTS, 
                       "2\nDivide By Factor\nDo Not Change");

  int i;
  const char *text;
  float factors[3];

  vtkImageData *dummy_input = vtkImageData::New();
  dummy_input->SetScalarType(info->InputVolumeScalarType);
  dummy_input->SetNumberOfScalarComponents(info->InputVolumeNumberOfComponents);
  dummy_input->SetOrigin(info->InputVolumeOrigin[0],
                         info->InputVolumeOrigin[1],
                         info->InputVolumeOrigin[2]);
  dummy_input->SetSpacing(info->InputVolumeSpacing[0],
                          info->InputVolumeSpacing[1],
                          info->InputVolumeSpacing[2]);
  dummy_input->SetDimensions(info->InputVolumeDimensions);
  
  vtkImageResample *dummy_resample = vtkImageResample::New();
  dummy_resample->SetInput(dummy_input);
  dummy_resample->SetInterpolationModeToCubic();
  for (i = 0; i < 3; ++i)
    {
    text = info->GetGUIProperty(info, i, VVP_GUI_VALUE);
    factors[i] = text ? atof(text) : 2.0;
    dummy_resample->SetAxisMagnificationFactor(i, factors[i]);
    }

  vtkImageData *dummy_output = dummy_resample->GetOutput();
  dummy_output->UpdateInformation();
  
  int spacing_choice = 1;
  text = info->GetGUIProperty(info, 4, VVP_GUI_VALUE);
  if (text)
    {
    if (!strcmp(text, "Divide By Factor"))
      {
      spacing_choice = 1;
      }
    }

  info->OutputVolumeScalarType = dummy_output->GetScalarType();
  info->OutputVolumeNumberOfComponents = 
    dummy_output->GetNumberOfScalarComponents();
  
  for (i = 0; i < 3; ++i)
    {
    info->OutputVolumeOrigin[i] = dummy_output->GetOrigin()[i];
    info->OutputVolumeDimensions[i] = dummy_output->GetWholeExtent()[2 * i + 1]
      - dummy_output->GetWholeExtent()[2 * i] + 1;
    if (spacing_choice == 1)
      {
      info->OutputVolumeSpacing[i] = dummy_output->GetSpacing()[i];
      }
    else
      {
      info->OutputVolumeSpacing[i] = info->InputVolumeSpacing[i];
      }
    }

  dummy_input->Delete();
  dummy_resample->Delete();

  return 1;
}

extern "C" {
  
void VV_PLUGIN_EXPORT vvVTKResampleInit(vtkVVPluginInfo *info)
{
  // Always check the version

  vvPluginVersionCheck();

  // Setup information that never changes

  info->ProcessData = ProcessData;
  info->UpdateGUI = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Resample Filter (VTK)");
  info->SetProperty(info, VVP_GROUP, "Utility");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
          "Resample a volume by some magnification factors.");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "Compute a resampled volume. This filter will produce a resampled version of the input volume using the technique specified. The basic process is that a number of input voxels will go into producing just one output voxel. For example with a resample factor of two on each axis eight input voxels will go towards creating one output voxel. The value of the output voxel is determined by the interpolation technique specified. Nearest Neighbor, Linear, and Cubic perform their respective interpolation on all the pertinate input voxels (eight in this example) to compute the resulting output voxel. This filter does not change the data type or number of components of the volume.");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "0");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "5");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}

}


