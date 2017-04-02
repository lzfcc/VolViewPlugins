/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/** Generic interface for protocol communication between an ITK filter
    and the VolView Plugin Interface */

#ifndef _itkVVSurfaceBoundedConnectedThreshold_txx
#define _itkVVSurfaceBoundedConnectedThreshold_txx

#include "vvITKSurfaceBoundedConnectedThreshold.h"
#include "itkMinimumMaximumImageCalculator.h"

namespace VolView 
{

namespace PlugIn
{

/*
 *    Constructor
 */
template <class TInputPixelType >
SurfaceBoundedConnectedThreshold<TInputPixelType>
::SurfaceBoundedConnectedThreshold()
{

  const unsigned int numberOfLandmarks = 9;  // nodes in the surface
 
  m_ImportFilter               = ImportFilterType::New();

  m_MedianFilter               = MedianFilterType::New();

  m_MedianFilter->SetInput( m_ImportFilter->GetOutput() );

  itk::Size< 3 > radius;
  radius.Fill( 2 );       // process 5x5x5 neighborhoods.

  m_MedianFilter->SetRadius( radius );

  m_ConnectedThresholdFilter  = ConnectedThresholdFilterType::New();

  m_ConnectedThresholdFilter->SetInput( m_MedianFilter->GetOutput() );

  m_DoSegmentation             = false;
  m_ProduceDoubleOutput        = false;
  m_EngraveSurface             = false;
  
}




/*
 *    Destructor
 */
template <class TInputPixelType >
SurfaceBoundedConnectedThreshold<TInputPixelType>
::~SurfaceBoundedConnectedThreshold()
{
}




/*
 *  Set whether the filter should produce a double output or not.
 */
template <class TInputPixelType >
void 
SurfaceBoundedConnectedThreshold<TInputPixelType>
::SetProduceDoubleOutput( bool doubleOutput )
{
   m_ProduceDoubleOutput = doubleOutput;
}



/*
 *  Set whether the filter should just engrave the surface on the image pixels.
 */
template <class TInputPixelType >
void 
SurfaceBoundedConnectedThreshold<TInputPixelType>
::SetEngraveSurface( bool engraveSurface )
{
   m_EngraveSurface = engraveSurface;
}


/*
 *  Set whether the filter should do segmentation with confidence connected or not
 */
template <class TInputPixelType >
void 
SurfaceBoundedConnectedThreshold<TInputPixelType>
::SetDoSegmentation( bool doSegmentation )
{
   m_DoSegmentation = doSegmentation;
}




/*
 *  Get the pointer to the ConnectedThreshold filter
 */
template <class TInputPixelType >
typename SurfaceBoundedConnectedThreshold<TInputPixelType>::ConnectedThresholdFilterType *
SurfaceBoundedConnectedThreshold<TInputPixelType>
::GetFilter()
{
   return m_ConnectedThresholdFilter;
}


/*
 *  Get the pointer to the median filter
 */
template <class TInputPixelType >
typename SurfaceBoundedConnectedThreshold<TInputPixelType>::MedianFilterType*
SurfaceBoundedConnectedThreshold<TInputPixelType>
::GetMedianFilter()
{
   return m_MedianFilter;
}






/*
 *  Performs the actual filtering on the data 
 */
template <class TInputPixelType >
void 
SurfaceBoundedConnectedThreshold<TInputPixelType>
::ProcessData( const vtkVVProcessDataStruct * pds )
{

  this->SetUpdateMessage("Receiving the Surface...");

  vtkVVPluginInfo * info = this->GetPluginInfo();

  const unsigned int numberOfMarkersGroups = info->NumberOfMarkersGroups;
  if( numberOfMarkersGroups < 2  )
    {
    info->SetProperty( info, VVP_ERROR, "This plugin requires you to provide at 1 group of markers in addition to the Default group" ); 
    return;
    }

  // Names of the three expected groups
  std::string seedsName   = "Seeds";

  unsigned int seedsGroupId =0;
    
  for(unsigned int gid=0; gid < numberOfMarkersGroups; ++gid)
    {
    if( seedsName == info->MarkersGroupName[gid] )
      {
      seedsGroupId = gid;
      break;
      }
    }

  if( seedsGroupId == 0 )
    {
    info->SetProperty( info, VVP_ERROR, "The groups of markers labeled 'Seeds' is missing" ); 
    return;
    }

  SizeType   size;
  IndexType  start;

  double     origin[3];
  double     spacing[3];

  size[0]     =  info->InputVolumeDimensions[0];
  size[1]     =  info->InputVolumeDimensions[1];
  size[2]     =  info->InputVolumeDimensions[2];

  for(unsigned int i=0; i<3; i++)
    {
    origin[i]   =  info->InputVolumeOrigin[i];
    spacing[i]  =  info->InputVolumeSpacing[i];
    start[i]    =  0;
    }


  RegionType region;

  region.SetIndex( start );
  region.SetSize( size );

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

  // Execute the filters and progressively remove temporary memory
  this->SetCurrentFilterProgressWeight( 0.1 );
  this->SetUpdateMessage("Preprocessing: Spline Surface...");
   


  const unsigned int numberOfMarkers = info->NumberOfMarkers;

  const MarkersGroupIdType     * markersGroupId     = info->MarkersGroupId;
  
  // Recover the Seeds.
  // For this we use the group Id of the markers group "Seeds".
  unsigned int numberOfSeeds = 0;
  IndexType seed;
  for( unsigned int m=0; m<numberOfMarkers; ++m)
    {
    if( markersGroupId[m] == seedsGroupId )
      {
      VolView::PlugIn::FilterModuleBase::Convert3DMarkerToIndex( info, m, seed );
      m_ConnectedThresholdFilter->AddSeed( seed );
      numberOfSeeds++;
      }
    }

  if( !numberOfSeeds )
    {
    info->SetProperty( info, VVP_ERROR, "The groups of markers labeled 'Seeds' does not have any seeds" ); 
    return;
    }


  if( m_EngraveSurface )
    {
    this->SetUpdateMessage("Engraving surface...");
    this->EngraveSurfaceIntoImage( pds ); 
    }
  else
    {
    this->SetUpdateMessage("Generating surface...");
    this->SetCurrentFilterProgressWeight( 0.9 );
    }

} // end of ProcessData



/**  Set the pixels over the surface to the minimum value of the image.
 *   This is called here 'engraving' the surface into the image. */
template <class TInputPixelType >
void
SurfaceBoundedConnectedThreshold<TInputPixelType>
::EngraveSurfaceIntoImage( const vtkVVProcessDataStruct * pds )
{

  vtkVVPluginInfo * info = this->GetPluginInfo();

  // Copy the data (with casting) to the output buffer provided by the PlugIn API
  typedef  InputImageType    EngravedImageType;
  typedef  typename EngravedImageType::PixelType    MaskedPixelType;
  typename EngravedImageType::Pointer engravedImage = EngravedImageType::New();

  m_MedianFilter->Update();

  typename InputImageType::ConstPointer inputImage = m_MedianFilter->GetOutput();

  engravedImage->SetRegions( inputImage->GetBufferedRegion() );
  engravedImage->SetSpacing( inputImage->GetSpacing() );
  engravedImage->SetOrigin(  inputImage->GetOrigin() );

  engravedImage->Allocate();


  typedef itk::ImageRegionIterator< EngravedImageType >  EngravedIteratorType;

  EngravedIteratorType et( engravedImage, engravedImage->GetBufferedRegion() );
  et.GoToBegin(); 

  typedef itk::ImageRegionConstIterator< InputImageType >  InputIteratorType;

  typename InputImageType::RegionType region = inputImage->GetBufferedRegion();
  
  InputIteratorType it( inputImage, region  ); 
  it.GoToBegin();

  while( !et.IsAtEnd() )
    {
    et.Set( it.Get() );
    ++et;
    ++it;
    }

  typedef itk::MinimumMaximumImageCalculator< InputImageType > CalculatorType;
  typename CalculatorType::Pointer minimumCalculator = CalculatorType::New();
  minimumCalculator->SetImage( inputImage );
  minimumCalculator->ComputeMinimum();

  TInputPixelType engravingValue = minimumCalculator->GetMinimum();

  // Visit all the surface points and engrave them in the image

  const unsigned int numberOfSurfaces = info->NumberOfSplineSurfaces;
  unsigned int numberOfPoints = 0;
  for(unsigned int s=0; s<numberOfSurfaces; s++)
    {
    numberOfPoints += info->NumberOfSplineSurfacePoints[s];
    }

  PointType point;
  IndexType index;
  float * pointCoor = info->SplineSurfacePoints;
  for(unsigned int p=0; p < numberOfPoints; p++)
    {
    point[0] = *pointCoor++;
    point[1] = *pointCoor++;
    point[2] = *pointCoor++;
    engravedImage->TransformPhysicalPointToIndex( point, index );
    if( region.IsInside( index ) )
      {
      engravedImage->SetPixel( index, engravingValue );
      }
    }
   

  if( m_DoSegmentation )
    {

    m_ConnectedThresholdFilter->SetInput( engravedImage );

    m_ConnectedThresholdFilter->Update();

    typename OutputImageType::ConstPointer outputImage =
                                      m_ConnectedThresholdFilter->GetOutput();

    typedef itk::ImageRegionConstIterator< OutputImageType >  OutputIteratorType;

    OutputIteratorType ot( outputImage, outputImage->GetBufferedRegion() );
    ot.GoToBegin(); 

    if( m_ProduceDoubleOutput )
      {
      it.GoToBegin(); // restart the loop.

      // When producing composite output, the output image must have the same
      // type as the input image. Therefore, we use InputPixelType instead of
      // OutputPixelType.
      InputPixelType * outData = (InputPixelType *)(pds->outData);

      InputPixelType maxFromInput = 
           static_cast< InputPixelType >( 
                 atof( this->GetInputVolumeScalarMaximum( info ) ) );

      InputPixelType minFromInput = 
           static_cast< InputPixelType >( 
                 atof( this->GetInputVolumeScalarMinimum( info ) ) );

      const OutputPixelType outsideSegmentedRegion = 
                  itk::NumericTraits< OutputPixelType >::Zero;

      typename InputImageType::ConstPointer inputImage = m_ImportFilter->GetOutput();
      typename InputImageType::RegionType   region     = inputImage->GetBufferedRegion();

      InputIteratorType it( inputImage, region  ); 
      it.GoToBegin();

      while( !ot.IsAtEnd() )
        {
        *outData = it.Get();  // copy input pixel
        ++outData;
        if( ot.Get() != outsideSegmentedRegion )  // copy segmented pixel
          {
          *outData = maxFromInput;
          }
        else
          {
          *outData = minFromInput;
          }
        ++outData;
        ++ot;
        ++it;
        }
      }
    else
      {
      OutputPixelType * outData = (OutputPixelType *)(pds->outData);

      while( !ot.IsAtEnd() )
        {
        *outData = ot.Get();  // copy output pixel
        ++outData;
        ++ot;
        }

      }
    }
  else
    {
    // Now copy this into the output to return to VolView

    MaskedPixelType * outData = (MaskedPixelType *)(pds->outData);

    et.GoToBegin();
    while( !et.IsAtEnd() )
      {
      *outData = et.Get();  // copy output pixel
      ++outData;
      ++et;
      }

    }

}



} // end of namespace PlugIn

} // end of namespace Volview

#endif
