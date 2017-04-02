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
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkProbeFilter.h"
#include "vtkCallbackCommand.h"
#include "vtkAlgorithm.h"
#include "vtkImageCast.h"
#include "vtkDataArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkImageAppendComponents.h"
#include "vtkImageExtractComponents.h"
#include "vtkMultiThreader.h"

#include "vtkVVPluginAPI.h"

extern "C" {  
  static void vvMergeTetsProgress(vtkObject *obj, unsigned long, void *inf, 
                               void *vtkNotUsed(prog))
  {
    vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
    vtkAlgorithm *src = vtkAlgorithm::SafeDownCast(obj);
    if (src)
      {
      info->UpdateProgress(info,src->GetProgress(),"Discretizing volume..."); 
      /* check for abort */
      src->SetAbortExecute(atoi(info->GetProperty(info,VVP_ABORT_PROCESSING)));
      }
  }
}

extern "C" {  
  static void vvMergeTetsAppendProgress(vtkObject *obj, unsigned long, void *inf, 
                               void *vtkNotUsed(prog))
  {
    vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
    vtkAlgorithm *src = vtkAlgorithm::SafeDownCast(obj);
    if (src)
      {
      info->UpdateProgress(info,src->GetProgress(),"Appending volume..."); 
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
  
  // create a Probe Filter 
  vtkProbeFilter *probe = vtkProbeFilter::New();
  vtkImageAppendComponents *append = vtkImageAppendComponents::New();

  // setup progress
  vtkCallbackCommand *cc = vtkCallbackCommand::New();
  cc->SetCallback(vvMergeTetsProgress);
  cc->SetClientData(inf);
  probe->AddObserver(vtkCommand::ProgressEvent,cc);
  cc->Delete();
  
  vtkCallbackCommand *c1 = vtkCallbackCommand::New();
  c1->SetCallback(vvMergeTetsAppendProgress);
  c1->SetClientData(inf);
  append->AddObserver(vtkCommand::ProgressEvent,c1);
  c1->Delete();
  
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
  ii->Update();

  vtkImageCast *cast = vtkImageCast::New();
  cast->SetInput(ii->GetOutput());
  cast->SetOutputScalarTypeToFloat();

  // Take care of the case where we already have 4 components ...
  vtkImageExtractComponents *extract = vtkImageExtractComponents::New();
  if (info->InputVolumeNumberOfComponents == 4)
    {
    extract->SetInputConnection(ii->GetOutputPort());
    extract->SetComponents(0, 1, 2);
    cast->SetInputConnection(extract->GetOutputPort());
    }

  // setup the input 2
  vtkUnstructuredGrid *grid = vtkUnstructuredGrid::New();
  grid->DeepCopy((vtkUnstructuredGrid *) pds->inData2);

  // Set the parameters
  const char *result = info->GetGUIProperty(info, 0, VVP_GUI_VALUE);
  if ((strlen(result)!=strlen("Unspecified")) || 
    strcmp(result, "Unspecified"))
    {
    grid->GetPointData()->SetActiveScalars(result);
    } 

  // The probe filter doesn't like a multiple component image as a 
  // probe set.  Create an appropriate single component set to 
  // probe correctly.
  vtkImageData *gridTemplate = vtkImageData::New();
  gridTemplate->Initialize();
  gridTemplate->SetExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
  gridTemplate->SetSpacing (info->InputVolumeSpacing[0],
                            info->InputVolumeSpacing[1],
                            info->InputVolumeSpacing[2]);
  gridTemplate->SetOrigin (info->InputVolumeOrigin[0],
                           info->InputVolumeOrigin[1],
                           info->InputVolumeOrigin[2]);
  gridTemplate->SetScalarTypeToFloat();
  gridTemplate->SetNumberOfScalarComponents(1);
  gridTemplate->AllocateScalars();

  // get the output, would be nice to have VTK write directly 
  // into the output buffer but... VTK is often broken in that regard
  // so we will try, but check afterwards to see if it worked
  probe->SetInput(gridTemplate);
  probe->SetSource(grid);
  
  append->SetInputConnection(0, cast->GetOutputPort());
  append->SetInput(1, probe->GetOutput());
  append->Update();

  vtkImageData *od = vtkImageData::SafeDownCast(append->GetOutput());
  if (od)
    {
    int size = dim[0] * dim[1] * pds->NumberOfSlicesToProcess * 
      info->OutputVolumeNumberOfComponents;
    memcpy(pds->outData, od->GetScalarPointer(), 
           size*(od->GetPointData()->GetScalars()->GetDataTypeSize()));
    }

  // clean up
  ii->Delete();
  grid->Delete();
  cast->Delete();
  append->Delete();
  probe->Delete();
  gridTemplate->Delete();
  extract->Delete();
  return 0;
}

static int UpdateGUI(void *inf)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "Merge Field:");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_CHOICE);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT , "Unspecified");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP,
                       "Field to include in the merge volumes.  Unspecified uses the default scalar field of the unstructured grid.  Choosing something else changes the field to be imported.  The filter supports up to 4 components.  When processing a fifth component, the filter overwrites the last component instead of appending.");
  if (info->UnstructuredGridScalarFields != 0)
    {
    info->SetGUIProperty(info, 0, VVP_GUI_HINTS, 
      info->UnstructuredGridScalarFields);
    }
  else
    {
    info->SetGUIProperty(info, 0, VVP_GUI_HINTS, "1\nUnspecified");
    }

  info->OutputVolumeScalarType = VTK_FLOAT;
  if (info->OutputVolumeNumberOfComponents < 4)
    {
    info->OutputVolumeNumberOfComponents = 
      info->InputVolumeNumberOfComponents+1;
    }
  else
    {
    info->OutputVolumeNumberOfComponents = 
      info->InputVolumeNumberOfComponents;
    }
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
  
void VV_PLUGIN_EXPORT vvVTKMergeTetsInit(vtkVVPluginInfo *info)
{
  /* always check the version */
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Merge Volumetric Results");
  info->SetProperty(info, VVP_GROUP, "NIRFast Modules");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
          "Dicretize an unstructured grid.");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "This filter takes in a volume and a VTK unstructured grid.  It dicretizes the unstructured grid to fit the image and then appends the discretized image as a new component of the original volume.");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING,          "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,            "0");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,                   "1");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,                    "0");
  info->SetProperty(info, VVP_REQUIRES_SECOND_INPUT,                 "1");
  info->SetProperty(info, VVP_SECOND_INPUT_IS_UNSTRUCTURED_GRID,     "1");
  info->SetProperty(info, VVP_RESULTING_COMPONENTS_ARE_INDEPENDENT,  "1");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,                 "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES,                "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT,              "0");
}

}
