/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vvITKPaintbrushRunnerBase.h"

// =======================================================================
// The main class definition
// =======================================================================
template <class PixelType, class LabelPixelType > 
class ThresholdImageToPaintbrushRunner : 
  public PaintbrushRunnerBase< PixelType, LabelPixelType >
{
public:
  // define our typedefs
  typedef PaintbrushRunnerBase< PixelType, LabelPixelType > Superclass;
  typedef typename Superclass::ImageConstIteratorType ImageConstIteratorType;
  typedef typename Superclass::LabelIteratorType LabelIteratorType;

  ThresholdImageToPaintbrushRunner() {};

  // Description:
  // The actual execute method
  virtual int Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds );
};


// =======================================================================
// Main execute method
template <class PixelType, class LabelPixelType> 
int ThresholdImageToPaintbrushRunner<PixelType,LabelPixelType>::
Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds )
{
  this->Superclass::Execute(info, pds);

  const float lowerThreshold = atof( info->GetGUIProperty(info, 0, VVP_GUI_VALUE ));
  const float upperThreshold = atof( info->GetGUIProperty(info, 1, VVP_GUI_VALUE ));
  const unsigned int labelValue = atoi( info->GetGUIProperty(info, 2, VVP_GUI_VALUE ));
  const bool replace = atoi( info->GetGUIProperty(info, 3, VVP_GUI_VALUE)) ? true : false;
  
  ImageConstIteratorType iit( this->m_ImportFilter->GetOutput(),
      this->m_ImportFilter->GetOutput()->GetBufferedRegion());
  LabelIteratorType lit( this->m_LabelImportFilter->GetOutput(),
      this->m_LabelImportFilter->GetOutput()->GetBufferedRegion());

  info->UpdateProgress(info,0.1,"Beginning thresholding..");

  unsigned long nPixelsThresholded = 0;
  PixelType pixel;
  for (iit.GoToBegin(), lit.GoToBegin(); !iit.IsAtEnd(); ++iit, ++lit)
    {
    pixel = iit.Get();
    if (pixel >= lowerThreshold && pixel <= upperThreshold)
      {
      lit.Set(labelValue);
      ++nPixelsThresholded;
      }
    else if (replace)
      {
      lit.Set(0);
      }
    }

  info->UpdateProgress(info,1.0,"Done thresholding.");

  char results[1024];
  sprintf(results,"Number of Pixels thresholded: %lu", nPixelsThresholded);
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
  
  if (info->InputVolumeNumberOfComponents != 1)
    {
    info->SetProperty(
      info, VVP_ERROR, "The input volume must be single component.");
    return 1;
    }

  // Execute
  vvITKPaintbrushRunnerExecuteTemplateMacro( 
    ThresholdImageToPaintbrushRunner, info, pds );
}


// =======================================================================
static int UpdateGUI(void *inf)
{
  char tmp[1024], lower[256], upper[256];
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  /* set the range of the sliders */
  double stepSize = 1.0;
  if (info->InputVolumeScalarType == VTK_FLOAT || 
      info->InputVolumeScalarType == VTK_DOUBLE) 
    { 
    /* for float and double use a step size of 1/200 th the range */
    stepSize = info->InputVolumeScalarRange[1]*0.005 - 
      info->InputVolumeScalarRange[0]*0.005; 
    }
  sprintf(tmp,"%g %g %g", 
          info->InputVolumeScalarRange[0],
          info->InputVolumeScalarRange[1],
          stepSize);
  sprintf(lower,"%g", info->InputVolumeScalarRange[0]);
  sprintf(upper,"%g", info->InputVolumeScalarRange[1]);

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "Lower Threshold");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT, lower);
  info->SetGUIProperty(info, 0, VVP_GUI_HELP, "Lower value for the range");
  info->SetGUIProperty(info, 0, VVP_GUI_HINTS , tmp);

  info->SetGUIProperty(info, 1, VVP_GUI_LABEL, "Upper Threshold");
  info->SetGUIProperty(info, 1, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 1, VVP_GUI_DEFAULT, upper);
  info->SetGUIProperty(info, 1, VVP_GUI_HELP, "Upper value for the range");
  info->SetGUIProperty(info, 1, VVP_GUI_HINTS , tmp);

  info->SetGUIProperty(info, 2, VVP_GUI_LABEL, "Paintbrush Label");
  info->SetGUIProperty(info, 2, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 2, VVP_GUI_DEFAULT, "1");
  info->SetGUIProperty(info, 2, VVP_GUI_HELP, "Paintbrush label value to fill. Voxels within the supplied threshold range will be accumulated into the label value for the selected paintbrush.");
  info->SetGUIProperty(info, 2, VVP_GUI_HINTS , "1 255 1");
  
  info->SetGUIProperty(info, 3, VVP_GUI_LABEL, "Replace ?");
  info->SetGUIProperty(info, 3, VVP_GUI_TYPE, VVP_GUI_CHECKBOX);
  info->SetGUIProperty(info, 3, VVP_GUI_DEFAULT, "1");
  info->SetGUIProperty(info, 3, VVP_GUI_HELP, "The thresholding can replace the existing paintbrush label values or augment to the existing paintbrush label values.");
    
  info->OutputVolumeScalarType = info->InputVolumeScalarType;
  info->OutputVolumeNumberOfComponents = info->InputVolumeNumberOfComponents;
  memcpy(info->OutputVolumeDimensions,info->InputVolumeDimensions,3*sizeof(int));
  memcpy(info->OutputVolumeSpacing,info->InputVolumeSpacing,3*sizeof(float));
  memcpy(info->OutputVolumeOrigin,info->InputVolumeOrigin,3*sizeof(float));

  // really the memory consumption is one copy of the input + the size of the 
  // label pixel
  sprintf(tmp,"%f", (double)(info->InputVolumeScalarSize + 1));
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED, tmp); 

  return 1;
}


// =======================================================================
extern "C" {
  
void VV_PLUGIN_EXPORT vvITKThresholdImageToPaintbrushInit(vtkVVPluginInfo *info)
{
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI   = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Threshold to Paintbrush");
  info->SetProperty(info, VVP_GROUP, "NIRFast Modules");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
                            "Threshold to a Paintbrush label map.");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "This plugin takes an image and appends a paintbrush label map for voxels that lie within the supplied thresholds. Both threshold values are inclusive. The label value indicates the sketch that is appended into the paintbrush.");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "1");
  info->SetProperty(info, VVP_REQUIRES_LABEL_INPUT,         "1");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "4");
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

