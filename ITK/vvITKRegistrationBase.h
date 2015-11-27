/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*  Base class for Landmark initialized registration */

#ifndef _vvITKRegistrationBase_h
#define _vvITKRegistrationBase_h

#include "vtkVVPluginAPI.h"

#include "vvITKFilterModuleBase.h"

#include "itkImage.h"
#include "itkMatrix.h"
#include "itkCommand.h"
#include "itkResampleImageFilter.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkImportImageFilter.h"
#include "itkVersorRigid3DTransform.h"
#include "vnl/algo/vnl_symmetric_eigensystem.h"
#include "itkCenteredTransformInitializer.h"
#include "itkMinimumMaximumImageCalculator.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkLandmarkBasedTransformInitializer.h"

namespace VolView {

namespace PlugIn {

// =======================================================================
// The main class definition
// =======================================================================
template<class TFixedPixelType, class TMovingPixelType>
class RegistrationBaseRunner : public ::itk::Object
{
public:

  /** Standard class typedefs */
  typedef RegistrationBaseRunner        Self;
  typedef ::itk::Object                      Superclass;
  typedef ::itk::SmartPointer<Self>          Pointer;
  typedef ::itk::SmartPointer<const Self>    ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(RegistrationBaseRunner, ::itk::Object);

  itkStaticConstMacro( Dimension, unsigned int, 3 );

  typedef  TFixedPixelType     FixedPixelType;      
  typedef  TMovingPixelType    MovingPixelType;    
  typedef  float               InternalPixelType;
  
private:

  typedef itk::Image< FixedPixelType,    Dimension >   FixedImageType;
  typedef itk::Image< MovingPixelType,   Dimension >   MovingImageType;

  typedef typename FixedImageType::SizeType            SizeType;
  typedef typename FixedImageType::IndexType           IndexType;
  typedef typename FixedImageType::RegionType          RegionType;

  typedef itk::ImportImageFilter< FixedPixelType, 3>   FixedImportFilterType;
  typedef itk::ImportImageFilter< MovingPixelType, 3>  MovingImportFilterType;

  typedef itk::ResampleImageFilter< 
                           MovingImageType, 
                           FixedImageType >    ResampleFilterType;

   // WARNING: The following declaration only works fine when the type of the
   // fixed image can contain the type of the moving image without information
   // loses.  This is the case for a PET image being the Fixed image, and CT
   // being a Moving image.
   typedef itk::ResampleImageFilter< 
                           MovingImageType, 
                           FixedImageType >       FinalResampleFilterType;
  
  typedef itk::VersorRigid3DTransform< double >   TransformType;
 
  typedef TransformType::ParametersType           ParametersType;

  typedef itk::CenteredTransformInitializer< 
                                    TransformType, 
                                    FixedImageType, 
                                    MovingImageType >  TransformInitializerType;

  typedef itk::LinearInterpolateImageFunction< 
                                MovingImageType,
                                double             > LinearInterpolatorType;

  typedef itk::MinimumMaximumImageCalculator<
                                FixedImageType > MinMaxCalculatorType;

  typedef itk::RescaleIntensityImageFilter<
                                FixedImageType,
                                FixedImageType > RescaleIntensityImageFilter;
  
  typedef itk::LandmarkBasedTransformInitializer< TransformType, 
          FixedImageType, MovingImageType > LandmarkBasedTransformInitializerType;
  
public:


protected:

  // Command/Observer intended to update the progress
  typedef itk::MemberCommand< RegistrationBaseRunner >  CommandType;

  // Description:
  // The funciton to call for progress of the optimizer
  virtual void ProgressUpdate( itk::Object * caller, const itk::EventObject & event );

  // Description:
  // Computes a rigid transform that minimize the distances betwee two groups of Landmarks
  void ComputeTransformFromLandmarks();

  // Description:
  // The constructor
  RegistrationBaseRunner();

  // Description:
  // The destructor
  ~RegistrationBaseRunner();

  // Description:
  // Imports the two input images from Volview into ITK
  virtual void ImportPixelBuffer( vtkVVPluginInfo *info, 
                                  const vtkVVProcessDataStruct * pds );

  // Description:
  // Initialize the trasform using geometric information from the 
  // fixed and moving images.
  virtual void InitializeTransform();


