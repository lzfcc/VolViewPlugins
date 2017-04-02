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

#include "vvITKScalarKmeansMarkovRandomField.h"



template <class InputPixelType>
class ScalarKmeansMarkovRandomFieldeRunner
  {
  public:
    typedef VolView::PlugIn::ScalarKmeansMarkovRandomField< InputPixelType >   ModuleType;
    
  public:
    ScalarKmeansMarkovRandomFieldeRunner() {}
    void Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds )
    {
      ModuleType  module;
      module.SetPluginInfo( info );
      module.SetUpdateMessage("Computing Markov Random Field...");
      module.ProcessData( pds  );
      const ::itk::OStringStream & statisticsStream = module.GetStatistics();
      info->SetProperty( info, VVP_REPORT_TEXT, statisticsStream.str().c_str() );
    }


  protected:

    // Progress update for the MRF filter
    void ProgressUpdate( itk::ProcessObject * caller, const itk::EventObject & event )
      {
      if( itk::ProgressEvent().CheckEvent(& event ) )
        {
        this->info->UpdateProgress( this->info, caller->GetProgress(), NULL );
        }
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
      ScalarKmeansMarkovRandomFieldeRunner<signed char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_CHAR:
      {
      ScalarKmeansMarkovRandomFieldeRunner<unsigned char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_SHORT:
      {
      ScalarKmeansMarkovRandomFieldeRunner<signed short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_SHORT:
      {
      ScalarKmeansMarkovRandomFieldeRunner<unsigned short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_INT:
      {
      ScalarKmeansMarkovRandomFieldeRunner<signed int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_INT:
      {
      ScalarKmeansMarkovRandomFieldeRunner<unsigned int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_LONG:
      {
      ScalarKmeansMarkovRandomFieldeRunner<signed long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_LONG:
      {
      ScalarKmeansMarkovRandomFieldeRunner<unsigned long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_FLOAT:
      {
      ScalarKmeansMarkovRandomFieldeRunner<float> runner;
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

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "Number of Classes");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT, "4");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP, "Number of classes to be used for the Markov Random Field computation. The number of classes is a critical decision in this algorithm, you want to make sure that you add at least one class more than the expected number of classes.");
  info->SetGUIProperty(info, 0, VVP_GUI_HINTS , "1 20 1");

  info->SetGUIProperty(info, 1, VVP_GUI_LABEL, "Smoothing Factor");
  info->SetGUIProperty(info, 1, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 1, VVP_GUI_DEFAULT, "1.0" );
  info->SetGUIProperty(info, 1, VVP_GUI_HELP, "Degree of smoothness on the classified regions. The higher this number is, the more homogeneous will be the classified regions.");
  info->SetGUIProperty(info, 1, VVP_GUI_HINTS, "1.0 5.0 1.0");

  info->SetGUIProperty(info, 2, VVP_GUI_LABEL, "Maximum number of Iterations");
  info->SetGUIProperty(info, 2, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 2, VVP_GUI_DEFAULT, "100");
  info->SetGUIProperty(info, 2, VVP_GUI_HELP, "The filter will stop if it reaches this maximum number of iterations, or if it goes through an iteration without changing the label of any pixel. Should you need more iteration you can always rerun this filter.");
  info->SetGUIProperty(info, 2, VVP_GUI_HINTS , "1 500 1");

  info->SetGUIProperty(info, 3, VVP_GUI_LABEL, "Error Tolerance");
  info->SetGUIProperty(info, 3, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 3, VVP_GUI_DEFAULT, "0.0001" );
  info->SetGUIProperty(info, 3, VVP_GUI_HELP, "This tolerance value is used as a threshold for defining a stopping criterion.");
  info->SetGUIProperty(info, 3, VVP_GUI_HINTS, "0.0 1.0 0.01");

  info->SetGUIProperty(info, 4, VVP_GUI_LABEL, "Use Mask Input");
  info->SetGUIProperty(info, 4, VVP_GUI_TYPE, VVP_GUI_CHECKBOX);
  info->SetGUIProperty(info, 4, VVP_GUI_DEFAULT, "0" );
  info->SetGUIProperty(info, 4, VVP_GUI_HELP, "Optionally a mask input given as a second input to mask out segments of the input volume. The mask must be of the same dimensions as the input and have zero in the regions that are to be discarded.");
  
  info->SetGUIProperty(info, 5, VVP_GUI_LABEL, "Mask outside value");
  info->SetGUIProperty(info, 5, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 5, VVP_GUI_DEFAULT, "255" );
  info->SetGUIProperty(info, 5, VVP_GUI_HELP, "Outside value that the masking opertaion assigns to pixels that are being discarded. This value must be a value that is very different from the rest of the image so that KMeans or MRF filtering does not get affected.");
  info->SetGUIProperty(info, 5, VVP_GUI_HINTS, "0.0 10000.0 1");



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
  
void VV_PLUGIN_EXPORT vvITKScalarKmeansMarkovRandomFieldInit(vtkVVPluginInfo *info)
{
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI   = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "K-Means + Markov Random Field (ITK)");
  info->SetProperty(info, VVP_GROUP, "Segmentation - Statistics");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
          "Runs a K-Means classification followed by a Random Markov Field refinement.");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION, "This plugin performs a classification using a K-Means algorithms and then further refines the classification by taking spatial coherence into account. The refinement is done with a Markov Random Field algorithm. Optionally, a mask image may be specified to mask out a certain region and run Kmeans on the smallest rectangular bounding box encapsulating the mask. The mask is expected to be a file with pixel type UnsignedChar and have non-zero value in the foreground. Note that the regions masked away (discarded) also constitute a class.");

  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "0");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "6");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,   "16");
  info->SetProperty(info, VVP_REQUIRES_SECOND_INPUT,        "1");
  info->SetProperty(info, VVP_SECOND_INPUT_OPTIONAL,        "1");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}

}
