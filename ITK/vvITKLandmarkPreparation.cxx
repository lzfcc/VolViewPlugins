/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* perform registration of PET-CT images using Mutual Information and a Rigid Transform */

#include "vtkVVPluginAPI.h"

#include "vvITKRegistrationBase.h"
#include "itkVersorRigid3DTransform.h"

namespace VolView {

namespace PlugIn {


// =======================================================================
// The main class definition
// =======================================================================
template<class TFixedPixelType, class TMovingPixelType>
class VV_PLUGIN_EXPORT LandmarkPreparationRunner :
 public RegistrationBaseRunner<
                           TFixedPixelType,
                           TMovingPixelType >
{
public:

  /** Standard class typedefs */
  typedef LandmarkPreparationRunner     Self;
  typedef RegistrationBaseRunner<TFixedPixelType,TMovingPixelType>   Superclass;
  typedef ::itk::SmartPointer<Self>          Pointer;
  typedef ::itk::SmartPointer<const Self>    ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(LandmarkPreparationRunner, RegistrationBaseRunner);

  itkStaticConstMacro( Dimension, unsigned int, 3 );

  typedef  typename Superclass::FixedPixelType   FixedPixelType;      // PET Image
  typedef  typename Superclass::MovingPixelType  MovingPixelType;     // CT  Image
  typedef  float                        InternalPixelType;
  
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

  typedef itk::LinearInterpolateImageFunction< 
                                MovingImageType,
                                double             > LinearInterpolatorType;

public:

  // Description:
  // Sets up the pipeline and invokes the registration process
  int Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds );


protected:

  // Command/Observer intended to update the progress
  typedef itk::MemberCommand< LandmarkPreparationRunner >  CommandType;

  // Description:
  // The funciton to call for progress of the optimizer
  void ProgressUpdate( itk::Object * caller, const itk::EventObject & event );

  // Description:
  // The constructor
  LandmarkPreparationRunner();

