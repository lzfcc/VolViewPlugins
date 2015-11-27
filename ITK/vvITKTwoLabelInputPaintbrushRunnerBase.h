/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef _vvITKTwoLabelInputPaintbrushRunnerBase_h
#define _vvITKTwoLabelInputPaintbrushRunnerBase_h

#include "vvITKPaintbrushRunnerBase.h"

template <class PixelType, class Pixel2Type, class LabelPixelType > 
class PaintbrushRunnerBaseTwoInputs : 
  public PaintbrushRunnerBase< PixelType, LabelPixelType >
{
public:
  // define our typedefs
  typedef PaintbrushRunnerBaseTwoInputs< 
      PixelType, Pixel2Type, LabelPixelType >         PaintbrushRunnerBaseTwoInputsType;
  typedef PaintbrushRunnerBase< 
      PixelType, LabelPixelType >                     PaintbrushRunnerBaseType;
  typedef typename PaintbrushRunnerBaseType::LabelImageType LabelImageType;
  typedef itk::Image< PixelType, 3 >                  Image1Type;
  typedef itk::Image< Pixel2Type, 3 >                 Image2Type;
  typedef itk::ImportImageFilter< Pixel2Type, 3>      ImportFilter2Type;
  typedef itk::ImageRegion<3>     RegionType;
  typedef itk::Index<3>           IndexType;
  typedef itk::Size<3>            SizeType;

  // Description:
  // The constructor
  PaintbrushRunnerBaseTwoInputs();

  // Description:
  // Imports the two input images from Volview into ITK
  virtual void ImportPixelBuffer( vtkVVPluginInfo *info, 
                                  const vtkVVProcessDataStruct * pds );

  // Description:
  // Sets up the pipeline and invokes the registration process
  virtual int Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds );

protected:

  // delare out instance variables
  typename ImportFilter2Type::Pointer      m_ImportFilter2;
};

// =======================================================================
// Constructor
template <class PixelType, class Pixel2Type, class LabelPixelType >
PaintbrushRunnerBaseTwoInputs<PixelType,Pixel2Type,LabelPixelType>
::PaintbrushRunnerBaseTwoInputs() 
{
  this->m_ImportFilter2 = ImportFilter2Type::New();
}

// =======================================================================
// Import data
template <class PixelType, class Pixel2Type, class LabelPixelType >
void PaintbrushRunnerBaseTwoInputs<PixelType,Pixel2Type,LabelPixelType>::
ImportPixelBuffer( vtkVVPluginInfo *info, const vtkVVProcessDataStruct * pds )
{
  PaintbrushRunnerBaseType::ImportPixelBuffer(info,pds);

  SizeType   size;
  IndexType  start;
  double origin[3], spacing[3];
  
  for(unsigned int i=0; i<3; i++)
    {
    size[i]     =  info->InputVolume2Dimensions[i];
    origin[i]   =  info->InputVolume2Origin[i];
    spacing[i]  =  info->InputVolume2Spacing[i];
    start[i]    =  0;
    }
  
  RegionType region(start, size);
  
  m_ImportFilter2->SetSpacing( spacing );
  m_ImportFilter2->SetOrigin(  origin  );
  m_ImportFilter2->SetRegion(  region  );
  
  Pixel2Type *dataBlockStart2 = static_cast< Pixel2Type * >( pds->inData2 );
  m_ImportFilter2->SetImportPointer( dataBlockStart2, 
                                    region.GetNumberOfPixels(), false);
  
  // Execute the filter
  m_ImportFilter2->Update();
} 


// =======================================================================
// Main execute method
template <class PixelType, class Pixel2Type, class LabelPixelType >
int PaintbrushRunnerBaseTwoInputs<PixelType,Pixel2Type,LabelPixelType>::
Execute( vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds )
{
  this->m_Info = info;
  this->ImportPixelBuffer( info, pds );  
  return 0;
}


#define vvITKPaintbrushRunnerExecuteTemplateMacro2( RunnerType, info, pds ) \
  int result = 0; \
   \
  try  \
    { \
    switch( info->InputVolumeScalarType ) \
      { \
      case VTK_UNSIGNED_CHAR: \
        { \
        switch( info->InputVolume2ScalarType ) \
          { \
          case VTK_UNSIGNED_CHAR: \
            { \
            RunnerType<unsigned char,unsigned char, unsigned char> runner; \
            result = runner.Execute( info, pds ); \
            break;  \
            } \
          case VTK_SHORT: \
            { \
            RunnerType<unsigned char,signed short, unsigned char> runner; \
            result = runner.Execute( info, pds ); \
            break;  \
            } \
          case VTK_UNSIGNED_INT: \
            { \
            RunnerType<unsigned char,unsigned int, unsigned char> runner; \
            result = runner.Execute( info, pds ); \
            break;  \
            } \
          case VTK_FLOAT: \
            { \
            RunnerType<unsigned char,float, unsigned char> runner; \
            result = runner.Execute( info, pds ); \
            break;  \
            } \
          } \
        break; \
        } \
      case VTK_SHORT: \
        { \
        switch( info->InputVolume2ScalarType ) \
          { \
          case VTK_UNSIGNED_CHAR: \
            { \
            RunnerType<signed short,unsigned char, unsigned char> runner; \
            result = runner.Execute( info, pds ); \
            break;  \
            } \
          case VTK_SHORT: \
            { \
            RunnerType<signed short,signed short, unsigned char> runner; \
            result = runner.Execute( info, pds ); \
            break;  \
            } \
          case VTK_FLOAT: \
            { \
            RunnerType<signed short,float, unsigned char> runner; \
            result = runner.Execute( info, pds ); \
            break;  \
            } \
          } \
        break; \
        } \
      case VTK_UNSIGNED_INT: \
        { \
        switch( info->InputVolume2ScalarType ) \
          { \
          case VTK_UNSIGNED_CHAR: \
            { \
            RunnerType<unsigned int,unsigned char, unsigned char> runner; \
            result = runner.Execute( info, pds ); \
            break;  \
            } \
          case VTK_SHORT: \
            { \
            RunnerType<unsigned int,signed short, unsigned char> runner; \
            result = runner.Execute( info, pds ); \
            break;  \
            } \
          case VTK_UNSIGNED_INT: \
            { \
            RunnerType<unsigned int,unsigned int, unsigned char> runner; \
            result = runner.Execute( info, pds ); \
            break;  \
            } \
          case VTK_FLOAT: \
            { \
            RunnerType<unsigned int,float, unsigned char> runner; \
            result = runner.Execute( info, pds ); \
            break;  \
            } \
          } \
        break; \
        } \
      case VTK_FLOAT: \
        { \
        switch( info->InputVolume2ScalarType ) \
          { \
          case VTK_UNSIGNED_CHAR: \
            { \
            RunnerType<float,unsigned char, unsigned char> runner; \
            result = runner.Execute( info, pds ); \
            break;  \
            } \
          case VTK_SHORT: \
            { \
            RunnerType<float,signed short, unsigned char> runner; \
            result = runner.Execute( info, pds ); \
            break;  \
            } \
          case VTK_FLOAT: \
            { \
            RunnerType<float,float, unsigned char> runner; \
            result = runner.Execute( info, pds ); \
            break;  \
            } \
          } \
        break; \
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
