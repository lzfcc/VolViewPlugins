/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* Computes pixel-wise the Mean value of the image in the Series. */

#include "vvITKFilterModuleSeriesInput.h"

#include "itkImageLinearConstIteratorWithIndex.h"



template <class TInputPixelType>
class MeanFromSeriesRunner
  {
  public:
      typedef  TInputPixelType                     PixelType;
      typedef  itk::Image< PixelType, 3 >          SingleImageType; 
      typedef  itk::Image< PixelType, 4 >          SeriesImageType; 

      typedef  VolView::PlugIn::FilterModuleSeriesInput< 
                                                  SingleImageType, 
                                                  SeriesImageType >   ModuleType;

      typedef typename SeriesImageType::ConstPointer   SeriesImagePointer;

      typedef itk::ImageLinearConstIteratorWithIndex< 
                                         SeriesImageType > IteratorType;
      
      typedef typename itk::NumericTraits< PixelType >::AccumulateType  PixelAccumulateType;

  public:
    MeanFromSeriesRunner() {}
    void Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds )
    {
      ModuleType  module;
      module.SetPluginInfo( info );
      module.SetUpdateMessage("Computing the Mean value of the Series...");
      // Set the parameters on it
      // Execute the filter
      module.ProcessData( pds  );
      SeriesImagePointer series = module.GetInput2();
      
      typename SeriesImageType::RegionType region = series->GetBufferedRegion();

      unsigned int numberOfVolumesInSeries = region.GetSize()[3];
      unsigned int totalNumberOfPixels     = region.GetNumberOfPixels();
        
      PixelType * outData = static_cast< PixelType * >( pds->outData );

      unsigned int counter = 0;
      unsigned int numberOfPixelsProcessed = 0;
      unsigned int numberOfPixelsPerProgressUpdate = int( totalNumberOfPixels / 100.0 );
      
      IteratorType it( series, series->GetBufferedRegion() );
      it.SetDirection(3); // walk faster along the 4D dimension.
      it.GoToBegin();
      while( !it.IsAtEnd() )
        {
        PixelAccumulateType sum = itk::NumericTraits< PixelAccumulateType >::Zero;
        while( !it.IsAtEndOfLine() )
          {
          sum += it.Get();
          ++it;
          }
        *outData = static_cast< PixelType >( sum / numberOfVolumesInSeries );
        outData++;
        it.NextLine();
        counter += numberOfVolumesInSeries;
        numberOfPixelsProcessed += numberOfVolumesInSeries;
        if( counter >= numberOfPixelsPerProgressUpdate )
          {
          float progress = float(numberOfPixelsProcessed)/float(totalNumberOfPixels);
          info->UpdateProgress(info, progress, "Computing the Mean");
          counter = 0;
          }
        }
    }
  };




static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{

  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  try 
  {
  switch( info->InputVolumeSeriesScalarType )
  {
    case VTK_CHAR:
      {
      MeanFromSeriesRunner<signed char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_CHAR:
      {
      MeanFromSeriesRunner<unsigned char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_SHORT:
      {
      MeanFromSeriesRunner<signed short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_SHORT:
      {
      MeanFromSeriesRunner<unsigned short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_INT:
      {
      MeanFromSeriesRunner<signed int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_INT:
      {
      MeanFromSeriesRunner<unsigned int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_LONG:
      {
      MeanFromSeriesRunner<signed long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_LONG:
      {
      MeanFromSeriesRunner<unsigned long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_FLOAT:
      {
      MeanFromSeriesRunner<float> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_DOUBLE:
      {
      MeanFromSeriesRunner<double> runner;
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


static int UpdateGUI(void *inf)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP, "0");
  
  info->OutputVolumeScalarType = 
                       info->InputVolumeSeriesScalarType;
  info->OutputVolumeNumberOfComponents = 
                       info->InputVolumeSeriesNumberOfComponents;
  memcpy(info->OutputVolumeDimensions,info->InputVolumeSeriesDimensions,
         3*sizeof(int));
  memcpy(info->OutputVolumeSpacing,info->InputVolumeSeriesSpacing,
         3*sizeof(float));
  memcpy(info->OutputVolumeOrigin,info->InputVolumeSeriesOrigin,
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
  
void VV_PLUGIN_EXPORT vvITKMeanFromSeriesInit(vtkVVPluginInfo *info)
{
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI   = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Mean from Series (ITK)");
  info->SetProperty(info, VVP_GROUP, "Series");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
                                "Compute the Mean from a Series");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "This filters computes the pixel-wise Mean from a set of volumes passed in the form of a series. The output images has the same dimension as any of the input images as well as the same pixel type.");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "0");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "0");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,    "0"); 
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "1");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}

}
