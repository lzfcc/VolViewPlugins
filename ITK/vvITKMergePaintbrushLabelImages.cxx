/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vvITKTwoLabelInputPaintbrushRunnerBase.h"

// =======================================================================
// The main class definition
// =======================================================================
template <class Pixel1Type, class Pixel2Type, class LabelPixelType > 
class MergePaintbrushLabelImagesRunner : 
  public PaintbrushRunnerBaseTwoInputs< Pixel1Type, Pixel2Type, LabelPixelType >
{
public:
  // define our typedefs
  typedef PaintbrushRunnerBaseTwoInputs< Pixel1Type, Pixel2Type, LabelPixelType > Superclass;
  typedef typename Superclass::LabelImageType         LabelImageType;
  typedef itk::Image< Pixel1Type, 3 >                 Image1Type;
  typedef itk::Image< Pixel2Type, 3 >                 Image2Type;
  typedef itk::ImageRegionConstIterator< Image1Type > Image1ConstIteratorType;
  typedef itk::ImageRegionConstIterator< Image2Type > Image2ConstIteratorType;
  typedef itk::ImageRegionIterator< LabelImageType > LabelIteratorType;

  MergePaintbrushLabelImagesRunner() {};

  // Description:
  // The actual execute method
  virtual int Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds );
};


// =======================================================================
// Main execute method
template <class Pixel1Type, class Pixel2Type, class LabelPixelType> 
int MergePaintbrushLabelImagesRunner<Pixel1Type,Pixel2Type,LabelPixelType>::
Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds )
{
  this->Superclass::Execute(info, pds);

  const bool overwriteColocatedLabels = atoi( info->GetGUIProperty( 
    info, 0, VVP_GUI_VALUE)) ? true : false;  

  Image2ConstIteratorType iit2( this->m_ImportFilter2->GetOutput(),
      this->m_ImportFilter2->GetOutput()->GetBufferedRegion());
  LabelIteratorType lit( this->m_LabelImportFilter->GetOutput(),
      this->m_LabelImportFilter->GetOutput()->GetBufferedRegion());

  info->UpdateProgress(info,0.1,"Beginning merge..");

  unsigned long nLabelsChanged = 0;
  LabelPixelType converted, old;
  Pixel2Type value;
  for (iit2.GoToBegin(), lit.GoToBegin(); !iit2.IsAtEnd(); ++iit2, ++lit)
    {
    value = iit2.Get();

    // Cast with clamping by ensuring that the source value is within the 
    // range of the target type before casting.
    if(value >= std::numeric_limits<LabelPixelType>::max())
      {
      converted = std::numeric_limits<LabelPixelType>::max();
      } 
    else if(value <= std::numeric_limits<LabelPixelType>::min()) 
      {
      converted = std::numeric_limits<LabelPixelType>::min();
      } 
    else 
      {
      converted = static_cast<LabelPixelType>(value);
      }

    if (converted)
      {
      old = lit.Get();
      if ((old != converted) && (overwriteColocatedLabels || !old))
        {
        lit.Set(converted);
        ++nLabelsChanged;
        }
      }
    }

  info->UpdateProgress(info,1.0,"Done merging.");

  char results[1024];
  sprintf(results,"Number of pixels changed during merge: %lu", nLabelsChanged);
  info->SetProperty(info, VVP_REPORT_TEXT, results);
  
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
  
  // The label images being merged must have the same dimension as the input image.
  int *dim1 = info->InputVolumeDimensions;
  int *dim2 = info->InputVolume2Dimensions;

  if (dim1[0] != dim2[0] || dim1[1] != dim2[1] || dim1[2] != dim2[2])
    {
    char buffer[256];
    sprintf(buffer, "The dimensions of the current label map (if any) and the label map to be merged are not the same. This first one has dimensions (%d, %d, %d) while the second one has dimensions (%d, %d, %d)", 
            dim1[0], dim1[1], dim1[2], dim2[0], dim2[1], dim2[2]);
    info->SetProperty(info, VVP_REPORT_TEXT, buffer);
    return 1;
    }  
  
  // The label map image must be a scalar image.
  if (info->InputVolume2NumberOfComponents != 1)
    {
    info->SetProperty(
      info, VVP_ERROR, "The paintbrush label map must be single component.");
    return 1;
    }

  // Execute
  vvITKPaintbrushRunnerExecuteTemplateMacro2( 
    MergePaintbrushLabelImagesRunner, info, pds );
}


// =======================================================================
static int UpdateGUI(void *inf)
{
  char tmp[1024];
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  // When doing a merge, if there 
  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "Overwrite during conflict");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_CHECKBOX);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT, "0");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP, "When performing a merge, if the same voxel has a non-zero value in the current paintbrush (if any) and in the supplied label map, and these two values are different (ie there is a conflict), should we overwrite the current paintbrush labels or not ? In other words, not checking this checkbox makes the currently displayed label image immutable to changes by the label image being merged in.");


  // Output is the same as the input.. its unchanged.
  info->OutputVolumeScalarType = info->InputVolumeScalarType;
  info->OutputVolumeNumberOfComponents = info->InputVolumeNumberOfComponents;
  memcpy(info->OutputVolumeDimensions,info->InputVolumeDimensions,3*sizeof(int));
  memcpy(info->OutputVolumeSpacing,info->InputVolumeSpacing,3*sizeof(float));
  memcpy(info->OutputVolumeOrigin,info->InputVolumeOrigin,3*sizeof(float));
  
  
  // really the memory consumption is one copy of the input + the size of the 
  // label pixel * 2 
  sprintf(tmp,"%f", (double)(info->InputVolumeScalarSize + 2));
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED, tmp); 

  return 1;
}


// =======================================================================
extern "C" {
  
void VV_PLUGIN_EXPORT vvITKMergePaintbrushLabelImagesInit(vtkVVPluginInfo *info)
{
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI   = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Merge paintbrushes");
  info->SetProperty(info, VVP_GROUP, "NIRFast Modules");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
     "Merge with paintbrush supplied");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "This plugin takes an label image via the second input. It appends it with the currently selected paintbrush label map, if present. If no paintbrush label map is present, it creates a blank one and initializes it with the supplied image. This this plugin may be used to merge a paintbrush label map with another, or may be used to convert an image into a paintbrush label map. The label map images being merged must have the same dimensions. See the \"Overwrite\" option for conflict resolution.");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "1");
  info->SetProperty(info, VVP_REQUIRES_LABEL_INPUT,         "1");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "1");
  info->SetProperty(info, VVP_REQUIRES_SECOND_INPUT,        "1");
}

}

