/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* perform registration of PET-CT images using 
 * Mutual Information and a Rigid Transform */

#include "vtkVVPluginAPI.h"

#include "vvITKRegistrationBase.h"

#include "itkImageRegistrationMethod.h"
#include "itkNormalizeImageFilter.h"
#include "itkIntensityWindowingImageFilter.h"
#include "itkMattesMutualInformationImageToImageMetric.h"
#include "itkVersorRigid3DTransform.h"
#include "itkVersorRigid3DTransformOptimizer.h"

namespace VolView {

namespace PlugIn {


// =======================================================================
// The main class definition
// =======================================================================
template<class TFixedPixelType, class TMovingPixelType>
class MultimodalityRegistrationRigidBRunner : 
  public RegistrationBaseRunner<TFixedPixelType,TMovingPixelType >
{
public:

  /** Standard class typedefs */
  typedef MultimodalityRegistrationRigidBRunner            Self;
  typedef RegistrationBaseRunner<
           TFixedPixelType,TMovingPixelType> Superclass;
  typedef ::itk::SmartPointer<Self>          Pointer;
  typedef ::itk::SmartPointer<const Self>    ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(MultimodalityRegistrationRigidBRunner, RegistrationBaseRunner);

  itkStaticConstMacro( Dimension, unsigned int, 3 );

  typedef  typename Superclass::FixedPixelType     FixedPixelType;      // PET Image
  typedef  typename Superclass::MovingPixelType    MovingPixelType;     // CT  Image
  typedef  float                                   InternalPixelType;
  
private:

  typedef itk::Image< FixedPixelType,    Dimension >   FixedImageType;
  typedef itk::Image< MovingPixelType,   Dimension >   MovingImageType;
  typedef itk::Image< InternalPixelType, Dimension >   InternalImageType;

  typedef typename FixedImageType::SizeType            SizeType;
  typedef typename FixedImageType::IndexType           IndexType;
  typedef typename FixedImageType::RegionType          RegionType;

  typedef itk::IntensityWindowingImageFilter< 
                           FixedImageType, 
                           FixedImageType >       FixedWindowingFilterType;

  typedef itk::NormalizeImageFilter< 
                           FixedImageType,
                           InternalImageType >    FixedNormalizeFilterType;

  typedef itk::NormalizeImageFilter< 
                           MovingImageType,
                           InternalImageType >    MovingNormalizeFilterType;

  typedef itk::ResampleImageFilter< 
                           InternalImageType, 
                           InternalImageType >    ResampleFilterType;

   // WARNING: The following declaration only works fine when the type of the
   // fixed image can contain the type of the moving image without information
   // loses.  This is the case for a PET image being the Fixed image, and CT
   // being a Moving image.
   typedef itk::ResampleImageFilter< 
                           MovingImageType, 
                           FixedImageType >       FinalResampleFilterType;
  
  typedef itk::VersorRigid3DTransform< double >   TransformType;
 
  typedef TransformType::ParametersType           ParametersType;

  typedef itk::VersorRigid3DTransformOptimizer    OptimizerType;

  typedef itk::LinearInterpolateImageFunction< 
                                InternalImageType,
                                double             > LinearInterpolatorType;

  typedef itk::MattesMutualInformationImageToImageMetric< 
                                InternalImageType, 
                                InternalImageType >    MutualInformationMetricType;


  typedef MutualInformationMetricType                MetricType;                                          
                                          

  typedef OptimizerType::ScalesType       OptimizerScalesType;


  typedef itk::ImageRegistrationMethod< 
                                   InternalImageType, 
                                   InternalImageType    > RegistrationType;


  // Enums for defining different levels of registration quality.
  typedef enum {
    Low,
    High
  } QualityLevelType;

public:

  // Description:
  // Sets up the pipeline and invokes the registration process
  int Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds );


protected:

  // Command/Observer intended to update the progress
  typedef itk::MemberCommand< MultimodalityRegistrationRigidBRunner >  CommandType;

  // Description:
  // The funciton to call for progress of the optimizer
  void ProgressUpdate( itk::Object * caller, const itk::EventObject & event );

  // Description:
  // The constructor
  MultimodalityRegistrationRigidBRunner();

  // Description:
  // The destructor
  ~MultimodalityRegistrationRigidBRunner();

  void SetInitialTransform( const TransformType * transform );

  void InitializeRegistration();

  void RegisterCurrentResolutionLevel();

  const ParametersType & GetParameters() const;

  const TransformType * GetTransform() const;