  // Description:
  // The destructor
  ~LandmarkPreparationRunner();

private:
  // declare out instance variables

};

  
  
// =======================================================================
// progress Callback
template<class TFixedPixelType, class TMovingPixelType>
void 
LandmarkPreparationRunner<TFixedPixelType,TMovingPixelType>::
ProgressUpdate( itk::Object * caller, const itk::EventObject & event )
{
  if( typeid( itk::ProgressEvent ) == typeid( event ) )
    {
    this->m_Info->UpdateProgress(this->m_Info,this->m_Resampler->GetProgress(), "Landmark based Resampling..."); 
    }
}

// =======================================================================
// Constructor
template<class TFixedPixelType, class TMovingPixelType>
LandmarkPreparationRunner<TFixedPixelType,TMovingPixelType>::
LandmarkPreparationRunner() 
{
}


// =======================================================================
// Destructor
template<class TFixedPixelType, class TMovingPixelType>
LandmarkPreparationRunner<TFixedPixelType,TMovingPixelType>::
~LandmarkPreparationRunner() 
{
}


// =======================================================================
// Main execute method
template<class TFixedPixelType, class TMovingPixelType>
int 
LandmarkPreparationRunner<TFixedPixelType,TMovingPixelType>::
Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds )
{
  this->m_Info = info;
  
  // The order in which these two methods are invoked is important:
  this->ImportPixelBuffer( this->m_Info, pds );  


  // do we use the Landmarks for aligning the volumes ?
  const char *result = this->m_Info->GetGUIProperty(this->m_Info, 1, VVP_GUI_VALUE);
  if (result && !strcmp(result,"Yes, Use Landmarks"))
    {
    this->m_Cout << "Computing Landmark-based transformation" << std::endl;
    this->ComputeTransformFromLandmarks();
    }
  else 
    {
    this->m_Cout << "Computing geometrically centered transformation" << std::endl;
    this->InitializeTransform();
    }


  this->m_Resampler->SetTransform( this->m_Transform );
  this->m_Resampler->SetInput( this->m_MovingImage );
  this->m_Resampler->SetSize( this->m_FixedImage->GetLargestPossibleRegion().GetSize());
  this->m_Resampler->SetOutputOrigin( this->m_FixedImage->GetOrigin() );
  this->m_Resampler->SetOutputSpacing( this->m_FixedImage->GetSpacing());
  this->m_Resampler->SetDefaultPixelValue(0);
  this->m_Resampler->UpdateLargestPossibleRegion();
  
  // set some output information,
  char results[2048];

  typedef TransformType::VersorType           VersorType;
  typedef TransformType::OutputVectorType     VectorType;
  typedef VersorType::VectorType              AxisType;
  typedef TransformType::InputPointType       CenterType;

  VersorType versor = this->m_Transform->GetVersor();
  AxisType   axis   = versor.GetAxis();
  CenterType center = this->m_Transform->GetCenter();
  VectorType translation = this->m_Transform->GetTranslation();

  sprintf(results,"Translation: %g %g %g\n"
                  "Rotation Axis: %f %f %f \n"
                  "Rotation Angle: %f\n"
				  "Rotation Center: %g %g %g", 
          translation[0],
          translation[1],
          translation[2],
          axis[0],
          axis[1],
          axis[2],
          versor.GetAngle(),
          center[0],
          center[1],
          center[2]
          );
  this->m_Info->SetProperty(this->m_Info, VVP_REPORT_TEXT, results);
  
  const char *result1 = info->GetGUIProperty(info, 2, VVP_GUI_VALUE);
  const bool appendImages = (result1 && !strcmp(result1,"Append The Volumes"));
  
  // Rescale components (only if we are appending volumes)?
  bool rescaleComponents = true;
  const int rescaleDynamicRangeForMerging = atoi( info->GetGUIProperty( 
    info, 0, VVP_GUI_VALUE));
  if( !rescaleDynamicRangeForMerging || !appendImages )  
    {
    rescaleComponents = false;
    }
  
  this->CopyOutputData( this->m_Info, pds, appendImages, rescaleComponents );
  
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
            typedef VolView::PlugIn::LandmarkPreparationRunner<unsigned char,unsigned char> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_SHORT:
            {
            typedef VolView::PlugIn::LandmarkPreparationRunner<unsigned char,signed short> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_FLOAT:
            {
            typedef VolView::PlugIn::LandmarkPreparationRunner<unsigned char,float> PreparationRunnerType;
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
            typedef VolView::PlugIn::LandmarkPreparationRunner<signed short,unsigned char> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_SHORT:
            {
            typedef VolView::PlugIn::LandmarkPreparationRunner<signed short,signed short> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_FLOAT:
            {
            typedef VolView::PlugIn::LandmarkPreparationRunner<signed short,float> PreparationRunnerType;
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
            typedef VolView::PlugIn::LandmarkPreparationRunner<float,unsigned char> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_SHORT:
            {
            typedef VolView::PlugIn::LandmarkPreparationRunner<float,signed short> PreparationRunnerType;
            PreparationRunnerType::Pointer runner = PreparationRunnerType::New();
            result = runner->Execute( info, pds );
            break; 
            }
          case VTK_FLOAT:
            {
            typedef VolView::PlugIn::LandmarkPreparationRunner<float,float> PreparationRunnerType;
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
  info->SetGUIProperty(info, 1, VVP_GUI_HELP,
                       "Do you want to use the Landmarks in order to align the two volumes ?.");
  info->SetGUIProperty(info, 1, VVP_GUI_HINTS, "2\nDo not use Landmarks\nYes, Use Landmarks");

  info->SetGUIProperty(info, 2, VVP_GUI_LABEL, "Output Format");
  info->SetGUIProperty(info, 2, VVP_GUI_TYPE, VVP_GUI_CHOICE);
  info->SetGUIProperty(info, 2, VVP_GUI_DEFAULT , "Append The Volumes");
  info->SetGUIProperty(info, 2, VVP_GUI_HELP, "How do you want the output stored? There are two choices here. Appending creates a single output volume that has two components, the first component from the input volume and the second component is from the registered second input. The second choice is to Relace the current volume. In this case the Registered second input replaces the original volume.");
  info->SetGUIProperty(info, 2, VVP_GUI_HINTS, "2\nAppend The Volumes\nReplace The Current Volume");


  info->OutputVolumeScalarType = info->InputVolumeScalarType;

  memcpy(info->OutputVolumeDimensions,info->InputVolumeDimensions,
         3*sizeof(int));
  memcpy(info->OutputVolumeSpacing,info->InputVolumeSpacing,
         3*sizeof(float));
  memcpy(info->OutputVolumeOrigin,info->InputVolumeOrigin,
         3*sizeof(float));

  // memory consumption is double of the fixed image, because both images
  // are merged at the end.
  sprintf(tmp,"%f", 2 * info->InputVolumeScalarSize);
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED, tmp); 

  //
  // what output format is selected
  const char *result = info->GetGUIProperty(info, 2, VVP_GUI_VALUE);
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
  
void VV_PLUGIN_EXPORT vvITKLandmarkPreparationInit(vtkVVPluginInfo *info)
{
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI   = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Landmark Preparation");
  info->SetProperty(info, VVP_GROUP, "Registration");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
                            "Align fixed and moving images using landmarks");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "This filter takes two volumes and merge them. The volumes don't have to be of same dimensions. They are expected however to have some overlapping region in physical space.");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "0");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "3");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,    "0"); 
  info->SetProperty(info, VVP_REQUIRES_SECOND_INPUT,        "1");
}

}
