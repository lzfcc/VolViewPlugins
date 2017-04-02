/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/** This module refines the labeling of an image by applying iterations of a
 * MarkovRandomField filter. The module expects two inputs, the first one is
 * the current labeling of the image, the second one is the original image from
 * which the labeling was generated.  */
    

#ifndef _vvITKMarkovRandomField_h
#define _vvITKMarkovRandomField_h

#include "vvITKFilterModuleTwoInputs.h"

#include "itkImage.h"
#include "itkFixedArray.h"
#include "itkScalarToArrayCastImageFilter.h"
#include "itkMRFImageFilter.h"
#include "itkDistanceToCentroidMembershipFunction.h"
#include "itkMinimumDecisionRule.h"
#include "itkImageClassifierBase.h"


namespace VolView
{

namespace PlugIn
{

template< class TInputPixelType >
class MarkovRandomField : public 
                   FilterModuleTwoInputs<
                        itk::MRFImageFilter< 
                              itk::Image< itk::FixedArray<TInputPixelType,1>, 3 >,
                              itk::Image<unsigned char,3> >,
                              itk::Image<unsigned char,3>,
                              itk::Image<TInputPixelType,3> > {

public:

  typedef itk::Image<unsigned char,3>         LabelImageType;
  typedef itk::Image<TInputPixelType,3>       InputImageType;
  typedef itk::FixedArray<TInputPixelType,1>  ArrayPixelType;
  typedef itk::Image< ArrayPixelType, 3 >     ArrayImageType;

  typedef itk::MRFImageFilter< ArrayImageType,
                               LabelImageType >    MRFFilterType;

  typedef FilterModuleTwoInputs< MRFFilterType,
                                 LabelImageType,
                                 InputImageType >  Superclass;


  typedef typename MRFFilterType::OutputImageType  OutputImageType;
  typedef typename OutputImageType::PixelType      OutputPixelType;


   typedef itk::ScalarToArrayCastImageFilter< 
                   InputImageType,
                   ArrayImageType > ScalarToArrayFilterType;


public:

  /**  Constructor */
  MarkovRandomField() 
    {
    }



  /**  Destructor */
  virtual ~MarkovRandomField() 
    {
    }


  /**  ProcessData performs the actual filtering on the data.
       In this class, this method only initialize the import
       filter for the second input, then it lets the ProcessData
       method of the base class perform the rest of the operations. */
  void 
  ProcessData( const vtkVVProcessDataStruct * pds )
  {
    // Let superclass perform initial connections
    this->Superclass::ProcessData( pds );

    MRFFilterType * filter = this->GetFilter();

    vtkVVPluginInfo *info = this->GetPluginInfo();

    const unsigned int neighborhoodRadius        = atoi( info->GetGUIProperty(info, 0, VVP_GUI_VALUE ));
    const unsigned int numberOfClasses           = atoi( info->GetGUIProperty(info, 1, VVP_GUI_VALUE ));
    const unsigned int maximumNumberOfIterations = atoi( info->GetGUIProperty(info, 2, VVP_GUI_VALUE ));
    const float        smoothingFactor           = atof( info->GetGUIProperty(info, 3, VVP_GUI_VALUE ));
    const float        errorTolerance            = atof( info->GetGUIProperty(info, 4, VVP_GUI_VALUE ));

    filter->SetNeighborhoodRadius( neighborhoodRadius );
    filter->SetNumberOfClasses( numberOfClasses );
    filter->SetMaximumNumberOfIterations( maximumNumberOfIterations );
    filter->SetSmoothingFactor( smoothingFactor );
    filter->SetErrorTolerance( errorTolerance );

    typename ScalarToArrayFilterType::Pointer 
                   scalarToArrayFilter = ScalarToArrayFilterType::New();

    scalarToArrayFilter->SetInput( this->GetInput2() );
  
    scalarToArrayFilter->ReleaseDataFlagOn();

    filter->SetInput(  scalarToArrayFilter->GetOutput()  );

    typedef itk::ImageClassifierBase< 
                              ArrayImageType,
                              LabelImageType >   SupervisedClassifierType;

    typedef typename SupervisedClassifierType::Pointer  ClassifierPointer;
    
    ClassifierPointer classifier = SupervisedClassifierType::New();

    typedef itk::MinimumDecisionRule DecisionRuleType;

    typedef typename DecisionRuleType::Pointer  DecisionRulePointer;
    
    DecisionRulePointer classifierDecisionRule = DecisionRuleType::New();

    classifier->SetDecisionRule( classifierDecisionRule.GetPointer() );

    typedef itk::Statistics::DistanceToCentroidMembershipFunction< 
                                                      ArrayPixelType > 
                                                         MembershipFunctionType;

    typedef typename MembershipFunctionType::Pointer     MembershipFunctionPointer;

    vnl_vector<double> centroid(1); 
    for( unsigned int i=0; i < numberOfClasses; i++ )
      {
      MembershipFunctionPointer membershipFunction = 
                                     MembershipFunctionType::New();
      centroid[0] = 0.0; // FIXME : PUT HERE THE MEAN FOR CLASS ITH
      membershipFunction->SetCentroid( centroid );
      classifier->AddMembershipFunction( membershipFunction );
      }

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
    typename OutputImageType::ConstPointer outputImage = filter->GetOutput();

    typedef itk::ImageRegionConstIterator< OutputImageType >  OutputIteratorType;

    OutputIteratorType ot( outputImage, outputImage->GetBufferedRegion() );

    typedef unsigned char OutputVolumePixelType;
    OutputVolumePixelType * outData = (OutputVolumePixelType *)(pds->outData);

    ot.GoToBegin(); 
    while( !ot.IsAtEnd() )
      {
      *outData = static_cast<unsigned char>( ( ot.Get() ) );
      ++ot;
      ++outData;
      }

  } // end of ProcessData



private:


};


} // end namespace PlugIn

} // end namespace VolView

#endif
