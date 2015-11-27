/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vvITKPaintbrushRunnerBase.h"
#include "itkImageRegionExclusionIteratorWithIndex.h"

// =======================================================================
// The main class definition
// =======================================================================
template <class PixelType, class LabelPixelType > 
class PaintbrushCropRunner : 
  public PaintbrushRunnerBase< PixelType, LabelPixelType >
{
public:
  // define our typedefs
  typedef PaintbrushRunnerBase< PixelType, LabelPixelType > Superclass;
  typedef typename Superclass::LabelImageType LabelImageType;
  typedef itk::ImageRegionExclusionIteratorWithIndex< LabelImageType > ExclusionIteratorType;

  PaintbrushCropRunner() {};

  // Description:
  // The actual execute method
  virtual int Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds );
};


// =======================================================================
// Main execute method
template <class PixelType, class LabelPixelType> 
int PaintbrushCropRunner<PixelType,LabelPixelType>::
Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds )
{
  this->Superclass::Execute(info, pds);

  /* convert cropping planes into indices */
  int idx[6];
  for (int j = 0; j < 6; ++j)
    {
    idx[j] = (int)((info->CroppingPlanes[j]-info->InputVolumeOrigin[j/2])/
      info->InputVolumeSpacing[j/2] + 0.5);
    if (idx[j] < 0) 
      {
      idx[j] = 0;
      }
    if (idx[j] >= info->InputVolumeDimensions[j/2])
      {
      idx[j] = info->InputVolumeDimensions[j/2] -1;
      }
    }

  typename Superclass::IndexType index;
  typename Superclass::SizeType size;
  index[0] = idx[0];
  index[1] = idx[2];
  index[2] = idx[4];
  size[0] = idx[1]-idx[0]+1;
  size[1] = idx[3]-idx[2]+1;
  size[2] = idx[5]-idx[4]+1;
  typename Superclass::RegionType region(index,size);

  ExclusionIteratorType lit( this->m_LabelImportFilter->GetOutput(),
      this->m_LabelImportFilter->GetOutput()->GetBufferedRegion());
  lit.SetExclusionRegion(region);

  info->UpdateProgress(info,0.1,"Beginning Cropping..");

  for (lit.GoToBegin(); !lit.IsAtEnd(); ++lit)
    {
    lit.Set(0);
    }

  info->UpdateProgress(info,1.0,"Done cropping.");

  return 0;
}


// =======================================================================
static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
  
  // do some error checking
  if (!pds->inLabelData)
    {
    info->SetProperty(
      info, VVP_ERROR, "Create a label map with the paintbrush first.");
    return 1;
    }
  
  if (info->InputVolumeNumberOfComponents != 1)
    {
    info->SetProperty(
      info, VVP_ERROR, "The input volume must be single component.");
    return 1;
    }

  // Execute
  vvITKPaintbrushRunnerExecuteTemplateMacro( 
    PaintbrushCropRunner, info, pds );
}


// =======================================================================
static int UpdateGUI(void *inf)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  info->OutputVolumeScalarType = info->InputVolumeScalarType;
  info->OutputVolumeNumberOfComponents = info->InputVolumeNumberOfComponents;
  memcpy(info->OutputVolumeDimensions,info->InputVolumeDimensions,3*sizeof(int));
  memcpy(info->OutputVolumeSpacing,info->InputVolumeSpacing,3*sizeof(float));
  memcpy(info->OutputVolumeOrigin,info->InputVolumeOrigin,3*sizeof(float));

  // really the memory consumption is one copy of the input + the size of the 
  // label pixel
  char tmp[256];
  sprintf(tmp,"%f", (double)(info->InputVolumeScalarSize + 1));
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED, tmp); 

  return 1;
}


// =======================================================================
extern "C" {
  
void VV_PLUGIN_EXPORT vvITKPaintbrushCropInit(vtkVVPluginInfo *info)
{
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI   = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Crop paintbrush labels");
  info->SetProperty(info, VVP_GROUP, "NIRFast Modules");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
                            "Crop paintbrush label map.");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "This plugin takes a paintbrush label map and crops it to the extent defined by the cropping planes.");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "1");
  info->SetProperty(info, VVP_REQUIRES_LABEL_INPUT,         "1");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "0");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,    "0"); 
  info->SetProperty(info, VVP_REQUIRES_SECOND_INPUT,        "0");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}

}


