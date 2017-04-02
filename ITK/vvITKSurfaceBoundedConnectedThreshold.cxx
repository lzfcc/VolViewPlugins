/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* This module encapsulates the full preprocessing required for
   applying the DeformableModel image filter for segmenting the 
   volume.  It requires seed points and the original image as inputs. */


#include "vvITKSurfaceBoundedConnectedThreshold.txx"



template <class InputPixelType>
class SurfaceBoundedConnectedThresholdRunner
  {
  public:
      typedef VolView::PlugIn::SurfaceBoundedConnectedThreshold< 
                                            InputPixelType >   ModuleType;
  public:
    SurfaceBoundedConnectedThresholdRunner() {}
    void Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds )
    {
      const bool         engraveSurface   = atoi( info->GetGUIProperty(info, 0, VVP_GUI_VALUE ));
      const bool         doSegmentation   = atoi( info->GetGUIProperty(info, 1, VVP_GUI_VALUE ));
      const bool         compositeOutput  = atoi( info->GetGUIProperty(info, 2, VVP_GUI_VALUE ));
      const float upper            = atof( info->GetGUIProperty(info, 3, VVP_GUI_VALUE ) );
      const float lower            = atof( info->GetGUIProperty(info, 4, VVP_GUI_VALUE ) );
      const int   replaceValue     = atoi( info->GetGUIProperty(info, 5, VVP_GUI_VALUE ) );
      const int   r                = atoi( info->GetGUIProperty(info, 6, VVP_GUI_VALUE ) );


      ModuleType  module;
      module.SetPluginInfo( info );
      module.SetUpdateMessage("Computing...");
      module.SetDoSegmentation( doSegmentation );
      module.SetProduceDoubleOutput( compositeOutput );
      module.SetEngraveSurface( engraveSurface );


      // Set the parameters on it
      module.GetFilter()->SetUpper( static_cast<InputPixelType>( upper  ) );
      module.GetFilter()->SetLower( static_cast<InputPixelType>( lower ) );
      module.GetFilter()->SetReplaceValue( replaceValue );

      // Radius of 0 means no median filtering
      itk::Size< 3 > radius;
      radius.Fill( r );       // process (2*radius+1)^3 neighborhoods.
      module.GetMedianFilter()->SetRadius( radius );

      // Execute the filter
      module.ProcessData( pds  );
    }
  };