  // Description:
  // Copies the resulting data into the output image
  virtual void CopyOutputData( vtkVVPluginInfo *info, 
                               const vtkVVProcessDataStruct * pds,
                               bool Append, bool RescaleComponents=true);

protected:
  // declare out instance variables

  typename FixedImportFilterType::Pointer        m_FixedImporter;
  typename MovingImportFilterType::Pointer       m_MovingImporter;

  typename ResampleFilterType::Pointer           m_Resampler;

  typename TransformType::Pointer                m_Transform;

  typename TransformInitializerType::Pointer     m_TransformInitializer;
 
  typename LinearInterpolatorType::Pointer       m_LinearInterpolator;

  typename MinMaxCalculatorType::Pointer         m_MinMaxCalculator;

  typename RescaleIntensityImageFilter::Pointer  m_RescaleIntensityFilter;

  typename FixedImageType::ConstPointer          m_FixedImage;
  typename MovingImageType::ConstPointer         m_MovingImage;

  typename CommandType::Pointer                  m_CommandObserver;

  vtkVVPluginInfo *                              m_Info;

  typedef std::ofstream                          OstreamType;
  OstreamType                                    m_Cout;

};

  
  
// =======================================================================
// progress Callback
template<class TFixedPixelType, class TMovingPixelType>
void 
RegistrationBaseRunner<TFixedPixelType,TMovingPixelType>::
ProgressUpdate( itk::Object * caller, const itk::EventObject & event )
{
  if( typeid( itk::ProgressEvent ) == typeid( event ) )
    {
    m_Info->UpdateProgress(m_Info,m_Resampler->GetProgress(), "Resampling..."); 
    }
}

// =======================================================================
// Constructor
template<class TFixedPixelType, class TMovingPixelType>
RegistrationBaseRunner<TFixedPixelType,TMovingPixelType>::
RegistrationBaseRunner() 
{
  m_Cout.open("log.txt");

  m_CommandObserver    = CommandType::New();
  m_CommandObserver->SetCallbackFunction( 
    this, &RegistrationBaseRunner::ProgressUpdate );

  m_FixedImporter  = FixedImportFilterType::New();
  m_MovingImporter = MovingImportFilterType::New();

  m_Transform = TransformType::New();
  m_Transform->SetIdentity();

  m_TransformInitializer = TransformInitializerType::New();

  m_LinearInterpolator = LinearInterpolatorType::New();

  m_Resampler = ResampleFilterType::New();
  m_Resampler->AddObserver( itk::ProgressEvent(), m_CommandObserver );
}


// =======================================================================
// Destructor
template<class TFixedPixelType, class TMovingPixelType>
RegistrationBaseRunner<TFixedPixelType,TMovingPixelType>::
~RegistrationBaseRunner() 
{
  m_Cout.close();
}


// =======================================================================
// Import data
template<class TFixedPixelType, class TMovingPixelType>
void 
RegistrationBaseRunner<TFixedPixelType,TMovingPixelType>::
ImportPixelBuffer( vtkVVPluginInfo *info, const vtkVVProcessDataStruct * pds )
{
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
  region.SetSize(  size  );
  
  m_FixedImporter->SetSpacing( spacing );
  m_FixedImporter->SetOrigin(  origin  );
  m_FixedImporter->SetRegion(  region  );
  
  unsigned int totalNumberOfPixels = region.GetNumberOfPixels();
  unsigned int numberOfComponents = info->InputVolumeNumberOfComponents;
  unsigned int numberOfPixelsPerSlice = size[0] * size[1];
  
  FixedPixelType *fixedDataBlockStart = static_cast< FixedPixelType * >( pds->inData );
  
  m_FixedImporter->SetImportPointer( fixedDataBlockStart, 
                                    totalNumberOfPixels, false);
  
  size[0]     =  info->InputVolume2Dimensions[0];
  size[1]     =  info->InputVolume2Dimensions[1];
  size[2]     =  info->InputVolume2Dimensions[2];
  
  for(unsigned int i=0; i<3; i++)
    {
    origin[i]   =  info->InputVolume2Origin[i];
    spacing[i]  =  info->InputVolume2Spacing[i];
    start[i]    =  0;
    }
  
  region.SetIndex( start );
  region.SetSize(  size  );
  
  m_MovingImporter->SetSpacing( spacing );
  m_MovingImporter->SetOrigin(  origin  );
  m_MovingImporter->SetRegion(  region  );
  
  totalNumberOfPixels = region.GetNumberOfPixels();
  numberOfComponents = info->InputVolume2NumberOfComponents;
  numberOfPixelsPerSlice = size[0] * size[1];
  
  MovingPixelType * movingDataBlockStart = static_cast< MovingPixelType * >( pds->inData2 );
  
  m_MovingImporter->SetImportPointer( movingDataBlockStart, 
                                     totalNumberOfPixels, false);
  
  // Execute the import filters
  m_FixedImporter->Update();
  m_MovingImporter->Update();

  m_FixedImage  = m_FixedImporter->GetOutput();
  m_MovingImage = m_MovingImporter->GetOutput();

} 

 
// =======================================================================
// Import data
template<class TFixedPixelType, class TMovingPixelType>
void 
RegistrationBaseRunner<TFixedPixelType,TMovingPixelType>::
InitializeTransform()
{
  // Initialize the transform by setting the center of rotation at the
  // geometrical center of the fixed image, and computing an initial
  // translation that will map the center of the fixed image onto the center of
  // the moving image.
  m_TransformInitializer->SetTransform( m_Transform );
  m_TransformInitializer->SetFixedImage(  m_FixedImage );
  m_TransformInitializer->SetMovingImage( m_MovingImage );
  m_TransformInitializer->GeometryOn();
  m_TransformInitializer->InitializeTransform();
}


// =======================================================================
//  Copy the output data into the volview data structure 
template<class TFixedPixelType, class TMovingPixelType>
void 
RegistrationBaseRunner<TFixedPixelType,TMovingPixelType>::
CopyOutputData( vtkVVPluginInfo *info, const vtkVVProcessDataStruct * pds, bool Append,
                                          bool RescaleComponents )
{
  // get some useful info
  unsigned int numberOfComponents = info->OutputVolumeNumberOfComponents;
  typedef itk::ImageRegionConstIterator< FixedImageType > OutputIteratorType;
  FixedPixelType * outData = static_cast< FixedPixelType * >( pds->outData );

  // Append the two images
  if( Append )
    {
    typename FixedImageType::ConstPointer fixedImage = m_FixedImage;
    // Copy the data (with casting) of the first image into the output buffer
    OutputIteratorType ot( fixedImage, fixedImage->GetBufferedRegion() );
    
    // copy the input image
    ot.GoToBegin(); 
    while( !ot.IsAtEnd() )
      {
      *outData = ot.Get();
      ++ot;
      outData += numberOfComponents;
      }
    outData = static_cast< FixedPixelType * >( pds->outData ) + 1;
    
    if (RescaleComponents)    // We must Rescale components for merging
      {
      // We will rescale every component to the same type as the first component
      // type, which is the FixedPixelType. Here we just find the maximum value
      // in the fixed image.
      m_MinMaxCalculator = MinMaxCalculatorType::New();
      m_MinMaxCalculator->SetImage( fixedImage );
      m_MinMaxCalculator->Compute();
      }
    }
  else
    {
    // Just start copying the resampled moving image.
    outData = static_cast< FixedPixelType * >( pds->outData );
    }
  
  
  // Copy the data (with casting) to the output buffer 
  typename FixedImageType::ConstPointer sampledImage;
  
  if( Append && RescaleComponents )
    {
    m_RescaleIntensityFilter = RescaleIntensityImageFilter::New();
    m_RescaleIntensityFilter->SetInput( m_Resampler->GetOutput() );
    m_RescaleIntensityFilter->SetOutputMinimum( m_MinMaxCalculator->GetMinimum() );
    m_RescaleIntensityFilter->SetOutputMaximum( m_MinMaxCalculator->GetMaximum() );
    m_RescaleIntensityFilter->Update();
    sampledImage = m_RescaleIntensityFilter->GetOutput();
    }
  else
    {
    sampledImage = m_Resampler->GetOutput();
    }
  
  
  OutputIteratorType ot2( sampledImage, 
                          sampledImage->GetBufferedRegion() );
  
  // copy the registered image
  ot2.GoToBegin(); 
  while( !ot2.IsAtEnd() )
    {
    *outData = ot2.Get();
    ++ot2;
    outData += numberOfComponents;
    }
} 

 
// =======================================================================
// Use the landmakrs (3D markers) in order to compute a rigid transform.
template<class TFixedPixelType, class TMovingPixelType>
void
RegistrationBaseRunner<TFixedPixelType,TMovingPixelType>::
ComputeTransformFromLandmarks()
{

  // The transform should be initialized here in order to get a Center of rotation.
  this->InitializeTransform();
  TransformType::TranslationType translation = m_Transform->GetTranslation();

  // We expect two groups of Markers, named "Fixed" and "Moving".

  const unsigned int numberOfMarkersGroups = m_Info->NumberOfMarkersGroups;

  if( numberOfMarkersGroups < 2 )
    {
    m_Info->SetProperty( m_Info, VVP_ERROR, "You are required to provide two groups of markers, with names 'Fixed' and 'Moving'." ); 
    return ;
    }


  const unsigned int numberOfMarkers = m_Info->NumberOfMarkers;

  if( numberOfMarkers < 2 )
    {
    m_Info->SetProperty( m_Info, VVP_ERROR, "You should provide at least one marker on each group." ); 
    return ;
    }


  // Names of the two expected groups
  std::string fixedName    = "Fixed";
  std::string movingName   = "Moving";

  unsigned int fixedGroupId  =0;
  unsigned int movingGroupId =0;
    
  for(unsigned int gid=0; gid < numberOfMarkersGroups; ++gid)
    {
    if( fixedName == m_Info->MarkersGroupName[gid] )
      {
      fixedGroupId = gid;
      continue;
      }
    if( movingName == m_Info->MarkersGroupName[gid] )
      {
      movingGroupId = gid;
      continue;
      }
    }


  if( fixedGroupId == 0 )
    {
    m_Info->SetProperty( m_Info, VVP_ERROR, "You are required to provide a group of markers with the name 'Fixed' " ); 
    return;
    }

  if( movingGroupId == 0 )
    {
    m_Info->SetProperty( m_Info, VVP_ERROR, "You are required to provide a group of markers with the name 'Moving' " ); 
    return;
    }

  
  typedef typename 
    LandmarkBasedTransformInitializerType::LandmarkPointType PointType;
  typedef typename 
    LandmarkBasedTransformInitializerType::LandmarkPointContainer PointContainer;

  PointContainer fixedLandmarks;
  PointContainer movingLandmarks;

  IndexType seedIndex;
  PointType seedPosition;

  const MarkersGroupIdType * markersGroupId     = m_Info->MarkersGroupId;

  for( unsigned int m=0; m<numberOfMarkers; ++m)
    {
    if( markersGroupId[m] == fixedGroupId )
      {
      VolView::PlugIn::FilterModuleBase::Convert3DMarkerToIndex( m_Info, m, seedIndex );
      m_FixedImage->TransformIndexToPhysicalPoint( seedIndex, seedPosition );
      fixedLandmarks.push_back( seedPosition );
      }
    if( markersGroupId[m] == movingGroupId )
      {
      VolView::PlugIn::FilterModuleBase::Convert3DMarkerToIndex( m_Info, m, seedIndex );
      // Note that both indices must be mapped by the fixed image since the
      // user selects the points in the fixed coordinate system.
      m_FixedImage->TransformIndexToPhysicalPoint( seedIndex, seedPosition );
      movingLandmarks.push_back( seedPosition );
      }
    }

  if( movingLandmarks.size() != fixedLandmarks.size() )
    {
    m_Info->SetProperty( m_Info, VVP_ERROR, "The 'Fixed' and 'Moving' groups should have equal number of markers" ); 
    return;
    }


  m_Transform->SetIdentity();
  
  typename LandmarkBasedTransformInitializerType::Pointer landmarkInitializer =
          LandmarkBasedTransformInitializerType::New();
  landmarkInitializer->SetFixedLandmarks( fixedLandmarks );
  landmarkInitializer->SetMovingLandmarks( movingLandmarks );
  landmarkInitializer->SetTransform( m_Transform );
  landmarkInitializer->InitializeTransform();

  // Since the landmarks were applied in a frame with the two centers aligned, 
  // we must add this value.
  TransformType::TranslationType newTranslation = 
            m_Transform->GetTranslation() + translation;
  m_Transform->SetTranslation( newTranslation );
  
  m_Cout << "Full transform as initialized from Landmarks" << std::endl;
  m_Transform->Print( m_Cout );

}


} // end namespace PlugIn

} // end namespace VolView


#endif

