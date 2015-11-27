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

#ifndef _itkVVSurfaceBoundedConnectedThreshold_h
#define _itkVVSurfaceBoundedConnectedThreshold_h

#include "vtkVVPluginAPI.h"

#include <string.h>
#include <stdlib.h>
#include <fstream>

#include "vvITKFilterModuleBase.h"

#include "itkImportImageFilter.h"
#include "itkConnectedThresholdImageFilter.h"
#include "itkMedianImageFilter.h"


namespace VolView 
{

namespace PlugIn
{

template <class TInputPixelType >
class SurfaceBoundedConnectedThreshold : public FilterModuleBase {

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

  typedef  itk::ConnectedThresholdImageFilter< 
                              InputImageType,  
                              OutputImageType >     ConnectedThresholdFilterType;

  typedef typename ConnectedThresholdFilterType::Pointer  ConnectedThresholdFilterPointer;

  typedef itk::MedianImageFilter< InputImageType, 
                                  InputImageType > MedianFilterType;

  typedef typename MedianFilterType::Pointer       MedianFilterPointer;

  typedef float                                    CoordinateRepresentationType;
  
  typedef itk::Point< CoordinateRepresentationType,
                                         Dimension 
                                             >     PointType;

  //  Set whether the segmented image should be composed in double output with the
  //  input image
  void SetProduceDoubleOutput( bool );
 
  //  Set whether the surface should be engraved in the image.
  void SetEngraveSurface( bool );

  //  Set whether the image should segmented using the confidence connected filter.
  void SetDoSegmentation( bool );

  // Return the Confidence Connected filter
  ConnectedThresholdFilterType * GetFilter();
  
  // Return the median filter
  MedianFilterType * GetMedianFilter();
  
public:
    SurfaceBoundedConnectedThreshold();
   ~SurfaceBoundedConnectedThreshold();

    virtual void ProcessData( const vtkVVProcessDataStruct * pds );
    virtual void EngraveSurfaceIntoImage( const vtkVVProcessDataStruct * pds );

private:
    typename ImportFilterType::Pointer              m_ImportFilter;

    ConnectedThresholdFilterPointer                m_ConnectedThresholdFilter;

    MedianFilterPointer                             m_MedianFilter;

    bool                                            m_ProduceDoubleOutput;
    bool                                            m_EngraveSurface;
    bool                                            m_DoSegmentation;

};

} // end of namespace PlugIn

} // end of namespace Volview

#endif
