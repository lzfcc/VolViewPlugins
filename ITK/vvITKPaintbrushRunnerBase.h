/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef _vvITKPaintbrushRunnerBase_h
#define _vvITKPaintbrushRunnerBase_h

#include "vtkVVPluginAPI.h"

#include "itkImage.h"
#include "itkImportImageFilter.h"
#include "itkImageRegionIterator.h"

#include <cstdio> // sprintf()

template <class PixelType, class LabelPixelType > class PaintbrushRunnerBase
{
public:
  // define our typedefs
  typedef PaintbrushRunnerBase< PixelType, LabelPixelType > PaintbrushRunnerBaseType;
  typedef itk::Image< PixelType, 3 >                  ImageType; 
  typedef itk::Image< LabelPixelType, 3 >             LabelImageType; 
  typedef itk::ImportImageFilter< PixelType, 3>       ImportFilterType;
  typedef itk::ImportImageFilter< LabelPixelType, 3>  LabelImportFilterType;
  typedef itk::ImageRegion<3>     RegionType;
  typedef itk::Index<3>           IndexType;
  typedef itk::Size<3>            SizeType;

  // Description:
  // The constructor
  PaintbrushRunnerBase();

  // Description:
  // Imports the two input images from Volview into ITK
  virtual void ImportPixelBuffer( vtkVVPluginInfo *info, 
                                  const vtkVVProcessDataStruct * pds );

  // Description:
  // Sets up the pipeline and invokes the registration process
  virtual int Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds );

protected:

  typedef itk::ImageRegionConstIterator< ImageType > ImageConstIteratorType;
  typedef itk::ImageRegionIterator< LabelImageType > LabelIteratorType;

  // delare out instance variables
  typename ImportFilterType::Pointer    m_ImportFilter;
  typename LabelImportFilterType::Pointer m_LabelImportFilter;
  vtkVVPluginInfo *m_Info;
};

// =======================================================================
// Constructor
template <class PixelType, class LabelPixelType> 
PaintbrushRunnerBase<PixelType,LabelPixelType>::PaintbrushRunnerBase() 
{
  m_ImportFilter  = ImportFilterType::New();
  m_LabelImportFilter = LabelImportFilterType::New();
  m_Info = NULL;
}

// =======================================================================
// Import data
template <class PixelType, class LabelPixelType> 
void PaintbrushRunnerBase<PixelType,LabelPixelType>::
ImportPixelBuffer( vtkVVPluginInfo *info, const vtkVVProcessDataStruct * pds )
{
  SizeType   size;
  IndexType  start;
  double origin[3], spacing[3];
  
  for(unsigned int i=0; i<3; i++)
    {
    size[i]     =  info->InputVolumeDimensions[i];
    origin[i]   =  info->InputVolumeOrigin[i];
    spacing[i]  =  info->InputVolumeSpacing[i];
    start[i]    =  0;
    }
  
  RegionType region(start, size);
  
  m_ImportFilter->SetSpacing( spacing );
  m_ImportFilter->SetOrigin(  origin  );
  m_ImportFilter->SetRegion(  region  );
  
  PixelType *dataBlockStart = static_cast< PixelType * >( pds->inData );
  m_ImportFilter->SetImportPointer( dataBlockStart, 
                                    region.GetNumberOfPixels(), false);
  
  m_LabelImportFilter->SetSpacing( spacing );
  m_LabelImportFilter->SetOrigin(  origin  );
  m_LabelImportFilter->SetRegion(  region  );
  
  LabelPixelType *labelDataBlockStart = static_cast< LabelPixelType * >( pds->inLabelData );
  m_LabelImportFilter->SetImportPointer( labelDataBlockStart, 
                                     region.GetNumberOfPixels(), false);

  // Execute the filter
  m_ImportFilter->Update();
  m_LabelImportFilter->Update();  
} 


// =======================================================================
// Main execute method
template <class PixelType, class LabelPixelType> 
int PaintbrushRunnerBase<PixelType,LabelPixelType>::
Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds )
{
  m_Info = info;
  this->ImportPixelBuffer( info, pds );  
  return 0;
}


#define vvITKPaintbrushRunnerExecuteTemplateMacro( RunnerType, info, pds ) \
  int result = 0; \
   \
  try  \
  { \
  switch( info->InputVolumeScalarType ) \
    { \
    case VTK_CHAR: \
      { \
      RunnerType<signed char, unsigned char> runner; \
      result = runner.Execute( info, pds ); \
      break;  \
      } \
    case VTK_UNSIGNED_CHAR: \
      { \
      RunnerType<unsigned char,unsigned char> runner; \
      result = runner.Execute( info, pds ); \
      break;  \
      } \
    case VTK_SHORT: \
      { \
      RunnerType<signed short,unsigned char> runner; \
      result = runner.Execute( info, pds ); \
      break;  \
      } \
    case VTK_UNSIGNED_SHORT: \
      { \
      RunnerType<unsigned short,unsigned char> runner; \
      result = runner.Execute( info, pds ); \
      break;  \
      } \
    case VTK_INT: \
      { \
      RunnerType<signed int,unsigned char> runner; \
      result = runner.Execute( info, pds ); \
      break;  \
      } \
    case VTK_UNSIGNED_INT: \
      { \
      RunnerType<unsigned int,unsigned char> runner; \
      result = runner.Execute( info, pds ); \
      break;  \
      } \
    case VTK_LONG: \
      { \
      RunnerType<signed long,unsigned char> runner; \
      result = runner.Execute( info, pds ); \
      break;  \
      } \
    case VTK_UNSIGNED_LONG: \
      { \
      RunnerType<unsigned long,unsigned char> runner; \
      result = runner.Execute( info, pds ); \
      break;  \
      } \
    case VTK_FLOAT: \
      { \
      RunnerType<float,unsigned char> runner; \
      result = runner.Execute( info, pds ); \
      break;  \
      } \
    } \
  } \
  catch( itk::ExceptionObject & except ) \
    { \
    info->SetProperty( info, VVP_ERROR, except.what() );  \
    return -1; \
    } \
  return result;

#endif

