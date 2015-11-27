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
#include "vtkImageCheckerboard.h"
#include "vtkCallbackCommand.h"
#include "vtkAlgorithm.h"
#include "vtkDataArray.h"

#include "vtkVVPluginAPI.h"

extern "C" {  
  static void vvCheckerBoardProgress(vtkObject *obj, unsigned long, void *inf, 
                               void *vtkNotUsed(prog))
  {
    vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
    vtkAlgorithm *src = vtkAlgorithm::SafeDownCast(obj);
    if (src)
      {
      info->UpdateProgress(info,src->GetProgress(),"Computing CheckerBoards..."); 
      /* check for abort */
      src->SetAbortExecute(atoi(info->GetProperty(info,VVP_ABORT_PROCESSING)));
      }
  }
}

static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
  int *dim = info->InputVolumeDimensions;
  int *dim2 = info->InputVolume2Dimensions;
  int *odim = info->OutputVolumeDimensions;
  
  // make sure the two volumes match
  if (dim[0] != dim2[0] || dim[1] != dim2[1] || dim[2] != dim2[2] ||
      info->InputVolumeScalarType != info->InputVolume2ScalarType ||
      info->InputVolumeNumberOfComponents != 
      info->InputVolume2NumberOfComponents)
    {
    info->SetProperty(info, VVP_ERROR, "The two inputs do not appear to be of the same data type, dimensions, and/or number of scalar components.");
    return 1;
    }
  
  // create a Gaussian Filter 
  vtkImageCheckerboard *ig = vtkImageCheckerboard::New();
  // Set the parameters on it
  ig->SetNumberOfDivisions(
    atoi(info->GetGUIProperty(info,0,VVP_GUI_VALUE)),
    atoi(info->GetGUIProperty(info,1,VVP_GUI_VALUE)),
    atoi(info->GetGUIProperty(info,2,VVP_GUI_VALUE)));

  // setup progress
  vtkCallbackCommand *cc = vtkCallbackCommand::New();
  cc->SetCallback(vvCheckerBoardProgress);
  cc->SetClientData(inf);
  ig->AddObserver(vtkCommand::ProgressEvent,cc);
  cc->Delete();
  
  // setup the input 1
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
  ig->SetInput1(ii->GetOutput());
  
  // setup the input 2
  vtkImageImport *ii2 = vtkImageImport::New();
  ii2->SetDataExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
  ii2->SetWholeExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
  ii2->SetDataScalarType(info->InputVolume2ScalarType);
  ii2->SetNumberOfScalarComponents(
    info->InputVolume2NumberOfComponents);
  ii2->SetDataOrigin(info->InputVolume2Origin[0],
                     info->InputVolume2Origin[1],
                     info->InputVolume2Origin[2]);
  ii2->SetDataSpacing(info->InputVolume2Spacing[0],
                      info->InputVolume2Spacing[1],
                      info->InputVolume2Spacing[2]);
  ii2->SetImportVoidPointer(pds->inData2);
  ig->SetInput2(ii2->GetOutput());

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
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "I CheckerBoard Factor");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT, "2");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP, "How many checker board divisions for this axis");
  info->SetGUIProperty(info, 0, VVP_GUI_HINTS , "1 10 1");

  info->SetGUIProperty(info, 1, VVP_GUI_LABEL, "J CheckerBoard Factor");
  info->SetGUIProperty(info, 1, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 1, VVP_GUI_DEFAULT, "2");
  info->SetGUIProperty(info, 1, VVP_GUI_HELP, "How many checker board divisions for this axis");
  info->SetGUIProperty(info, 1, VVP_GUI_HINTS , "1 10 1");

  info->SetGUIProperty(info, 2, VVP_GUI_LABEL, "K CheckerBoard Factor");
  info->SetGUIProperty(info, 2, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 2, VVP_GUI_DEFAULT, "2");
  info->SetGUIProperty(info, 2, VVP_GUI_HELP, "How many checker board divisions for this axis");
  info->SetGUIProperty(info, 2, VVP_GUI_HINTS , "1 10 1");


  info->OutputVolumeScalarType = info->InputVolumeScalarType;
  info->OutputVolumeNumberOfComponents = info->InputVolumeNumberOfComponents;
  int i;
  for (i = 0; i < 3; i++)
    {
    info->OutputVolumeDimensions[i] = info->InputVolumeDimensions[i];
    info->OutputVolumeSpacing[i] = info->InputVolumeSpacing[i];
    info->OutputVolumeOrigin[i] = info->InputVolumeOrigin[i];
    }

  return 1;
}

extern "C" {
  
void VV_PLUGIN_EXPORT vvVTKCheckerBoardInit(vtkVVPluginInfo *info)
{
  /* always check the version */
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "CheckerBoard Filter (VTK)");
  info->SetProperty(info, VVP_GROUP, "Utility");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
          "Merges two volumes into one such that both can be seen");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "This filter displays two images as one using a checkerboard pattern. This filter can be used to compare two images. The checkerboard pattern is controlled by the Number Of Divisions on each axis. This controls the number of checkerboard divisions in the whole extent of the image. This filter requires two inputs and they must be of the same dimensions, number of scalar components, and scalar type (e.g. both float or both short etc). This filter does not change the data type or number of components of the volume.");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "0");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "3");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_REQUIRES_SECOND_INPUT,        "1");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}

}
