/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* Computes the histogram of an Image */

#include "vvITKFilterModule.h"

#include "itkUnaryFunctorImageFilter.h"

namespace itk {
/** 
 * Creat a NULL filter using the UnaryFunctorImageFilter
 */
namespace Function 
{
  template< class TValue>
  class Null
  {
  public:
    Null() {}
    ~Null() {}
    inline TValue operator()( const TValue & A )
    { return A; }
  }; 
}


template <class TImage>
class ITK_EXPORT NullImageFilter :
    public UnaryFunctorImageFilter<TImage,TImage, 
          Function::Null< typename TImage::PixelType> >
{
public:
  /** Standard class typedefs. */
  typedef NullImageFilter  Self;
  typedef UnaryFunctorImageFilter<TImage,TImage, 
             Function::Null< typename TImage::PixelType> >  Superclass;
  typedef SmartPointer<Self>   Pointer;
  typedef SmartPointer<const Self>  ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);
  
protected:
  NullImageFilter() {}
  virtual ~NullImageFilter() {}

private:
  NullImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

};
} // end of itk namespace


template <class InputPixelType>
class HistogramPlotRunner
  {

    public:
        typedef  InputPixelType                       PixelType;
        typedef  itk::Image< PixelType, 3 >           ImageType; 
        typedef  itk::NullImageFilter< ImageType >    FilterType;
        typedef  VolView::PlugIn::FilterModule< FilterType >   ModuleType;

    public:
      HistogramPlotRunner() {}
      void Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds )
      {
        ModuleType  module;
        module.SetPluginInfo( info );
        module.SetUpdateMessage("Computing intensities with a HistogramPlot...");
        // Execute the filter
        module.ProcessData( pds  );

        double * xy = static_cast<double *>(pds->outDataPlotting);
        const int nc = info->OutputPlottingNumberOfColumns;
        const int nr = info->OutputPlottingNumberOfRows;
        const double factor = (atan(1.0)*4.0) * (2.0 / nr);
        int x=0;
        // Set the values of the independent variable
        for(x=0; x<nr; x++)
          {
          *xy = x;
          xy++;
          }
        // Set the values of the first series array
        for(x=0; x<nr; x++)
          {
          *xy = sin(x*factor);
          xy++;
          }
        // Set the values of the second series array
        for(x=0; x<nr; x++)
          {
          *xy = cos(x*factor);
          xy++;
          }
      }
    };



static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{

  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  try 
  {
  switch( info->InputVolumeScalarType )
    {
    case VTK_CHAR:
      {
      HistogramPlotRunner<signed char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_CHAR:
      {
      HistogramPlotRunner<unsigned char> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_SHORT:
      {
      HistogramPlotRunner<signed short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_SHORT:
      {
      HistogramPlotRunner<unsigned short> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_INT:
      {
      HistogramPlotRunner<signed int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_INT:
      {
      HistogramPlotRunner<unsigned int> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_LONG:
      {
      HistogramPlotRunner<signed long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_UNSIGNED_LONG:
      {
      HistogramPlotRunner<unsigned long> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_FLOAT:
      {
      HistogramPlotRunner<float> runner;
      runner.Execute( info, pds );
      break; 
      }
    case VTK_DOUBLE:
      {
      HistogramPlotRunner<double> runner;
      runner.Execute( info, pds );
      break; 
      }
    }
  }
  catch( itk::ExceptionObject & except )
  {
    info->SetProperty( info, VVP_ERROR, except.what() ); 
    return -1;
  }
  return 0;
}




static int UpdateGUI(void *inf)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP, "0");
  
  info->OutputVolumeScalarType = info->InputVolumeScalarType;
  info->OutputVolumeNumberOfComponents = 
    info->InputVolumeNumberOfComponents;
  memcpy(info->OutputVolumeDimensions,info->InputVolumeDimensions,
         3*sizeof(int));
  memcpy(info->OutputVolumeSpacing,info->InputVolumeSpacing,
         3*sizeof(float));
  memcpy(info->OutputVolumeOrigin,info->InputVolumeOrigin,
         3*sizeof(float));

  // Using arbitrary numbers here, just for testing.
  info->OutputPlottingNumberOfRows = 256;
  info->OutputPlottingNumberOfColumns = 2;

  return 1;
}


extern "C" {
  
void VV_PLUGIN_EXPORT vvITKHistogramPlotInit(vtkVVPluginInfo *info)
{
  vvPluginVersionCheck();

  // setup information that never changes
  info->ProcessData = ProcessData;
  info->UpdateGUI   = UpdateGUI;
  info->SetProperty(info, VVP_NAME, "Histogram Plot (ITK)");
  info->SetProperty(info, VVP_GROUP, "Plotting");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
                            "Plots the image histogram");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION,
    "This filter displays the image histogram at a given number of bins");
  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "1");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "0");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "0");
  info->SetProperty(info, VVP_REQUIRED_Z_OVERLAP,           "0");
  info->SetProperty(info, VVP_PER_VOXEL_MEMORY_REQUIRED,    "0"); 
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "1");
  info->SetProperty(info, VVP_PLOTTING_X_AXIS_TITLE, "Image intensity");
  info->SetProperty(info, VVP_PLOTTING_Y_AXIS_TITLE, "Frequency");
}

}
