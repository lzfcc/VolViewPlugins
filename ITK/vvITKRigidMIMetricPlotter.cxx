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

#include "vvITKFilterModuleSeriesInput.h"
#include "vvITKRegistrationBase.h"

#include "itkImageRegistrationMethod.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkMutualInformationHistogramImageToImageMetric.h"
#include "itkVersorRigid3DTransform.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkExhaustiveOptimizer.h"
#include "itkImageFileWriter.h"



namespace VolView {

namespace PlugIn {


// =======================================================================
// The main class definition
// =======================================================================
template<class TFixedPixelType, class TMovingPixelType>
class RigidMIMetricPlotter : 
  public RegistrationBaseRunner<TFixedPixelType,TMovingPixelType >
{
public:

  /** Standard class typedefs */
  typedef RigidMIMetricPlotter            Self;
  typedef RegistrationBaseRunner<
           TFixedPixelType,TMovingPixelType> Superclass;
  typedef ::itk::SmartPointer<Self>          Pointer;
  typedef ::itk::SmartPointer<const Self>    ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(RigidMIMetricPlotter, RegistrationBaseRunner);

  itkStaticConstMacro( Dimension, unsigned int, 3 );

  typedef  typename Superclass::FixedPixelType     FixedPixelType;      // PET Image
  typedef  typename Superclass::MovingPixelType    MovingPixelType;     // CT  Image
  typedef  unsigned char                           InternalPixelType;
  
private:

  typedef itk::Image< FixedPixelType,    Dimension >   FixedImageType;
  typedef itk::Image< MovingPixelType,   Dimension >   MovingImageType;
  typedef itk::Image< InternalPixelType, Dimension >   InternalImageType;

  typedef typename FixedImageType::SizeType            SizeType;
  typedef typename FixedImageType::IndexType           IndexType;
  typedef typename FixedImageType::RegionType          RegionType;

  typedef itk::RescaleIntensityImageFilter< 
                           FixedImageType,
                           InternalImageType >    FixedNormalizeFilterType;

  typedef itk::RescaleIntensityImageFilter< 
                           MovingImageType,
                           InternalImageType >    MovingNormalizeFilterType;

  typedef itk::ResampleImageFilter< 
                           InternalImageType, 
                           InternalImageType >    ResampleFilterType;

  
  typedef itk::VersorRigid3DTransform< double >   TransformType;
 
  typedef TransformType::ParametersType           ParametersType;

  typedef itk::ExhaustiveOptimizer                OptimizerType;
  typedef OptimizerType::ParametersType           GridNodeType; 

  typedef itk::LinearInterpolateImageFunction< 
                                InternalImageType,
                                double             > LinearInterpolatorType;

  typedef itk::NearestNeighborInterpolateImageFunction< 
                                InternalImageType,
                                double             > NearestNeighborInterpolatorType;


  typedef itk::MutualInformationHistogramImageToImageMetric< 
                                InternalImageType, 
                                InternalImageType >    MutualInformationMetricType;


  typedef MutualInformationMetricType                MetricType;                                          
                                          

  typedef OptimizerType::ScalesType                  OptimizerScalesType;


  typedef itk::ImageRegistrationMethod< 
                                   InternalImageType, 
                                   InternalImageType    > RegistrationType;

  typedef typename MetricType::MeasureType         MeasureType;
  typedef itk::Image< MeasureType,      Dimension >  MetricImageType;
  typedef itk::ImageRegionIterator< MetricImageType > MetricIteratorType;


public:

  // Description:
  // Sets up the pipeline and invokes the registration process
  int Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds );


protected:

  // Command/Observer intended to update the progress
  typedef itk::MemberCommand< RigidMIMetricPlotter >  CommandType;

  // Description:
  // The funciton to call for progress of the optimizer
  void ProgressUpdate( itk::Object * caller, const itk::EventObject & event );

  // Description:
  // The constructor
  RigidMIMetricPlotter();

  // Description:
  // The destructor
  ~RigidMIMetricPlotter();

  void SetInitialTransform( const TransformType * transform );

  void InitializeRegistration();

  void RegisterCurrentResolutionLevel();

  const ParametersType & GetParameters() const;

  const TransformType * GetTransform() const;

