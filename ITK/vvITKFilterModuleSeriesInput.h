/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/** 

Generic interface for protocol communication between an ITK filter and the
VolView Plugin Interface.  This module factorize the functionality of filters
requiring Series Inputs. The N input images must be 3D and are expected to have
the same extent, origin and spacing.

*/


#ifndef _vvITKFilterModuleSeriesInput_h
#define _vvITKFilterModuleSeriesInput_h

#include "vvITKFilterModuleBase.h"

#include <string.h>
#include <stdlib.h>

#include "itkImage.h"
#include "itkImportImageFilter.h"
#include "itkImageRegionConstIterator.h"


namespace VolView
{

namespace PlugIn
{

 
template <class TInputImage1, class TInputImageSeries >
class FilterModuleSeriesInput : public FilterModuleBase {

public:

  typedef FilterModuleBase   Superclass;

   // Instantiate the image types
  typedef TInputImage1        Input1ImageType;
  typedef TInputImageSeries   Input2ImageType;

  typedef typename Input1ImageType::PixelType     Input1PixelType;
  typedef typename Input2ImageType::PixelType     Input2PixelType;

  itkStaticConstMacro( Dimension1, unsigned int, 
         itk::GetImageDimension< Input1ImageType >::ImageDimension );

  itkStaticConstMacro( Dimension2, unsigned int, 
         itk::GetImageDimension< Input2ImageType >::ImageDimension );


  // Instantiate the two ImportImageFilters These filters are used for building
  // ITK images using the data passed in a buffer.

  typedef itk::ImportImageFilter< Input1PixelType, 
                                  Dimension1       > ImportFilter1Type;

  typedef itk::ImportImageFilter< Input2PixelType, 
                                  Dimension2       > ImportFilter2Type;


  typedef typename Superclass::SizeType      SizeType;
  typedef typename Superclass::IndexType     IndexType;
  typedef typename Superclass::RegionType    RegionType;



public:

  /**  Constructor */
  FilterModuleSeriesInput() 
    {
    m_ImportFilter1      = ImportFilter1Type::New();
    m_ImportFilter2      = ImportFilter2Type::New();
    }



  /**  Destructor */
  virtual ~FilterModuleSeriesInput() 
    {
    }


  /** Give access to the input images */
  const Input1ImageType * GetInput1() 
    {  return m_ImportFilter1->GetOutput(); }

  const Input2ImageType * GetInput2() 
    {  return m_ImportFilter2->GetOutput(); }


  /**  ProcessData performs the actual filtering on the data.
       In this class, this method only initialize the import
       filter for the second input, then it lets the ProcessData
       method of the base class perform the rest of the operations. */
  virtual void 
  ProcessData( const vtkVVProcessDataStruct * pds )
  {
    vtkVVPluginInfo *info = this->GetPluginInfo();

    this->InitializeProgressValue();
    // Methods must be called by derived classes at the 
    // beginning of their corresponding ProcessData().

    // Configure the import filter for the second input
    SizeType   size;
    IndexType  start;

    double     origin[3];
    double     spacing[3];

     if( !pds->inDataSeries )
      {
      info->SetProperty( info, VVP_ERROR, "This plugin requires a volume series as input but the current series input pointer is NULL." ); 
      return;
      }


    size[0]     =  info->InputVolumeDimensions[0];
    size[1]     =  info->InputVolumeDimensions[1];
    size[2]     =  pds->NumberOfSlicesToProcess;

    for(unsigned int i=0; i<3; i++)
      {
      origin[i]   =  info->InputVolumeOrigin[i];
      spacing[i]  =  info->InputVolumeSpacing[i];
      start[i]    =  0;
      }

    RegionType region;

    region.SetIndex( start );
    region.SetSize(  size  );
   
    m_ImportFilter1->SetSpacing( spacing );
    m_ImportFilter1->SetOrigin(  origin  );
    m_ImportFilter1->SetRegion(  region  );

    const unsigned int totalNumberOfPixels1 = region.GetNumberOfPixels();

    const bool         importFilterWillDeleteTheInputBuffer = false;

    const unsigned int numberOfPixelsPerSlice1 = size[0] * size[1];

    Input1PixelType * dataBlock1Start = 
                          static_cast< Input1PixelType * >( pds->inData )  
                        + numberOfPixelsPerSlice1 * pds->StartSlice;

    m_ImportFilter1->SetImportPointer( dataBlock1Start, 
                                       totalNumberOfPixels1,
                                       importFilterWillDeleteTheInputBuffer );

    m_ImportFilter1->Update();

    typedef typename Input2ImageType::RegionType    SeriesRegionType;   
    typedef typename Input2ImageType::IndexType     SeriesIndexType;   
    typedef typename Input2ImageType::SizeType      SeriesSizeType;   
    typedef typename Input2ImageType::PointType     SeriesPointType;   
    typedef typename Input2ImageType::SpacingType   SeriesSpacingType;   

    SeriesPointType   origin2;
    SeriesSpacingType spacing2;
    SeriesIndexType   start2;
    SeriesSizeType    size2;

    for(unsigned int j=0; j<3; j++)
      {
      origin2[j]   =  info->InputVolumeSeriesOrigin[j];
      spacing2[j]  =  info->InputVolumeSeriesSpacing[j];
      start2[j]    =  0;
      size2[j]     =  info->InputVolumeSeriesDimensions[j];
      }

    // Warning: Only do this if the plugin can process 4D natively
    if( Dimension2 == 4 )
      {
      // For lack of better values, ... we initialize here the time dimension.
      origin2[3]  = 0.0;
      spacing2[3] = 1.0;  // There should be a delta time measure here ?
      start2[3]   = 0;
      size2[3]    =  info->InputVolumeSeriesNumberOfVolumes;
      }

    SeriesRegionType region2;

    region2.SetIndex( start2 );
    region2.SetSize(  size2  );

    m_ImportFilter2->SetSpacing( spacing2 );
    m_ImportFilter2->SetOrigin(  origin2  );
    m_ImportFilter2->SetRegion(  region2  );

    const unsigned int totalNumberOfPixels2 = region.GetNumberOfPixels();

    const unsigned int numberOfPixelsPerVolume = size2[0] * size2[1] * size2[2];

    Input2PixelType * dataBlock2Start = 
                          static_cast< Input2PixelType * >( pds->inDataSeries ); 

    m_ImportFilter2->SetImportPointer( dataBlock2Start, 
                                       totalNumberOfPixels2,
                                       importFilterWillDeleteTheInputBuffer );

    m_ImportFilter2->Update();


    // Note that the filter has not been connected yet to the 
    // importers. This can not be done in this class since in the
    // generic case the filter input types may be different from
    // the types used in the input filters. It is up to the derived 
    // class to connect the pipeline.
   
  } // end of ProcessData



private:
    typename ImportFilter1Type::Pointer    m_ImportFilter1;
    typename ImportFilter2Type::Pointer    m_ImportFilter2;
};


} // end namespace PlugIn

} // end namespace VolView

#endif