  void SetQualityLevel( QualityLevelType level );

  void PrepareLevel( unsigned int factor );


private:
  // declare out instance variables

  typename FixedWindowingFilterType::Pointer     m_FixedWindower;

  typename FixedNormalizeFilterType::Pointer     m_FixedNormalizer;
  typename MovingNormalizeFilterType::Pointer    m_MovingNormalizer;

  typename ResampleFilterType::Pointer           m_FixedResampler;
  typename ResampleFilterType::Pointer           m_MovingResampler;

  typename OptimizerType::Pointer                m_Optimizer;

  typename MetricType::Pointer                   m_Metric;

  typename LinearInterpolatorType::Pointer       m_InternalLinearInterpolator;

  typename RegistrationType::Pointer             m_RegistrationMethod;

  MovingPixelType                                m_BackgroundLevel;

  unsigned int                                   m_Level;
  
  bool                                           m_ReportTimers;

  QualityLevelType                               m_QualityLevel;

  std::vector<unsigned int>                      m_LevelFactor;

  bool                                           m_OperationCancelled;
};

  
  
// =======================================================================
// progress Callback
template<class TFixedPixelType, class TMovingPixelType>
void 
MultimodalityRegistrationRigidBRunner<TFixedPixelType,TMovingPixelType>::
ProgressUpdate( itk::Object * caller, const itk::EventObject & event )
{

  char tstr[1024];
  if( itk::IterationEvent().CheckEvent( &event ) )
    {

    // Test whether during the GUI update, the Abort button was pressed
    int abort = atoi( this->m_Info->GetProperty( this->m_Info, VVP_ABORT_PROCESSING ) );
    if( abort )
      {
      this->m_Optimizer->StopOptimization();
      this->m_OperationCancelled = true;
      return;
      }

    OptimizerType::DerivativeType gradient = this->m_Optimizer->GetGradient();
    OptimizerType::ScalesType     scales   = this->m_Optimizer->GetScales();

    double magnitude2 = 0.0;

    for(unsigned int i=0; i<gradient.size(); i++)
      {
      const double fc = gradient[i] / scales[i];
      magnitude2 += fc * fc;
      }  

    const double gradientMagnitude = sqrt( magnitude2 );

    this->m_Cout << this->m_Optimizer->GetCurrentIteration() << "   ";
    this->m_Cout << this->m_Optimizer->GetValue() << "   ";
    this->m_Cout << gradientMagnitude << "   ";
    this->m_Cout << this->m_Optimizer->GetCurrentStepLength() << "  ";
    this->m_Cout << this->m_Optimizer->GetCurrentPosition() << std::endl;


  switch( this->m_Level )
    {
    case 0: 
        sprintf(tstr,"Quarter Resolution Iteration : %i Value: %g", 
                this->m_Optimizer->GetCurrentIteration(),
                this->m_Optimizer->GetValue());
        break;
    case 1:
        sprintf(tstr,"Half Resolution Iteration : %i Value: %g", 
                this->m_Optimizer->GetCurrentIteration(),
                this->m_Optimizer->GetValue());
        break;
    case 2:
        sprintf(tstr,"Full Resolution Iteration : %i Value: %g", 
                this->m_Optimizer->GetCurrentIteration(),
                this->m_Optimizer->GetValue());
        break;
    }

    const float progress =  0.9 * this->m_Optimizer->GetCurrentIteration() /
                                  this->m_Optimizer->GetNumberOfIterations();

    this->m_Info->UpdateProgress(this->m_Info, progress, tstr); 

    return;
    }

  if( typeid( itk::ProgressEvent ) == typeid( event ) )
    {

    // Test whether during the GUI update, the Abort button was pressed
    int abort = atoi( this->m_Info->GetProperty( this->m_Info, VVP_ABORT_PROCESSING ) );
    if( abort )
      {
      itk::ProcessObject::Pointer process = dynamic_cast< itk::ProcessObject *>( caller );
      process->SetAbortGenerateData(true);
      this->m_OperationCancelled = true;
      return;
      }


    this->m_Info->UpdateProgress(this->m_Info,0.9 + 0.1*this->m_Resampler->GetProgress(), 
                           "Resampling..."); 
    return;
    }
}

// =======================================================================
// Constructor
template<class TFixedPixelType, class TMovingPixelType>
MultimodalityRegistrationRigidBRunner<TFixedPixelType,TMovingPixelType>::
MultimodalityRegistrationRigidBRunner() 
{
  this->m_FixedWindower    = FixedWindowingFilterType::New();

  this->m_FixedNormalizer  = FixedNormalizeFilterType::New();
  this->m_MovingNormalizer = MovingNormalizeFilterType::New();

  this->m_FixedResampler   = ResampleFilterType::New();
  this->m_MovingResampler  = ResampleFilterType::New();

  this->m_InternalLinearInterpolator = LinearInterpolatorType::New();

  this->m_Metric = MetricType::New();

  this->m_Optimizer = OptimizerType::New();
  this->m_Optimizer->MinimizeOn();
  this->m_Optimizer->SetRelaxationFactor( 0.9 );

  this->m_RegistrationMethod = RegistrationType::New();

  this->m_Metric->SetNumberOfHistogramBins( 256 );

  this->m_RegistrationMethod->SetMetric( this->m_Metric );
  this->m_RegistrationMethod->SetOptimizer( this->m_Optimizer );
  this->m_RegistrationMethod->SetTransform( this->m_Transform );
  this->m_RegistrationMethod->SetInterpolator( this->m_InternalLinearInterpolator );
  this->m_Optimizer->AddObserver( itk::IterationEvent(), this->m_CommandObserver );

  this->m_Level = 0;
  this->m_QualityLevel = Low;
  this->m_OperationCancelled = false;
}


// =======================================================================
// Destructor
template<class TFixedPixelType, class TMovingPixelType>
MultimodalityRegistrationRigidBRunner<TFixedPixelType,TMovingPixelType>::
~MultimodalityRegistrationRigidBRunner() 
{

}

 
// =======================================================================
//  Set the quality level. This defines the trade-off between registration
//  quality and computation time.
template<class TFixedPixelType, class TMovingPixelType>
void 
MultimodalityRegistrationRigidBRunner<TFixedPixelType,TMovingPixelType>::
SetQualityLevel( QualityLevelType level )
{
  this->m_QualityLevel = level;
}



// =======================================================================
//  Prepare the subsampled images.
template<class TFixedPixelType, class TMovingPixelType>
void 
MultimodalityRegistrationRigidBRunner<TFixedPixelType,TMovingPixelType>::
PrepareLevel( unsigned int factor )
{
  this->m_Cout << "Preparing Level at factor = " << factor << std::endl;

  typename InternalImageType::SpacingType spacing;
  typename InternalImageType::RegionType  region;
  typename InternalImageType::SizeType    size;
  typedef typename InternalImageType::SizeType::SizeValueType    SizeValueType;

  // Sub-sample the Fixed image
  this->m_FixedResampler->SetInput( this->m_FixedNormalizer->GetOutput() );

  spacing = this->m_FixedImage->GetSpacing();
  region  = this->m_FixedImage->GetLargestPossibleRegion();
  size    = region.GetSize();

  for(unsigned int i=0; i<Dimension; i++)
    {
    spacing[i] *= factor;
    size[i]    = static_cast< SizeValueType >( size[i] / factor );
    }

  this->m_FixedResampler->SetOutputSpacing( spacing );
  this->m_FixedResampler->SetOutputOrigin( this->m_FixedImage->GetOrigin() );
  this->m_FixedResampler->SetSize( size );
  this->m_FixedResampler->SetOutputStartIndex( region.GetIndex() );
  this->m_FixedResampler->SetTransform( itk::IdentityTransform<double>::New() );

  this->m_FixedResampler->UpdateLargestPossibleRegion();

  this->m_RegistrationMethod->SetFixedImage( this->m_FixedResampler->GetOutput() );

  // Sub-sample the Moving image
  this->m_MovingResampler->SetInput( this->m_MovingNormalizer->GetOutput() );

  spacing = this->m_MovingImage->GetSpacing();
  region  = this->m_MovingImage->GetLargestPossibleRegion();
  size    = region.GetSize();

  for(unsigned int i=0; i<Dimension; i++)
    {
    spacing[i] *= factor;
    size[i]    = static_cast< SizeValueType >( size[i] / factor );
    }

  this->m_MovingResampler->SetOutputSpacing( spacing );
  this->m_MovingResampler->SetOutputOrigin( this->m_MovingImage->GetOrigin() );
  this->m_MovingResampler->SetSize( size );
  this->m_MovingResampler->SetOutputStartIndex( region.GetIndex() );
  this->m_MovingResampler->SetTransform( itk::IdentityTransform<double>::New() );

  this->m_MovingResampler->UpdateLargestPossibleRegion();

  this->m_RegistrationMethod->SetMovingImage( this->m_MovingResampler->GetOutput() );


  // Computing the FixedImageRegion from the Cropping (if cropping is ON).  The
  // FixedImageRegion is the region of the fixed image over which the metric
  // will be computed. Selecting smaller regions speed up computation and
  // facilitates to focus the registration match in areas that are of
  // particular importance for the application at hand.
  typename InternalImageType::RegionType  fixedImageRegion;
  typename InternalImageType::IndexType   fixedImageRegionStart;
  typename InternalImageType::SizeType    fixedImageRegionSize;

  // convert cropping planes into indexes 
  int idx[6];
  for ( unsigned int j = 0; j < 6; ++j )
    {
    idx[j] = (int)( (this->m_Info->CroppingPlanes[j]-
                     this->m_Info->InputVolumeOrigin[j/2])/
                     this->m_Info->InputVolumeSpacing[j/2] + 0.5);

    // Truncate them if they go out of bounds
    if (idx[j] < 0) 
      {
      idx[j] = 0;
      }
    if (idx[j] >= this->m_Info->InputVolumeDimensions[j/2])
      {
      idx[j] = this->m_Info->InputVolumeDimensions[j/2] -1;
      }
    }

  fixedImageRegionStart[0] = static_cast<int>( idx[0] / factor );
  fixedImageRegionStart[1] = static_cast<int>( idx[2] / factor );
  fixedImageRegionStart[2] = static_cast<int>( idx[4] / factor );

  fixedImageRegionSize[0] =  static_cast<int>( (idx[1] - idx[0] + 1 ) / factor );
  fixedImageRegionSize[1] =  static_cast<int>( (idx[3] - idx[2] + 1 ) / factor );
  fixedImageRegionSize[2] =  static_cast<int>( (idx[5] - idx[4] + 1 ) / factor );

  fixedImageRegion.SetIndex( fixedImageRegionStart );
  fixedImageRegion.SetSize( fixedImageRegionSize );

  this->m_Cout << "fixedImageRegion set to " << std::endl;
  this->m_Cout << fixedImageRegion << std::endl;

  this->m_RegistrationMethod->SetFixedImageRegion( fixedImageRegion );

}


// =======================================================================
//  Register one of the resolution levels.
template<class TFixedPixelType, class TMovingPixelType>
void 
MultimodalityRegistrationRigidBRunner<TFixedPixelType,TMovingPixelType>::
InitializeRegistration()
{
  this->m_Cout << "InitializeRegistration() begin" << std::endl;


  // do we use the Landmarks for aligning the volumes ?
  const char *result = this->m_Info->GetGUIProperty(this->m_Info, 1, VVP_GUI_VALUE);
  if (result && !strcmp(result,"Yes, Use Landmarks"))
    {
    // Initialize the transform using the Landmarks provided by the user.  
    // If there are one or two landmarks pairs, then only translation will 
    // be computed.  With three or more landmark pairs, then rotations will
    // be computed too.
    this->m_Cout << "Computing Landmark-based transformation" << std::endl;
    this->ComputeTransformFromLandmarks();
    }
  else 
    {
    // Initialize the transform by setting the center of rotation at the
    // geometrical center of the fixed image, and computing an initial
    // translation that will map the center of the fixed image onto the center of
    // the moving image. 
    this->m_Cout << "Computing Geometrically centered transformation" << std::endl;
    this->InitializeTransform();
    }



  // Conect Moving and Fixed images to their normalizer filters.
  // This should be done here, in order to manage both the case of
  // a single volume used as moving image, and the case of a series.
  this->m_FixedNormalizer->SetInput(  this->m_FixedImage  );
  this->m_MovingNormalizer->SetInput( this->m_MovingImage );


  // Insert scale factor for multiresolution.
  this->m_LevelFactor.push_back( 4 );
  this->m_LevelFactor.push_back( 2 );
  this->m_LevelFactor.push_back( 1 );

  this->PrepareLevel( this->m_LevelFactor[0] );

  typename InternalImageType::SizeType    fixedImageSize     = this->m_FixedImage->GetBufferedRegion().GetSize();
  typename InternalImageType::SpacingType fixedImageSpacing  = this->m_FixedImage->GetSpacing();
  typename InternalImageType::PointType   fixedImageOrigin   = this->m_FixedImage->GetOrigin();

  typedef OptimizerType::ScalesType       OptimizerScalesType;

  const unsigned int numberOfParameters = this->m_Transform->GetNumberOfParameters();

  OptimizerScalesType optimizerScales( numberOfParameters );

  optimizerScales[0] = 1.0; // scale for rotation
  optimizerScales[1] = 1.0; // scale for rotation
  optimizerScales[2] = 1.0; // scale for rotation

  const double magicFactor = 10.0; 

  optimizerScales[3] = 1.0 / ( magicFactor * fixedImageSize[0] * fixedImageSpacing[0] );
  optimizerScales[4] = 1.0 / ( magicFactor * fixedImageSize[1] * fixedImageSpacing[1] );
  optimizerScales[5] = 1.0 / ( magicFactor * fixedImageSize[2] * fixedImageSpacing[2] );

  this->m_Cout << "optimizerScales = " << optimizerScales << std::endl;

  this->m_Optimizer->SetScales( optimizerScales );

  typename RegistrationType::ParametersType initialParameters = 
                                    this->m_Transform->GetParameters();      

  this->m_Cout << "Initial Transform " << std::endl;
  this->m_Transform->Print( this->m_Cout );

  this->m_RegistrationMethod->SetInitialTransformParameters( initialParameters );

  this->m_Cout << "InitializeRegistration() ends" << std::endl;
}


 
// =======================================================================
//  Register one of the resolution levels.
template<class TFixedPixelType, class TMovingPixelType>
void 
MultimodalityRegistrationRigidBRunner<TFixedPixelType,TMovingPixelType>::
RegisterCurrentResolutionLevel()
{
  const unsigned int numberOfQualityOptions   = 2;
  const unsigned int numberOfResolutionLevels = 3;

  int    IterationsList[  numberOfResolutionLevels ][ numberOfQualityOptions ];
  double MaximumStepList[ numberOfResolutionLevels ][ numberOfQualityOptions ];
  double MinimumStepList[ numberOfResolutionLevels ][ numberOfQualityOptions ];
  double GradientTolList[ numberOfResolutionLevels ][ numberOfQualityOptions ];
  double PercentOfSamplesList[ numberOfResolutionLevels ][ numberOfQualityOptions ];


  // Quality level Low = Fastest run time, Lowest Quality
  MaximumStepList[ 0 ][ Low ] = 1.00; 
  MaximumStepList[ 1 ][ Low ] = 0.10; 
  MaximumStepList[ 2 ][ Low ] = 0.01; 

  MinimumStepList[ 0 ][ Low ] = 0.0100; 
  MinimumStepList[ 1 ][ Low ] = 0.0010; 
  MinimumStepList[ 2 ][ Low ] = 0.0001; 

  IterationsList[ 0 ][ Low ] = 100;
  IterationsList[ 1 ][ Low ] = 100;
  IterationsList[ 2 ][ Low ] = 100;

  GradientTolList[ 0 ][ Low ] = 0.1;
  GradientTolList[ 1 ][ Low ] = 0.1;
  GradientTolList[ 2 ][ Low ] = 0.1;
  
  PercentOfSamplesList[ 0 ][ Low ] = 0.3;
  PercentOfSamplesList[ 1 ][ Low ] = 0.2;
  PercentOfSamplesList[ 2 ][ Low ] = 0.1;
  
 
  // Quality level High = Slowest run time, Highest Quality
  MaximumStepList[ 0 ][ High ] = 0.5; 
  MaximumStepList[ 1 ][ High ] = 0.5; 
  MaximumStepList[ 2 ][ High ] = 0.5; 

  MinimumStepList[ 0 ][ High ] = 0.001; 
  MinimumStepList[ 1 ][ High ] = 0.001; 
  MinimumStepList[ 2 ][ High ] = 0.001; 

  IterationsList[ 0 ][ High ] = 500;
  IterationsList[ 1 ][ High ] = 500;
  IterationsList[ 2 ][ High ] = 500;

  GradientTolList[ 0 ][ High ] = 0.01;
  GradientTolList[ 1 ][ High ] = 0.01;
  GradientTolList[ 2 ][ High ] = 0.01;
 
  PercentOfSamplesList[ 0 ][ High ] = 0.8;
  PercentOfSamplesList[ 1 ][ High ] = 0.7;
  PercentOfSamplesList[ 2 ][ High ] = 0.6;
  
  this->PrepareLevel( this->m_LevelFactor[ this->m_Level ] );
      
  this->m_Optimizer->SetMaximumStepLength( MaximumStepList[ this->m_Level ][ this->m_QualityLevel ] );
  this->m_Optimizer->SetMinimumStepLength( MinimumStepList[ this->m_Level ][ this->m_QualityLevel ] );
  this->m_Optimizer->SetNumberOfIterations( IterationsList[ this->m_Level ][ this->m_QualityLevel ] );
  this->m_Optimizer->SetGradientMagnitudeTolerance( GradientTolList[ this->m_Level ][ this->m_QualityLevel ] );

  typename InternalImageType::RegionType  region;
  region = this->m_FixedResampler->GetOutput()->GetBufferedRegion();

  const unsigned int numberOfSamples = 
    static_cast< unsigned int >( region.GetNumberOfPixels() * 
                                 PercentOfSamplesList[ this->m_Level ][ this->m_QualityLevel ] );

  this->m_Cout << "NumberOfSamples = " << numberOfSamples << std::endl;

  this->m_Metric->SetNumberOfSpatialSamples( numberOfSamples );


  this->m_RegistrationMethod->SetInitialTransformParameters( 
                                  this->m_Transform->GetParameters() ); 

  this->m_RegistrationMethod->StartRegistration(); 

  this->m_Level++;

}


// =======================================================================
// Main execute method
template<class TFixedPixelType, class TMovingPixelType>
int 
MultimodalityRegistrationRigidBRunner<TFixedPixelType,TMovingPixelType>::
Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds )
{
  this->m_OperationCancelled = false;

  this->m_Info = info;
  
  this->ImportPixelBuffer( info, pds );  


  // Select the trade-off between registration quality and computation time.
  const char *registrationQuality = info->GetGUIProperty(info, 2, VVP_GUI_VALUE);
  if (registrationQuality )
    {
    if( !strcmp(registrationQuality,"Medium quality - takes short time"))
      {
      this->SetQualityLevel( Low );
      }
    if( !strcmp(registrationQuality,"High quality - takes long time"))
      {
      this->SetQualityLevel( High );
      }
    }



  // Select how many resolutions levels to use for the registration 
  const char *multiResolutionLevels = info->GetGUIProperty(info, 3, VVP_GUI_VALUE);
  unsigned int numberOfResolutionLevelsToUse = 1;
  if (multiResolutionLevels )
    {
    if( !strcmp(multiResolutionLevels,"One - Only Quarter resolution"))
      {
      numberOfResolutionLevelsToUse = 1;
      }
    if( !strcmp(multiResolutionLevels,"Two - Quarter and Half resolutions"))
      {
      numberOfResolutionLevelsToUse = 2;
      }
    if( !strcmp(multiResolutionLevels,"Three - Quarter, Half and Full resolutions"))
      {
      numberOfResolutionLevelsToUse = 3;
      }
    }


  this->InitializeRegistration();


  for(unsigned int resolution=0; resolution < numberOfResolutionLevelsToUse; resolution++)
    {
    this->RegisterCurrentResolutionLevel();  
    }


  // now get the resulting parameters
  typename RegistrationType::ParametersType finalParameters = 
    this->m_RegistrationMethod->GetLastTransformParameters();
  
  this->m_Transform->SetParameters( finalParameters );
  
  this->m_Cout << "finalTransform = " << std::endl;
  this->m_Transform->Print( this->m_Cout );

  this->m_Resampler->SetTransform( this->m_Transform );
  this->m_Resampler->SetInput( this->m_MovingImporter->GetOutput() );
  this->m_Resampler->SetSize( 
    this->m_FixedImporter->GetOutput()->GetLargestPossibleRegion().GetSize());
  this->m_Resampler->SetOutputOrigin( this->m_FixedImporter->GetOutput()->GetOrigin() );
  this->m_Resampler->SetOutputSpacing( this->m_FixedImporter->GetOutput()->GetSpacing());
  this->m_Resampler->SetDefaultPixelValue(0);
  
  info->UpdateProgress(info,0.8,"Starting Resample ...");      
  this->m_Resampler->UpdateLargestPossibleRegion();
  
  const char *result1 = info->GetGUIProperty(info, 4, VVP_GUI_VALUE);
  const bool appendImages = (result1 && !strcmp(result1,"Append The Volumes"));

  // Rescale components ?
  bool rescaleComponents = true;
  const int rescaleDynamicRangeForMerging = atoi( info->GetGUIProperty( 
    info, 0, VVP_GUI_VALUE));
  if( !rescaleDynamicRangeForMerging )  
    {
    rescaleComponents = false;
    }
  
  this->CopyOutputData( info, pds, appendImages, rescaleComponents );
 
  
  // set some output information,
  char results[1024];

  typedef TransformType::VersorType VersorType;
  VersorType versor = this->m_Transform->GetVersor();
  TransformType::OffsetType offset = this->m_Transform->GetOffset();
  typedef VersorType::VectorType   AxisType;
  AxisType axis = versor.GetAxis();

  sprintf(results,"Number of Iterations Used: %d\nTranslation: %g %g %g\nRotation Axis %f %f %f %f\nOffset: %g %g %g", 
          this->m_Optimizer->GetCurrentIteration(),
          finalParameters[3],
          finalParameters[4],
          finalParameters[5],
          axis[0],
          axis[1],
          axis[2],
          versor.GetAngle(),
          offset[0],
          offset[1],
          offset[2]
          );
  info->SetProperty(info, VVP_REPORT_TEXT, results);
  
  return 0;
}


} // end namespace PlugIn

} // end namespace VolView




