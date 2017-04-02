/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/** This module performs first a classification of an image using the KMeans
 * algorithm and then refines the classification by applying iterations of a
 * MarkovRandomField algorithms. */
    

#ifndef _vvITKScalarKmeansMarkovFieldToPaintbrush_h
#define _vvITKScalarKmeansMarkovFieldToPaintbrush_h

#include "vvITKFilterModuleTwoInputs.h"

#include "itkImage.h"
#include "itkFixedArray.h"
#include "itkScalarToArrayCastImageFilter.h"
#include "itkMRFImageFilter.h"
#include "itkDistanceToCentroidMembershipFunction.h"
#include "itkMinimumDecisionRule.h"
#include "itkImageClassifierBase.h"
#include "itkImageKmeansModelEstimator.h"
#include "vnl/vnl_math.h"
#include "itkOutputWindow.h"
#include "itkMaskImageFilter.h"
#include "itkNumericTraits.h"
#include "itkImageRegion.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkImageMaskSpatialObject.h"
#include "itkPasteImageFilter.h"
#include <algorithm>


namespace VolView
{

namespace PlugIn
{


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


template< class TInputPixelType, class TLabelPixelType >
class ScalarKmeansMarkovFieldToPaintbrush : public FilterModuleBase {

public:

  itkStaticConstMacro( Dimension, unsigned int, 3 );

  typedef TInputPixelType                             InputPixelType;
  typedef TLabelPixelType                             LabelPixelType;
  typedef itk::Image<LabelPixelType,Dimension>        LabelImageType;
  typedef itk::Image<InputPixelType,Dimension>        InputImageType;
  typedef itk::Vector<InputPixelType,1>               ArrayPixelType;
  typedef itk::Image< ArrayPixelType, Dimension >     ArrayImageType;
  typedef unsigned char                               MaskPixelType;
  typedef typename InputImageType::RegionType         ImageRegionType;
   
  typedef itk::RegionOfInterestImageFilter< 
      InputImageType, InputImageType >                RegionOfInterestFilterType;
    
  typedef itk::MRFImageFilter< ArrayImageType,
                               LabelImageType >       MRFFilterType;

  typedef FilterModuleBase                            Superclass;


  //typedef typename MRFFilterType::OutputImageType  OutputImageType;
  //typedef typename OutputImageType::PixelType      OutputPixelType;


   typedef itk::ScalarToArrayCastImageFilter< 
                   InputImageType,
                   ArrayImageType > ScalarToArrayFilterType;

  // Instantiate the ImportImageFilter
  // This filter is used for building an ITK image using 
  // the data passed in a buffer.
  typedef itk::ImportImageFilter< InputPixelType, 
                                  Dimension  > ImportFilterType;

  typedef itk::ImportImageFilter< MaskPixelType, 
                                  Dimension  > MaskImportFilterType;

  typedef itk::ImportImageFilter< MaskPixelType, 
                                  Dimension  > LabelImportFilterType;


  // We may optionally specify an image mask and generate a rectangular
  // bounding box to contrain the MRF filter to run within this
  // bounding box.
  typedef itk::ImageMaskSpatialObject< Dimension > ImageMaskSpatialObjectType;
  typedef typename ImageMaskSpatialObjectType::RegionType MaskRegionType;

  typedef itk::PasteImageFilter< LabelImageType > PasteFilterType;


public:

  /**  Constructor */
  ScalarKmeansMarkovFieldToPaintbrush() 
    {
    m_ImportFilter               = ImportFilterType::New();
    m_MaskImportFilter           = MaskImportFilterType::New();
    m_LabelImportFilter          = LabelImportFilterType::New();
    }



  /**  Destructor */
  virtual ~ScalarKmeansMarkovFieldToPaintbrush() 
    {
    }