static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{

  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  try 
  {
  switch( info->InputVolumeScalarType )
    {
    case VTK_CHAR:
      {
      SurfaceBoundedConnectedThresholdRunner<signed char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_CHAR:
      {
      SurfaceBoundedConnectedThresholdRunner<unsigned char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_SHORT:
      {
      SurfaceBoundedConnectedThresholdRunner<signed short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_SHORT:
      {
      SurfaceBoundedConnectedThresholdRunner<unsigned short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_INT:
      {
      SurfaceBoundedConnectedThresholdRunner<signed int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_INT:
      {
      SurfaceBoundedConnectedThresholdRunner<unsigned int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_LONG:
      {
      SurfaceBoundedConnectedThresholdRunner<signed long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_LONG:
      {
      SurfaceBoundedConnectedThresholdRunner<unsigned long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_FLOAT:
      {
      SurfaceBoundedConnectedThresholdRunner<float> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_DOUBLE:
      {
      SurfaceBoundedConnectedThresholdRunner<double> runner;
      runner.Execute( info, pds );
      break; 
      }
    }
  }
  catch( itk::ExceptionObject & except )
  {
    info->SetProperty( info, VVP_ERROR, except.what() ); 
    return -1;
  }
  return 0;
}


static int UpdateGUI( void *inf )
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "Engrave Surface");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_CHECKBOX);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT, "0");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP, "Selects whether the surfaces should be engraved in the image values or not.");

  info->SetGUIProperty(info, 1, VVP_GUI_LABEL, "Segment Inside Region");
  info->SetGUIProperty(info, 1, VVP_GUI_TYPE, VVP_GUI_CHECKBOX);
  info->SetGUIProperty(info, 1, VVP_GUI_DEFAULT, "0");
  info->SetGUIProperty(info, 1, VVP_GUI_HELP, "Enabling this option will make the filter run a region growing segmenation bounded by the mask defined by the 3D markers.");

  info->SetGUIProperty(info, 2, VVP_GUI_LABEL, "Produce composite output");
  info->SetGUIProperty(info, 2, VVP_GUI_TYPE, VVP_GUI_CHECKBOX);
  info->SetGUIProperty(info, 2, VVP_GUI_DEFAULT, "0");
  info->SetGUIProperty(info, 2, VVP_GUI_HELP, "This filter produce by default a binary image as output. Enabling this option will instead generate a composite output combining the input image and the binary mask as an image of two components.");

  info->SetGUIProperty(info, 3, VVP_GUI_LABEL,"Upper Threshold");
  info->SetGUIProperty(info, 3, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 3, VVP_GUI_DEFAULT,  VolView::PlugIn::FilterModuleBase::GetInputVolumeScalarMaximum( info ) );
  info->SetGUIProperty(info, 3, VVP_GUI_HELP, "Upper threshold. Only pixels with intensities lower than this threshold will be considered to be added to the region.");
  info->SetGUIProperty(info, 3, VVP_GUI_HINTS , VolView::PlugIn::FilterModuleBase::GetInputVolumeScalarRange( info ) );

  info->SetGUIProperty(info, 4, VVP_GUI_LABEL,"Lower Threshold");
  info->SetGUIProperty(info, 4, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 4, VVP_GUI_DEFAULT,  VolView::PlugIn::FilterModuleBase::GetInputVolumeScalarMinimum( info ) );
  info->SetGUIProperty(info, 4, VVP_GUI_HELP, "Lower threshold. Only pixels with intensities higher than this threshold will be considered to be added to the region.");
  info->SetGUIProperty(info, 4, VVP_GUI_HINTS , VolView::PlugIn::FilterModuleBase::GetInputVolumeScalarRange( info ) );

  info->SetGUIProperty(info, 5, VVP_GUI_LABEL, "Replace Value");
  info->SetGUIProperty(info, 5, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 5, VVP_GUI_DEFAULT, "255");
  info->SetGUIProperty(info, 5, VVP_GUI_HELP, "Value to assign to the binary mask of the segmented region. The rest of the image will be set to zero.");
  info->SetGUIProperty(info, 5, VVP_GUI_HINTS , "1 255.0 1.0");

  info->SetGUIProperty(info, 6, VVP_GUI_LABEL, "Median filter radius");
  info->SetGUIProperty(info, 6, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 6, VVP_GUI_DEFAULT, "0");
  info->SetGUIProperty(info, 6, VVP_GUI_HELP, "Median filter radius. Neighborhood processed is (2r + 1). 0 Radius is equivalent to no filtering.");
  info->SetGUIProperty(info, 6, VVP_GUI_HINTS , "0 4.0 1.0");

  
  
  // Depending on this option the plugin will return the same input volume with
  // all pixels outside the curvilinear box set to zero.

  const char * gui0 = info->GetGUIProperty(info, 0, VVP_GUI_VALUE ); 
  const char * gui1 = info->GetGUIProperty(info, 1, VVP_GUI_VALUE ); 
  const char * gui2 = info->GetGUIProperty(info, 2, VVP_GUI_VALUE ); 

  const bool engraveSurface        = ( gui0  && bool ( atoi( gui0 ) ) );
  const bool doSegmentation        = ( gui1  && bool ( atoi( gui1 ) ) );
  const bool doCompositeOutput     = ( gui2  && bool ( atoi( gui2 ) ) );

  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,    "1"); 
  info->SetProperty(info, VVP_PRODUCES_MESH_ONLY,           "1");

  if( engraveSurface  && !doSegmentation ) 
    {
    info->OutputVolumeScalarType = info->InputVolumeScalarType;
    info->OutputVolumeNumberOfComponents = 1;
    info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,    "1"); // FIXME: It should depend on the input pixel type...
    info->SetProperty(info, VVP_PRODUCES_MESH_ONLY,           "0");

    // During the startup of the application this string is not yet defined.
    // We should then check for it before trying to use it.
    if( doCompositeOutput )
      {
      info->OutputVolumeScalarType = info->InputVolumeScalarType;
      info->OutputVolumeNumberOfComponents = 2;
      info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,    "2");
      }
    }
  else 
    {
    if( doSegmentation )
      {
      info->OutputVolumeScalarType = VTK_UNSIGNED_CHAR;
      info->OutputVolumeNumberOfComponents = 1;
      info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,    "1"); // FIXME: It should depend on the input pixel type...
      info->SetProperty(info, VVP_PRODUCES_MESH_ONLY,           "0");

      // During the startup of the application this string is not yet defined.
      // We should then check for it before trying to use it.
      if( doCompositeOutput )
        {
        info->OutputVolumeScalarType = info->InputVolumeScalarType;
        info->OutputVolumeNumberOfComponents = 2;
        info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,    "2");
        }
      }
    }

  memcpy(info->OutputVolumeDimensions,info->InputVolumeDimensions,
         3*sizeof(int));
  memcpy(info->OutputVolumeSpacing,info->InputVolumeSpacing,
         3*sizeof(float));
  memcpy(info->OutputVolumeOrigin,info->InputVolumeOrigin,
         3*sizeof(float));


  return 1;
}



extern "C" {
  
void VV_PLUGIN_EXPORT vvITKSurfaceBoundedConnectedThresholdInit(vtkVVPluginInfo *info)
{
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI   = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Surface Bounded Connected Threshold (ITK)");
  info->SetProperty(info, VVP_GROUP, "Segmentation - Region Growing");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
                                    "Computes region growing in a bounded region");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "This plugins computes a region growing segmentation limited to a curvilinear region defined by spline surfaces. The splines are defined with sets of 3D markers. Three groups of 3D markers are expected.");

  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "0");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "7");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,    "0");
  info->SetProperty(info, VVP_PRODUCES_MESH_ONLY,           "1");
  info->SetProperty(info, VVP_REQUIRES_SPLINE_SURFACES,     "1");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}

}