static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
  
  // do some error checking
  if (info->InputVolumeScalarType != VTK_FLOAT &&
      info->InputVolumeScalarType != VTK_UNSIGNED_CHAR &&
      info->InputVolumeScalarType != VTK_SHORT )
    {
    info->SetProperty( info, VVP_ERROR,
      "The Fixed image must have pixel type floats, signed short or unsigned char.");
    return 1;
    }
  

  // do some error checking
  if (info->InputVolume2ScalarType != VTK_FLOAT &&
      info->InputVolume2ScalarType != VTK_UNSIGNED_CHAR &&
      info->InputVolume2ScalarType != VTK_SHORT )
    {
    info->SetProperty( info, VVP_ERROR,
      "The Moving image must have pixel type floats, signed short or unsigned char.");
    return 1;
    }
  
  if (info->InputVolumeNumberOfComponents  != 1 ||
      info->InputVolume2NumberOfComponents != 1)
    {
    info->SetProperty(
      info, VVP_ERROR, "The two input volumes must be single component.");
    return 1;
    }

  int result = 0;
  

  try 
    {
    switch( info->InputVolumeScalarType )
      {
      case VTK_UNSIGNED_CHAR:
        {
        switch( info->InputVolume2ScalarType )
          {
          case VTK_UNSIGNED_CHAR:
            {
            typedef VolView::PlugIn::MultimodalityRegistrationRigidBRunner<unsigned char,unsigned char> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_SHORT:
            {
            typedef VolView::PlugIn::MultimodalityRegistrationRigidBRunner<unsigned char,signed short> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_FLOAT:
            {
            typedef VolView::PlugIn::MultimodalityRegistrationRigidBRunner<unsigned char,float> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          }
        break;
        }
      case VTK_SHORT:
        {
        switch( info->InputVolume2ScalarType )
          {
          case VTK_UNSIGNED_CHAR:
            {
            typedef VolView::PlugIn::MultimodalityRegistrationRigidBRunner<signed short,unsigned char> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_SHORT:
            {
            typedef VolView::PlugIn::MultimodalityRegistrationRigidBRunner<signed short,signed short> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_FLOAT:
            {
            typedef VolView::PlugIn::MultimodalityRegistrationRigidBRunner<signed short,float> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          }
        break;
        }
      case VTK_FLOAT:
        {
        switch( info->InputVolume2ScalarType )
          {
          case VTK_UNSIGNED_CHAR:
            {
            typedef VolView::PlugIn::MultimodalityRegistrationRigidBRunner<float,unsigned char> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_SHORT:
            {
            typedef VolView::PlugIn::MultimodalityRegistrationRigidBRunner<float,signed short> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_FLOAT:
            {
            typedef VolView::PlugIn::MultimodalityRegistrationRigidBRunner<float,float> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          }
        break;
        }
      } // end of switch for InputVolumeScalarType
    }
  catch( itk::ExceptionObject & except )
    {
    info->SetProperty( info, VVP_ERROR, except.what() ); 
    return -1;
    }
  return result;
}


static int UpdateGUI(void *inf)
{
  char tmp[1024];
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "Rescale components");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_CHECKBOX);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT, "1");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP, "Enabling this option will rescale all components to the dynamic range of the first component. This provides the same window level settings for a blended output.");

  info->SetGUIProperty(info, 1, VVP_GUI_LABEL, "Use Landmarks");
  info->SetGUIProperty(info, 1, VVP_GUI_TYPE, VVP_GUI_CHOICE);
  info->SetGUIProperty(info, 1, VVP_GUI_DEFAULT , "Do not use Landmarks");
  info->SetGUIProperty(info, 1, VVP_GUI_HELP, "Do you want to use the Landmarks in order to align the two volumes before starting the registration ?.");
  info->SetGUIProperty(info, 1, VVP_GUI_HINTS, "2\nDo not use Landmarks\nYes, Use Landmarks");

  info->SetGUIProperty(info, 2, VVP_GUI_LABEL, "Quality");
  info->SetGUIProperty(info, 2, VVP_GUI_TYPE, VVP_GUI_CHOICE);
  info->SetGUIProperty(info, 2, VVP_GUI_DEFAULT , "Medium quality - takes short time");
  info->SetGUIProperty(info, 2, VVP_GUI_HELP, "Select your trade-off between registration quality and computation time. High quality registration requires longer computation times.");
  info->SetGUIProperty(info, 2, VVP_GUI_HINTS, "2\nMedium quality - takes short time\nHigh quality - takes long time");

  info->SetGUIProperty(info, 3, VVP_GUI_LABEL, "Multi-Resolution ");
  info->SetGUIProperty(info, 3, VVP_GUI_TYPE, VVP_GUI_CHOICE);
  info->SetGUIProperty(info, 3, VVP_GUI_DEFAULT , "One - Only Quarter resolution");
  info->SetGUIProperty(info, 3, VVP_GUI_HELP, "Select how many multi-resolutions levels to use. They always start from the coarsest which is downsampled to one quarter of resolution, followed by one half, and finishing on full resolution.");
  info->SetGUIProperty(info, 3, VVP_GUI_HINTS, "3\nOne - Only Quarter resolution\nTwo - Quarter and Half resolutions\nThree - Quarter, Half and Full resolutions");

  info->SetGUIProperty(info, 4, VVP_GUI_LABEL, "Output Format");
  info->SetGUIProperty(info, 4, VVP_GUI_TYPE, VVP_GUI_CHOICE);
  info->SetGUIProperty(info, 4, VVP_GUI_DEFAULT , "Append The Volumes");
  info->SetGUIProperty(info, 4, VVP_GUI_HELP, "How do you want the output stored? There are two choices here. Appending creates a single output volume that has two components, the first component from the input volume and the second component is from the registered second input. The second choice is to Relace the current volume. In this case the Registered second input replaces the original volume.");
  info->SetGUIProperty(info, 4, VVP_GUI_HINTS, "2\nAppend The Volumes\nReplace The Current Volume");

  info->OutputVolumeScalarType = info->InputVolumeScalarType;
  memcpy(info->OutputVolumeDimensions,info->InputVolumeDimensions,
         3*sizeof(int));
  memcpy(info->OutputVolumeSpacing,info->InputVolumeSpacing,
         3*sizeof(float));
  memcpy(info->OutputVolumeOrigin,info->InputVolumeOrigin,
         3*sizeof(float));

  // really the memory consumption is one copy of the resampled output for
  // the resample filter plus the gradient for the 1/8th res volume plus the
  // two 1/8th res resampled inputs
  sprintf(tmp,"%f",
          info->InputVolumeScalarSize + 3.0*sizeof(float)/8.0 + 0.5);
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED, tmp); 

  // what output format is selected
  const char *result = info->GetGUIProperty(info, 4, VVP_GUI_VALUE);
  if (result && !strcmp(result,"Append The Volumes"))
    {
    info->OutputVolumeNumberOfComponents = 
      info->InputVolumeNumberOfComponents + 
      info->InputVolume2NumberOfComponents;
    }
  else
    {
    info->OutputVolumeNumberOfComponents =
      info->InputVolume2NumberOfComponents;
    }
  
  return 1;
}


extern "C" {
  
void VV_PLUGIN_EXPORT vvITKMultimodalityRegistrationRigidBInit(vtkVVPluginInfo *info)
{
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI   = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Multimodality Registration: Rigid (I)");
  info->SetProperty(info, VVP_GROUP, "Registration");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
                            "Multimodality registration using Mattes Mutual Information and Rigid transform");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "This filter takes two volumes and registers them. There are two choices for the output format. Appending creates a single output volume that has two components, the first component is from the input volume and the second component is from the registered and resampled second input volume. The second choice is to Replace the current volume. In this case the registered and resampled second input replaces the original volume. The two input volumes must have one component and be of the same data type. The registration is done on quarter resolution volumes first (one quarter on each axis) and then if that converges the registration continues with one half resolution volumes. The optimization is done using a regular gradient descent optimizer with a centered quaternion and rigid transform based transform. The error metric is Mattes Mutual Information.");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "0");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "5");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,    "0"); 
  info->SetProperty(info, VVP_REQUIRES_SECOND_INPUT,        "1");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}

}
