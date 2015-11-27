/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* This plugin performs an integer MaskNegated operation between the first input and
 * the second input. The second input is expected to be the mask. The pixel type
 * of both images must be integer: {char, unsigned char, short, unsigned short,
 * int, unsigned int, long, unsigned long) (not float or double).
 */

#include "vvITKMaskNegated.h"



template <class InputPixelType>
class MaskNegatedRunner
  {
  public:
    typedef itk::Image< InputPixelType, 3 > InputImageType;
    typedef itk::Image< unsigned char,  3 > InputMaskNegatedImageType;
    
    typedef VolView::PlugIn::MaskNegated< InputImageType,
                                   InputMaskNegatedImageType >   ModuleType;
    
  public:
    MaskNegatedRunner() {}
    void Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds )
    {
      ModuleType  module;
      module.SetPluginInfo( info );
      module.SetUpdateMessage("MaskNegateding the image...");
      module.ProcessData( pds  );
    }
  };




static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{

  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  if( info->InputVolumeNumberOfComponents != 1 )
    {
    info->SetProperty( info, VVP_ERROR, "This filter requires a single-component data set as input" ); 
    return -1;
    }

  if( info->InputVolume2ScalarType != VTK_CHAR && 
      info->InputVolume2ScalarType != VTK_UNSIGNED_CHAR )
    {
    info->SetProperty( info, VVP_ERROR, "This filter requires a second input of pixel type char or unsigned char." ); 
    return -1;
    }

  try 
  {
  switch( info->InputVolumeScalarType )
    {
    case VTK_CHAR:
      {
      MaskNegatedRunner<signed char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_CHAR:
      {
      MaskNegatedRunner<unsigned char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_SHORT:
      {
      MaskNegatedRunner<signed short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_SHORT:
      {
      MaskNegatedRunner<unsigned short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_INT:
      {
      MaskNegatedRunner<signed int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_INT:
      {
      MaskNegatedRunner<unsigned int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_LONG:
      {
      MaskNegatedRunner<signed long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_LONG:
      {
      MaskNegatedRunner<unsigned long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_FLOAT:
      {
      MaskNegatedRunner<float> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_DOUBLE:
      {
      MaskNegatedRunner<double> runner;
      runner.Execute( info, pds );
      break; 
      }
    default:
      {
      info->SetProperty( info, VVP_ERROR, "The pixel type of this image is not supported in this filter" ); 
      return -1;
      }
    }
  }
  catch( itk::ExceptionObject & except )
  {
    info->SetProperty( info, VVP_ERROR, except.what() ); 
    return -1;
  }

  info->UpdateProgress( info, 1.0, "MaskNegateding Done !");

  return 0;
}


static int UpdateGUI(void *inf)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP, "0");
  
  info->OutputVolumeScalarType = info->InputVolumeScalarType;

  info->OutputVolumeNumberOfComponents = 1;

  memcpy(info->OutputVolumeDimensions,info->InputVolumeDimensions,
         3*sizeof(int));
  memcpy(info->OutputVolumeSpacing,info->InputVolumeSpacing,
         3*sizeof(float));
  memcpy(info->OutputVolumeOrigin,info->InputVolumeOrigin,
         3*sizeof(float));

  return 1;
}


extern "C" {
  
void VV_PLUGIN_EXPORT vvITKMaskNegatedInit(vtkVVPluginInfo *info)
{
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI   = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Masking Negated (ITK)");
  info->SetProperty(info, VVP_GROUP, "Utility");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
                                    "Remove regions by masking with another image.");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "This filter sets to zero all the pixels that are non-zero in a mask image provided as second input. It is commonly used for removing regions of the image when performing progressive segmentation.");

  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "0");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "0");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,   "16");
  info->SetProperty(info, VVP_REQUIRES_SECOND_INPUT,        "1");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}

}