  void PrepareLevel();


private:
  // declare out instance variables

  typename FixedNormalizeFilterType::Pointer     m_FixedNormalizer;
  typename MovingNormalizeFilterType::Pointer    m_MovingNormalizer;

  typename ResampleFilterType::Pointer           m_FixedResampler;
  typename ResampleFilterType::Pointer           m_MovingResampler;

  typename OptimizerType::Pointer                m_Optimizer;

  typename MetricType::Pointer                   m_Metric;

  typename LinearInterpolatorType::Pointer       m_InternalLinearInterpolator;

  typename NearestNeighborInterpolatorType::Pointer       m_InternalNearestNeighborInterpolator;

  typename RegistrationType::Pointer             m_RegistrationMethod;

  MovingPixelType                                m_BackgroundLevel;

  unsigned int                                   m_Level;
  
  bool                                           m_ReportTimers;

  std::vector<unsigned int>                      m_LevelFactor;

  bool                                           m_OperationCancelled;

  unsigned long                                  m_CurrentIteration;

  typedef std::ofstream                          OstreamType;
  OstreamType                                    m_CoutMetric;

  double                                         m_StepLength;
  
  OptimizerType::StepsType                        m_NumberOfMetricSteps;

  typename MetricImageType::Pointer              m_MetricImage;
};

  
  
// =======================================================================
// progress Callback
template<class TFixedPixelType, class TMovingPixelType>
void 
RigidMIMetricPlotter<TFixedPixelType,TMovingPixelType>::
ProgressUpdate( itk::Object * caller, const itk::EventObject & event )
{

  char tstr[1024];
  if( itk::IterationEvent().CheckEvent(& event ) )
    {

    // Test whether during the GUI update, the Abort button was pressed
    int abort = atoi( this->m_Info->GetProperty( this->m_Info, VVP_ABORT_PROCESSING ) );
    if( abort )
      {
      // The Amoeba vnl optimizer doesn't have a way to be stopped..
      this->m_OperationCancelled = true;
      return; // nothing left to do at this level
      }


    // Present information on the GUI according to the type of optimizer
    // currently in use.
    this->m_CoutMetric << this->m_Level << " ";
    this->m_CoutMetric << this->m_CurrentIteration << " ";
    this->m_CoutMetric << this->m_Optimizer->GetCurrentValue() << " ";
    this->m_CoutMetric << this->m_Optimizer->GetCurrentPosition() << std::endl;

    typedef typename MetricImageType::IndexType     IndexType;
    typedef typename IndexType::IndexValueType      IndexValueType;
    
    IndexType start;

    switch( this->m_Level )
      {
      case 0: 
        {
        sprintf(tstr,"Evaluating metric at Quarter Resolution: Value: %g Position: %g %g %g", 
                  this->m_Optimizer->GetCurrentValue(),
                  this->m_Optimizer->GetCurrentPosition()[3], 
                  this->m_Optimizer->GetCurrentPosition()[4], 
                  this->m_Optimizer->GetCurrentPosition()[5] );

          GridNodeType gridNode = this->m_Optimizer->GetCurrentIndex();
          start[0] = static_cast<IndexValueType>(gridNode[3]);
          start[1] = static_cast<IndexValueType>(gridNode[4]);
          start[2] = static_cast<IndexValueType>(gridNode[5]);
          this->m_MetricImage->SetPixel( start, this->m_Optimizer->GetCurrentValue());
          break;
          }
      case 1:
          {
          sprintf(tstr,"Half Resolution Iteration : Value: %g Position: %g %g %g",
                  this->m_Optimizer->GetCurrentValue(),
                  this->m_Optimizer->GetCurrentPosition()[3], 
                  this->m_Optimizer->GetCurrentPosition()[4], 
                  this->m_Optimizer->GetCurrentPosition()[5] );

          GridNodeType gridNode = this->m_Optimizer->GetCurrentIndex();
          start[0] = static_cast<IndexValueType>(gridNode[3]);
          start[1] = static_cast<IndexValueType>(gridNode[4]);
          start[2] = static_cast<IndexValueType>(gridNode[5]);
          this->m_MetricImage->SetPixel( start, this->m_Optimizer->GetCurrentValue());
          break;
          }
      case 2:
          {
          sprintf(tstr,"Full Resolution Iteration : Value: %g Position: %g %g %g",
                  this->m_Optimizer->GetCurrentValue(),
                  this->m_Optimizer->GetCurrentPosition()[3], 
                  this->m_Optimizer->GetCurrentPosition()[4], 
                  this->m_Optimizer->GetCurrentPosition()[5] );

          GridNodeType gridNode = this->m_Optimizer->GetCurrentIndex();
          start[0] = static_cast<IndexValueType>(gridNode[3]);
          start[1] = static_cast<IndexValueType>(gridNode[4]);
          start[2] = static_cast<IndexValueType>(gridNode[5]);
          this->m_MetricImage->SetPixel( start, this->m_Optimizer->GetCurrentValue());
          break;
          }
      }

      const float progress =  0.9 * this->m_CurrentIteration /
                                    this->m_Optimizer->GetMaximumNumberOfIterations();

      this->m_Info->UpdateProgress(this->m_Info, progress, tstr); 


      this->m_CurrentIteration++;

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

    return;
    }
}

// =======================================================================
// Constructor
template<class TFixedPixelType, class TMovingPixelType>
RigidMIMetricPlotter<TFixedPixelType,TMovingPixelType>::
RigidMIMetricPlotter() 
{
  this->m_CoutMetric.open("Metric.log");
  this->m_CoutMetric << 
    "ResolutionLevel Iteration MetricValue VersorTransformParamters (3 rotation + 3 translation)" 
                      << std::endl;
  
  this->m_FixedNormalizer  = FixedNormalizeFilterType::New();
  this->m_MovingNormalizer = MovingNormalizeFilterType::New();

  // Given that we are using 'unsigned char' as internal pixel type
  this->m_FixedNormalizer->SetOutputMinimum(   0 ); 
  this->m_FixedNormalizer->SetOutputMaximum( 255 );
  this->m_MovingNormalizer->SetOutputMinimum(   0 ); 
  this->m_MovingNormalizer->SetOutputMaximum( 255 );

  this->m_FixedResampler   = ResampleFilterType::New();
  this->m_MovingResampler  = ResampleFilterType::New();

  this->m_InternalLinearInterpolator = LinearInterpolatorType::New();

  this->m_InternalNearestNeighborInterpolator = NearestNeighborInterpolatorType::New();

  this->m_Metric = MetricType::New();

  this->m_Optimizer = OptimizerType::New();

  this->m_RegistrationMethod = RegistrationType::New();

  typedef typename MetricType::HistogramSizeType    HistogramSizeType;

  HistogramSizeType  histogramSize;

  histogramSize[0] = 256;
  histogramSize[1] = 256;

  this->m_Metric->SetHistogramSize( histogramSize );

  // The Amoeba optimizer doesn't need the cost function to compute derivatives
  this->m_Metric->ComputeGradientOff();

  this->m_RegistrationMethod->SetMetric( this->m_Metric );
  this->m_RegistrationMethod->SetTransform( this->m_Transform );

  this->m_RegistrationMethod->SetInterpolator( this->m_InternalLinearInterpolator );
//  this->m_RegistrationMethod->SetInterpolator( this->m_InternalNearestNeighborInterpolator );


  this->m_RegistrationMethod->SetOptimizer( this->m_Optimizer );

  this->m_Optimizer->AddObserver( itk::IterationEvent(), this->m_CommandObserver );

  this->m_Level = 0;
  this->m_OperationCancelled = false;
  this->m_CurrentIteration = 0;

  this->m_MetricImage         = MetricImageType::New();
}


// =======================================================================
// Destructor
template<class TFixedPixelType, class TMovingPixelType>
RigidMIMetricPlotter<TFixedPixelType,TMovingPixelType>::
~RigidMIMetricPlotter() 
{

}

 


// =======================================================================
//  Prepare the subsampled images.
template<class TFixedPixelType, class TMovingPixelType>
void 
RigidMIMetricPlotter<TFixedPixelType,TMovingPixelType>::
PrepareLevel()
{
  double factor = this->m_LevelFactor[ this->m_Level ]; 

  if( this->m_Level < 2 )  // Levels 0 and 1 require resampling.
    {
    this->m_Cout << "Level " << this->m_Level << "Using resampled images at factor " << factor << std::endl;
    typename InternalImageType::SpacingType spacing;
    typename InternalImageType::RegionType  region;
    typename InternalImageType::SizeType    size;
    typedef typename InternalImageType::SizeType::SizeValueType    SizeValueType;

    // Sub-sample the Fixed image
    this->m_FixedResampler->SetInput( this->m_FixedNormalizer->GetOutput() );

    spacing = this->m_FixedImage->GetSpacing();
    region  = this->m_FixedImage->GetLargestPossibleRegion();
    size    = region.GetSize();

    OptimizerType::StepsType step( this->m_Transform->GetNumberOfParameters() );
    for(unsigned int i=0; i<Dimension; i++)
      {
      spacing[i] *= factor;
      size[i]    = static_cast< SizeValueType >( size[i] / factor );
      step[i] = static_cast< unsigned long >(this->m_NumberOfMetricSteps[i]/factor);
      }
    //this->m_Optimizer->SetNumberOfSteps( step );

    double stepLength = this->m_StepLength * factor;

    //this->m_Optimizer->SetStepLength( stepLength );

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
    }
  else  // Level 2 can use the images directly
    {
    this->m_Cout << "Level " << this->m_Level << " Using images directly from the Normalizer filters, without any resampling" << std::endl;
    this->m_FixedNormalizer->UpdateLargestPossibleRegion();
    this->m_MovingNormalizer->UpdateLargestPossibleRegion();
    this->m_RegistrationMethod->SetFixedImage( this->m_FixedNormalizer->GetOutput() );
    this->m_RegistrationMethod->SetMovingImage( this->m_MovingNormalizer->GetOutput() );
    }


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
RigidMIMetricPlotter<TFixedPixelType,TMovingPixelType>::
InitializeRegistration()
{



  // do we use the Landmarks for aligning the volumes ?
  const char *result = this->m_Info->GetGUIProperty( this->m_Info, 0, VVP_GUI_VALUE );
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

  typename InternalImageType::SizeType    fixedImageSize     = this->m_FixedImage->GetBufferedRegion().GetSize();
  typename InternalImageType::SpacingType fixedImageSpacing  = this->m_FixedImage->GetSpacing();
  typename InternalImageType::PointType   fixedImageOrigin   = this->m_FixedImage->GetOrigin();

  typedef OptimizerType::ScalesType       OptimizerScalesType;

  const unsigned int numberOfParameters = this->m_Transform->GetNumberOfParameters();

  OptimizerScalesType optimizerScales( numberOfParameters );

  optimizerScales[0] = 0; // scale for rotation
  optimizerScales[1] = 0; // scale for rotation
  optimizerScales[2] = 0; // scale for rotation
  optimizerScales[3] = 1;
  optimizerScales[4] = 1;
  optimizerScales[5] = 1;


  this->m_Optimizer->SetScales( optimizerScales );

  // Do not consider any pixels with levels at zero. This is used for
  // clampling the intensities of the fixed images. It is equivalent
  // to masking that FixedImage with a threshold at this level.
  this->m_Metric->SetPaddingValue( itk::NumericTraits< InternalPixelType >::Zero );
  this->m_Metric->SetUsePaddingValue(true);

  typename RegistrationType::ParametersType initialParameters = 
                                    this->m_Transform->GetParameters();      

  this->m_Cout << "Initial Transform " << std::endl;
  this->m_Transform->Print( this->m_Cout );

  this->m_RegistrationMethod->SetInitialTransformParameters( initialParameters );



  typename MetricImageType::SizeType   size;
  typename MetricImageType::IndexType  start;
  typename MetricImageType::RegionType region;

  size[0] = this->m_Optimizer->GetNumberOfSteps()[3]*2+1;
  size[1] = this->m_Optimizer->GetNumberOfSteps()[4]*2+1;
  size[2] = this->m_Optimizer->GetNumberOfSteps()[5]*2+1;
  
  start[0] = 0;
  start[1] = 0;
  start[2] = 0;

  region.SetIndex( start );
  region.SetSize( size );

  this->m_MetricImage->SetRegions( region );
  this->m_MetricImage->Allocate();

}


 
// =======================================================================
//  Register one of the resolution levels.
template<class TFixedPixelType, class TMovingPixelType>
void 
RigidMIMetricPlotter<TFixedPixelType,TMovingPixelType>::
RegisterCurrentResolutionLevel()
{
  const unsigned int numberOfResolutionLevels = 3;


  this->PrepareLevel();
      
  this->m_RegistrationMethod->SetInitialTransformParameters( 
                                  this->m_Transform->GetParameters() ); 

  this->m_RegistrationMethod->StartRegistration(); 

  this->m_Optimizer->InvokeEvent( itk::IterationEvent() );

  this->m_Level++;

}


// =======================================================================
// Main execute method
template<class TFixedPixelType, class TMovingPixelType>
int 
RigidMIMetricPlotter<TFixedPixelType,TMovingPixelType>::
Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds )
{
  this->m_OperationCancelled = false;

  this->m_Info = info;
  
  this->ImportPixelBuffer( info, pds );  



  OptimizerType::StepsType steps( this->m_Transform->GetNumberOfParameters() );
  steps[1]=0;
  steps[2]=0;
  steps[3]=0;
  steps[3]=atoi( info->GetGUIProperty(info, 1, VVP_GUI_VALUE ));
  steps[4]=atoi( info->GetGUIProperty(info, 2, VVP_GUI_VALUE ));
  steps[5]=atoi( info->GetGUIProperty(info, 3, VVP_GUI_VALUE ));
  this->m_Optimizer->SetNumberOfSteps(steps);
  
  this->m_NumberOfMetricSteps.SetSize(this->m_Transform->GetNumberOfParameters() );
  for( unsigned int i=0; i<this->m_Transform->GetNumberOfParameters(); i++)
    {
    this->m_NumberOfMetricSteps[i] = steps[i];
    }

  this->m_Optimizer->SetStepLength(static_cast<double>(atof(
         info->GetGUIProperty(info, 4, VVP_GUI_VALUE ))));
  this->m_StepLength = static_cast<double>(atof(
         info->GetGUIProperty(info, 4, VVP_GUI_VALUE )));

  // Select how many resolutions levels to use for the registration 
  const char *multiResolutionLevels = info->GetGUIProperty(info, 5, VVP_GUI_VALUE);
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

  typename OptimizerType::ParametersType finalParameters = 
    this->m_Optimizer->GetMaximumMetricValuePosition();

  
  // set some output information,
  char results[1024];

  typedef TransformType::VersorType VersorType;
  VersorType versor = this->m_Transform->GetVersor();
  TransformType::OffsetType offset = this->m_Transform->GetOffset();
  typedef VersorType::VectorType   AxisType;
  AxisType axis = versor.GetAxis();

  sprintf(results,"Number of Metric Evaluations: %d\nMetric peaked at: %g %g %g\nRotation Axis %f %f %f %f\nOffset: %g %g %g. Trace written to Metric.log", 
          this->m_CurrentIteration,
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



  //Write metric image
  typedef itk::ImageFileWriter< MetricImageType > MetricImageWriterType;
  typename MetricImageWriterType::Pointer writer = MetricImageWriterType::New();
  writer->SetInput( this->m_MetricImage );
  writer->SetFileName("Metric.mhd");
  writer->Update();

  
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
            typedef VolView::PlugIn::RigidMIMetricPlotter<unsigned char,unsigned char> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_SHORT:
            {
            typedef VolView::PlugIn::RigidMIMetricPlotter<unsigned char,signed short> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_FLOAT:
            {
            typedef VolView::PlugIn::RigidMIMetricPlotter<unsigned char,float> PreparationRunnerType;
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
            typedef VolView::PlugIn::RigidMIMetricPlotter<signed short,unsigned char> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_SHORT:
            {
            typedef VolView::PlugIn::RigidMIMetricPlotter<signed short,signed short> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_FLOAT:
            {
            typedef VolView::PlugIn::RigidMIMetricPlotter<signed short,float> PreparationRunnerType;
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
            typedef VolView::PlugIn::RigidMIMetricPlotter<float,unsigned char> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_SHORT:
            {
            typedef VolView::PlugIn::RigidMIMetricPlotter<float,signed short> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_FLOAT:
            {
            typedef VolView::PlugIn::RigidMIMetricPlotter<float,float> PreparationRunnerType;
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

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "Use Landmarks");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_CHOICE);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT , "Do not use Landmarks");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP, "Do you want to use the Landmarks in order to align the two volumes before evaluating metric.");
  info->SetGUIProperty(info, 0, VVP_GUI_HINTS, "2\nDo not use Landmarks\nYes, Use Landmarks");

  info->SetGUIProperty(info, 1, VVP_GUI_LABEL, "Number of steps along X");
  info->SetGUIProperty(info, 1, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 1, VVP_GUI_DEFAULT , "5");
  info->SetGUIProperty(info, 1, VVP_GUI_HELP, "Select number of steps metric will move away from the initial position along x,y and z.");
  info->SetGUIProperty(info, 1, VVP_GUI_HINTS, "0 200 1");

  info->SetGUIProperty(info, 2, VVP_GUI_LABEL, "Number of steps along Y");
  info->SetGUIProperty(info, 2, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 2, VVP_GUI_DEFAULT , "5");
  info->SetGUIProperty(info, 2, VVP_GUI_HELP, "Select number of steps metric will move away from the initial position along x,y and z.");
  info->SetGUIProperty(info, 2, VVP_GUI_HINTS, "0 200 1");

  info->SetGUIProperty(info, 3, VVP_GUI_LABEL, "Number of steps along Z");
  info->SetGUIProperty(info, 3, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 3, VVP_GUI_DEFAULT , "0");
  info->SetGUIProperty(info, 3, VVP_GUI_HELP, "Select number of steps metric will move away from the initial position along x,y and z.");
  info->SetGUIProperty(info, 3, VVP_GUI_HINTS, "0 200 1");

  info->SetGUIProperty(info, 5, VVP_GUI_LABEL, "Multi-Resolution ");
  info->SetGUIProperty(info, 5, VVP_GUI_TYPE, VVP_GUI_CHOICE);
  info->SetGUIProperty(info, 5, VVP_GUI_DEFAULT , "One - Only Quarter resolution");
  info->SetGUIProperty(info, 5, VVP_GUI_HELP, "Select how many multi-resolutions levels to use. They always start from the coarsest which is downsampled to one quarter of resolution, followed by one half, and finishing on full resolution.");
  info->SetGUIProperty(info, 5, VVP_GUI_HINTS, "3\nOne - Only Quarter resolution\nTwo - Quarter and Half resolutions\nThree - Quarter, Half and Full resolutions");

  static char inpVolSpacingStr[30];
  sprintf(inpVolSpacingStr, "%g", info->InputVolumeSpacing[0]);
  const double temp = info->InputVolumeSpacing[0] * 10;
  static char inpVolSpacingMaxStr[250];
  sprintf(inpVolSpacingMaxStr, "%g %g %g",  0 , temp, info->InputVolumeSpacing[0]/10 );
  info->SetGUIProperty(info, 4, VVP_GUI_LABEL, "Step size");
  info->SetGUIProperty(info, 4, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 4, VVP_GUI_DEFAULT , ".25");
  info->SetGUIProperty(info, 4, VVP_GUI_HELP, "How big should each step be ?");
  info->SetGUIProperty(info, 4, VVP_GUI_HINTS, "0 20 .05");

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
  const char *result = info->GetGUIProperty(info, 3, VVP_GUI_VALUE);
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
  
void VV_PLUGIN_EXPORT vvITKRigidMIMetricPlotterInit(vtkVVPluginInfo *info)
{
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI   = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Multimodality registration : Metric Plotter");
  info->SetProperty(info, VVP_GROUP, "Registration");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
                            "Helper plugin to plot the metric.");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
                            "You may choose to plot the metric at any desired resolution level. By varying the size of the metric along X, Y and Z directions, you can very the number of metric evaluations on either side of the initial position. This initial position may be chosen by the landmark plugin. By default it is assumed to be geometrical center of the two images. By varying the step size, you may vary the metric spacing. (Metric: Mutual Information (Collignon))");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "0");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "6");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,    "0"); 
  info->SetProperty(info, VVP_REQUIRES_SECOND_INPUT,        "1");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}

}


