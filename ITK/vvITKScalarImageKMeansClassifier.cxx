/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* perform a pixel-wise intensity transformation using a ScalarImageKMeansClassifier function */

#include "vvITKFilterModule.h"
#include "vnl/vnl_math.h"
#include "itkOutputWindow.h"
#include "itkMaskImageFilter.h"
#include "itkNumericTraits.h"
#include "itkImportImageFilter.h"
#include "itkScalarImageKmeansImageFilter.h"
#include "itkImageMaskSpatialObject.h"

// Simple class to compute intra class statistics
template <class TPixelType>
class StatisticsPair
{
typedef typename itk::NumericTraits< TPixelType >::RealType SumType;

public:
  unsigned long   pixels;
  SumType         sum;
  SumType         squaredSum;
  TPixelType      maximum;
  TPixelType      minimum;
public: 
  StatisticsPair()
  {
    this->pixels = 0L;
    this->sum    = itk::NumericTraits< SumType >::Zero;
    this->squaredSum    = itk::NumericTraits< SumType >::Zero;
  }
  void Clear()
  {
    this->pixels      = 0L;
    this->sum         = itk::NumericTraits< SumType >::Zero;
    this->squaredSum  = itk::NumericTraits< SumType >::Zero;
    this->minimum     = itk::NumericTraits< TPixelType >::max();
    this->maximum     = itk::NumericTraits< TPixelType >::NonpositiveMin();
  }
  void AddValue( const TPixelType & value )
  {
    this->pixels++;
    this->sum += static_cast< SumType >( value );
    this->squaredSum += static_cast< SumType >( value * value );
    if( value > maximum )
      {
      maximum = value;
      }
    if( value < minimum )
      {
      minimum = value;
      }
  }
};



