/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* This plugin adapts the MarkovRandomField filter.
   It does not perform any preprocessing. The user should provide
   the speed image and the initial level set as inputs. */

#include "vvITKMarkovRandomField.h"



template <class InputPixelType>
class MarkovRandomFieldeRunner
  {
  public:
    typedef VolView::PlugIn::MarkovRandomField< InputPixelType >   ModuleType;
    
  public:
    MarkovRandomFieldeRunner() {}
    void Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds )
    {
      char tmp[1024];

      ModuleType  module;
      module.SetPluginInfo( info );
      module.SetUpdateMessage("Computing Markov Random Field...");
      module.ProcessData( pds  );
      info->SetProperty( info, VVP_REPORT_TEXT, tmp );

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

  try 
  {
  switch( info->InputVolumeScalarType )
    {
    case VTK_CHAR:
      {
      MarkovRandomFieldeRunner<signed char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_CHAR:
      {
      MarkovRandomFieldeRunner<unsigned char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_SHORT:
      {
      MarkovRandomFieldeRunner<signed short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_SHORT:
      {
      MarkovRandomFieldeRunner<unsigned short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_INT:
      {
      MarkovRandomFieldeRunner<signed int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_INT:
      {
      MarkovRandomFieldeRunner<unsigned int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_LONG:
      {
      MarkovRandomFieldeRunner<signed long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_LONG:
      {
      MarkovRandomFieldeRunner<unsigned long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_FLOAT:
      {
      MarkovRandomFieldeRunner<float> runner;
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

  info->UpdateProgress( info, 1.0, "Segmentation Threshold LevelSet Done !");

  return 0;
}


static int UpdateGUI(void *inf)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "Neigborhood Radius");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT, "2");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP, "Integer radius of the neighborhood used to compute the update of every pixel. The neighborhood is a rectangular region that extends this number of pixels around the pixel being computed. Setting a radius of 2 will use a neighborhood of size 5x5x5.");
  info->SetGUIProperty(info, 0, VVP_GUI_HINTS , "1 5 1");

  info->SetGUIProperty(info, 1, VVP_GUI_LABEL, "Number of Classes");
  info->SetGUIProperty(info, 1, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 1, VVP_GUI_DEFAULT, "4");
  info->SetGUIProperty(info, 1, VVP_GUI_HELP, "Number of classes to be used for the Markov Random Field computation. The number of classes is a critical decision in this algorithm, you want to make sure that you add at least one class more than the expected number of classes.");
  info->SetGUIProperty(info, 1, VVP_GUI_HINTS , "1 20 1");

  info->SetGUIProperty(info, 2, VVP_GUI_LABEL, "Maximum number of Iterations");
  info->SetGUIProperty(info, 2, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 2, VVP_GUI_DEFAULT, "100");
  info->SetGUIProperty(info, 2, VVP_GUI_HELP, "The filter will stop if it reaches this maximum number of iterations, or if it goes through an iteration without changing the label of any pixel. Should you need more iteration you can always rerun this filter.");
  info->SetGUIProperty(info, 2, VVP_GUI_HINTS , "1 500 1");

  info->SetGUIProperty(info, 3, VVP_GUI_LABEL, "Smoothing Factor");
  info->SetGUIProperty(info, 3, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 3, VVP_GUI_DEFAULT, "1.0" );
  info->SetGUIProperty(info, 3, VVP_GUI_HELP, "Degree of smoothness on the classified regions. The higher this number is, the more homogeneous will be the classified regions.");
  info->SetGUIProperty(info, 3, VVP_GUI_HINTS, "0.0 100.0 1.0");

  info->SetGUIProperty(info, 4, VVP_GUI_LABEL, "Error Tolerance");
  info->SetGUIProperty(info, 4, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 4, VVP_GUI_DEFAULT, "0.0001" );
  info->SetGUIProperty(info, 4, VVP_GUI_HELP, "This tolerance value is used as a threshold for defining a stopping criterion.");
  info->SetGUIProperty(info, 4, VVP_GUI_HINTS, "0.0 1.0 0.01");


  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP, "0");
  
  info->OutputVolumeScalarType = VTK_UNSIGNED_CHAR; 

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
  
void VV_PLUGIN_EXPORT vvITKMarkovRandomFieldInit(vtkVVPluginInfo *info)
{
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI   = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Markov Random Field (ITK)");
  info->SetProperty(info, VVP_GROUP, "Segmentation - Statistics");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
          "Refines a segmentation using a Random Markov Field computatoin.");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "This plugin take a labeled image and refines the label assignemts by taking into account the spatial coherence of the image. This is done running a Markov Random Field computation on the labeled image. This filter expects two inputs, the first one is a labeled image, the second one is the grayscale image from which the initial labeling was generated.");

  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "0");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "5");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,   "16");
  info->SetProperty(info, VVP_REQUIRES_SECOND_INPUT,        "1");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}

}
