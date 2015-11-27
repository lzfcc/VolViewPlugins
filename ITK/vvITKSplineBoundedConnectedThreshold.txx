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

#ifndef _itkVVSplineBoundedConnectedThreshold_txx
#define _itkVVSplineBoundedConnectedThreshold_txx

#include "vvITKSplineBoundedConnectedThreshold.h"
#include "itkMinimumMaximumImageCalculator.h"

namespace VolView 
{

namespace PlugIn
{

/*
 *    Constructor
 */
template <class TInputPixelType >
SplineBoundedConnectedThreshold<TInputPixelType>
::SplineBoundedConnectedThreshold()
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

  m_Spline                     = SplineType::New();

  m_SourceLandmarks            = PointSetType::New();
  m_TargetLandmarks            = PointSetType::New();

  m_DoSegmentation             = false;
  m_ProduceDoubleOutput        = false;
  m_EngraveSurface             = false;
  
  this->SetNumberOfPointsAlongColumns( 21 );
  this->SetNumberOfPointsAlongRows( 21 );

  LandmarkContainerPointer sourceLandmarks = m_SourceLandmarks->GetPoints();
  LandmarkContainerPointer targetLandmarks = m_TargetLandmarks->GetPoints();


  targetLandmarks->Reserve( numberOfLandmarks );
  sourceLandmarks->Reserve( numberOfLandmarks );

}




/*
 *    Destructor
 */
template <class TInputPixelType >
SplineBoundedConnectedThreshold<TInputPixelType>
::~SplineBoundedConnectedThreshold()
{
}






/*
 *  Set the number of points along the colums
 */
template <class TInputPixelType >
void 
SplineBoundedConnectedThreshold<TInputPixelType>
::SetNumberOfPointsAlongColumns( unsigned int num )
{
   m_SidePointsCol = num;
}



/*
 *  Set the number of points along the rows
 */
template <class TInputPixelType >
void 
SplineBoundedConnectedThreshold<TInputPixelType>
::SetNumberOfPointsAlongRows( unsigned int num )
{
   m_SidePointsRow = num;
}


/*
 *  Set the stiffness value that allows to make the surface move from
 *  interpolation (passing through the landmarks) to approximation (not
 *  touching the landmarks).
 */
template <class TInputPixelType >
void 
SplineBoundedConnectedThreshold<TInputPixelType>
::SetStiffness( double stiffness )
{
   m_Spline->SetStiffness( stiffness );
}



/*
 *  Set whether the filter should produce a double output or not.
 */
template <class TInputPixelType >
void 
SplineBoundedConnectedThreshold<TInputPixelType>
::SetProduceDoubleOutput( bool doubleOutput )
{
   m_ProduceDoubleOutput = doubleOutput;
}



/*
 *  Set whether the filter should just engrave the surface on the image pixels.
 */
template <class TInputPixelType >
void 
SplineBoundedConnectedThreshold<TInputPixelType>
::SetEngraveSurface( bool engraveSurface )
{
   m_EngraveSurface = engraveSurface;
}


/*
 *  Set whether the filter should do segmentation with confidence connected or not
 */
template <class TInputPixelType >
void 
SplineBoundedConnectedThreshold<TInputPixelType>
::SetDoSegmentation( bool doSegmentation )
{
   m_DoSegmentation = doSegmentation;
}




/*
 *  Get the pointer to the ConnectedThreshold filter
 */
template <class TInputPixelType >
typename SplineBoundedConnectedThreshold<TInputPixelType>::ConnectedThresholdFilterType *
SplineBoundedConnectedThreshold<TInputPixelType>
::GetFilter()
{
   return m_ConnectedThresholdFilter;
}





/*
 *  Performs the actual filtering on the data 
 */
template <class TInputPixelType >
void 
SplineBoundedConnectedThreshold<TInputPixelType>
::ProcessData( const vtkVVProcessDataStruct * pds )
{

  this->SetUpdateMessage("Computing Surface Spline...");

  vtkVVPluginInfo * info = this->GetPluginInfo();

  const unsigned int numberOfMarkersGroups = info->NumberOfMarkersGroups;
  if( numberOfMarkersGroups < 3  )
    {
    info->SetProperty( info, VVP_ERROR, "This plugin requires you to provide at 2 groups of markers in addition to the Default group" ); 
    return;
    }

  // Names of the three expected groups
  std::string surfaceName = "Surface";
  std::string seedsName   = "Seeds";

  unsigned int seedsGroupId =0;
  unsigned int surfaceGroupId =0;
    
  for(unsigned int gid=0; gid < numberOfMarkersGroups; ++gid)
    {
    if( seedsName == info->MarkersGroupName[gid] )
      {
      seedsGroupId = gid;
      continue;
      }
    if( surfaceName == info->MarkersGroupName[gid] )
      {
      surfaceGroupId = gid;
      continue;
      }
    }

  if( seedsGroupId == 0 )
    {
    info->SetProperty( info, VVP_ERROR, "The groups of markers labeled 'Seeds' is missing" ); 
    return;
    }
  if( surfaceGroupId == 0 )
    {
    info->SetProperty( info, VVP_ERROR, "The groups of markers labeled 'Surface' is missing" ); 
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



  LandmarkContainerPointer targetLandmarks = m_TargetLandmarks->GetPoints();
  LandmarkIterator targetLandmarkItr = targetLandmarks->Begin();

  LandmarkType landmark;

  unsigned int node = 0;

  unsigned int landmarkId = 0;

  const MarkersCoordinatesType * markersCoordinates = info->Markers;

  // Load first the landmarks of the "Surface" group.
  unsigned int numberOfSurfacePoints = 0;
  for( unsigned int k=0; k<numberOfMarkers; k++ )
    {
    if( markersGroupId[k] == surfaceGroupId )
      {
      landmark[0] = *markersCoordinates++; 
      landmark[1] = *markersCoordinates++;
      landmark[2] = *markersCoordinates++;
      m_GridNodes[node++] = landmark;
      targetLandmarks->InsertElement( landmarkId++, landmark );
      numberOfSurfacePoints++;
      }
    else
      {
      markersCoordinates += 3;
      }
    }

  
  if( !numberOfSurfacePoints )
    {
    info->SetProperty( info, VVP_ERROR, "The groups of markers labeled 'Surface' should have 9 markers" ); 
    return;
    }




  // Set the Source landmaks. These are defined roughly in the
  // plane of the target landmark, using three of the corners
  // in the 3x3 grid. This nodes are defined by node[0], node[2] and node[6]
  LandmarkContainerPointer sourceLandmarks = m_SourceLandmarks->GetPoints();
  LandmarkIterator sourceLandmarkItr = sourceLandmarks->Begin();


  VectorType  Vx = m_GridNodes[2] - m_GridNodes[0];
  VectorType  Vy = m_GridNodes[6] - m_GridNodes[0];

  sourceLandmarkItr.Value() =  m_GridNodes[0];
  
  ++sourceLandmarkItr;
  sourceLandmarkItr.Value() = m_GridNodes[0] + ( Vx / 2.0 );
   
  ++sourceLandmarkItr;
  sourceLandmarkItr.Value() = m_GridNodes[2];
   
  ++sourceLandmarkItr;
  sourceLandmarkItr.Value() = m_GridNodes[0] + ( Vy / 2.0 );
   
  ++sourceLandmarkItr;
  sourceLandmarkItr.Value() = m_GridNodes[0] + ( ( Vy / 2.0 ) + (Vx / 2.0) ); 
   
  ++sourceLandmarkItr;
  sourceLandmarkItr.Value() = m_GridNodes[2] + ( Vy / 2.0 );
   
  ++sourceLandmarkItr;
  sourceLandmarkItr.Value() = m_GridNodes[6];
   
  ++sourceLandmarkItr;
  sourceLandmarkItr.Value() = m_GridNodes[6] + ( Vx / 2.0 );
   
  ++sourceLandmarkItr;
  sourceLandmarkItr.Value() = m_GridNodes[6] + Vx;
  

  // This should actually be a covariant vector.
  m_CannonicalNormal = CrossProduct( Vx, Vy );
  m_CannonicalOrigin = m_GridNodes[0];

  if( m_EngraveSurface )
    {
    this->SetUpdateMessage("Engraving surface...");
    this->EngraveSurfaceIntoImage( pds ); 
    }
  else
    {
    this->SetUpdateMessage("Generating surface...");
    this->SetCurrentFilterProgressWeight( 0.9 );

    this->SetNumberOfPointsAlongColumns( 21 );
    this->SetNumberOfPointsAlongRows( 21 );
    this->GenerateSurfacePoints( pds );
    this->PostProcessData( pds );
    }

} // end of ProcessData




/*
 *  Performs the actual filtering on the data 
 */
template <class TInputPixelType >
void 
SplineBoundedConnectedThreshold<TInputPixelType>
::GenerateSurfacePoints( const vtkVVProcessDataStruct * pds )
{
  m_Spline->SetTargetLandmarks( m_TargetLandmarks );
  m_Spline->SetSourceLandmarks( m_SourceLandmarks );

  m_Spline->ComputeWMatrix();


  VectorType  Vx = m_GridNodes[2] - m_GridNodes[0];
  VectorType  Vy = m_GridNodes[6] - m_GridNodes[0];

  // Create the set of source points. These are the cannonical 
  // positions of the points forming the surface for visualization
  m_SourcePoints.clear();
 
  for(unsigned int row=0; row < m_SidePointsRow; row++ )
    {
    double factorX =  static_cast<float>( row ) / 
                      static_cast<float>( m_SidePointsRow-1 );

    PointType pointBase = m_GridNodes[0] + ( Vx * factorX );


    for(unsigned int col=0; col< m_SidePointsCol; col++ )
      {
      double factorY =  static_cast<float>( col ) / 
                        static_cast<float>( m_SidePointsCol-1 );

      PointType point = pointBase + ( Vy * factorY );

      m_SourcePoints.push_back( point );
      }
    }

  // Map the source points throught the transform in order
  // to obtain the actual points in the surface.
  m_TargetPoints.clear();
  PointIterator sourcePointItr = m_SourcePoints.begin();

  PointType destination;

  while( sourcePointItr != m_SourcePoints.end() )
    {
    destination = m_Spline->TransformPoint( *sourcePointItr );
    m_TargetPoints.push_back( destination );
    ++sourcePointItr;
    }
}



/*
 *  Performs post-processing of data. 
 *  This involves an intensity window operation and
 *  data copying into the volview provided buffer.
 */
template <class TInputPixelType >
void 
SplineBoundedConnectedThreshold<TInputPixelType>
::PostProcessData( const vtkVVProcessDataStruct * pds )
{

  // A change in ProcessData signature could prevent this const_cast...
  vtkVVProcessDataStruct * opds = const_cast<vtkVVProcessDataStruct *>( pds );
  vtkVVPluginInfo * info = this->GetPluginInfo();


  // now put the results into the data structure
  const unsigned int numberOfPoints =  m_SidePointsCol * m_SidePointsRow;
  opds->NumberOfMeshPoints = numberOfPoints;

  float * points = new float[ numberOfPoints * 3 ];
  opds->MeshPoints = points;
  float * outputPointsItr = points;


  ConstPointIterator pointItr  = m_TargetPoints.begin();
  ConstPointIterator pointsEnd = m_TargetPoints.end();

  while( pointItr != pointsEnd )
    {
    *outputPointsItr++ =  (*pointItr)[0]; 
    *outputPointsItr++ =  (*pointItr)[1]; 
    *outputPointsItr++ =  (*pointItr)[2]; 
    ++pointItr;
    }

  opds->NumberOfMeshCells = ( m_SidePointsRow - 1 ) * ( m_SidePointsCol - 1 );
  
  // Cell connectivity entries follow the format of vtkCellArray:
  // n1, id1, id2,... idn1,  n2, id1, id2,.. idn2....

  unsigned int numEntries = opds->NumberOfMeshCells * 5; // each cell id + 4 points ids.
  int * cellsTopology = new int [ numEntries ];
  opds->MeshCells = cellsTopology;

  int * cellsTopItr = cellsTopology;

  for(unsigned int row=0; row < m_SidePointsRow - 1 ; row++ )
    {
    for(unsigned int col=0; col < m_SidePointsCol - 1; col++ )
      {
      const unsigned int cornerPoint = row * m_SidePointsCol + col;
      const unsigned int pointId1 = cornerPoint;
      const unsigned int pointId2 = cornerPoint + 1;
      const unsigned int pointId3 = cornerPoint + 1 + m_SidePointsCol;
      const unsigned int pointId4 = cornerPoint + m_SidePointsCol;

      *cellsTopItr++ = 4;
      *cellsTopItr++ = pointId1;
      *cellsTopItr++ = pointId2;
      *cellsTopItr++ = pointId3;
      *cellsTopItr++ = pointId4;

      }
    }
    
  // return the polygonal data
  info->AssignPolygonalData(info, opds);

  // clean up
  delete [] cellsTopology;
  delete [] points;
  
} // end of PostProcessData



/**  Set the pixels over the surface to the minimum value of the image.
 *   This is called here 'engraving' the surface into the image. */
template <class TInputPixelType >
void
SplineBoundedConnectedThreshold<TInputPixelType>
::EngraveSurfaceIntoImage( const vtkVVProcessDataStruct * pds )
{

  this->SetNumberOfPointsAlongColumns( 200 );
  this->SetNumberOfPointsAlongRows( 200 );

  this->GenerateSurfacePoints( pds );

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
  PointIterator targetPointItr = m_TargetPoints.begin();
  PointIterator targetPointEnd = m_TargetPoints.end();

  while( targetPointItr != targetPointEnd )
    {
    IndexType index;
    engravedImage->TransformPhysicalPointToIndex( *targetPointItr, index );
    if( region.IsInside( index ) )
      {
      engravedImage->SetPixel( index, engravingValue );
      }
    ++targetPointItr;
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

      vtkVVPluginInfo * info = this->GetPluginInfo();
      
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
