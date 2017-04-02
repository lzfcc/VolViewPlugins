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
#include "vtkCellArray.h"
#include "vtkImageImport.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSynchronizedTemplates3D.h"
#include "vtkCallbackCommand.h"
#include "vtkAlgorithm.h"

#include "vtkVVPluginAPI.h"

extern "C" {  
  static void vvIsoProgress(vtkObject *obj, unsigned long, void *inf, 
                            void *vtkNotUsed(prog))
  {
    vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
    vtkAlgorithm *src = vtkAlgorithm::SafeDownCast(obj);
    if (src)
      {
      info->UpdateProgress(info,src->GetProgress(),"Contouring..."); 
      /* check for abort */
      src->SetAbortExecute(atoi(info->GetProperty(info,VVP_ABORT_PROCESSING)));
      }
  }
}

static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
  int *dim = info->InputVolume2Dimensions;
  if (dim[0] <= 0 || dim[1] <= 0 || dim[2] <= 0)
    {
    char buffer[256];
    sprintf(buffer, "Invalid dimension for second input (%d, %d, %d)", 
            dim[0], dim[1], dim[2]);
    info->SetProperty(info, VVP_REPORT_TEXT, buffer);
    return 1;
    }

  // create a Contour Filter 
  vtkSynchronizedTemplates3D *ig = vtkSynchronizedTemplates3D::New();
  // Set the parameters on it
  ig->SetValue(0,atof(info->GetGUIProperty(info,0,VVP_GUI_VALUE)));
  ig->SetComputeNormals(atoi(info->GetGUIProperty(info,1,VVP_GUI_VALUE)));
  
  // setup progress
  vtkCallbackCommand *cc = vtkCallbackCommand::New();
  cc->SetCallback(vvIsoProgress);
  cc->SetClientData(inf);
  ig->AddObserver(vtkCommand::ProgressEvent,cc);
  cc->Delete();
  
  // setup the input
  vtkImageImport *ii = vtkImageImport::New();
  ii->SetDataExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
  ii->SetWholeExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
  ii->SetDataScalarType(info->InputVolume2ScalarType);
  ii->SetNumberOfScalarComponents(
    info->InputVolume2NumberOfComponents);
  ii->SetDataOrigin(info->InputVolume2Origin[0], 
                    info->InputVolume2Origin[1],
                    info->InputVolume2Origin[2]);
  ii->SetDataSpacing(info->InputVolume2Spacing[0],
                     info->InputVolume2Spacing[1],
                     info->InputVolume2Spacing[2]);
  ii->SetImportVoidPointer(pds->inData2);
  ig->SetInput(ii->GetOutput());
  
  // get the output
  vtkPolyData *od = ig->GetOutput();
  // run the filter
  ig->Update();

  // now put the results into the data structure
  if (od->GetPoints())
    {
    pds->NumberOfMeshPoints = od->GetPoints()->GetNumberOfPoints();
    pds->MeshPoints = (float *)od->GetPoints()->GetVoidPointer(0);
    }
  else
    {
    pds->NumberOfMeshPoints = 0;
    pds->MeshPoints = NULL;
    }
  if (od->GetPointData() && od->GetPointData()->GetNormals())
    {
    pds->MeshNormals = 
      (float *)od->GetPointData()->GetNormals()->GetVoidPointer(0);    
    }
  else
    {
    pds->MeshNormals = NULL;
    }
  if (od->GetPolys())
    {
    pds->NumberOfMeshCells = od->GetPolys()->GetNumberOfCells();
    int numEntries = od->GetPolys()->GetNumberOfConnectivityEntries();
    pds->MeshCells = new int [numEntries];
    vtkIdType *ptr = od->GetPolys()->GetPointer();
    int i;
    for (i = 0; i < numEntries; ++i)
      {
      pds->MeshCells[i] = *ptr;
      ptr++;
      }
    }
  else
    {
    pds->NumberOfMeshCells = 0;
    pds->MeshCells = NULL;
    }

  // return the polygonal data
  info->AssignPolygonalData(info, pds);

  // report the results
  char results[1024];
  sprintf(results,"Isosurfacing produced %d polygons", pds->NumberOfMeshCells);
  info->SetProperty(info, VVP_REPORT_TEXT,results);
  
  // clean up
  delete [] pds->MeshCells;
  ii->Delete();
  ig->Delete();
  return 0;
}

static int UpdateGUI(void *inf)
{
  char tmp[1024];
  double stepSize = 1.0;
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "Isovalue");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT, "0");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP, "The value of the isosurface");

  /* set the range of the sliders */
  if (info->InputVolume2ScalarType == VTK_FLOAT || 
      info->InputVolume2ScalarType == VTK_DOUBLE) 
    { 
    /* for float and double use a step size of 1/200 th the range */
    stepSize = info->InputVolume2ScalarTypeRange[1]*0.005 - 
      info->InputVolume2ScalarTypeRange[0]*0.005; 
    }
  sprintf(tmp,"%g %g %g", 
          info->InputVolume2ScalarTypeRange[0],
          info->InputVolume2ScalarTypeRange[1],
          stepSize);
  info->SetGUIProperty(info, 0, VVP_GUI_HINTS , tmp);

  info->SetGUIProperty(info, 1, VVP_GUI_LABEL, "Generate Normals");
  info->SetGUIProperty(info, 1, VVP_GUI_TYPE, VVP_GUI_CHECKBOX);
  info->SetGUIProperty(info, 1, VVP_GUI_DEFAULT, "0");
  info->SetGUIProperty(info, 1, VVP_GUI_HELP, "Should normals be generated while the isosurface is generated");

  return 1;
}

extern "C" {
  
void VV_PLUGIN_EXPORT vvVTKIsoInit(vtkVVPluginInfo *info)
{
  /* always check the version */
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Isosurface (VTK)");
  info->SetProperty(info, VVP_GROUP, "Surface Generation");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
                      "Extract an isosurface from another volume");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
                      "This filter will generate a surface at the specified value for the second input that was specified. It leave the current volume alone. This allows you to display the isosurface from one volume with the volume rendering from another. If the volume to be isosurfaced has more than one component then the first component will be the one isosurfaced. If you want to isosurface the current volume you can use the contours page which has more options.");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "0");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "2");
  info->SetProperty(info, VVP_PRODUCES_MESH_ONLY,           "1");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_REQUIRES_SECOND_INPUT,        "1");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}

}
