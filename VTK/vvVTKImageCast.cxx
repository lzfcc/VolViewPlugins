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
#include "vtkImageCast.h"
#include "vtkCallbackCommand.h"
#include "vtkAlgorithm.h"
#include "vtkDataArray.h"

#include "vtkVVPluginAPI.h"

extern "C" {  
  static void vvImageCastProgress(vtkObject *obj, 
                                          unsigned long, void *inf, 
                                          void *vtkNotUsed(prog))
  {
    vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
    vtkAlgorithm *src = vtkAlgorithm::SafeDownCast(obj);
    if (src)
      {
      info->UpdateProgress(info,src->GetProgress(),
                           "Casting..."); 
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
  vtkImageCast *ig = vtkImageCast::New();

  // setup progress
  vtkCallbackCommand *cc = vtkCallbackCommand::New();
  cc->SetCallback(vvImageCastProgress);
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
  ig->SetOutputScalarType( info->OutputVolumeScalarType );
  
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
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "OutputDataType");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_CHOICE);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT , "Char");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP,
                       "Scalar type of the output. For multi component data, this is the scalar type of each component.");
  info->SetGUIProperty(info, 0, VVP_GUI_HINTS, 
             "9\nunsigned char\nshort\nunsigned short\nint\nunsigned int\nlong\nunsigned long\nfloat\ndouble");


  info->OutputVolumeScalarType = info->InputVolumeScalarType;

  const char *result = info->GetGUIProperty(info, 0, VVP_GUI_VALUE);
  if (result)
    {
    for (int i = VTK_UNSIGNED_CHAR; i <= VTK_DOUBLE; i++)
      {
      if (strcmp( result, vtkImageScalarTypeNameMacro(i) ) == 0)
        {
        info->OutputVolumeScalarType = i;
        std::cout << "Casting to " << vtkImageScalarTypeNameMacro(i) << std::endl;
        }
      }
    }
  
  info->OutputVolumeNumberOfComponents = info->InputVolumeNumberOfComponents;
  for (int i = 0; i < 3; i++)
    {
    info->OutputVolumeDimensions[i] = info->InputVolumeDimensions[i];
    info->OutputVolumeSpacing[i] = info->InputVolumeSpacing[i];
    info->OutputVolumeOrigin[i] = info->InputVolumeOrigin[i];
    }
  
  return 1;
}

extern "C" {
  
void VV_PLUGIN_EXPORT vvVTKImageCastInit(vtkVVPluginInfo *info)
{
  /* always check the version */
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Cast Image");
  info->SetProperty(info, VVP_GROUP, "Utility");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
          "Cast an image with one datatype into another");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "Cast an image with one datatype into another. No rounding is performed.");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "1");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "1");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "1");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}

}
