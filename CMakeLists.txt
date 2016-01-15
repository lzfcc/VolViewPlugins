##=========================================================================
## 
##   Copyright (c) Kitware, Inc.
##   All rights reserved.
##   See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.
## 
##      This software is distributed WITHOUT ANY WARRANTY; without even
##      the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##      PURPOSE.  See the above copyright notice for more information.
## 
##=========================================================================
# additional plugins, 
#
# checkerboard (done), register volumes, remove component, grid source
# 
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

PROJECT (VV_PLUGINS)

# add the plugins 

# if we are not in the volview tree then include the following
IF (NOT KWVolView_SOURCE_DIR)
  # include module to find vtk
  INCLUDE (${CMAKE_ROOT}/Modules/FindVTK.cmake)
  # Find the VTK source directory.
  FIND_PATH(VTK_SOURCE_DIR VTKConfig.cmake.in
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereSource1]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereSource2]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereSource3]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereSource4]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereSource5]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereSource6]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereSource7]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereSource8]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereSource9]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereSource10]
    ../VTK
    )
  
  # if vtk was found include the use vtk file
  IF (USE_VTK_FILE)
    INCLUDE(${USE_VTK_FILE})
  ENDIF (USE_VTK_FILE)
ENDIF (NOT KWVolView_SOURCE_DIR)
    
# Probably could be cleaned up... c plugins

ADD_LIBRARY(vvBoundary MODULE C/vvBoundary.c)
ADD_LIBRARY(vvComponentArithmetic MODULE C/vvComponentArithmetic.cxx)
ADD_LIBRARY(vvCrop MODULE C/vvCrop.c)
ADD_LIBRARY(vvMergeVolumes MODULE C/vvMergeVolumes.cxx)
ADD_LIBRARY(vvThreshold MODULE C/vvThreshold.cxx)
ADD_LIBRARY(vvPixelMath MODULE C/vvPixelMath.cxx)
ADD_LIBRARY(vvImageMathematics MODULE C/vvImageMathematics.cxx)
ADD_LIBRARY(vvLzfStatistics MODULE C/vvLzfStatistics.cxx)
ADD_LIBRARY(vvLzfFDT MODULE C/vvLzfFDT.cxx)
ADD_LIBRARY(vvLzfHeartDiff MODULE C/vvLzfHeartDiff.cxx)
ADD_LIBRARY(vvLzfStenosisDetection MODULE C/vvLzfStenosisDetection.cxx)
ADD_LIBRARY(vvReadText MODULE C/vvReadText.cxx)
ADD_LIBRARY(vvLzfConnectivity MODULE C/vvLzfConnectivity.cxx)


# Make sure it still compiles

ADD_LIBRARY(vvSample MODULE C/vvSample.cxx)

# VTK plugins

ADD_LIBRARY(vvVTKMergeTets MODULE VTK/vvVTKMergeTets.cxx)
TARGET_LINK_LIBRARIES(vvVTKMergeTets vtkImaging vtkIO vtkGraphics)

ADD_LIBRARY(vvVTKCheckerBoard MODULE VTK/vvVTKCheckerBoard.cxx)
TARGET_LINK_LIBRARIES(vvVTKCheckerBoard vtkImaging)

ADD_LIBRARY(vvVTKDilate3D MODULE VTK/vvVTKDilate3D.cxx)
TARGET_LINK_LIBRARIES(vvVTKDilate3D vtkImaging vtkIO)

ADD_LIBRARY(vvVTKErode3D MODULE VTK/vvVTKErode3D.cxx)
TARGET_LINK_LIBRARIES(vvVTKErode3D vtkImaging)

ADD_LIBRARY(vvVTKGradientMagnitude MODULE VTK/vvVTKGradientMagnitude.cxx)
TARGET_LINK_LIBRARIES(vvVTKGradientMagnitude vtkImaging)

ADD_LIBRARY(vvVTKMedian MODULE VTK/vvVTKMedian.cxx)
TARGET_LINK_LIBRARIES(vvVTKMedian vtkImaging)

ADD_LIBRARY(vvVTKResample MODULE VTK/vvVTKResample.cxx)
TARGET_LINK_LIBRARIES(vvVTKResample vtkImaging)

ADD_LIBRARY(vvVTKShrink MODULE VTK/vvVTKShrink.cxx)
TARGET_LINK_LIBRARIES(vvVTKShrink vtkImaging)

