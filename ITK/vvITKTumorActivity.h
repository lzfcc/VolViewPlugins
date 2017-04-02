/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/** This module implements Masking by setting to zero in the current image all
 * the pixels that are non-zero in a second image provided as mask.  */
    

#ifndef _vvITKTumorActivity_h
#define _vvITKTumorActivity_h

#include "vvITKFilterModuleTwoInputs.h"
#include "itkMaskImageFilter.h"


namespace VolView
{

namespace PlugIn
{

template< class TInputImageType, class TMaskImageType>
class TumorActivity : public 
                   FilterModuleTwoInputs<
                              itk::MaskImageFilter<
                                    TInputImageType,
                                    TMaskImageType,
                                    TInputImageType>,
                              TInputImageType,
                              TMaskImageType > {

public:

  typedef  itk::MaskImageFilter<
                      TInputImageType,
                      TMaskImageType,
                      TInputImageType> MaskImageFilterType;

  typedef  FilterModuleTwoInputs<
                      MaskImageFilterType,
                      TInputImageType,
                      TMaskImageType > Superclass;

  typedef typename MaskImageFilterType::OutputImageType  OutputImageType;

public:

  /**  Constructor */
  TumorActivity() 
    {
    }



  /**  Destructor */
  virtual ~TumorActivity() 
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

    MaskImageFilterType * maskingFilter = this->GetFilter();

    vtkVVPluginInfo *info = this->GetPluginInfo();

    maskingFilter->SetInput1(  this->GetInput1()  );
    maskingFilter->SetInput2(  this->GetInput2()  );

    // Execute the filter
    try
      {
      maskingFilter->Update();
      }
    catch( itk::ProcessAborted & )
      {
      return;
      }

    // Copy the data (with casting) to the output buffer provided by the PlugIn API
    typename OutputImageType::ConstPointer outputImage = maskingFilter->GetOutput();

    typedef itk::ImageRegionConstIterator< OutputImageType >  OutputIteratorType;

    OutputIteratorType ot( outputImage, outputImage->GetBufferedRegion() );

    typedef typename OutputImageType::PixelType OutputPixelType;
    OutputPixelType * outData = (OutputPixelType *)(pds->outData);

    typedef typename TMaskImageType::ConstPointer            MaskPointer;
    MaskPointer maskImage = this->GetInput2();

    typedef itk::ImageRegionConstIterator< TMaskImageType >  MaskIteratorType;
    MaskIteratorType mt( maskImage, maskImage->GetBufferedRegion() );

    typedef typename TInputImageType::ConstPointer           InputPointer;
    InputPointer inputImage = this->GetInput1();

    typedef itk::ImageRegionConstIterator< TInputImageType >  InputIteratorType;
    InputIteratorType it( inputImage, inputImage->GetBufferedRegion() );


    it.GoToBegin();
    mt.GoToBegin();
    ot.GoToBegin(); 

    typedef typename TInputImageType::PixelType   InputPixelType;
    typedef typename itk::NumericTraits< InputPixelType >::ScalarRealType   SumType;

    SumType sum = itk::NumericTraits< SumType >::Zero;
    unsigned long int count = 0;

    while( !ot.IsAtEnd() )
      {
      *outData = ot.Get();
      if( mt.Get() )
        {
        sum += it.Get();
        count++;
        }
      ++ot;
      ++it;
      ++mt;
      ++outData;
      }

    SumType average = sum / count;

    ::itk::OStringStream statisticsStream;

    statisticsStream << "Sum      = " << sum     << "\n";
    statisticsStream << "Count    = " << count   << "\n";
    statisticsStream << "Average  = " << average << "\n";

    info->SetProperty( info, VVP_REPORT_TEXT, statisticsStream.str().c_str() );

  } // end of ProcessData



private:


};


} // end namespace PlugIn

} // end namespace VolView

#endif