template <class InputPixelType>
class ScalarImageKMeansClassifierRunner
  {
  public:
      itkStaticConstMacro( Dimension, unsigned int, 3 );
  
      typedef  InputPixelType                       PixelType;
      typedef  itk::Image< PixelType, 3 >           ImageType; 
      typedef  itk::ScalarImageKmeansImageFilter< 
                                                  ImageType  >    FilterType;
      typedef  VolView::PlugIn::FilterModule< FilterType >        ModuleType;
      typedef unsigned char                             MaskPixelType;
      typedef itk::Image< MaskPixelType, Dimension >    MaskImageType; 
      typedef itk::ImportImageFilter< MaskPixelType, 
                                  Dimension  >          MaskImportFilterType;
      typedef itk::ImageMaskSpatialObject< Dimension > ImageMaskSpatialObjectType;
      typedef typename ImageMaskSpatialObjectType::RegionType MaskRegionType;

  public:
    ScalarImageKMeansClassifierRunner() {}

    
    void Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds )
    {
      ModuleType  module;
      module.SetPluginInfo( info );
      module.SetUpdateMessage("Performing Classification with a K-Means algorithm");
      const unsigned int numberOfClasses  = atoi( info->GetGUIProperty(info, 0, VVP_GUI_VALUE ) );
      const unsigned int contiguousLabels = atoi( info->GetGUIProperty(info, 1, VVP_GUI_VALUE ) );
      const int useMask = atoi( info->GetGUIProperty( info, 2, VVP_GUI_VALUE));
      int outsideMaskValue = atoi( info->GetGUIProperty( info, 3, VVP_GUI_VALUE ));
   
      typedef unsigned char MaskPixelType;
      typedef itk::MaskImageFilter< ImageType, MaskImageType,
                                    ImageType >
                                    MaskFilterType;
      typename MaskFilterType::Pointer maskFilter;
      if( useMask )
        {
        maskFilter = MaskFilterType::New();

        // DO some crazy error checking
        if( info->InputVolume2ScalarType != VTK_UNSIGNED_CHAR )
          {
          info->SetProperty( info, VVP_ERROR,
            "The mask image must have pixel type unsigned char.");
          return;
          }
        if( info->InputVolume2NumberOfComponents != 1 )
          {
          info->SetProperty( info, VVP_ERROR, "The mask image must be single component.");
          return;
          }
        if( (info->InputVolume2Dimensions[0] != info->InputVolumeDimensions[0]) ||
         (info->InputVolume2Dimensions[1] != info->InputVolumeDimensions[1]) ||
         (info->InputVolume2Dimensions[2] != info->InputVolumeDimensions[2]) )
          {
          info->SetProperty( info, VVP_ERROR, 
              "The mask image must have the same dimensions as the input image.");
          return;
          }
          
        maskFilter = MaskFilterType::New();
        typename MaskImportFilterType::Pointer maskImportFilter
                                  = MaskImportFilterType::New();
        maskFilter->SetInput1( module.GetFilter()->GetInput() );

        // Get spacing, origin info etc.. use the same ones as the input image
        typename ImageType::SizeType   size;
        typename ImageType::IndexType  start;

        double     origin[Dimension];
        double     spacing[Dimension];

        size[0]     =  info->InputVolumeDimensions[0];
        size[1]     =  info->InputVolumeDimensions[1];
        size[2]     =  info->InputVolumeDimensions[2];

        for(unsigned int i=0; i<Dimension; i++)
          {
          origin[i]   =  info->InputVolumeOrigin[i];
          spacing[i]  =  info->InputVolumeSpacing[i];
          start[i]    =  0;
          }

        typename ImageType::RegionType region;

        region.SetIndex( start );
        region.SetSize(  size  );      
      
        maskImportFilter->SetSpacing( spacing );
        maskImportFilter->SetOrigin(  origin  );
        maskImportFilter->SetRegion(  region  );

        // Import
        const unsigned int totalNumberOfPixels = region.GetNumberOfPixels();
        const bool         importFilterWillDeleteTheInputBuffer = false;
        const unsigned int numberOfPixelsPerSlice = size[0] * size[1];

        MaskPixelType *   maskdataBlockStart = 
                            static_cast< MaskPixelType * >( pds->inData2 )  
                          + numberOfPixelsPerSlice * pds->StartSlice;
        maskImportFilter->SetImportPointer( maskdataBlockStart, 
                                        totalNumberOfPixels,
                                        importFilterWillDeleteTheInputBuffer );
        maskImportFilter->Update();
        maskFilter->SetInput2( maskImportFilter->GetOutput() );        
          
        
        // Make sure outsideMaskValue is within bounds
        if( outsideMaskValue > 
          itk::NumericTraits< PixelType >::max() )
          {
          outsideMaskValue = 
          itk::NumericTraits< PixelType >::max();
          }
        else if( outsideMaskValue < 
            itk::NumericTraits< PixelType >::min() )
          {
          outsideMaskValue = 
            itk::NumericTraits< PixelType >::min();
          }
        maskFilter->SetOutsideValue( outsideMaskValue );

        module.GetFilter()->SetInput( maskFilter->GetOutput() );
        

        // constrain computation of the classification statistics to the bounding
        // box of the mask region.
        typename ImageMaskSpatialObjectType::Pointer maskSpatialObject = 
                ImageMaskSpatialObjectType::New();
        maskSpatialObject->SetImage( maskImportFilter->GetOutput() );
        this->m_ClassificationRegion = maskSpatialObject->GetAxisAlignedBoundingBoxRegion();
        module.GetFilter()->SetImageRegion( this->m_ClassificationRegion );
        }
      
      
      module.GetFilter()->SetUseNonContiguousLabels( !contiguousLabels );

      // If we have a mask, we will add a class with mean of the outside value
      // 
      if (useMask)
        {
        for(unsigned int k=0; k<numberOfClasses-1; k++)
          {
          module.GetFilter()->AddClassWithInitialMean( 0.0 );
          }
        module.GetFilter()->AddClassWithInitialMean( outsideMaskValue );
        }
      else
        {
        for(unsigned int k=0; k<numberOfClasses; k++)
          {
          module.GetFilter()->AddClassWithInitialMean( 0.0 );
          }
        }

      // Execute the filter
      module.ProcessData( pds  );

      
      // Compute intra-class statistics..
      // And write intra-class statistics to a file 
      // And display intra-class statistics in a text window
      typename FilterType::OutputImageType::ConstPointer outputImage = 
                                                module.GetFilter()->GetOutput();

      typedef itk::ImageRegionConstIterator< typename FilterType::OutputImageType >  OutputIteratorType;
      if( !useMask )
        {
        this->m_ClassificationRegion = outputImage->GetBufferedRegion();
        }
      OutputIteratorType ot( outputImage, this->m_ClassificationRegion );

      
      typedef StatisticsPair< typename FilterType::OutputPixelType > StatisticsPairType;
      typedef std::map< typename FilterType::OutputPixelType, StatisticsPairType > HistogramType;
      HistogramType histogram;

      typename HistogramType::iterator itr = histogram.begin();
      while( itr != histogram.end() )
        {
        itr->second.Clear();
        ++itr;
        }

      typename FilterType::InputImageType::ConstPointer 
                      image = module.GetFilter()->GetInput();
      typedef itk::ImageRegionConstIterator< typename FilterType::InputImageType >  InputIteratorType;
      if( !useMask )
        {
        this->m_ClassificationRegion = image->GetBufferedRegion();
        }
      InputIteratorType it( image, this->m_ClassificationRegion );
      it.GoToBegin();

      ot.GoToBegin(); 
      while( !ot.IsAtEnd() )
        {
        const typename FilterType::OutputPixelType pixelValue = ot.Get();
        histogram[pixelValue].AddValue( it.Get() );
        ++ot;
        ++it;
        }
      
      std::ostringstream      FinalStatisticsLog;
      FinalStatisticsLog << "Class\tPixels\tMean\tStdDev\tMin\tMax" << std::endl;

      typename HistogramType::const_iterator citr = histogram.begin();
      while( citr != histogram.end() )
        {
        const unsigned long count = citr->second.pixels;
        const float density       = citr->second.sum / (float)count;
        const float stddev        = vcl_sqrt((citr->second.squaredSum / 
              static_cast< double >(count)) - (density * density) );
        FinalStatisticsLog << static_cast< float >(citr->first) << 
          "\t" << count << "\t" << density << "\t" << 
          stddev << "\t" << static_cast<float>(citr->second.minimum) << "\t" <<
          static_cast<float>(citr->second.maximum) << std::endl;
        ++citr;
        }
      FinalStatisticsLog << std::ends;

      // Display and write statistics log to a file
      itk::OutputWindow::Pointer outputWnd = itk::OutputWindow::New();
      itk::OutputWindow::SetInstance( outputWnd );
      outputWnd->DisplayText( FinalStatisticsLog.str().c_str() );
      std::ofstream finalStatisticsLog( 
          "ScalarKMeansClassifierStatistics.txt", std::ios::trunc);
      finalStatisticsLog << FinalStatisticsLog.str() << std::endl;
      finalStatisticsLog.close();
    
    }


  private:
    MaskRegionType m_ClassificationRegion;
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
      ScalarImageKMeansClassifierRunner<signed char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_CHAR:
      {
      ScalarImageKMeansClassifierRunner<unsigned char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_SHORT:
      {
      ScalarImageKMeansClassifierRunner<signed short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_SHORT:
      {
      ScalarImageKMeansClassifierRunner<unsigned short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_INT:
      {
      ScalarImageKMeansClassifierRunner<signed int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_INT:
      {
      ScalarImageKMeansClassifierRunner<unsigned int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_LONG:
      {
      ScalarImageKMeansClassifierRunner<signed long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_LONG:
      {
      ScalarImageKMeansClassifierRunner<unsigned long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_FLOAT:
      {
      ScalarImageKMeansClassifierRunner<float> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_DOUBLE:
      {
      ScalarImageKMeansClassifierRunner<double> runner;
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

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "Number of Classes");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT, "4");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP, "Number of classes to be used for the K-Means classification. The number of classes is a critical decision in this algorithm, you want to make sure that you add at least one class more to the expected number of classes.");
  info->SetGUIProperty(info, 0, VVP_GUI_HINTS , "1 20 1");

info->SetGUIProperty(info, 1, VVP_GUI_LABEL, "Use Contiguous Labels");
  info->SetGUIProperty(info, 1, VVP_GUI_TYPE, VVP_GUI_CHECKBOX);
  info->SetGUIProperty(info, 1, VVP_GUI_DEFAULT, "0");
  info->SetGUIProperty(info, 1, VVP_GUI_HELP, "This filter produces a labeled image. In order to facilitate visualization, the labels assigned to pixels are distributed across the dynamic range of the pixels. For some applications, however, it is preferable to have contiguous label values starting from zero. When you this checkbox is on the output image will have contiguous labels, and you will have to adjust the visualization parameters in order to see the content of the image.");

  info->SetGUIProperty(info, 2, VVP_GUI_LABEL, "Use Mask Input");
  info->SetGUIProperty(info, 2, VVP_GUI_TYPE, VVP_GUI_CHECKBOX);
  info->SetGUIProperty(info, 2, VVP_GUI_DEFAULT, "0" );
  info->SetGUIProperty(info, 2, VVP_GUI_HELP, "Optionally a mask input given as a second input to mask out segments of the input volume. The mask must be of the same dimensions as the input and have zero in the regions that are to be discarded.");
  
  info->SetGUIProperty(info, 3, VVP_GUI_LABEL, "Mask outside value");
  info->SetGUIProperty(info, 3, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 3, VVP_GUI_DEFAULT, "255" );
  info->SetGUIProperty(info, 3, VVP_GUI_HELP, "Outside value that the masking opertaion assigns to pixels that are being discarded. This value must be a value that is very different from the rest of the image so that KMeans or MRF filtering does not get affected.");
  info->SetGUIProperty(info, 3, VVP_GUI_HINTS, "0.0 10000.0 1");


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
  
void VV_PLUGIN_EXPORT vvITKScalarImageKMeansClassifierInit(vtkVVPluginInfo *info)
{
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI   = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Scalar Image K-Means");
  info->SetProperty(info, VVP_GROUP, "Segmentation - Statistics");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
    "Performs classification of a Scalar image using the K-Means algorithm");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "This filters performs statistical classification in a scalar image by applying the K-Means algortihm. The use must provide a number of classes in order to initialize the classification. The output is a labeled image encoded in 8 bits. It is assumed that no more than 256 class will be expected as output. Optionally, a mask image may be specified to mask out a certain region and run Kmeans on the smallest rectangular bounding box encapsulating the mask. The mask is expected to be a file with pixel type UnsignedChar and have non-zero value in the foreground. Note that the regions masked away (discarded) also constitute a class.");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "0");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "4");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,    "0"); 
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
  info->SetProperty(info, VVP_REQUIRES_SECOND_INPUT,        "1");
  info->SetProperty(info, VVP_SECOND_INPUT_OPTIONAL,        "1");
}

}