  /**  ProcessData performs the actual filtering on the data.
       In this class, this method only initialize the import
       filter for the second input, then it lets the ProcessData
       method of the base class perform the rest of the operations. */
  void ProcessData( const vtkVVProcessDataStruct * pds )
  {
    SizeType   size;
    IndexType  start;

    double     origin[Dimension];
    double     spacing[Dimension];

    vtkVVPluginInfo * info = this->GetPluginInfo();

    size[0]     =  info->InputVolumeDimensions[0];
    size[1]     =  info->InputVolumeDimensions[1];
    size[2]     =  info->InputVolumeDimensions[2];

    for(unsigned int i=0; i<Dimension; i++)
      {
      origin[i]   =  info->InputVolumeOrigin[i];
      spacing[i]  =  info->InputVolumeSpacing[i];
      start[i]    =  0;
      }

    RegionType region;

    region.SetIndex( start );
    region.SetSize(  size  );

    m_ImportFilter->SetSpacing( spacing );
    m_ImportFilter->SetOrigin(  origin  );
    m_ImportFilter->SetRegion(  region  );

    const unsigned int totalNumberOfPixels = region.GetNumberOfPixels();

    const bool         importFilterWillDeleteTheInputBuffer = false;

    const unsigned int numberOfPixelsPerSlice = size[0] * size[1];

    InputPixelType *   dataBlockStart = 
                          static_cast< InputPixelType * >( pds->inData )  
                        + numberOfPixelsPerSlice * pds->StartSlice;

    m_ImportFilter->SetImportPointer( dataBlockStart, 
                                      totalNumberOfPixels,
                                      importFilterWillDeleteTheInputBuffer );

    m_ImportFilter->Update();

    m_LabelImportFilter->SetSpacing( spacing );
    m_LabelImportFilter->SetOrigin(  origin  );
    m_LabelImportFilter->SetRegion(  region  );

    LabelPixelType *labelDataBlockStart = static_cast< LabelPixelType * >( pds->inLabelData );
    m_LabelImportFilter->SetImportPointer( labelDataBlockStart, 
                                       region.GetNumberOfPixels(), false);

    m_LabelImportFilter->Update();


    typename MRFFilterType::Pointer filter = MRFFilterType::New();


    const unsigned int numberOfClasses           = atoi( info->GetGUIProperty(info, 0, VVP_GUI_VALUE ));
    const float        smoothingFactor           = atof( info->GetGUIProperty(info, 1, VVP_GUI_VALUE ));
    const unsigned int maximumNumberOfIterations = atoi( info->GetGUIProperty(info, 2, VVP_GUI_VALUE ));
    const float        errorTolerance            = atof( info->GetGUIProperty(info, 3, VVP_GUI_VALUE ));
    const int useMask = atoi( info->GetGUIProperty( info, 4, VVP_GUI_VALUE));
    int outsideMaskValue = atoi( info->GetGUIProperty( info, 5, VVP_GUI_VALUE ));

    
      
      
    
    filter->SetNeighborhoodRadius( 1 ); // 3x3x3 neighborhood
    filter->SetNumberOfClasses( numberOfClasses );
    filter->SetMaximumNumberOfIterations( maximumNumberOfIterations );
    filter->SetSmoothingFactor( smoothingFactor );
    filter->SetErrorTolerance( errorTolerance );

    typename ScalarToArrayFilterType::Pointer 
                   scalarToArrayFilter = ScalarToArrayFilterType::New();

    scalarToArrayFilter->ReleaseDataFlagOn();

    filter->SetInput(  scalarToArrayFilter->GetOutput()  );

    typedef itk::Image< MaskPixelType, Dimension > MaskImageType; 
    typedef itk::MaskImageFilter< typename ImportFilterType::OutputImageType,
                                  MaskImageType,
                                  typename ScalarToArrayFilterType::InputImageType >
                                    MaskFilterType;
    typename MaskFilterType::Pointer maskFilter;
    if( useMask )
      {
    
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
      maskFilter->SetInput1( m_ImportFilter->GetOutput() );

      // Use the same spacing.... as the input image
      m_MaskImportFilter->SetSpacing( spacing );
      m_MaskImportFilter->SetOrigin(  origin  );
      m_MaskImportFilter->SetRegion(  region  );

      MaskPixelType *   maskdataBlockStart = 
                          static_cast< MaskPixelType * >( pds->inData2 )  
                        + numberOfPixelsPerSlice * pds->StartSlice;
      m_MaskImportFilter->SetImportPointer( maskdataBlockStart, 
                                      totalNumberOfPixels,
                                      importFilterWillDeleteTheInputBuffer );
      m_MaskImportFilter->Update();
      maskFilter->SetInput2( m_MaskImportFilter->GetOutput() );
      
      // Make sure outsideMaskValue is within bounds
      if( outsideMaskValue > 
          itk::NumericTraits< typename ImportFilterType::OutputImageType::PixelType >::max() )
        {
        outsideMaskValue = 
          itk::NumericTraits< typename ImportFilterType::OutputImageType::PixelType >::max();
        }
      else if( outsideMaskValue < 
          itk::NumericTraits< typename ImportFilterType::OutputImageType::PixelType >::min() )
        {
        outsideMaskValue = 
          itk::NumericTraits< typename ImportFilterType::OutputImageType::PixelType >::min();
        }

      // Connect pipeline
      maskFilter->SetOutsideValue( outsideMaskValue );
      maskFilter->UpdateLargestPossibleRegion();
      
      
      // constrain computation of the classification statistics to the bounding
      // box of the mask region.
      typename ImageMaskSpatialObjectType::Pointer maskSpatialObject = 
              ImageMaskSpatialObjectType::New();
      maskSpatialObject->SetImage( m_MaskImportFilter->GetOutput() );
      m_ClassificationRegion = maskSpatialObject->GetAxisAlignedBoundingBoxRegion();
      
      m_RegionOfInterestFilter = RegionOfInterestFilterType::New();
      m_RegionOfInterestFilter->SetInput( maskFilter->GetOutput() );
      m_RegionOfInterestFilter->SetRegionOfInterest( m_ClassificationRegion );
      
      scalarToArrayFilter->SetInput( m_RegionOfInterestFilter->GetOutput() );
      }
    else
      {
      scalarToArrayFilter->SetInput( m_ImportFilter->GetOutput() );
      }
 
    
    typedef itk::Statistics::DistanceToCentroidMembershipFunction< 
                                                      ArrayPixelType > 
                                                         MembershipFunctionType;

    typedef typename MembershipFunctionType::Pointer     MembershipFunctionPointer;

    typedef std::vector< MembershipFunctionPointer > MembershipFunctionPointerVector;

    typedef itk::ImageKmeansModelEstimator< 
                                ArrayImageType, 
                                MembershipFunctionType
                                             > ImageKmeansModelEstimatorType;

    typename ImageKmeansModelEstimatorType::Pointer applyKmeansModelEstimator = 
                                               ImageKmeansModelEstimatorType::New();


    scalarToArrayFilter->Update();

    applyKmeansModelEstimator->SetInputImage( scalarToArrayFilter->GetOutput() );
    applyKmeansModelEstimator->SetNumberOfModels( numberOfClasses );
    applyKmeansModelEstimator->SetThreshold( 0.01 );
    applyKmeansModelEstimator->Update();

    MembershipFunctionPointerVector membershipFunctions = 
                          applyKmeansModelEstimator->GetMembershipFunctions(); 

    typedef std::vector<double> TempVectorType;
    typedef TempVectorType::iterator TempVectorIterator;

    std::vector<double> kmeansResultForClass(membershipFunctions.size());
      
    std::vector< double > weights; // 27 weights for a 3x3x3 neighborhood
    weights.push_back(1.0);
    weights.push_back(1.5);
    weights.push_back(1.0);
    weights.push_back(1.5);
    weights.push_back(2.0);
    weights.push_back(1.5);
    weights.push_back(1.0);
    weights.push_back(1.5);
    weights.push_back(1.0);

    weights.push_back(1.5);
    weights.push_back(2.0);
    weights.push_back(1.5);
    weights.push_back(2.0);
    weights.push_back(0.0);  // This is the central pixel
    weights.push_back(2.0);
    weights.push_back(1.5);
    weights.push_back(2.0);
    weights.push_back(1.5);

    weights.push_back(1.0);
    weights.push_back(1.5);
    weights.push_back(1.0);
    weights.push_back(1.5);
    weights.push_back(2.0);
    weights.push_back(1.5);
    weights.push_back(1.0);
    weights.push_back(1.5);
    weights.push_back(1.0);
    
    
    // Here we will normalize the weights by the mean distance of the membership
    // functions. A ballpark figure to choose is the centroid of the classes as estimated
    // by the KmeansModelEstimator. This makes the two functions, ImageFidelity and the
    // Clique-smoothing comparable.....
    double meanDistance = 0;
    for(unsigned int classIndex=0; classIndex < membershipFunctions.size(); 
      classIndex++ )
      {
      kmeansResultForClass[classIndex] = 
        (double) (membershipFunctions[classIndex]->GetCentroid())[0];
      meanDistance += kmeansResultForClass[classIndex]; 
      }
    meanDistance /= membershipFunctions.size();
    double totalWeight = 0;
    for(std::vector< double >::const_iterator wcIt = weights.begin(); 
        wcIt != weights.end(); ++wcIt )
      {
      totalWeight += *wcIt;
      }
    for(std::vector< double >::iterator wIt = weights.begin(); 
        wIt != weights.end(); wIt++ )
      {
      *wIt = static_cast< double > ( (*wIt) * meanDistance / (2 * totalWeight));
      }
    
    filter->SetMRFNeighborhoodWeight( weights );


    TempVectorIterator  classStart = kmeansResultForClass.begin();
    TempVectorIterator  classEnd   = kmeansResultForClass.end();

    std::sort( classStart, classEnd );
  
//	itk::Array<double> temp =  membershipFunctions[0]->GetCentroid();
    vnl_vector<double> temp =  membershipFunctions[0]->GetCentroid();
    for(unsigned int classIndex=0; classIndex < membershipFunctions.size(); 
      classIndex++ )
      {
      temp[0] = (double) kmeansResultForClass[classIndex];
      membershipFunctions[classIndex]->SetCentroid(temp);
      }  


    typedef itk::ImageClassifierBase< 
                              ArrayImageType,
                              LabelImageType >   SupervisedClassifierType;

    typedef typename SupervisedClassifierType::Pointer  ClassifierPointer;
    
    ClassifierPointer classifier = SupervisedClassifierType::New();

    typedef itk::MinimumDecisionRule DecisionRuleType;

    typedef typename DecisionRuleType::Pointer  DecisionRulePointer;
    
    DecisionRulePointer classifierDecisionRule = DecisionRuleType::New();

    classifier->SetDecisionRule( classifierDecisionRule.GetPointer() );
    classifier->SetNumberOfClasses( numberOfClasses );
    classifier->SetInputImage( scalarToArrayFilter->GetOutput()  );

    for( unsigned int mf=0; mf<numberOfClasses; mf++ )
      {
      classifier->AddMembershipFunction( membershipFunctions[mf] );
      }

    classifier->Update();
 

    filter->SetClassifier( classifier );


    // Execute the filter
    try
      {
      filter->Update();
      }
    catch( itk::ProcessAborted & )
      {
      return;
      }

    // Copy the data (with casting) to the output buffer provided by the PlugIn API
    typename LabelImageType::Pointer filterOutputImage;
    typename LabelImageType::Pointer outputImage;
    if (useMask)
      {
      // The MRF filter will give us an output that is the size of teh bounding 
      // box of the mask image. We will use an iterator to generate an image of 
      // the size of teh original image and paste the MRF filter output
      // to the appropriate region.
      //
      filterOutputImage = LabelImageType::New();
      filterOutputImage->SetBufferedRegion( m_ImportFilter->GetRegion() );
      filterOutputImage->Allocate();
      filterOutputImage->FillBuffer( outsideMaskValue );

      m_PasteFilter = PasteFilterType::New();
      m_PasteFilter->SetDestinationIndex( m_ClassificationRegion.GetIndex() );
      m_PasteFilter->SetSourceImage( filter->GetOutput() );
      m_PasteFilter->SetDestinationImage( filterOutputImage );
      m_PasteFilter->SetSourceRegion( filter->GetOutput()->GetBufferedRegion() );
      m_PasteFilter->Update();
      outputImage = m_PasteFilter->GetOutput();
      }
    else
      {
      outputImage = filter->GetOutput();
      }

    typedef itk::ImageRegionConstIterator< LabelImageType >  OutputIteratorType;

    if( !useMask )
      {
      m_ClassificationRegion = outputImage->GetBufferedRegion();
      }
    OutputIteratorType ot( outputImage, m_ClassificationRegion );

    
    typedef StatisticsPair<InputPixelType> StatisticsPairType;
    typedef std::vector< StatisticsPairType > HistogramType;
    HistogramType histogram;
    histogram.resize( numberOfClasses );

    typename HistogramType::iterator itr = histogram.begin();
    while( itr != histogram.end() )
      {
      itr->Clear();
      ++itr;
      }

    typename InputImageType::ConstPointer inputImage;
    if( useMask )
      {
      inputImage = maskFilter->GetOutput();
      }
    else
      {
      inputImage = m_ImportFilter->GetOutput();
      }
    
    typedef itk::ImageRegionConstIteratorWithIndex< InputImageType >  InputIteratorType;
    InputIteratorType it( inputImage, inputImage->GetBufferedRegion() );
    it.GoToBegin();

    typedef itk::ImageRegionIterator< LabelImageType > LabelIteratorType;
    LabelIteratorType lit( this->m_LabelImportFilter->GetOutput(),
        this->m_LabelImportFilter->GetOutput()->GetBufferedRegion());
    lit.GoToBegin();
    
    //InputPixelType * outData = (InputPixelType *)(pds->outData);

    ot.GoToBegin(); 

    // Value to fill in masked away regions (discarded regions) 
    const LabelPixelType fillOutsideMaskValue = 
            static_cast< LabelPixelType >(numberOfClasses);
    while( !it.IsAtEnd() )
      {
      if( m_ClassificationRegion.IsInside( it.GetIndex() ) )
        {
        const LabelPixelType pixelValue =
              static_cast<LabelPixelType>( ( ot.Get() ) );
        lit.Set(pixelValue);
        histogram[pixelValue].AddValue( it.Get() );
        ++ot;
        }
      else
        {
        lit.Set(fillOutsideMaskValue);
        }
      //*outData = it.Get();
      //++outData;
      ++it;
      ++lit;
      }
    
    m_FinalStatisticsLog << "Class\tPixels\tMean\tStdDev\tMin\tMax" << std::endl;

    m_FinalStatisticsMessage << "Class           # Pixels          #Density" << "\n";
    for(unsigned int nc=0; nc<numberOfClasses; nc++)
      {
      const unsigned long count = histogram[nc].pixels;
      const float density       = histogram[nc].sum / (float)count;
      const float stddev        = vcl_sqrt((histogram[nc].squaredSum / 
            static_cast< double >(count)) - (density * density) );
      m_FinalStatisticsLog << nc << "\t" << count << "\t" << density 
          << "\t" << stddev << "\t" << static_cast<float>(histogram[nc].minimum) 
          << "\t" << static_cast<float>(histogram[nc].maximum) << std::endl;
      
      if( count )
        {
        m_FinalStatisticsMessage.width(5);
        m_FinalStatisticsMessage << nc;
        m_FinalStatisticsMessage << " :  ";
        m_FinalStatisticsMessage.setf( std::ios_base::right);
        m_FinalStatisticsMessage.width(16);
        m_FinalStatisticsMessage << count;
        m_FinalStatisticsMessage << "  ";
        m_FinalStatisticsMessage.setf( std::ios_base::fixed);
        m_FinalStatisticsMessage.width(16);
        m_FinalStatisticsMessage.precision(8);
        m_FinalStatisticsMessage << density;
        m_FinalStatisticsMessage << "\n";
        }
      }

    // Display and write statistics log to a file
    m_FinalStatisticsLog << std::ends;
    itk::OutputWindow::Pointer outputWnd = itk::OutputWindow::New();
    itk::OutputWindow::SetInstance( outputWnd );
    outputWnd->DisplayText( m_FinalStatisticsLog.str().c_str() );
    std::ofstream finalStatisticsLog( 
        "ScalarKMeansMarkovRandomFieldStatistics.txt", std::ios::trunc);
    finalStatisticsLog << m_FinalStatisticsLog.str() << std::endl;
    finalStatisticsLog.close();
    
    if( filter->GetStopCondition() == MRFFilterType::MaximumNumberOfIterations )
      { 
      m_FinalStatisticsMessage << "Iteration count reached" << std::endl;
      }
    else if( filter->GetStopCondition() == MRFFilterType::ErrorTolerance )
      { 
      m_FinalStatisticsMessage << "Error Tolerance reached" << std::endl;
      }
    m_FinalStatisticsMessage << "MRF filter ran for " << filter->GetNumberOfIterations() 
      << " iterations." << std::endl;
    
  } // end of ProcessData


  const  ::itk::OStringStream & GetStatistics()
  {
    return  m_FinalStatisticsMessage;
  }

private:

    typename ImportFilterType::Pointer           m_ImportFilter;
    typename MaskImportFilterType::Pointer       m_MaskImportFilter;
    typename LabelImportFilterType::Pointer      m_LabelImportFilter;
    typename RegionOfInterestFilterType::Pointer m_RegionOfInterestFilter; 
    typename PasteFilterType::Pointer            m_PasteFilter;
    
    ::itk::OStringStream m_FinalStatisticsMessage;
    std::ostringstream      m_FinalStatisticsLog;

    ImageRegionType m_ImageRegion;
    MaskRegionType m_ClassificationRegion;

};


} // end namespace PlugIn

} // end namespace VolView

#endif
