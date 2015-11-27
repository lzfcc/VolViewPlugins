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

#ifndef _itkVVSplineBoundedConfidenceConnected_h
#define _itkVVSplineBoundedConfidenceConnected_h

#include "vtkVVPluginAPI.h"

#include <string.h>
#include <stdlib.h>
#include <fstream>

#include "vvITKFilterModuleBase.h"

#include "itkImportImageFilter.h"
#include "itkConfidenceConnectedImageFilter.h"
#include "itkElasticBodySplineKernelTransform.h"
#include "itkMedianImageFilter.h"


namespace VolView 
{

namespace PlugIn
{

template <class TInputPixelType >
class SplineBoundedConfidenceConnected : public FilterModuleBase {

public:

   // Pixel type of the input buffer
  typedef TInputPixelType                InputPixelType;

  itkStaticConstMacro( Dimension, unsigned int, 3 );

  typedef itk::Image< InputPixelType,  Dimension >  InputImageType;

  // Instantiate the ImportImageFilter
  // This filter is used for building an ITK image using 
  // the data passed in a buffer.
  typedef itk::ImportImageFilter< InputPixelType, 
                                  Dimension       > ImportFilterType;

  typedef typename ImportFilterType::RegionType    RegionType;
  typedef typename RegionType::SizeType            SizeType;
  typedef typename RegionType::IndexType           IndexType;

  typedef  unsigned char                            OutputPixelType;
  typedef  itk::Image< OutputPixelType, Dimension > OutputImageType; 

  typedef  itk::ConfidenceConnectedImageFilter< 
                              InputImageType,  
                              OutputImageType >     ConfidenceConnectedFilterType;

  typedef typename ConfidenceConnectedFilterType::Pointer  ConfidenceConnectedFilterPointer;

  typedef itk::MedianImageFilter< InputImageType, 
                                  InputImageType > MedianFilterType;

  typedef typename MedianFilterType::Pointer       MedianFilterPointer;

  typedef float                                    CoordinateRepresentationType;
  
  typedef itk::Point< CoordinateRepresentationType,
                                         Dimension 
                                             >     PointType;

  typedef std::vector< PointType >                 PointListType;
  typedef typename PointListType::iterator         PointIterator;
  typedef typename PointListType::const_iterator   ConstPointIterator;

  typedef itk::ElasticBodySplineKernelTransform< 
                            CoordinateRepresentationType,
                            Dimension >            SplineType;                

  typedef typename SplineType::Pointer             SplinePointer;

  typedef typename SplineType::PointSetType        PointSetType;
  typedef typename PointSetType::Pointer           PointSetPointer;

  typedef typename PointSetType::PointsContainer   LandmarkContainer;
  typedef typename LandmarkContainer::Pointer      LandmarkContainerPointer;
  typedef typename LandmarkContainer::Iterator     LandmarkIterator;
  typedef typename PointSetType::PointType         LandmarkType;
  typedef typename PointType::VectorType           VectorType;
  typedef const LandmarkType *                     LandmarkConstPointerType;

  //  Set the number of points along the rows
  void SetNumberOfPointsAlongRows( unsigned int );

  //  Set the number of points along the columns
  void SetNumberOfPointsAlongColumns( unsigned int );

  //  Set the stiffness that allow to range from 
  //  interpolation to approximation
  void SetStiffness( double );
     
  //  Set whether the segmented image should be composed in double output with the
  //  input image
  void SetProduceDoubleOutput( bool );
 
  //  Set whether the surface should be engraved in the image.
  void SetEngraveSurface( bool );

  //  Set whether the image should segmented using the confidence connected filter.
  void SetDoSegmentation( bool );

  // Return the Confidence Connected filter
  ConfidenceConnectedFilterType * GetFilter();
  
public:
    SplineBoundedConfidenceConnected();
   ~SplineBoundedConfidenceConnected();

    virtual void ProcessData( const vtkVVProcessDataStruct * pds );
    virtual void PostProcessData( const vtkVVProcessDataStruct * pds );
    virtual void EngraveSurfaceIntoImage( const vtkVVProcessDataStruct * pds );
    virtual void GenerateSurfacePoints( const vtkVVProcessDataStruct * pds );

private:
    typename ImportFilterType::Pointer              m_ImportFilter;

    ConfidenceConnectedFilterPointer                m_ConfidenceConnectedFilter;

    MedianFilterPointer                             m_MedianFilter;

    unsigned int                                    m_SidePointsCol;
    unsigned int                                    m_SidePointsRow;

    SplinePointer                                   m_Spline;

    PointListType                                   m_SourcePoints;
    PointListType                                   m_TargetPoints;

    PointSetPointer                                 m_SourceLandmarks;
    PointSetPointer                                 m_TargetLandmarks;

    PointType                                       m_CannonicalOrigin;
    VectorType                                      m_CannonicalNormal;

    bool                                            m_ProduceDoubleOutput;
    bool                                            m_EngraveSurface;
    bool                                            m_DoSegmentation;

    // Auxiliary array for parameterizing the surface
    LandmarkType                                    m_GridNodes[9];
};

} // end of namespace PlugIn

} // end of namespace Volview

#endif
