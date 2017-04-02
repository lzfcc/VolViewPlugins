/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* perform an intensity transformation by computing 
   the median value of a neighborhood  */

#include "vvITKFilterModule.h"

#include "itkVotingBinaryHoleFillingImageFilter.h"




template <class InputPixelType>
class VotingBinaryHoleFillingRunner
  {
  public:
      typedef  InputPixelType                       PixelType;
      typedef  itk::Image< PixelType, 3 >           ImageType; 
      typedef  itk::VotingBinaryHoleFillingImageFilter< ImageType,  ImageType >   FilterType;
      typedef  VolView::PlugIn::FilterModule< FilterType >       ModuleType;

  public:
    VotingBinaryHoleFillingRunner() {}
    void Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds )
    {
      itk::Size< 3 > radius;
      radius[0] = atoi( info->GetGUIProperty(info, 0, VVP_GUI_VALUE ) );
      radius[1] = atoi( info->GetGUIProperty(info, 1, VVP_GUI_VALUE ) );
      radius[2] = atoi( info->GetGUIProperty(info, 2, VVP_GUI_VALUE ) );

      const int backgroundValue = atoi( info->GetGUIProperty(info, 3, VVP_GUI_VALUE ) );
      const int foregroundValue = atoi( info->GetGUIProperty(info, 4, VVP_GUI_VALUE ) );
      const int majorityValue   = atoi( info->GetGUIProperty(info, 5, VVP_GUI_VALUE ) );

      ModuleType  module;
      module.SetPluginInfo( info );
      module.SetUpdateMessage("Transforming intensities with a VotingBinary filter...");
      module.GetFilter()->SetRadius( radius );
      module.GetFilter()->SetBackgroundValue( backgroundValue );
      module.GetFilter()->SetForegroundValue( foregroundValue );
      module.GetFilter()->SetMajorityThreshold( majorityValue );

      // Execute the filter
      module.ProcessData( pds  );

      char tmp[1024];
      sprintf(tmp,"Number of pixels changed = %d",
              module.GetFilter()->GetNumberOfPixelsChanged());
      info->SetProperty( info, VVP_REPORT_TEXT, tmp );
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
      VotingBinaryHoleFillingRunner<signed char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_CHAR:
      {
      VotingBinaryHoleFillingRunner<unsigned char> runner;
      runner.Execute( info, pds );
      break; 
      }
    default:
      info->SetProperty( info, VVP_ERROR, "This filter is intended for 8 bits binary images only" ); 
      return -1;
    }

  }
  catch( itk::ExceptionObject & except )
  {
    info->SetProperty( info, VVP_ERROR, except.what() ); 
    return -1;
  }
  return 0;
}


static int UpdateGUI(void *inf)
{
  char tmp[1024];
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "Radius X");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT, "2");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP, "Integer radius along the X axis of the neighborhood used to compute the VotingBinary. The neighborhood is a rectangular region that extends this number of pixels around the pixel being computed. Setting a radius of 2 will use a neighborhood of size 5.");
  info->SetGUIProperty(info, 0, VVP_GUI_HINTS , "1 5 1");

  info->SetGUIProperty(info, 1, VVP_GUI_LABEL, "Radius Y");
  info->SetGUIProperty(info, 1, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 1, VVP_GUI_DEFAULT, "2");
  info->SetGUIProperty(info, 1, VVP_GUI_HELP, "Integer radius along the Y axis of the neighborhood used to compute the VotingBinary. The neighborhood is a rectangular region that extends this number of pixels around the pixel being computed. Setting a radius of 2 will use a neighborhood of size 5.");
  info->SetGUIProperty(info, 1, VVP_GUI_HINTS , "1 5 1");

  info->SetGUIProperty(info, 2, VVP_GUI_LABEL, "Radius Z");
  info->SetGUIProperty(info, 2, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 2, VVP_GUI_DEFAULT, "2");
  info->SetGUIProperty(info, 2, VVP_GUI_HELP, "Integer radius along the Z axis of the neighborhood used to compute the VotingBinary. The neighborhood is a rectangular region that extends this number of pixels around the pixel being computed. Setting a radius of 2 will use a neighborhood of size 5.");
  info->SetGUIProperty(info, 2, VVP_GUI_HINTS , "1 5 1");

  info->SetGUIProperty(info, 3, VVP_GUI_LABEL, "Background value");
  info->SetGUIProperty(info, 3, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 3, VVP_GUI_DEFAULT, VolView::PlugIn::FilterModuleBase::GetInputVolumeScalarMinimum( info ) );
  info->SetGUIProperty(info, 3, VVP_GUI_HELP, "Value associated to OFF pixels.");
  info->SetGUIProperty(info, 3, VVP_GUI_HINTS, VolView::PlugIn::FilterModuleBase::GetInputVolumeScalarRange( info ) );

  info->SetGUIProperty(info, 4, VVP_GUI_LABEL, "Foreground value");
  info->SetGUIProperty(info, 4, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 4, VVP_GUI_DEFAULT, VolView::PlugIn::FilterModuleBase::GetInputVolumeScalarMaximum( info ) );
  info->SetGUIProperty(info, 4, VVP_GUI_HELP, "Value associated to ON pixels.");
  info->SetGUIProperty(info, 4, VVP_GUI_HINTS, VolView::PlugIn::FilterModuleBase::GetInputVolumeScalarRange( info ) );

  info->SetGUIProperty(info, 5, VVP_GUI_LABEL, "Majority threshold");
  info->SetGUIProperty(info, 5, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 5, VVP_GUI_DEFAULT, "1" );
  info->SetGUIProperty(info, 5, VVP_GUI_HELP, "Number of neighbor pixels over 50% that must be ON in order to switch and OFF pixel to ON.");
  info->SetGUIProperty(info, 5, VVP_GUI_HINTS, "0 400 1" );


  const char * text = info->GetGUIProperty(info,2,VVP_GUI_VALUE);
  if( text )
    {
    sprintf(tmp,"%d", atoi( text ) ); 
    info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP, tmp);
    }
  else
    {
    info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP, "0");
    }
  
  
  info->OutputVolumeScalarType = info->InputVolumeScalarType;
  info->OutputVolumeNumberOfComponents = 
    info->InputVolumeNumberOfComponents;
  memcpy(info->OutputVolumeDimensions,info->InputVolumeDimensions,
         3*sizeof(int));
  memcpy(info->OutputVolumeSpacing,info->InputVolumeSpacing,
         3*sizeof(float));
  memcpy(info->OutputVolumeOrigin,info->InputVolumeOrigin,
         3*sizeof(float));

  // if multi component we have 1 scalar for input and 1 scalar for output
  if (info->InputVolumeNumberOfComponents > 1)
    {
    int sizeReq = 2*info->InputVolumeScalarSize;
    char tmps[500];
    sprintf(tmps,"%i",sizeReq);
    info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED, tmps); 
    }
  else
    {
    // otherwise no memory is required
    info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED, "0"); 
    }

  return 1;
}


extern "C" {
  
void VV_PLUGIN_EXPORT vvITKVotingBinaryHoleFillingInit(vtkVVPluginInfo *info)
{
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI   = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Binary Hole Filling (ITK)");
  info->SetProperty(info, VVP_GROUP, "Contour Evolution");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
          "Fills in holes and cavities using a Voting scheme");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "This filter is intended for binary images. It will evolve a contour by applying voting rules in such a way that holes and cavities are filled in progressively.");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "1");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "6");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,    "0");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}

}

