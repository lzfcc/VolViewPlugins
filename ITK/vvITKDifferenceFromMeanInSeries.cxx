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
class DifferenceFromMeanInSeriesRunner
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
      
      typedef itk::ImageRegionConstIterator< 
                                         SingleImageType > RegionIterator3DType;
      
      typedef itk::ImageRegionConstIterator< 
                                         SeriesImageType > RegionIterator4DType;
      
      typedef typename itk::NumericTraits< PixelType >::AccumulateType  PixelAccumulateType;

  public:
    DifferenceFromMeanInSeriesRunner() {}
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
      
      IteratorType it( series, region );
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

      const PixelType middle = static_cast< PixelType >( (
            itk::NumericTraits< PixelType >::max() 
          + itk::NumericTraits< PixelType >::NonpositiveMin() ) / 2.0 );

      PixelType * outDataSeries = static_cast< PixelType * >( pds->outDataSeries );

       if( !outDataSeries )
        {
        info->SetProperty( info, VVP_ERROR, "This plugin produces a volume series as output but the current receiving pointer is NULL." ); 
        return;
        }

     counter = 0;
      numberOfPixelsProcessed = 0;
      numberOfPixelsPerProgressUpdate = int( totalNumberOfPixels / 100.0 );
      
      RegionIterator4DType rit( series, region );
      rit.GoToBegin();

      unsigned int numberOfPixelInOneVolume = 
        totalNumberOfPixels / info->OutputVolumeSeriesNumberOfVolumes;

      for(int nv=0; nv < info->OutputVolumeSeriesNumberOfVolumes; nv++)
        {
        outData = static_cast< PixelType * >( pds->outData );
        for(int np=0; np < numberOfPixelInOneVolume; np++)
          {
          *outDataSeries = static_cast< PixelType >( rit.Get() - *outData + middle);
          ++outDataSeries;
          ++outData;
          ++rit;
          ++counter;
          if( counter >= numberOfPixelsPerProgressUpdate )
            {
            numberOfPixelsProcessed += numberOfPixelsPerProgressUpdate;
            float progress = float(numberOfPixelsProcessed)/float(totalNumberOfPixels);
            info->UpdateProgress(info, progress, "Computing the Differences");
            counter = 0;
            }
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
      DifferenceFromMeanInSeriesRunner<signed char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_CHAR:
      {
      DifferenceFromMeanInSeriesRunner<unsigned char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_SHORT:
      {
      DifferenceFromMeanInSeriesRunner<signed short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_SHORT:
      {
      DifferenceFromMeanInSeriesRunner<unsigned short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_INT:
      {
      DifferenceFromMeanInSeriesRunner<signed int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_INT:
      {
      DifferenceFromMeanInSeriesRunner<unsigned int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_LONG:
      {
      DifferenceFromMeanInSeriesRunner<signed long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_LONG:
      {
      DifferenceFromMeanInSeriesRunner<unsigned long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_FLOAT:
      {
      DifferenceFromMeanInSeriesRunner<float> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_DOUBLE:
      {
      DifferenceFromMeanInSeriesRunner<double> runner;
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
  
  // Setting up information for the output Volume
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

  // Setting up information for the output Series.
  info->OutputVolumeSeriesNumberOfVolumes =
                       info->InputVolumeSeriesNumberOfVolumes;
  info->OutputVolumeSeriesScalarType = 
                       info->InputVolumeSeriesScalarType;
  info->OutputVolumeSeriesNumberOfComponents = 
                       info->InputVolumeSeriesNumberOfComponents;
  memcpy(info->OutputVolumeSeriesDimensions,info->InputVolumeSeriesDimensions,
         3*sizeof(int));
  memcpy(info->OutputVolumeSeriesSpacing,info->InputVolumeSeriesSpacing,
         3*sizeof(float));
  memcpy(info->OutputVolumeSeriesOrigin,info->InputVolumeSeriesOrigin,
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
  
void VV_PLUGIN_EXPORT vvITKDifferenceFromMeanInSeriesInit(vtkVVPluginInfo *info)
{
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI   = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Difference from Series Mean (ITK)");
  info->SetProperty(info, VVP_GROUP, "Series");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
                                "Compute the differences from the Series mean");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "This filters computes the pixel-wise difference between the value of one pixel and the mean of that same pixel through the series. The input volumes are passed in the form of a series. The output image has dimension+1 of the input images.");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "0");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "0");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,    "0"); 
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "1");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "1");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}

}