IF( KWVolView_PLUGINS_USE_SPLINE )
  ADD_LIBRARY(vvVTKIso MODULE VTK/vvVTKIso.cxx)
  TARGET_LINK_LIBRARIES(vvVTKIso vtkImaging vtkGraphics)
ENDIF( KWVolView_PLUGINS_USE_SPLINE )

ADD_LIBRARY(vvVTKSmooth MODULE VTK/vvVTKSmooth.cxx)
TARGET_LINK_LIBRARIES(vvVTKSmooth vtkImaging)

ADD_LIBRARY(vvVTKImageCast MODULE VTK/vvVTKImageCast.cxx)
TARGET_LINK_LIBRARIES(vvVTKImageCast vtkImaging)

# Copy the plugins to a plugin directory
# Create plugins with vv prefix, so it does not clash with vv for VolView2
# and cause dependency problems with two targets being of the same name.
# Copy it over to a different folder and rename it with the usual vv prefix.

SET(PLUGINS_PREFIX "vv")

# List all plugins here

SET (PLUGINS 
  C/Boundary
  C/ComponentArithmetic
  C/Crop 
  C/MergeVolumes
  C/Threshold 
  C/PixelMath
  C/ImageMathematics
  C/LzfStatistics
  C/LzfFDT
  C/LzfStenosisDetection
  C/LzfHeartDiff
  C/ReadText
  C/LzfConnectivity
  VTK/VTKCheckerBoard
  VTK/VTKDilate3D
  VTK/VTKErode3D
  VTK/VTKGradientMagnitude
  VTK/VTKMergeTets
  VTK/VTKMedian
  VTK/VTKResample
  VTK/VTKShrink
  VTK/VTKSmooth 
  VTK/VTKImageCast
)

IF (CXX_TEST_PATH)
  FOREACH (PLUGIN ${PLUGINS})
    GET_FILENAME_COMPONENT(PLUG ${PLUGIN} NAME)
    ADD_CUSTOM_COMMAND(
      SOURCE ${PLUGINS_PREFIX}${PLUG}
      COMMAND ${CMAKE_COMMAND} 
      ARGS -E copy ${LIBRARY_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}/${CMAKE_SHARED_MODULE_PREFIX}${PLUGINS_PREFIX}${PLUG}${CMAKE_SHARED_MODULE_SUFFIX} ${CXX_TEST_PATH}/${CMAKE_CFG_INTDIR}/Plugins/${PLUGINS_PREFIX}${PLUG}${CMAKE_SHARED_MODULE_SUFFIX}
      TARGET ${PLUGINS_PREFIX}${PLUG}
      OUTPUTS ${CXX_TEST_PATH}/${CMAKE_CFG_INTDIR}/Plugins/${CMAKE_SHARED_MODULE_PREFIX}${PLUGINS_PREFIX}${PLUG}${MODULE_EXTENSION})

    # Add the plugin to the list of plugins to be installed. These will be
    # installed later on a per-application basis in VolViewApplications/
    SET( PLUGINS_INSTALL_FILES ${PLUGINS_INSTALL_FILES} 
         "${CXX_TEST_PATH}/\${BUILD_TYPE}/Plugins/${PLUGINS_PREFIX}${PLUG}${CMAKE_SHARED_MODULE_SUFFIX}")

 #  INSTALL( TARGETS
 #	  ${PLUGINS_PREFIX}${PLUG}
 #	  RUNTIME DESTINATION bin/Plugins
 #	  LIBRARY DESTINATION bin/Plugins
 #	  ARCHIVE DESTINATION bin/Plugins )
  ENDFOREACH (PLUGIN)

ENDIF (CXX_TEST_PATH)

ADD_SUBDIRECTORY( ITK )

# Add the plugin to the list of plugins to be installed. These will be
# installed later on a per-application basis in VolViewApplications/
SET( KWVolView_PLUGINS_INSTALL_FILES 
     "${PLUGINS_INSTALL_FILES};${VolViewITKPlugins_PLUGINS_INSTALL_FILES}" 
     CACHE INTERNAL 
     "Plugins from VolViewLibraries that may be installed by a VolViewApplication." )

INSTALL_FILES(${KWVolView_INSTALL_INCLUDE_DIR}/Plugins .h 
    vtkVVPluginAPI.h)

