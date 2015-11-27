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
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

PROJECT(VolViewITKPlugins)

#  Find where ITK was Built or Installed.
#
FIND_PACKAGE(ITK)
IF(ITK_FOUND)
  INCLUDE(${ITK_USE_FILE})
ELSE(ITK_FOUND)
  MESSAGE(FATAL_ERROR
          "Cannot build VolviewPlugIns without ITK.  Please set ITK_DIR.")
ENDIF(ITK_FOUND)



# list all plugins here
SET (PLUGINS 
  vvITKAntiAlias
  vvITKBinaryMedian
  vvITKCannyEdgeDetection
  #vvITKCannySegmentationLevelSetModule
  vvITKConfidenceConnected
  vvITKConnectedThreshold
  vvITKCurvatureAnisotropicDiffusion
  vvITKCurvatureFlow 
  #vvITKDeformableModelModule
  vvITKDanielssonDistanceMap
  vvITKFastMarching
  #vvITKFastMarchingModule
  vvITKGeodesicActiveContour
  #vvITKGeodesicActiveContourModule
  vvITKGradientAnisotropicDiffusion
  vvITKGradientMagnitude
  vvITKGradientMagnitudeRecursiveGaussian
  #vvITKHistogramPlot
  vvITKImageRegistration
  vvITKIntensityWindowing
  vvITKIsolatedConnected
  vvITKIsotropicFourthOrderLevelSet
  vvITKMedian
  #vvITKMeanFromSeries
  #vvITKMeanFromSeriesVolumeByVolume
  vvITKMask
  vvITKMaskNegated
  #vvITKMarkovRandomField
  #vvITKMIPFromSeries
  #vvITKDifferenceFromMeanInSeries
  #vvITKLandmarkWarping
  #vvITKLineProbe
  vvITKRescaleIntensityTo8Bits
  vvITKMultimodalityRegistrationRigid
  vvITKMultimodalityRegistrationAffine
  #vvITKMultimodalityRegistrationRigidB
  #vvITKMultimodalityRegistrationRigidJointHistogramPlotter
  #vvITKLandmarkPreparation
  #vvITKScalarImageKMeansClassifier
  vvITKScalarKmeansMarkovRandomField
  vvITKSigmoid
  #vvITKShapeDetectionModule
  #vvITKSurfaceSpline
  vvITKThresholdSegmentationLevelSet
  #vvITKTumorActivity
  #vvITKVectorConfidenceConnected
  vvITKVotingBinary
  vvITKVotingBinaryHoleFilling
  vvITKVotingBinaryIterativeHoleFilling
  #vvITKWatershedModule
  vvITKWatershedRGBModule
  #vvITKRigidMIMetricPlotter
  vvITKThresholdImageToPaintbrush
  vvITKScalarKmeansMarkovRandomFieldToPaintbrush
  vvITKPaintbrushCrop
  vvITKMergePaintbrushLabelImages
  )

IF (VOLVIEW_SUPPORTS_SPLINE_SURFACES)
  SET (PLUGINS ${PLUGINS}
    vvITKSurfaceBoundedConfidenceConnected
    vvITKSplineBoundedConfidenceConnected
    vvITKSplineBoundedConnectedThreshold
    vvITKSurfaceBoundedConnectedThreshold )
ENDIF (VOLVIEW_SUPPORTS_SPLINE_SURFACES)

set(WIN32_64BITS ${CMAKE_CL_64})

# copy the plugins to a plugin directory
IF (CXX_TEST_PATH)
  FOREACH (PLUG ${PLUGINS})
    ADD_LIBRARY( ${PLUG} MODULE ${PLUG}.cxx )
    if(WIN32_64BITS)
      set_source_files_properties(${PLUG}.cxx 
        PROPERTIES COMPILE_FLAGS "/bigobj")
    endif(WIN32_64BITS)
    TARGET_LINK_LIBRARIES( ${PLUG} ITKIO ITKCommon ITKBasicFilters ITKAlgorithms)
    ADD_CUSTOM_COMMAND(
      SOURCE ${PLUG}
      COMMAND ${CMAKE_COMMAND} 
      ARGS -E copy ${LIBRARY_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}/${CMAKE_SHARED_MODULE_PREFIX}${PLUG}${CMAKE_SHARED_MODULE_SUFFIX} ${CXX_TEST_PATH}/${CMAKE_CFG_INTDIR}/Plugins/${PLUG}${CMAKE_SHARED_MODULE_SUFFIX}
      TARGET ${PLUG}
      OUTPUTS ${CXX_TEST_PATH}/${CMAKE_CFG_INTDIR}/Plugins/${CMAKE_SHARED_MODULE_PREFIX}${PLUG}${MODULE_EXTENSION})

    # Add the plugin to the list of plugins to be installed. These will be
    # installed later on a per-application basis in VolViewApplications/
    SET( ITK_PLUGINS_INSTALL_FILES ${ITK_PLUGINS_INSTALL_FILES} 
         "${CXX_TEST_PATH}/\${BUILD_TYPE}/Plugins/${PLUG}${CMAKE_SHARED_MODULE_SUFFIX}")

 #  INSTALL( TARGETS
 #	  ${CMAKE_SHARED_MODULE_PREFIX}${PLUG}
 #	  RUNTIME DESTINATION bin/Plugins
 #	  LIBRARY DESTINATION bin/Plugins
 #	  ARCHIVE DESTINATION bin/Plugins )

  ENDFOREACH (PLUG)
ENDIF (CXX_TEST_PATH)

SET( ${PROJECT_NAME}_PLUGINS_INSTALL_FILES ${ITK_PLUGINS_INSTALL_FILES} CACHE INTERNAL 
     "Plugins from ${PROJECT_NAME} that may be installed by a VolViewApplication.")

