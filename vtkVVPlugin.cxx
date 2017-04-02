/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVVPlugin.h"

#include "vtkCellArray.h"
#include "vtkDynamicLoader.h"
#include "vtkDoubleArray.h"
#include "vtkDataObject.h"
#include "vtkDataObjectCollection.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkImageImport.h"
#include "vtkMetaImageWriter.h"

#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWLabelWithLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWProcessStatistics.h"
#include "vtkKWProgressGauge.h"
#include "vtkKWPushButton.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkKWXYPlotDialog.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

#include "vtkLargeInteger.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkKWLoadSaveDialog.h"

//BTX
#ifdef KWVolView_PLUGINS_USE_SPLINE
#include "vtkKW3DSplineSurfacesWidget.h"
#include "vtkSubdivisionSplineSurfaceWidget.h"
#include "vtkSplineSurfaceWidget.h"
#endif
//ETX

#include "vtkXYPlotActor.h"
#include "vtkVVPluginSelector.h"
#include "vtkKWOpenWizard.h"
#include "vtkKWOpenFileHelper.h"
#include "vtkKWOpenFileProperties.h"
#include "vtkKWImageWidget.h"
//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
#include "vtkVV4DOpenWizard.h"
#include "vtkVV4DSaveDialog.h"
#include "vtkVV4DSaveVolume.h"
#endif
//ETX
#include "vtkKWVolumeWidget.h"
#include "vtkVVWindow.h"
#include "vtkVVDataItemVolume.h"
#include "vtkVVHandleWidget.h"
#include "vtkVVWidgetInterface.h"
#include "vtkVVInteractorWidgetSelector.h"
#include "vtkVVSelectionFrame.h"
#include "vtkKWEPaintbrushWidget.h"
#include "vtkKWEPaintbrushRepresentation2D.h"
#include "vtkKWEPaintbrushDrawing.h"
#include "vtkKWEPaintbrushLabelData.h"
#include "vtkVolumeProperty.h"

#include "vtkVVSelectionFrameLayoutManager.h"

#include <vtksys/SystemTools.hxx>

#include <vtkstd/string>
#include <time.h>

/* this is the data structure describing one GUI element. */
class  vtkVVGUIItem 
{
public:
  /* the label for this widget */
  char *Label; 
  /* what type of GUI item should this be, menu, slider etc */    
  int GUIType;         
  /* the initial value for the widget */
  char *Default;
  /* a help string for baloon help for this widget */
  char *Help;
  /* this string is where additional information required to setup the GUI
   * element is specified. What goes in the Hints varies depending on the
   * type of the GUI item. For a slider it is the range of the slider
   * e.g. "0 255" */
  char *Hints;
  /* this is where the current value of the GUI item will be stored */
  char *Value;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkVVPlugin );
vtkCxxRevisionMacro(vtkVVPlugin, "$Revision: 1.31 $");

//----------------------------------------------------------------------------
vtkVVPlugin::vtkVVPlugin()
{
  this->DocLabel = vtkKWLabel::New();
  this->DocText = vtkKWLabelWithLabel::New();
  this->ReportText = vtkKWLabelWithLabel::New();
  this->StopWatchText = vtkKWLabelWithLabel::New();

  this->Widgets = 0;
  this->Window = 0;
  this->ProgressMinimum = 0;
  this->ProgressMaximum = 1;
  this->AbortProcessing = 0;
  
  this->Name = 0;
  this->Group = 0;
  this->TerseDocumentation = 0;
  this->FullDocumentation = 0;
  this->GUIItems = 0;
  this->SupportProcessingPieces = 0;
  this->SupportInPlaceProcessing = 0;
  this->SecondInputIsUnstructuredGrid = 0;
//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
  this->SupportProcessingSeriesByVolumes = 0;
  this->SeriesInputButton = 0;
  this->SeriesInputOpenWizard = 0;
  this->ProducesSeriesOutput  = 0;
  this->RequiresSeriesInput = 0;
#endif
#ifdef KWVolView_PLUGINS_USE_SPLINE
  this->RequiresSplineSurfaces = 0;
  this->ProducesMeshOnly = 0;
#endif
//ETX
  this->PerVoxelMemoryRequired = 0;
  this->RequiredZOverlap = 0;
  this->NumberOfGUIItems = 0;
  this->RequiresSecondInput = 0;
  this->SecondInputOptional = 0;  
  this->RequiresLabelInput = 0;
  this->SecondInputButton = 0;
  this->SecondInputOpenWizard = 0;
  this->ProducesPlottingOutput  = 0;
  this->PlottingXAxisTitle  = 0;
  this->PlottingYAxisTitle  = 0;

  int i;

  // initialize the info struct to be safe
  this->PluginInfo.magic1 = 0;
  this->PluginInfo.magic2 = 0;
  this->PluginInfo.Self = 0;

  this->PluginInfo.InputVolumeScalarType = 0; 
  this->PluginInfo.InputVolumeScalarSize = 0; 
  this->PluginInfo.InputVolumeNumberOfComponents = 0; 
  for (i = 0; i < 3; i++)
    {
    this->PluginInfo.InputVolumeDimensions[i] = 0; 
    this->PluginInfo.InputVolumeSpacing[i] = 0; 
    this->PluginInfo.InputVolumeOrigin[i] = 0; 
    }
  this->PluginInfo.InputVolumeScalarRange[0] = 0; 
  this->PluginInfo.InputVolumeScalarRange[1] = 1; 
  this->PluginInfo.InputVolumeScalarRange[2] = 0; 
  this->PluginInfo.InputVolumeScalarRange[3] = 1; 
  this->PluginInfo.InputVolumeScalarRange[4] = 0; 
  this->PluginInfo.InputVolumeScalarRange[5] = 1; 
  this->PluginInfo.InputVolumeScalarRange[6] = 0; 
  this->PluginInfo.InputVolumeScalarRange[7] = 1; 
  this->PluginInfo.InputVolumeScalarTypeRange[0] = 0; 
  this->PluginInfo.InputVolumeScalarTypeRange[1] = 1; 

  this->PluginInfo.InputVolume2ScalarType = 0; 
  this->PluginInfo.InputVolume2ScalarSize = 0; 
  this->PluginInfo.InputVolume2NumberOfComponents = 0; 
  for (i = 0; i < 3; i++)
    {
    this->PluginInfo.InputVolume2Dimensions[i] = 0; 
    this->PluginInfo.InputVolume2Spacing[i] = 0; 
    this->PluginInfo.InputVolume2Origin[i] = 0; 
    }
  this->PluginInfo.InputVolume2ScalarRange[0] = 0; 
  this->PluginInfo.InputVolume2ScalarRange[1] = 1; 
  this->PluginInfo.InputVolume2ScalarRange[2] = 0; 
  this->PluginInfo.InputVolume2ScalarRange[3] = 1; 
  this->PluginInfo.InputVolume2ScalarRange[4] = 0; 
  this->PluginInfo.InputVolume2ScalarRange[5] = 1; 
  this->PluginInfo.InputVolume2ScalarRange[6] = 0; 
  this->PluginInfo.InputVolume2ScalarRange[7] = 1; 
  this->PluginInfo.InputVolume2ScalarTypeRange[0] = 0; 
  this->PluginInfo.InputVolume2ScalarTypeRange[1] = 1; 

//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
  this->PluginInfo.InputVolumeSeriesNumberOfVolumes = 0; 
  this->PluginInfo.InputVolumeSeriesScalarType = 0; 
  this->PluginInfo.InputVolumeSeriesScalarSize = 0; 
  this->PluginInfo.InputVolumeSeriesNumberOfComponents = 0; 
  for (i = 0; i < 3; i++)
    {
    this->PluginInfo.InputVolumeSeriesDimensions[i] = 0; 
    this->PluginInfo.InputVolumeSeriesSpacing[i] = 0; 
    this->PluginInfo.InputVolumeSeriesOrigin[i] = 0; 
    }
  this->PluginInfo.InputVolumeSeriesScalarRange[0] = 0; 
  this->PluginInfo.InputVolumeSeriesScalarRange[1] = 1; 
  this->PluginInfo.InputVolumeSeriesScalarRange[2] = 0; 
  this->PluginInfo.InputVolumeSeriesScalarRange[3] = 1; 
  this->PluginInfo.InputVolumeSeriesScalarRange[4] = 0; 
  this->PluginInfo.InputVolumeSeriesScalarRange[5] = 1; 
  this->PluginInfo.InputVolumeSeriesScalarRange[6] = 0; 
  this->PluginInfo.InputVolumeSeriesScalarRange[7] = 1; 
  this->PluginInfo.InputVolumeSeriesScalarTypeRange[0] = 0; 
  this->PluginInfo.InputVolumeSeriesScalarTypeRange[1] = 1; 

  this->PluginInfo.OutputVolumeSeriesNumberOfVolumes = 0; 
  this->PluginInfo.OutputVolumeSeriesScalarType = 0; 
  this->PluginInfo.OutputVolumeSeriesScalarSize = 0; 
  this->PluginInfo.OutputVolumeSeriesNumberOfComponents = 0; 
  for (i = 0; i < 3; i++)
    {
    this->PluginInfo.OutputVolumeSeriesDimensions[i] = 0; 
    this->PluginInfo.OutputVolumeSeriesSpacing[i] = 0; 
    this->PluginInfo.OutputVolumeSeriesOrigin[i] = 0; 
    }
#endif

#ifdef KWVolView_PLUGINS_USE_MARKER
  this->PluginInfo.NumberOfMarkers = 0;
  this->PluginInfo.Markers = 0;
  this->PluginInfo.MarkersGroupId = 0;
  this->PluginInfo.NumberOfMarkersGroups = 0;
  this->PluginInfo.MarkersGroupName = 0;
#endif
//ETX

  this->PluginInfo.CroppingPlanes = 0;

  this->PluginInfo.OutputVolumeScalarType = 0; 
  this->PluginInfo.OutputVolumeNumberOfComponents = 0; 
  for (i = 0; i < 3; i++)
    {
    this->PluginInfo.OutputVolumeDimensions[i] = 0; 
    this->PluginInfo.OutputVolumeSpacing[i] = 0; 
    this->PluginInfo.OutputVolumeOrigin[i] = 0; 
    }

//BTX
#ifdef KWVolView_PLUGINS_USE_SPLINE
  this->PluginInfo.NumberOfSplineSurfaces = 0;
  this->PluginInfo.NumberOfSplineSurfacePoints = 0;
  this->PluginInfo.SplineSurfacePoints = 0;
#endif
//ETX

  this->PluginInfo.OutputPlottingNumberOfColumns = 0;
  this->PluginInfo.OutputPlottingNumberOfRows = 0;

  this->PluginInfo.UnstructuredGridScalarFields = 0;

  this->PluginInfo.UpdateProgress = 0;
  this->PluginInfo.AssignPolygonalData = 0;
  this->PluginInfo.SetProperty = 0;
  this->PluginInfo.GetProperty = 0;
  this->PluginInfo.SetGUIProperty = 0;
  this->PluginInfo.GetGUIProperty = 0;
  this->PluginInfo.ProcessData = 0;
  this->PluginInfo.UpdateGUI = 0;


  this->ResultingComponentsAreIndependent = -1;
  this->ResultingDistanceUnits   = 0;
  this->ResultingComponent1Units = 0;
  this->ResultingComponent2Units = 0;
  this->ResultingComponent3Units = 0;
  this->ResultingComponent4Units = 0;
}

//----------------------------------------------------------------------------
vtkVVPlugin::~vtkVVPlugin()
{
  int i;

  // Free all of the widgets
  this->DocLabel->Delete();
  this->DocText->Delete();
  this->ReportText->Delete();
  this->StopWatchText->Delete();

  if (this->SecondInputButton)
    {
    this->SecondInputButton->Delete();
    this->SecondInputButton = 0;
    }

  if (this->SecondInputOpenWizard)
    {
    this->SecondInputOpenWizard->Delete();
    this->SecondInputOpenWizard = 0;
    }
 
//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
  if (this->SeriesInputButton)
    {
    this->SeriesInputButton->Delete();
    this->SeriesInputButton = 0;
    }

  if (this->SeriesInputOpenWizard)
    {
    this->SeriesInputOpenWizard->Delete();
    this->SeriesInputOpenWizard = 0;
    }
#endif
//ETX
 
  if (this->Widgets)
    {
    for (i = 0; i < this->NumberOfGUIItems; ++i)
      {
      if (this->Widgets[2*i])
        {
        this->Widgets[2*i]->Delete();
        }
      if (this->Widgets[2*i+1])
        {
        this->Widgets[2*i+1]->Delete();
        }
      }
    delete [] this->Widgets;
    this->Widgets = 0;
    }

  if (this->NumberOfGUIItems)
    {
    for (i = 0; i < this->NumberOfGUIItems; ++i)
      {
      if (this->GUIItems[i].Default)
        {
        free(this->GUIItems[i].Default);
        }
      if (this->GUIItems[i].Label)
        {
        free(this->GUIItems[i].Label);
        }
      if (this->GUIItems[i].Help)
        {
        free(this->GUIItems[i].Help);
        }
      if (this->GUIItems[i].Hints)
        {
        free(this->GUIItems[i].Hints);
        }
      if (this->GUIItems[i].Value)
        {
        free(this->GUIItems[i].Value);
        }
      }
    free(this->GUIItems);
    }

  this->SetName(0);
  this->SetGroup(0);
  this->SetTerseDocumentation(0);
  this->SetFullDocumentation(0);
  
  this->SetResultingDistanceUnits(0);
  this->SetResultingComponent1Units(0);
  this->SetResultingComponent2Units(0);
  this->SetResultingComponent3Units(0);
  this->SetResultingComponent4Units(0);
  
  this->SetPlottingXAxisTitle(0);
  this->SetPlottingYAxisTitle(0);

//BTX
#ifdef KWVolView_PLUGINS_USE_MARKER
  if (this->PluginInfo.Markers)
    {
    delete [] this->PluginInfo.Markers;
    this->PluginInfo.Markers = 0;
    this->PluginInfo.NumberOfMarkers = 0;
    }
  if (this->PluginInfo.MarkersGroupId)
    {
    delete [] this->PluginInfo.MarkersGroupId;
    this->PluginInfo.MarkersGroupId = 0;
    }
  if (this->PluginInfo.MarkersGroupName)
    {
    delete [] this->PluginInfo.MarkersGroupName;
    this->PluginInfo.MarkersGroupName = 0;
    this->PluginInfo.NumberOfMarkersGroups = 0;
    }
#endif
//ETX
  if (this->PluginInfo.CroppingPlanes)
    {
    delete [] this->PluginInfo.CroppingPlanes;
    this->PluginInfo.CroppingPlanes = 0;
    }
//BTX
#ifdef KWVolView_PLUGINS_USE_SPLINE
  if(this->PluginInfo.NumberOfSplineSurfacePoints)
    {
    delete [] this->PluginInfo.NumberOfSplineSurfacePoints;
    this->PluginInfo.NumberOfSplineSurfacePoints = 0;
    this->PluginInfo.NumberOfSplineSurfaces = 0;
    }
  if(this->PluginInfo.SplineSurfacePoints)
    {
    delete [] this->PluginInfo.SplineSurfacePoints;
    this->PluginInfo.SplineSurfacePoints = 0;
    this->PluginInfo.NumberOfSplineSurfaces = 0;
    }
#endif
//ETX

  if(this->PluginInfo.UnstructuredGridScalarFields)
    {
    delete [] this->PluginInfo.UnstructuredGridScalarFields;
    this->PluginInfo.UnstructuredGridScalarFields = 0;
    }

  this->PluginInfo.Self = 0;
  this->PluginInfo.UpdateProgress = 0;
  this->PluginInfo.AssignPolygonalData = 0;
  this->PluginInfo.SetProperty = 0;
  this->PluginInfo.GetProperty = 0;
  this->PluginInfo.SetGUIProperty = 0;
  this->PluginInfo.GetGUIProperty = 0;
  this->PluginInfo.ProcessData = 0;
  this->PluginInfo.UpdateGUI = 0;
}

//----------------------------------------------------------------------------
void vtkVVPlugin::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->DocLabel);
  this->PropagateEnableState(this->DocText);
  this->PropagateEnableState(this->ReportText);
  this->PropagateEnableState(this->StopWatchText);

  if (this->Widgets)
    {
    int i;
    for (i = 0; i < this->NumberOfGUIItems; ++i)
      {
      this->PropagateEnableState(this->Widgets[2*i]);
      this->PropagateEnableState(this->Widgets[2*i + 1]);
      }
    }

  this->PropagateEnableState(this->SecondInputButton);
//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
  this->PropagateEnableState(this->SeriesInputButton);
#endif
//ETX
}

//----------------------------------------------------------------------------
void vtkVVPlugin::SetWindow(vtkVVWindowBase *arg)
{
  if (this->Window == arg)
    {
    return;
    }
  this->Window = arg;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
extern "C" 
{
  void  vtkVVPluginSetProperty(void *inf, int param, const char *val)
  {
    vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
    vtkVVPlugin *self = (vtkVVPlugin *)info->Self;
    self->SetProperty(param,val);
  }

  const char *vtkVVPluginGetProperty(void *inf, int param)
  {
    vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
    vtkVVPlugin *self = (vtkVVPlugin *)info->Self;
    return self->GetProperty(param);
  }

  void  vtkVVPluginSetGUIProperty(void *inf, int gui, 
                                  int param, const char *val)
  {
    vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
    vtkVVPlugin *self = (vtkVVPlugin *)info->Self;
    self->SetGUIProperty(gui, param,val);
  }

  const char *vtkVVPluginGetGUIProperty(void *inf, int gui, int param)
  {
    vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
    vtkVVPlugin *self = (vtkVVPlugin *)info->Self;
    return self->GetGUIProperty(gui, param);
  }
}


extern "C" 
{
  void  vtkVVPluginUpdateProgress(void *inf, float progress, const char *msg)
  {
    vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
    vtkVVPlugin *self = (vtkVVPlugin *)info->Self;
    if (info && 
        self && 
        self->GetWindow() && 
        self->GetWindow()->GetProgressGauge())
      {
      progress = self->ProgressMinimum + 
        (self->ProgressMaximum - self->ProgressMinimum)*progress;
      self->GetWindow()->GetProgressGauge()->SetValue(
        static_cast<int>(100.0*progress));
      if (progress >= 1.0)
        {
        self->GetWindow()->GetProgressGauge()->SetValue(static_cast<int>(0.0));
        }
      self->GetWindow()->SetStatusText(msg);
      self->GetWindow()->GetApplication()->ProcessPendingEvents();
      }
  }
}

extern "C" 
{
  void  vtkVVPluginAssignPolygonalData(void *inf, vtkVVProcessDataStruct *pds)
  {
    vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;
    vtkVVPlugin *self = (vtkVVPlugin *)info->Self;
    // now did this generate polydata?
    if (pds->NumberOfMeshPoints)
      {
      vtkPolyData *pd = vtkPolyData::New();
      vtkPoints *points = vtkPoints::New();
      vtkCellArray *ca = vtkCellArray::New();
      int i, j;
      
      // do the points
      points->SetNumberOfPoints(pds->NumberOfMeshPoints);
      for (i = 0; i < pds->NumberOfMeshPoints; ++i)
        {
        points->SetPoint(i,pds->MeshPoints + i*3);
        }
      // do the cells
      int *pos = pds->MeshCells;
      for (i = 0; i < pds->NumberOfMeshCells; ++i)
        {
        ca->InsertNextCell(*pos);
        for (j = 0; j < *pos; ++j)
          {
          ca->InsertCellPoint(pos[j+1]);
          }
        pos = pos + (*pos + 1);
        }

      // are there normals?
      if (pds->MeshNormals)
        {
        vtkFloatArray *normals = vtkFloatArray::New();
        normals->SetNumberOfComponents(3);
        normals->SetNumberOfTuples(pds->NumberOfMeshPoints);
        for (i = 0; i < pds->NumberOfMeshPoints; ++i)
          {
          normals->SetTuple(i,pds->MeshNormals + i*3);
          }
        pd->GetPointData()->SetNormals(normals);
        normals->Delete();
        }
      
      // are there scalars?
      if (pds->MeshScalars)
        {
        vtkFloatArray *scalars = vtkFloatArray::New();
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(pds->NumberOfMeshPoints);
        for (i = 0; i < pds->NumberOfMeshPoints; ++i)
          {
          scalars->SetTuple(i,pds->MeshScalars + i);
          }
        pd->GetPointData()->SetScalars(scalars);
        scalars->Delete();
        }

      pd->SetPoints(points);
      points->Delete();
      pd->SetPolys(ca);
      ca->Delete();

      // set the pd on the Window
      //self->GetWindow()->SetPolyData(pd);
      
      pd->Delete();
      }
  }
}

//----------------------------------------------------------------------------
void vtkVVPlugin::SetProperty(int param, const char *value)
{
  switch (param)
    {
    case VVP_ERROR:
      vtkKWMessageDialog::PopupMessage( 
        this->GetApplication(), 0, "Plugin Execution", value, 
        vtkKWMessageDialog::ErrorIcon);
      break;
    case VVP_NAME:
      this->SetName(value);
      break;
    case VVP_GROUP:
      this->SetGroup(value);
      break;
    case VVP_TERSE_DOCUMENTATION:
      this->SetTerseDocumentation(value);
      break;
    case VVP_FULL_DOCUMENTATION:
      this->SetFullDocumentation(value);
      break;
    case VVP_SUPPORTS_IN_PLACE_PROCESSING:
      this->SupportInPlaceProcessing = atoi(value);
      break;
    case VVP_SUPPORTS_PROCESSING_PIECES:
      this->SupportProcessingPieces = atoi(value);
      break;
//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
    case VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES:
      this->SupportProcessingSeriesByVolumes = atoi(value);
      break;
#endif
//ETX
    case VVP_NUMBER_OF_GUI_ITEMS:
      this->NumberOfGUIItems = atoi(value);
      break;
//BTX
#ifdef KWVolView_PLUGINS_USE_SPLINE
    case VVP_PRODUCES_MESH_ONLY:
      this->ProducesMeshOnly = atoi(value);
      break;
#endif
//ETX
    case VVP_REQUIRED_Z_OVERLAP:
      this->RequiredZOverlap = atoi(value);
      break;
    case VVP_PER_VOXEL_MEMORY_REQUIRED:
      this->PerVoxelMemoryRequired = atof(value);
      break;
    case VVP_ABORT_PROCESSING:
      this->AbortProcessing = atoi(value);
      break;
    case VVP_REPORT_TEXT:
      this->SetReportText(value);
      break;
    case VVP_REQUIRES_SECOND_INPUT:
      this->RequiresSecondInput = atoi(value);
      break;
    case VVP_SECOND_INPUT_IS_UNSTRUCTURED_GRID:
      this->SecondInputIsUnstructuredGrid = atoi(value);
      break;
    case VVP_SECOND_INPUT_OPTIONAL:
      this->SecondInputOptional = atoi(value);
      break;
    case VVP_REQUIRES_LABEL_INPUT:
      this->RequiresLabelInput = atoi(value);
      break;      
//BTX
#ifdef KWVolView_PLUGINS_USE_SPLINE
    case VVP_REQUIRES_SPLINE_SURFACES:
      this->RequiresSplineSurfaces = atoi(value);
      break;
#endif
//ETX
//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
    case VVP_REQUIRES_SERIES_INPUT:
      this->RequiresSeriesInput = atoi(value);
      break;
    case VVP_PRODUCES_OUTPUT_SERIES:
      this->ProducesSeriesOutput = atoi(value);
      break;
#endif
//ETX
    case VVP_PRODUCES_PLOTTING_OUTPUT:
      this->ProducesPlottingOutput = atoi(value);
      break;
    case VVP_RESULTING_COMPONENTS_ARE_INDEPENDENT:
      this->ResultingComponentsAreIndependent = atoi(value);
      break;
    case VVP_RESULTING_DISTANCE_UNITS:
      this->SetResultingDistanceUnits(value);
      break;
    case VVP_RESULTING_COMPONENT_1_UNITS:
      this->SetResultingComponent1Units(value);
      break;
    case VVP_RESULTING_COMPONENT_2_UNITS:
      this->SetResultingComponent2Units(value);
      break;
    case VVP_RESULTING_COMPONENT_3_UNITS:
      this->SetResultingComponent3Units(value);
      break;
    case VVP_RESULTING_COMPONENT_4_UNITS:
      this->SetResultingComponent4Units(value);
      break;
    case VVP_PLOTTING_X_AXIS_TITLE:
      this->SetPlottingXAxisTitle(value);
      break;
    case VVP_PLOTTING_Y_AXIS_TITLE:
      this->SetPlottingYAxisTitle(value);
      break;
    }
}

//----------------------------------------------------------------------------
const char *vtkVVPlugin::GetProperty(int param)
{
  vtkVVDataItemVolume *volume_data = vtkVVDataItemVolume::SafeDownCast(
                                  this->Window->GetSelectedDataItem());
  switch (param)
    {
    case VVP_NAME:
      return this->GetName();
      break;
    case VVP_GROUP:
      return this->GetGroup();
      break;
    case VVP_TERSE_DOCUMENTATION:
      return this->GetTerseDocumentation();
      break;
    case VVP_FULL_DOCUMENTATION:
      return this->GetFullDocumentation();
      break;
    case VVP_ABORT_PROCESSING:
      if (this->AbortProcessing)
        {
        return "1";
        }
      else
        {
        return "0";
        }
      break;
    case VVP_RESULTING_DISTANCE_UNITS:
      return this->GetResultingDistanceUnits();
      break;
    case VVP_RESULTING_COMPONENT_1_UNITS:
      return this->GetResultingComponent1Units();
      break;
    case VVP_RESULTING_COMPONENT_2_UNITS:
      return this->GetResultingComponent2Units();
      break;
    case VVP_RESULTING_COMPONENT_3_UNITS:
      return this->GetResultingComponent3Units();
      break;
    case VVP_RESULTING_COMPONENT_4_UNITS:
      return this->GetResultingComponent4Units();
      break;
      
    case VVP_INPUT_COMPONENTS_ARE_INDEPENDENT:
      if (volume_data->GetVolumeProperty()->GetIndependentComponents())
        {
        return "1";
        }
      else
        {
        return "0";
        }
      break;
    case VVP_INPUT_DISTANCE_UNITS:
      return volume_data->GetDistanceUnits();
      break;
    case VVP_INPUT_COMPONENT_1_UNITS:
      return volume_data->GetScalarUnits(0);
      break;
    case VVP_INPUT_COMPONENT_2_UNITS:
      return volume_data->GetScalarUnits(1);
      break;
    case VVP_INPUT_COMPONENT_3_UNITS:
      return volume_data->GetScalarUnits(2);
      break;
    case VVP_INPUT_COMPONENT_4_UNITS:
      return volume_data->GetScalarUnits(3);
      break;

      // these values may change as the second input is changed
    case VVP_INPUT_2_COMPONENTS_ARE_INDEPENDENT:
      if (this->SecondInputOpenWizard->GetOpenFileProperties()->GetIndependentComponents())
        {
        return "1";
        }
      else
        {
        return "0";
        }
      break;
    case VVP_INPUT_2_DISTANCE_UNITS:
      return this->SecondInputOpenWizard->GetOpenFileProperties()->GetDistanceUnits();
      break;
    case VVP_INPUT_2_COMPONENT_1_UNITS:
      return this->SecondInputOpenWizard->GetOpenFileProperties()->GetScalarUnits(0);
      break;
    case VVP_INPUT_2_COMPONENT_2_UNITS:
      return this->SecondInputOpenWizard->GetOpenFileProperties()->GetScalarUnits(1);
      break;
    case VVP_INPUT_2_COMPONENT_3_UNITS:
      return this->SecondInputOpenWizard->GetOpenFileProperties()->GetScalarUnits(2);
      break;
    case VVP_INPUT_2_COMPONENT_4_UNITS:
      return this->SecondInputOpenWizard->GetOpenFileProperties()->GetScalarUnits(3);
      break;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkVVPlugin::SetGUIProperty(int gui, int param, const char *value)
{
  if (gui < 0 || gui >= this->NumberOfGUIItems)
    {
    return;
    }
  
  vtkVVGUIItem *gi = this->GUIItems + gui;

  switch (param)
    {
    case VVP_GUI_LABEL:
      if (gi->Label)
        {
        free(gi->Label);
        gi->Label = 0;
        }
      if (value)
        {
        gi->Label = strdup(value);
        }
      break;
    case VVP_GUI_DEFAULT:
      if (gi->Default)
        {
        free(gi->Default);
        gi->Default = 0;
        }
      if (value)
        {
        gi->Default = strdup(value);
        }
      break;
    case VVP_GUI_HELP:
      if (gi->Help)
        {
        free(gi->Help);
        gi->Help = 0;
        }
      if (value)
        {
        gi->Help = strdup(value);
        }
      break;
    case VVP_GUI_HINTS:
      if (gi->Hints)
        {
        free(gi->Hints);
        gi->Hints = 0;
        }
      if (value)
        {
        gi->Hints = strdup(value);
        }
      break;
    case VVP_GUI_VALUE:
      if (gi->Value)
        {
        free(gi->Value);
        gi->Value = 0;
        }
      if (value)
        {
        gi->Value = strdup(value);
        }
      break;
    case VVP_GUI_TYPE:
      if (!strcmp(value,VVP_GUI_SCALE))
        {
        gi->GUIType = VV_GUI_SCALE;
        }
      if (!strcmp(value,VVP_GUI_CHOICE))
        {
        gi->GUIType = VV_GUI_CHOICE;
        }
      if (!strcmp(value,VVP_GUI_CHECKBOX))
        {
        gi->GUIType = VV_GUI_CHECKBOX;
        }
      break;
    }
}

//----------------------------------------------------------------------------
const char *vtkVVPlugin::GetGUIProperty(int gui, int param)
{
  if (gui < 0 || gui >= this->NumberOfGUIItems)
    {
    return 0;
    }
  
  vtkVVGUIItem *gi = this->GUIItems + gui;
  
  switch (param)
    {
    case VVP_GUI_LABEL:
      return gi->Label;
      break;
    case VVP_GUI_DEFAULT:
      return gi->Default;
      break;
    case VVP_GUI_HELP:
      return gi->Help;
      break;
    case VVP_GUI_HINTS:
      return gi->Hints;
      break;
    case VVP_GUI_VALUE:
      return gi->Value;
      break;
    case VVP_GUI_TYPE:
      switch (gi->GUIType)
        {
        case VV_GUI_SCALE:
          return VVP_GUI_SCALE;
          break;
        case VV_GUI_CHOICE:
          return VVP_GUI_CHOICE;
          break;
        case VV_GUI_CHECKBOX:
          return VVP_GUI_CHECKBOX;
          break;
        }
      break;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkVVPlugin::SetGUIProperty(const char *label, int param, const char *value)
{
  if (label)
    {
    int i;
    for (i = 0; i < this->NumberOfGUIItems; ++i)
      {
      vtkVVGUIItem *gi = &this->GUIItems[i];
      if (gi && gi->Label && !strcmp(gi->Label, label))
        {
        this->SetGUIProperty(i, param, value);
        }
      }
    }
}

//----------------------------------------------------------------------------
const char *vtkVVPlugin::GetGUIProperty(const char *label, int param)
{
  if (label)
    {
    int i;
    for (i = 0; i < this->NumberOfGUIItems; ++i)
      {
      vtkVVGUIItem *gi = &this->GUIItems[i];
      if (gi && gi->Label && !strcmp(gi->Label, label))
        {
        return this->GetGUIProperty(i, param);
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkVVPlugin::SetGUIPropertyValue(const char *label, const char *value)
{
  this->SetGUIProperty(label, VVP_GUI_VALUE, value);
}

//----------------------------------------------------------------------------
const char *vtkVVPlugin::GetGUIPropertyValue(const char *label)
{
  return this->GetGUIProperty(label, VVP_GUI_VALUE);
}

//----------------------------------------------------------------------------
int vtkVVPlugin::Load(const char *path, vtkKWApplication *app)
{
  // the file must exist
  vtkstd::string fullPath = path;

  // try loading the shared library / dll
  vtkLibHandle lib = vtkDynamicLoader::OpenLibrary(fullPath.c_str());
  if(!lib)
    {
    return 1;
    }

  // find the init function
  vtkstd::string initFuncName = path;
  // remove the extension 
  vtkstd::string::size_type slash_pos = initFuncName.rfind("/");
  if(slash_pos != vtkstd::string::npos)
    {
    initFuncName = initFuncName.substr(slash_pos + 1);
    }
  vtkstd::string::size_type dot_pos = initFuncName.find(".");
  if(dot_pos != vtkstd::string::npos)
    {
    initFuncName = initFuncName.substr(0, dot_pos);
    }
  initFuncName += "Init";
  VV_INIT_FUNCTION initFunction
    = (VV_INIT_FUNCTION)
    vtkDynamicLoader::GetSymbolAddress(lib, initFuncName.c_str());
  if ( !initFunction )
    {
    initFuncName = "_";
    initFuncName += path;
    initFuncName += "Init";
    initFunction = (VV_INIT_FUNCTION)(
      vtkDynamicLoader::GetSymbolAddress(lib, initFuncName.c_str()));
    }
  // if the symbol is found call it to set the name on the 
  // function blocker
  if(initFunction)
    {
    // create the data strucvture and initialize
    this->SetGroup(VTK_VV_PLUGIN_DEFAULT_GROUP);
    this->PluginInfo.Self = (void *)this;
    this->PluginInfo.UpdateProgress = vtkVVPluginUpdateProgress;
    this->PluginInfo.SetProperty = vtkVVPluginSetProperty;
    this->PluginInfo.GetProperty = vtkVVPluginGetProperty;
    this->PluginInfo.SetGUIProperty = vtkVVPluginSetGUIProperty;
    this->PluginInfo.GetGUIProperty = vtkVVPluginGetGUIProperty;
//BTX
#ifdef KWVolView_PLUGINS_USE_SPLINE
    this->PluginInfo.AssignPolygonalData = vtkVVPluginAssignPolygonalData;
#endif
//ETX
    this->PluginInfo.magic1 = VV_PLUGIN_API_VERSION;
    (*initFunction)(&this->PluginInfo);
    // a plugin will set magic1 to the plugin API it was compiled with
    // if it can work, otherwise zero. By default the rule is that a
    // plugin will assume that it will work with future API versions.
    if (!this->PluginInfo.magic1)
      {
      char *msg = new char [strlen(path) + 1024];
      sprintf(msg,"An attempt was made to load a plugin that is not compatible with the version of VolView being run. The plugin was located in the file %s",path);
      vtkKWMessageDialog::PopupMessage( 
        app, 0, "Load Plugin", msg, vtkKWMessageDialog::ErrorIcon);
      delete [] msg;
      this->NumberOfGUIItems = 0;
      return 3;
      }
    if (this->NumberOfGUIItems)
      {
      this->GUIItems = (vtkVVGUIItem *)
        malloc(this->NumberOfGUIItems*sizeof(vtkVVGUIItem));
      // initialize the strings to null 
      int i;
      for (i = 0; i < this->NumberOfGUIItems; ++i)
        {
        this->GUIItems[i].Default = 0;
        this->GUIItems[i].Label = 0;
        this->GUIItems[i].Help = 0;
        this->GUIItems[i].Hints = 0;
        this->GUIItems[i].Value = 0;
        }
      }
    return 0;
    }
  return 2;
}

//----------------------------------------------------------------------------
void vtkVVPlugin::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  // create the GUI components

  this->PluginInfo.UpdateGUI(&this->PluginInfo);

  int i, row = 0;

  this->DocLabel->SetParent(this);
  this->DocLabel->Create();
  this->DocLabel->SetText(this->TerseDocumentation);

  this->Script("grid %s -sticky nsw -row %d -column 0 -columnspan 2 -pady 4",
               this->DocLabel->GetWidgetName(), row++);

  this->Script("grid columnconfigure %s 0 -weight 0",
               this->GetWidgetName());

  this->Script("grid columnconfigure %s 1 -weight 1",
               this->GetWidgetName());

  // for each GUI component ...
  this->Widgets = new vtkKWWidget *[2*this->NumberOfGUIItems];
  for (i = 0; i < this->NumberOfGUIItems; ++i)
    {
    this->Widgets[2*i] = 0;
    this->Widgets[2*i+1] = 0;
    // create the actual widget
    switch (this->GUIItems[i].GUIType) 
      {
      case VV_GUI_SCALE:
      {
      vtkKWScaleWithEntry *s = vtkKWScaleWithEntry::New();
      s->SetParent(this);
      s->Create();
      s->SetLabelAndEntryPositionToTop();
      this->Widgets[2*i+1] = s;
      this->Script("grid %s -sticky nsew -row %i -column 0 -columnspan 2",
                   this->Widgets[2*i+1]->GetWidgetName(), row++);
      }
      break;
      case VV_GUI_CHOICE:
      {
      vtkKWLabel *l = vtkKWLabel::New();
      l->SetParent(this);
      l->Create();
      this->Widgets[2*i] = l;
      this->Script("grid %s -sticky w -row %i -column 0",
                   this->Widgets[2*i]->GetWidgetName(), row++);
      vtkKWMenuButton *s = vtkKWMenuButton::New();
      s->SetParent(this);
      s->Create();
      this->Widgets[2*i+1] = s;
      this->Script("grid %s -sticky w -row %i -column 1",
                   this->Widgets[2*i+1]->GetWidgetName(), row++);
      }
      break;
      case VV_GUI_CHECKBOX:
      {
      vtkKWCheckButton *s = vtkKWCheckButton::New();
      s->SetParent(this);
      s->Create();
      this->Widgets[2*i+1] = s;
      this->Script("grid %s -sticky nsw -row %i -column 0 -columnspan 2",
                   this->Widgets[2*i+1]->GetWidgetName(), row++);
      }
      break;
      }
    }

  // if the plugin requires a second input then add the button
  if (this->RequiresSecondInput)
    {
    this->SecondInputButton = vtkKWPushButton::New();
    this->SecondInputButton->SetParent(this);
    this->SecondInputButton->Create();
    this->SecondInputButton->SetText("Assign Second Input");
    this->SecondInputButton->SetCommand(this, "SecondInputCallback");
    this->Script(
      "grid %s -sticky nsew -padx 2 -pady 2 -row %i -column 0 -columnspan 2",
      this->SecondInputButton->GetWidgetName(), row++);

    // create the open wizard
    this->SecondInputOpenWizard = vtkKWOpenWizard::New();
    this->SecondInputOpenWizard->SetApplication(this->GetApplication());
    this->SecondInputOpenWizard->Create();
    this->SecondInputOpenWizard->SetTitle(NULL); // updated by Invoke()
    this->SecondInputOpenWizard->GetOpenFileHelper()->
      SetAllowVTKUnstructuredGrid(this->SecondInputIsUnstructuredGrid);
    }
  
//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
  // if the plugin requires a series input then add the button
  if (this->RequiresSeriesInput)
    {
    this->SeriesInputButton = vtkKWPushButton::New();
    this->SeriesInputButton->SetParent(this);
    this->SeriesInputButton->Create();
    this->SeriesInputButton->SetText("Assign Series Input");
    this->SeriesInputButton->SetCommand(this, "SeriesInputCallback");
    this->Script(
      "grid %s -sticky nsew -padx 2 -pady 2 -row %i -column 0 -columnspan 2",
      this->SeriesInputButton->GetWidgetName(), row++);

    // create the open wizard
    this->SeriesInputOpenWizard = vtkVV4DOpenWizard::New();
    this->SeriesInputOpenWizard->SetApplication(this->GetApplication());
    this->SeriesInputOpenWizard->Create();
    this->SeriesInputOpenWizard->SetTitle(NULL); // updated by Invoke()
    }
#endif
//ETX
  
  this->DocText->SetParent(this);
  this->DocText->Create();
  this->DocText->GetLabel()->SetImageToPredefinedIcon(
    vtkKWIcon::IconSilkHelp);
  this->DocText->ExpandWidgetOn();
  this->DocText->GetWidget()->AdjustWrapLengthToWidthOn();
  this->DocText->GetWidget()->SetText(this->FullDocumentation);

  this->Script("grid %s -sticky nsew -row %i -column 0 -columnspan 2 -pady 1",
               this->DocText->GetWidgetName(), row++);

  this->ReportText->SetParent(this);
  this->ReportText->Create();
  this->ReportText->GetLabel()->SetImageToPredefinedIcon(
    vtkKWIcon::IconSilkInformation);
  this->ReportText->ExpandWidgetOn();
  this->ReportText->GetWidget()->AdjustWrapLengthToWidthOn();

  this->Script("grid %s -sticky nsew -row %i -column 0 -columnspan 2 -pady 1",
               this->ReportText->GetWidgetName(), row++);

  this->SetReportText("");

  this->StopWatchText->SetParent(this);
  this->StopWatchText->Create();
  this->StopWatchText->GetLabel()->SetImageToPredefinedIcon(
    vtkKWIcon::IconTime);
  this->StopWatchText->ExpandWidgetOff();

  this->Script("grid %s -sticky nsew -row %i -column 0 -columnspan 2 -pady 1",
               this->StopWatchText->GetWidgetName(), row++);

  this->SetStopWatchText("");

  // Update
  this->Update();
  // set the initial values for each GUI component ...
  for (i = 0; i < this->NumberOfGUIItems; ++i)
    {
    // update the actual widgets
    if (!this->GUIItems[i].Default)
      {
      continue;
      }
    switch (this->GUIItems[i].GUIType) 
      {
      case VV_GUI_SCALE:
      {
      vtkKWScaleWithEntry *s = 
        vtkKWScaleWithEntry::SafeDownCast(this->Widgets[2*i+1]);
      s->SetValue(atof(this->GUIItems[i].Default));
      }
      break;
      case VV_GUI_CHOICE:
      {
      vtkKWMenuButton *s = vtkKWMenuButton::SafeDownCast(this->Widgets[2*i+1]);
      s->SetValue(this->GUIItems[i].Default);
      }
      break;
      case VV_GUI_CHECKBOX:
      {
      vtkKWCheckButton *s = 
        vtkKWCheckButton::SafeDownCast(this->Widgets[2*i+1]);
      s->SetSelectedState(atoi(this->GUIItems[i].Default));
      }
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkVVPlugin::SecondInputCallback()
{
  if (!this->SecondInputOpenWizard)
    {
    return;
    }

  // Get another input to use as the second input

  vtkKWLoadSaveDialog *loaddialog = this->SecondInputOpenWizard->GetLoadDialog();
  if (loaddialog)
    {
    loaddialog->RetrieveLastPathFromRegistry("OpenPath");
    }

  // Bring up the wizard, if succesfull then ...

  int res;
  if (this->SecondInputIsUnstructuredGrid)
    {
    res = this->SecondInputOpenWizard->InvokeQuiet();
    }
  else
    {
    res = this->SecondInputOpenWizard->Invoke();
    }
  if (res)
    {
    if (loaddialog)
      {
      loaddialog->SaveLastPathToRegistry("OpenPath");
      }
    this->UpdateAccordingToSecondInput();
    }
}

//----------------------------------------------------------------------------
const char* vtkVVPlugin::GetSecondInputFileName()
{
  if (this->SecondInputOpenWizard)
    {
    return this->SecondInputOpenWizard->GetFileName();
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkVVPlugin::SetSecondInputFileName(const char *fname)
{
  // Bring up the wizard, if succesfull then ...

  if (this->SecondInputOpenWizard && this->SecondInputOpenWizard->Invoke(fname, 0))
    {
    this->UpdateAccordingToSecondInput();
    }
}

//----------------------------------------------------------------------------
void vtkVVPlugin::UpdateAccordingToSecondInput()
{
  if (!this->SecondInputOpenWizard || !this->SecondInputOpenWizard->GetFileName())
    {
    return;
    }

  const char *fname = this->SecondInputOpenWizard->GetFileName();
  ostrstream title;
  vtksys_stl::string basename = vtksys::SystemTools::GetFilenameName(fname);
  title << "Second Input: " << basename.c_str() << ends;
  this->SecondInputButton->SetText(title.str());
  title.rdbuf()->freeze(0);
    
  // copy over the parameters now
  
  vtkKWOpenFileProperties *open_prop = 
    this->SecondInputOpenWizard->GetOpenFileProperties();
  this->PluginInfo.InputVolume2ScalarType = open_prop->GetScalarType();
  this->PluginInfo.InputVolume2ScalarSize = open_prop->GetScalarSize();
    
  this->PluginInfo.InputVolume2NumberOfComponents = 
    open_prop->GetNumberOfScalarComponents();
    
  int *wExt = open_prop->GetWholeExtent();
  this->PluginInfo.InputVolume2Dimensions[0] = wExt[1] - wExt[0] + 1;
  this->PluginInfo.InputVolume2Dimensions[1] = wExt[3] - wExt[2] + 1;
  this->PluginInfo.InputVolume2Dimensions[2] = wExt[5] - wExt[4] + 1;
  int i;
  for (i = 0; i < 3; i++)
    {
    this->PluginInfo.InputVolume2Spacing[i] = open_prop->GetSpacing()[i];
    }
    
  // compute the ranges
  // NOTE: I don't see how this was ever meant to work
  // this used to be the OpenWizard's 
  // ValidImageInfo->GetPointData()->GetScalars(), but as its name implies
  // this is only a structure describing the data, *not* the data itself!
  /*
  vtkDataArray *scalars = open_prop->GetPointData()->GetScalars();
  double ftmp[2];
  if (scalars)
    {
    for (i = 0; i < this->PluginInfo.InputVolume2NumberOfComponents; ++i)
      {
      scalars->GetRange(ftmp,i);
      this->PluginInfo.InputVolume2ScalarRange[2*i] = ftmp[0];
      this->PluginInfo.InputVolume2ScalarRange[2*i+1] = ftmp[1];
      }
    }
  */

  this->PluginInfo.InputVolume2ScalarTypeRange[0] = 
    open_prop->GetScalarTypeMin();
  this->PluginInfo.InputVolume2ScalarTypeRange[1] =
    open_prop->GetScalarTypeMax();
    
  // we do adjust the origin because the plugin is zero extent based
  double *origin = open_prop->GetOrigin();
  this->PluginInfo.InputVolume2Origin[0] = 
    origin[0] + open_prop->GetSpacing()[0]*open_prop->GetWholeExtent()[0];
  this->PluginInfo.InputVolume2Origin[1] = 
    origin[1] + open_prop->GetSpacing()[1]*open_prop->GetWholeExtent()[2];
  this->PluginInfo.InputVolume2Origin[2] = 
    origin[2] + open_prop->GetSpacing()[2]*open_prop->GetWholeExtent()[4];

  vtkUnstructuredGrid *res = 
    this->SecondInputOpenWizard->GetUnstructuredGridOutput(0);
  if (res)
    {
    ostrstream choices;
    int numArrays = res->GetPointData()->GetNumberOfArrays();
    choices << numArrays+1 << "\nUnspecified";
    for (int ctr=0; ctr<numArrays; ctr++)
      {
      choices << "\n" << res->GetPointData()->GetArray(ctr)->GetName();
      }
    choices << ends;

    int length = strlen(choices.str()) + 1;
    this->PluginInfo.UnstructuredGridScalarFields = new char [length+1];
    choices.rdbuf()->freeze(0);
    for (int newCtr=0; newCtr<length; newCtr++)
      {
      this->PluginInfo.UnstructuredGridScalarFields[newCtr] = 
        choices.str()[newCtr];
      }
    }

  // some of the GUI elements might depend on the second input, refresh
  // them now
  this->Update();
}

//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
//----------------------------------------------------------------------------
void vtkVVPlugin::SeriesInputCallback()
{
  if (!this->SeriesInputOpenWizard)
    {
    return;
    }

  // Get another input to use as the series input

  vtkKWLoadSaveDialog *loaddialog = this->SeriesInputOpenWizard->GetLoadDialog();
  if (loaddialog)
    {
    loaddialog->RetrieveLastPathFromRegistry("OpenPath");
    }

  // Bring up the wizard, if succesfull then ...
 
  if (this->SeriesInputOpenWizard->Invoke())
    {
    if (loaddialog)
      {
      loaddialog->SaveLastPathToRegistry("OpenPath");
      }
    this->UpdateAccordingToSeriesInput();
    }
}

//----------------------------------------------------------------------------
void vtkVVPlugin::GetSeriesInputFileNames(const char ** pattern,int & min, int & max)
{
  if (this->SeriesInputOpenWizard)
    {
      std::cout << "vtkVVPlugin::GetSeriesInputFileNames() needs to be implemented" << std::endl;
      std::cout << "The OpenWizard needs a method for returning pattern,min,max" << std::endl;
    *pattern = this->SeriesInputOpenWizard->GetFileName();
    min = 0;
    max = 0;
    }
  else
    {
    *pattern = NULL;
    min = 0;
    max = 0;
    }
}

//----------------------------------------------------------------------------
void vtkVVPlugin::SetSeriesInputFileNames(const char *pattern, int min, int max)
{
  // Bring up the wizard, if succesfull then ...

  // FIXME: It seems that another overload of Invoke() is needed in order to 
  // set the case of file series...  In the meantime... do nothing. This should only
  // matter for TeleVolView...
  std::cout << "vtkVVPlugin::SetSeriesInputFileNames() work in progress.." << std::endl;
  //  if (this->SeriesInputOpenWizard && this->SeriesInputOpenWizard->Invoke(*fname, 0))

  if (this->SeriesInputOpenWizard )
    {
    this->SeriesInputOpenWizard->SetSeriesPattern(pattern);
    this->SeriesInputOpenWizard->SetSeriesMinimum(min);
    this->SeriesInputOpenWizard->SetSeriesMaximum(max);
    this->UpdateAccordingToSeriesInput();
    }
}

//----------------------------------------------------------------------------
// Note that 'index' is in the range [0:numberOfDataSetsInTheSeries], not in
// the range [SeriesMinimum:SeriesMaximum]. This method is intended to be called
// from the VVWindow during televolview communication.
//
void vtkVVPlugin::SetSeriesInputFileName(const char *fname, int index)
{
  // Bring up the wizard, if succesfull then ...

  if (this->SeriesInputOpenWizard && this->SeriesInputOpenWizard->Invoke(fname, 0))
    {
    this->UpdateAccordingToSeriesInput();
    }
}

//----------------------------------------------------------------------------
void vtkVVPlugin::UpdateAccordingToSeriesInput()
{
  if (!this->SeriesInputOpenWizard || !this->SeriesInputOpenWizard->GetFileName())
    {
    return;
    }

  this->PluginInfo.InputVolumeSeriesNumberOfVolumes = 
                      this->SeriesInputOpenWizard->GetSeriesMaximum() -
                      this->SeriesInputOpenWizard->GetSeriesMinimum() + 1;

  const char *fname = this->SeriesInputOpenWizard->GetFileName();
  ostrstream title;
  vtksys_stl::string basename = vtksys::SystemTools::GetFilenameName(fname);
  title << "Series Input: " << basename.c_str() << ends;
  this->SeriesInputButton->SetText(title.str());
  title.rdbuf()->freeze(0);
    
  // copy over the parameters now
  vtkImageData *input = this->SeriesInputOpenWizard->GetValidImageInformation();
  this->PluginInfo.InputVolumeSeriesScalarType = input->GetScalarType();
  this->PluginInfo.InputVolumeSeriesScalarSize = input->GetScalarSize();
    
  this->PluginInfo.InputVolumeSeriesNumberOfComponents = 
    input->GetNumberOfScalarComponents();
    
  int *wExt = input->GetWholeExtent();
  this->PluginInfo.InputVolumeSeriesDimensions[0] = wExt[1] - wExt[0] + 1;
  this->PluginInfo.InputVolumeSeriesDimensions[1] = wExt[3] - wExt[2] + 1;
  this->PluginInfo.InputVolumeSeriesDimensions[2] = wExt[5] - wExt[4] + 1;
  int i;
  for (i = 0; i < 3; i++)
    {
    this->PluginInfo.InputVolumeSeriesSpacing[i] = input->GetSpacing()[i];
    }
    
  // compute the ranges
  vtkDataArray *scalars = input->GetPointData()->GetScalars();
  double ftmp[2];
  if (scalars)
    {
    for (i = 0; i < this->PluginInfo.InputVolumeSeriesNumberOfComponents; ++i)
      {
      scalars->GetRange(ftmp,i);
      this->PluginInfo.InputVolumeSeriesScalarRange[2*i] = ftmp[0];
      this->PluginInfo.InputVolumeSeriesScalarRange[2*i+1] = ftmp[1];
      }
    }
  this->PluginInfo.InputVolumeSeriesScalarTypeRange[0] =input->GetScalarTypeMin();
  this->PluginInfo.InputVolumeSeriesScalarTypeRange[1] =input->GetScalarTypeMax();
    
  // we do adjust the origin because the plugin is zero extent based
  double *origin = input->GetOrigin();
  this->PluginInfo.InputVolumeSeriesOrigin[0] = 
    origin[0] + input->GetSpacing()[0]*input->GetWholeExtent()[0];
  this->PluginInfo.InputVolumeSeriesOrigin[1] = 
    origin[1] + input->GetSpacing()[1]*input->GetWholeExtent()[2];
  this->PluginInfo.InputVolumeSeriesOrigin[2] = 
    origin[2] + input->GetSpacing()[2]*input->GetWholeExtent()[4];

  // some of the GUI elements might depend on the series input, refresh
  // them now
  this->Update();
}
#endif
//ETX

//----------------------------------------------------------------------------
void vtkVVPlugin::SetReportText(const char *text)
{
  if (!this->IsCreated())
    {
    return;
    }

  this->ReportText->GetWidget()->SetText(text);

  this->Script("grid %s %s", 
               ((text && *text) ? "" : "remove"), 
               this->ReportText->GetWidgetName());
}

//----------------------------------------------------------------------------
char* vtkVVPlugin::GetReportText()
{
  return this->ReportText->GetWidget()->GetText();
}

//----------------------------------------------------------------------------
void vtkVVPlugin::SetStopWatchText(const char *text)
{
  if (!this->IsCreated())
    {
    return;
    }

  this->StopWatchText->GetWidget()->SetText(text);

  this->Script("grid %s %s", 
               ((text && *text) ? "" : "remove"), 
               this->StopWatchText->GetWidgetName());
}

//----------------------------------------------------------------------------
char* vtkVVPlugin::GetStopWatchText()
{
  return this->StopWatchText->GetWidget()->GetText();
}

//----------------------------------------------------------------------------
void vtkVVPlugin::Update()
{
  // Update enable state

  this->UpdateEnableState();

  // Update data (which will update the GUI too)

  if (this->Window)
    {
    vtkVVDataItemVolume *volume_data = vtkVVDataItemVolume::SafeDownCast(
                                  this->Window->GetSelectedDataItem());
    if( volume_data)
      this->UpdateData(volume_data->GetImageData());
    }

  // Update the GUI
  
  this->UpdateGUI();
}

//----------------------------------------------------------------------------
void vtkVVPlugin::UpdateData(vtkImageData *input)
{
  // update the markers if available
  if (this->Window)
    {
    // free/resize old markers if needed

#ifdef KWVolView_PLUGINS_USE_MARKER
    vtkVVDataItem *dataItem = this->Window->GetSelectedDataItem();
    int nb_of_markers = vtkVVHandleWidget::GetNumberOfHandlesInDataItem(
          dataItem);
    //vtkKW3DMarkersWidget *markers = 
    //  this->Window->GetVolumeWidget()->GetMarkers3D();
    //int nb_of_markers  = markers->GetNumberOfMarkers();
    if (this->PluginInfo.NumberOfMarkers != nb_of_markers)
      {
      if (this->PluginInfo.Markers)
        {
        delete [] this->PluginInfo.Markers;
        }
      if (this->PluginInfo.MarkersGroupId)
        {
        delete [] this->PluginInfo.MarkersGroupId;
        }
      if (nb_of_markers)
        {
        this->PluginInfo.Markers = new float [nb_of_markers * 3];
        this->PluginInfo.MarkersGroupId = new MarkersGroupIdType [nb_of_markers ];
        }
      else
        {
        this->PluginInfo.Markers = 0;
        this->PluginInfo.MarkersGroupId = 0;
        }
      this->PluginInfo.NumberOfMarkers = nb_of_markers;
      }
    float *ptr = this->PluginInfo.Markers;
    for (int i = 0; i < nb_of_markers; ++i)
      {
      vtkVVHandleWidget *handle = 
        vtkVVHandleWidget::GetNthHandleInDataItem( dataItem, i );
      double pos[3];
      handle->GetWorldPosition(pos);
      *ptr++ = pos[0];
      *ptr++ = pos[1];
      *ptr++ = pos[2];
      }

    // Managing Markers Groups
    // Not implemented as of now in vtkVVHandleWidget.
    MarkersGroupIdType *mgptr = this->PluginInfo.MarkersGroupId;
    for (int i = 0; i < nb_of_markers; ++i)
      {
      MarkersGroupIdType groupId = 0; //markers->GetMarkerGroupId(i);
      *mgptr++ = groupId;
      }
    int nb_of_markers_groups  = 1; //markers->GetNumberOfMarkersGroups();
    typedef char * charptr;
    if (this->PluginInfo.NumberOfMarkersGroups != nb_of_markers_groups)
      {
      if (this->PluginInfo.MarkersGroupName)
        {
        for(int i=0; i < this->PluginInfo.NumberOfMarkersGroups; i++)
          {
          if( this->PluginInfo.MarkersGroupName[i] )
            {
            delete []  this->PluginInfo.MarkersGroupName[i];
            }
          }
        delete [] this->PluginInfo.MarkersGroupName;
        }
      if (nb_of_markers_groups)
        {
        this->PluginInfo.MarkersGroupName = new charptr [nb_of_markers_groups ];
        for(int i=0; i < nb_of_markers_groups; ++i)
          {
          this->PluginInfo.MarkersGroupName[i] = 0;
          }
        }
      else
        {
        this->PluginInfo.MarkersGroupName = 0;
        }
      this->PluginInfo.NumberOfMarkersGroups = nb_of_markers_groups;
      }
    charptr * groupNames = this->PluginInfo.MarkersGroupName;
    for (int i = 0; i < nb_of_markers_groups; ++i, ++groupNames )
      {
      if( *groupNames )
        {
        delete []  (*groupNames);
        }
      const char * name  = "Seeds"; //markers->GetMarkersGroupName(i);
      const int len = (int)strlen( name ) + 1;
      *groupNames = new char [ len ];
      memcpy(*groupNames, name, len );
      }
#endif


    // set the cropping plane values
    // FIXME: Cropping planes need hooks..
    if (!this->PluginInfo.CroppingPlanes)
      {
      this->PluginInfo.CroppingPlanes = new float [6];
      this->PluginInfo.CroppingPlanes[0] = VTK_FLOAT_MIN;
      this->PluginInfo.CroppingPlanes[2] = VTK_FLOAT_MIN;
      this->PluginInfo.CroppingPlanes[4] = VTK_FLOAT_MIN;
      this->PluginInfo.CroppingPlanes[1] = VTK_FLOAT_MAX;
      this->PluginInfo.CroppingPlanes[3] = VTK_FLOAT_MAX;
      this->PluginInfo.CroppingPlanes[5] = VTK_FLOAT_MAX;
      }
    vtkVVDataItemVolume *volume_data = vtkVVDataItemVolume::SafeDownCast(
                                    this->Window->GetSelectedDataItem());
    
    if (volume_data && volume_data->GetVolumeWidget(this->Window))
      {
      double *vw_planes = volume_data->GetVolumeWidget(this->Window)->
                                                  GetCroppingPlanes();
      for (int i = 0; i < 6; i++)
        {
        this->PluginInfo.CroppingPlanes[i] = vw_planes[i];
        }
      }
    }
  
  if (!input)
    {
    return;
    }

  // let the plugin know about the new data
  this->PluginInfo.InputVolumeScalarType = input->GetScalarType();
  this->PluginInfo.InputVolumeScalarSize = input->GetScalarSize();

  this->PluginInfo.InputVolumeNumberOfComponents = 
    input->GetNumberOfScalarComponents();

  int i;
  for (i = 0; i < 3; i++)
    {
    this->PluginInfo.InputVolumeDimensions[i] = input->GetDimensions()[i];
    this->PluginInfo.InputVolumeSpacing[i] = input->GetSpacing()[i];
    }

  // compute the ranges
  vtkDataArray *scalars = input->GetPointData()->GetScalars();
  double ftmp[2];
  for (i = 0; i < this->PluginInfo.InputVolumeNumberOfComponents; ++i)
    {
    scalars->GetRange(ftmp,i);
    this->PluginInfo.InputVolumeScalarRange[2*i] = ftmp[0];
    this->PluginInfo.InputVolumeScalarRange[2*i+1] = ftmp[1];
    }
  this->PluginInfo.InputVolumeScalarTypeRange[0] = input->GetScalarTypeMin();
  this->PluginInfo.InputVolumeScalarTypeRange[1] = input->GetScalarTypeMax();
  
  // we do adjust the origin because the plugin is zero extent based
  double *origin = input->GetOrigin();
  this->PluginInfo.InputVolumeOrigin[0] = 
    origin[0] + input->GetSpacing()[0]*input->GetWholeExtent()[0];
  this->PluginInfo.InputVolumeOrigin[1] = 
    origin[1] + input->GetSpacing()[1]*input->GetWholeExtent()[2];
  this->PluginInfo.InputVolumeOrigin[2] = 
    origin[2] + input->GetSpacing()[2]*input->GetWholeExtent()[4];

  // Update the GUI
  this->UpdateGUI();
}

//----------------------------------------------------------------------------
void vtkVVPlugin::UpdateGUI()
{
  int i;

  this->PluginInfo.UpdateGUI(&this->PluginInfo);

  if (!this->Widgets)
    {
    return;
    }

  // for each GUI component ...
  for (i = 0; i < this->NumberOfGUIItems; ++i)
    {
    // update the actual widgets
    switch (this->GUIItems[i].GUIType) 
      {
      case VV_GUI_SCALE:
      {
      vtkKWScaleWithEntry *s = 
        vtkKWScaleWithEntry::SafeDownCast(this->Widgets[2*i+1]);
      double range[3];
      sscanf(this->GUIItems[i].Hints,"%lf %lf %lf",
             range, range+1, range+2);
      s->SetResolution(range[2]);
      s->SetRange(range[0],range[1]);
      s->SetLabelText(this->GUIItems[i].Label);
      if (this->GUIItems[i].Help)
        {
        s->SetBalloonHelpString(this->GUIItems[i].Help);
        }
      }
      break;
      case VV_GUI_CHOICE:
      {
      vtkKWLabel *l = vtkKWLabel::SafeDownCast(this->Widgets[2*i]);
      l->SetText(this->GUIItems[i].Label);
      vtkKWMenuButton *s = vtkKWMenuButton::SafeDownCast(this->Widgets[2*i+1]);
      ostrstream old_value;
      old_value << s->GetValue() << ends;
      ostrstream first_value;
      s->GetMenu()->DeleteAllItems();
      // extract the GUI from the hints
      int numEntries;
      sscanf(this->GUIItems[i].Hints,"%i",&numEntries);
      const char *pos = this->GUIItems[i].Hints;
      char tmp[1024];
      int j;
      for (j = 0; j < numEntries; ++j)
        {
        pos = strchr(pos,'\n') + 1;
        int k = 0;
        while (pos[k] != '\n' && pos[k] != '\0')
          {
          tmp[k] = pos[k];
          k++;
          }
        tmp[k] = '\0';
        s->GetMenu()->AddRadioButton(tmp);
        if (!j)
          {
          first_value << tmp << ends;
          }
        }
      if (s->GetMenu()->HasItem(old_value.str()))
        {
        s->SetValue(old_value.str());
        }
      else
        {
        s->SetValue(first_value.str());
        }
      old_value.rdbuf()->freeze(0);
      first_value.rdbuf()->freeze(0);

      if (this->GUIItems[i].Help)
        {
        s->SetBalloonHelpString(this->GUIItems[i].Help);
        }
      }
      break;
      case VV_GUI_CHECKBOX:
      {
      vtkKWCheckButton *s = 
        vtkKWCheckButton::SafeDownCast(this->Widgets[2*i+1]);
      s->SetText(this->GUIItems[i].Label);
      if (this->GUIItems[i].Help)
        {
        s->SetBalloonHelpString(this->GUIItems[i].Help);
        }
      }
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkVVPlugin::GetGUIValues()
{
  if (!this->Widgets)
    {
    return;
    }

  int i;
  char tmp[1024];
  for (i = 0; i < this->NumberOfGUIItems; ++i)
    {
    switch (this->GUIItems[i].GUIType) 
      {
      case VV_GUI_SCALE:
      {
      vtkKWScaleWithEntry *s = 
        vtkKWScaleWithEntry::SafeDownCast(this->Widgets[2*i+1]);
      float v = s->GetValue();
      sprintf(tmp,"%f",v);
      this->SetGUIProperty(i, VVP_GUI_VALUE, tmp);
      }
      break;
      case VV_GUI_CHOICE:
      {
      vtkKWMenuButton *s = vtkKWMenuButton::SafeDownCast(this->Widgets[2*i+1]);
      this->SetGUIProperty(i, VVP_GUI_VALUE, s->GetValue());
      }
      break;
      case VV_GUI_CHECKBOX:
      {
      vtkKWCheckButton *s = 
        vtkKWCheckButton::SafeDownCast(this->Widgets[2*i+1]);
      sprintf(tmp, "%i",s->GetSelectedState());
      this->SetGUIProperty(i, VVP_GUI_VALUE, tmp);
      }
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkVVPlugin::SetGUIValues()
{
  if (!this->Widgets)
    {
    return;
    }

  int i;
  for (i = 0; i < this->NumberOfGUIItems; ++i)
    {
    if (!this->GUIItems[i].Value)
      {
      continue;
      }
    switch (this->GUIItems[i].GUIType) 
      {
      case VV_GUI_SCALE:
      {
      vtkKWScaleWithEntry *s = 
        vtkKWScaleWithEntry::SafeDownCast(this->Widgets[2*i+1]);
      s->SetValue(atof(this->GUIItems[i].Value));
      }
      break;
      case VV_GUI_CHOICE:
      {
      vtkKWMenuButton *s = vtkKWMenuButton::SafeDownCast(this->Widgets[2*i+1]);
      s->SetValue(this->GUIItems[i].Value);
      }
      break;
      case VV_GUI_CHECKBOX:
      {
      vtkKWCheckButton *s = 
        vtkKWCheckButton::SafeDownCast(this->Widgets[2*i+1]);
      s->SetSelectedState(atoi(this->GUIItems[i].Value));
      }
      break;
      }
    }
}

//----------------------------------------------------------------------------
// return 0 if there is not enough memory
// return 1 if there is enough memory but do not keep the input
// return 2 if there is enough memory and you can keep the input
int vtkVVPlugin::CheckMemory(vtkImageData *input)
{
  int *dim = input->GetDimensions();

  // how much memory do we need
  // compute in size as voxels for now 
  vtkLargeInteger inSize;
  int *ext = input->GetExtent();
  inSize = (ext[1]-ext[0]);
  inSize *= (ext[3]-ext[2]); 
  inSize *= (ext[5]-ext[4]);
  
  int outVolScalarSize = 1;
  switch (this->PluginInfo.OutputVolumeScalarType)
    {
    case VTK_FLOAT:
      outVolScalarSize = sizeof(float);
      break;
    case VTK_DOUBLE:
      outVolScalarSize = sizeof(double);
      break;
    case VTK_INT:
    case VTK_UNSIGNED_INT:
      outVolScalarSize = sizeof(int);
      break;
    case VTK_LONG:
    case VTK_UNSIGNED_LONG:
      outVolScalarSize = sizeof(long);
      break;
    case VTK_SHORT:
    case VTK_UNSIGNED_SHORT:
      outVolScalarSize = sizeof(short);
      break;
    }

  vtkLargeInteger outSize;
  outSize = this->PluginInfo.OutputVolumeDimensions[0];
  outSize *= this->PluginInfo.OutputVolumeDimensions[1]; 
  outSize *= this->PluginInfo.OutputVolumeDimensions[2];
  outSize *= this->PluginInfo.OutputVolumeNumberOfComponents;
  outSize *= outVolScalarSize;

  // how much intermediate space is required
  vtkLargeInteger plugSize;
  // special handling for float, we get one decimal of precision
  // PerVoxelMemory Required is bytes per voxel
  plugSize = inSize/ 10;
  plugSize = plugSize * (int)(10*this->PerVoxelMemoryRequired);

  // now make inSize in bytes not voxels
  inSize *= input->GetNumberOfScalarComponents();
  inSize *= input->GetScalarSize();

  vtkKWProcessStatistics *pr = vtkKWProcessStatistics::New();
  long av = pr->GetAvailableVirtualMemory();
  long ap = pr->GetAvailablePhysicalMemory();
  long tv = pr->GetTotalVirtualMemory();
  long tp = pr->GetTotalPhysicalMemory();
  
  pr->Delete();
  // if we received bogus results from pr just return
  if ( av < 0 || ap < 0 || tv < 0 || tp < 0 )
    {
    // assume everything is OK
    return 2;
    }
  
//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
  if (this->RequiresSeriesInput)
    {
    vtkLargeInteger seriesOneVolumeSize = this->PluginInfo.InputVolumeSeriesScalarSize;
    seriesOneVolumeSize = 
      seriesOneVolumeSize * this->PluginInfo.InputVolumeSeriesNumberOfComponents;
    for(int k=0; k<3; k++)
      {
      seriesOneVolumeSize = 
        seriesOneVolumeSize * this->PluginInfo.InputVolumeSeriesDimensions[k];
      }
    if(this->SupportProcessingSeriesByVolumes)
      {
      plugSize = inSize + seriesOneVolumeSize;
      }
    else
      {
      vtkLargeInteger seriesTotalSize = 
        seriesOneVolumeSize * this->PluginInfo.InputVolumeSeriesNumberOfVolumes; 
      plugSize = inSize + seriesTotalSize;
      }
    }
#endif
//ETX

  // how much memory do we need to run the plugin? There are a few different
  // options on how the plugin can be run.
  vtkLargeInteger runSize;
  vtkLargeInteger runPiecesSize;
  vtkLargeInteger runInPlaceSize;
  
  runSize = outSize + plugSize;
  runPiecesSize = outSize + plugSize;
  runPiecesSize *= 2;
  runInPlaceSize = outSize + plugSize;
  runInPlaceSize *= 2;
  
  // is the plugin is in place then we can ignore outSize
  if (this->SupportInPlaceProcessing)
    {
    runInPlaceSize = plugSize;
    }
  
  if (this->SupportProcessingPieces && 
      this->PluginInfo.OutputVolumeDimensions[0] == dim[0] &&
      this->PluginInfo.OutputVolumeDimensions[1] == dim[1] &&
      this->PluginInfo.OutputVolumeDimensions[2] == dim[2] &&
      this->PluginInfo.OutputVolumeScalarType == input->GetScalarType() &&
      this->PluginInfo.OutputVolumeNumberOfComponents == 
      input->GetNumberOfScalarComponents())
    {
    // ten pieces = 1/10 (then divide by 10 to handle floats)
    runPiecesSize = inSize / 100;
    runPiecesSize = runPiecesSize * (int)(10*this->PerVoxelMemoryRequired);
    // two output buffers each at 1/10 = 1/5
    runPiecesSize = outSize/5 + runPiecesSize;
    }


  // convert into K
  runSize = runSize / 1000;
  runInPlaceSize = runInPlaceSize / 1000;
  runPiecesSize = runPiecesSize / 1000;
  inSize = inSize / 1000;
  outSize = outSize / 1000;
  plugSize = plugSize / 1000;

  // now what is the deal, do we have enough memory?   
  if ( runSize.CastToUnsignedLong() <= .5 * tp &&
       runSize.CastToUnsignedLong() <= .8 * av)
    {
    // everything is OK and we can keep the input
    return 2;
    }
  
  // we can't fit the input and the output easily Can we do it at all? This
  // requires pieces or InPlace processing
  if ( runPiecesSize.CastToUnsignedLong() < .9*av ||
       runInPlaceSize.CastToUnsignedLong() < .9*av )
    {
    if (vtkKWMessageDialog::PopupYesNo( 
          this->GetApplication(), this->Window, "Apply Plugin",
          "Applying this plugin to your data will require most of your "
          "computer's memory. This could result in reduced performance "
          "of VolView. Due to the memory limits you will not be able to "
          "Undo this operation. Do you wish to continue?"
          , vtkKWMessageDialog::WarningIcon ) )
      {
      return 1;
      }
    else
      {
      return 0;
      }
    }

  // no way to load the data
  char buffer[1024];
  sprintf(buffer, "Applying this plugin to your data will NOT fit in your system memory. Please close some applications, increase the amount of swap space, or increase the amount of memory in the computer.\n\nNote: your available memory was estimated at %ld MB (physical or virtual), running this plugin will require %ld MB, in place %ld MB (%s supported), in pieces %ld MB (%s supported). Should this estimation be way off, you can attempt to ignore this message, but be aware that this application may crash.", 
          ((long)av / (long)1024), 
          (long)(runSize.CastToLong() / 1024), 
          (long)(runInPlaceSize.CastToLong() / 1024), 
          (this->SupportInPlaceProcessing ? "is" : "not"),
          (long)(runPiecesSize.CastToLong() / 1024),
          (this->SupportProcessingPieces ? "is" : "not")
    );
  if (vtkKWMessageDialog::PopupYesNo( 
        this->GetApplication(), this->Window, "Apply Plugin",
        buffer, vtkKWMessageDialog::WarningIcon))
    {
    return 2;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkVVPlugin::CanBeExecuted(vtkVVPluginSelector *vtkNotUsed(plugins))
{
  vtkVVDataItemVolume *volume_data = vtkVVDataItemVolume::SafeDownCast(
                                  this->Window->GetSelectedDataItem());
  if( volume_data)
    return 
      (this->Window && volume_data->GetImageData() && 
       (!this->RequiresSecondInput || this->SecondInputOptional ||
        (this->SecondInputOpenWizard && 
         this->SecondInputOpenWizard->GetReadyToLoad() != 
         vtkKWOpenWizard::DATA_IS_UNAVAILABLE))
//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
       &&
       (!this->RequiresSeriesInput || 
        (this->SeriesInputOpenWizard && 
         this->SeriesInputOpenWizard->GetReadyToLoad() != 
         vtkKWOpenWizard::DATA_IS_UNAVAILABLE)) 
#endif
//ETX
       );
  else 
    return false;
}

//----------------------------------------------------------------------------
int vtkVVPlugin::PreparePlugin(vtkVVPluginSelector *plugins)
{
  if (!this->CanBeExecuted(plugins))
    {
    this->SetReportText(
      "Plugin can not be executed, some parameters might be missing "
      "or wrong.");
    return 1;
    }

  // Ensure that a paintbrush widget is selected, if its required by the plugin
  if (this->RequiresLabelInput && !this->GetInputLabelImage())
    {
    this->SetReportText(
      "Plugin can not be executed. This plugin requires a labelmap input. "
      "A paintbrush sketch must be selected in the Widgets panel.");
    return 1;
    }

  // Ensure that a paintbrush widget is selected, if its required by the plugin
  if (this->RequiresLabelInput && !this->GetInputLabelImage())
    {
    this->SetReportText(
      "Plugin can not be executed. This plugin requires a labelmap input. "
      "A paintbrush sketch must be selected in the Widgets panel.");
    return 1;
    }
  
  // Load the second input if it is required

  if (this->RequiresSecondInput && this->SecondInputOpenWizard)
    {
    this->SecondInputOpenWizard->Load(0);
    }

//BTX
#ifdef KWVolView_PLUGINS_USE_SPLINE
  // Resample the spline surfaces if it  is required
  if( this->RequiresSplineSurfaces )
    {
    this->PrepareSplineSurfaces();
    }
#endif
//ETX

  return 0;
}

//BTX
#ifdef KWVolView_PLUGINS_USE_SPLINE
//----------------------------------------------------------------------------
int vtkVVPlugin::PrepareSplineSurfaces()
{
  // passing the spline surfaces as an array of points
  vtkKW3DSplineSurfacesWidget * surfacesWidget =
     this->Window->GetVolumeWidget()->GetSplineSurfaces3D();

  unsigned int numberOfSplineSurfaces = surfacesWidget->GetNumberOfSplineSurfaces();

  // Increase the resolution of the splines in order to ensure a density 
  // higher than the one in the image. For Subdivision splines, the final
  // number of points is proportional to handles * ( 4 ^ resolution ).
  unsigned int * previousResolution = new unsigned int [numberOfSplineSurfaces];

  vtkKW3DSplineSurfacesWidget::Iterator itrSpline = surfacesWidget->Begin();
  vtkKW3DSplineSurfacesWidget::Iterator endSpline = surfacesWidget->End();
  unsigned int k=0; 
  while( itrSpline != endSpline )
    {
    vtkSplineSurfaceWidget * splineSurface = itrSpline->second;
    try
      {
      vtkSubdivisionSplineSurfaceWidget * subdivisionSplineSurface =
          dynamic_cast<vtkSubdivisionSplineSurfaceWidget *>( splineSurface );
      previousResolution[k] = subdivisionSplineSurface->GetResolution();
      subdivisionSplineSurface->SetResolution( previousResolution[k] + 4 ); 
      subdivisionSplineSurface->GenerateSurfacePoints();
      }
    catch(...)
      {
      // find and alternative API for increasing resolution in the surface generation
      }
    ++itrSpline;
    ++k;
    }

  int arrayOfSurfacePointsMustBeCleared = 0;
  unsigned int totalNumberOfSplineSurfacePoints = 0;
  if(this->PluginInfo.NumberOfSplineSurfaces != numberOfSplineSurfaces)
    {
    if(this->PluginInfo.NumberOfSplineSurfacePoints)
      {
      delete [] this->PluginInfo.NumberOfSplineSurfacePoints;
      }
    this->PluginInfo.NumberOfSplineSurfacePoints = new int[ numberOfSplineSurfaces ];

    itrSpline = surfacesWidget->Begin();
    endSpline = surfacesWidget->End();
    unsigned i=0; 
    while( itrSpline != endSpline )
      {
      this->PluginInfo.NumberOfSplineSurfacePoints[i] = 
        surfacesWidget->GetNumberOfPointsInASplineSurface(itrSpline->first.c_str());
      totalNumberOfSplineSurfacePoints += 
            this->PluginInfo.NumberOfSplineSurfacePoints[i]; 
      ++i;
      ++itrSpline;
      }
    arrayOfSurfacePointsMustBeCleared = 1;
    this->PluginInfo.NumberOfSplineSurfaces = numberOfSplineSurfaces;
    }
  else
    {
    unsigned int nss=0;
    itrSpline = surfacesWidget->Begin();
    endSpline = surfacesWidget->End();
    unsigned i=0; 
    while( itrSpline != endSpline )
      {
      unsigned int numberOfPoints = 
        surfacesWidget->GetNumberOfPointsInASplineSurface( itrSpline->first.c_str() );
      if( this->PluginInfo.NumberOfSplineSurfacePoints[nss] != numberOfPoints )
        {
        this->PluginInfo.NumberOfSplineSurfacePoints[nss] = numberOfPoints;
        arrayOfSurfacePointsMustBeCleared = 1;
        }
      ++itrSpline;
      ++nss;
      }
    }

  if( arrayOfSurfacePointsMustBeCleared )
    {
    if(this->PluginInfo.SplineSurfacePoints)
      {
      delete [] this->PluginInfo.SplineSurfacePoints;
      }
    this->PluginInfo.SplineSurfacePoints = new float[3*totalNumberOfSplineSurfacePoints];
    }

  // Copy the points coordinates
  unsigned int numberOfPointCoordinates = 0;
  itrSpline = surfacesWidget->Begin();
  endSpline = surfacesWidget->End();
  unsigned sp=0; 
  while( itrSpline != endSpline )
    {
    const unsigned int numberOfPointsInThisSurface = 
             this->PluginInfo.NumberOfSplineSurfacePoints[sp];
    vtkPoints * points = surfacesWidget->GetPointsInASplineSurface(itrSpline->first.c_str());
    double point[3];
    for(unsigned int p=0; p<numberOfPointsInThisSurface; p++)
      {
      points->GetPoint(p, point );
      this->PluginInfo.SplineSurfacePoints[numberOfPointCoordinates++] = point[0];
      this->PluginInfo.SplineSurfacePoints[numberOfPointCoordinates++] = point[1];
      this->PluginInfo.SplineSurfacePoints[numberOfPointCoordinates++] = point[2];
      }
    ++itrSpline;
    ++sp;
    }

  // Restore the original resolution of the spline surfaces. 
  itrSpline = surfacesWidget->Begin();
  endSpline = surfacesWidget->End();
  unsigned kk=0; 
  while( itrSpline != endSpline )
    {
    vtkSplineSurfaceWidget * splineSurface = 
      surfacesWidget->GetSplineSurfaceWidget(itrSpline->first.c_str());
    try
      {
      vtkSubdivisionSplineSurfaceWidget * subdivisionSplineSurface =
          dynamic_cast<vtkSubdivisionSplineSurfaceWidget *>( splineSurface );
      subdivisionSplineSurface->SetResolution( previousResolution[kk] );
      subdivisionSplineSurface->GenerateSurfacePoints();
      }
    catch(...)
      {
      // find and alternative API for increasing resolution in the surface generation
      }
    ++itrSpline;
    ++kk;
    }


  delete [] previousResolution;

  return 0;
}
#endif
//ETX

//----------------------------------------------------------------------------
// Get the label map image, if any for this volume. If there are more than
// one, get the first one
vtkImageData * vtkVVPlugin::GetInputLabelImage()
{
  if (!this->RequiresLabelInput)
    {
    return NULL;
    }

  if (vtkVVWindow *win = vtkVVWindow::SafeDownCast(this->Window))
    {
    if (vtkVVWidgetInterface *wi = win->GetWidgetInterface())
      {
      vtkVVInteractorWidgetSelector *iws = wi->GetInteractorWidgetSelector();

      // If we have selected a paintbrush widget, use it

      int id = iws->GetIdOfSelectedPreset();
      if (id != -1)
        {
        if (vtkKWEPaintbrushWidget *paintbrush =
              vtkKWEPaintbrushWidget::SafeDownCast(
                iws->GetPresetInteractorWidget(id)))
          {
          vtkKWEPaintbrushRepresentation * rep =
            vtkKWEPaintbrushRepresentation::SafeDownCast(paintbrush->GetRepresentation());
          vtkKWEPaintbrushDrawing *drawing = rep->GetPaintbrushDrawing();
          return vtkKWEPaintbrushLabelData::SafeDownCast(
              drawing->GetPaintbrushData())->GetLabelMap();
          }
        }

      // Use any paintbrush in the list, since one is not selected.

      if (vtkVVSelectionFrame *sel_frame = 
          this->Window->GetSelectedSelectionFrame())
        {
        const int nb_interactors = sel_frame->GetNumberOfInteractorWidgets();
        for (int i = 0; i < nb_interactors; i++)
          {
          if (vtkKWEPaintbrushWidget *paintbrush = 
                vtkKWEPaintbrushWidget::SafeDownCast(
                    sel_frame->GetNthInteractorWidget(i)))
            {
            // Select this one.
            iws->SelectPreset(iws->GetIdOfInteractorWidget(paintbrush));
            vtkKWEPaintbrushRepresentation * rep =
              vtkKWEPaintbrushRepresentation::SafeDownCast(paintbrush->GetRepresentation());
            vtkKWEPaintbrushDrawing *drawing = rep->GetPaintbrushDrawing();
            return vtkKWEPaintbrushLabelData::SafeDownCast(
                drawing->GetPaintbrushData())->GetLabelMap();
            }
          }
        }

      // Since there isn't a paintbrush widget for this dataitem, create one

      wi->InteractorWidgetAddDefaultInteractorCallback(
          vtkVVInteractorWidgetSelector::PaintbrushWidget);
      id = iws->GetIdOfSelectedPreset();
      if (id != -1)
        {
        if (vtkKWEPaintbrushWidget *paintbrush =
              vtkKWEPaintbrushWidget::SafeDownCast(
                iws->GetPresetInteractorWidget(id)))
          {
          vtkKWEPaintbrushRepresentation * rep =
            vtkKWEPaintbrushRepresentation::SafeDownCast(paintbrush->GetRepresentation());
          vtkKWEPaintbrushDrawing *drawing = rep->GetPaintbrushDrawing();
          return vtkKWEPaintbrushLabelData::SafeDownCast(
              drawing->GetPaintbrushData())->GetLabelMap();
          }
        }
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkVVPlugin::Execute(vtkVVPluginSelector *plugins)
{
  this->SetReportText(NULL);

  if (this->GetStopWatchText() && *this->GetStopWatchText())
    {
    this->SetStopWatchText("Executing...");
    }
  
  // Validate params and load second input

  if (this->PreparePlugin(plugins))
    {
    return;
    }

  clock_t start_clock = clock();

  vtkVVDataItemVolume *volume_data = vtkVVDataItemVolume::SafeDownCast(
                                  this->Window->GetSelectedDataItem());
  if (!volume_data) return;

  // Execute the plugin
  this->ExecuteData(volume_data->GetImageData(), plugins);

  clock_t delta = clock() - start_clock;

  // Free the second input if it is required

  if (this->RequiresSecondInput && this->SecondInputOpenWizard)
    {
    this->SecondInputOpenWizard->Release(0);
    }

//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
  // Free the series input if it is required

  if (this->RequiresSeriesInput && this->SeriesInputOpenWizard)
    {
    this->SeriesInputOpenWizard->Release();
    }
#endif
//ETX


  this->GetWindow()->SetStatusText("");
  this->GetWindow()->GetProgressGauge()->SetValue(0);

  if (this->AbortProcessing)
    {
    this->SetReportText("Plugin execution was canceled!");
    }

  // Display how long it took

  char buf[100];
  sprintf(buf, "Done in %0.2f s.", (double)(delta) / (double)CLOCKS_PER_SEC);
  this->SetStopWatchText(buf);

  vtkImageData *inLabelImage = this->GetInputLabelImage();
  if (inLabelImage && this->RequiresLabelInput)
    {
    inLabelImage->Modified();

    // Collapse the history. Since the plugins modify the paintbrush data under
    // the hood, we can't manage undo/redo here.
    if (vtkKWEPaintbrushDrawing *drawing = this->GetPaintbrushDrawing())
      {
      drawing->CollapseHistory();
      }
    }

}

//----------------------------------------------------------------------------
void vtkVVPlugin::Cancel(vtkVVPluginSelector *vtkNotUsed(plugins))
{
  this->SetProperty(VVP_ABORT_PROCESSING,"1");
}

//----------------------------------------------------------------------------
void vtkVVPlugin::ExecuteData(vtkImageData *input, vtkVVPluginSelector *plugins)
{
  if (!input)
    {
    return;
    }

  // based on the parameters of the plugin, process the input data to produce
  // a new result. Make sure the properties are up to date as well
  this->UpdateData(input);
  
  // when plugin execute we break the connection with the reader
  //input->SetSource(0);

  // the structure that gets passed to the plugin
  vtkVVProcessDataStruct pds;
  this->AbortProcessing = 0;
  this->ProgressMinimum = 0;
  this->ProgressMaximum = 1;
//BTX
#ifdef KWVolView_PLUGINS_USE_SPLINE
  pds.NumberOfMeshPoints = 0;
  pds.MeshScalars = 0;
  pds.MeshNormals = 0;
#endif
#ifdef KWVolView_PLUGINS_USE_SERIES
  pds.inDataSeries = 0;
  pds.outDataSeries = 0;
#endif
//ETX
  pds.outDataPlotting = 0;
  
  // make sure the GUI values are up to date
  this->GetGUIValues();
  this->Update();

  // clear any undo data
  plugins->SetUndoData(0);
  this->SetResultingDistanceUnits(0);
  this->SetResultingComponent1Units(0);
  this->SetResultingComponent2Units(0);
  this->SetResultingComponent3Units(0);
  this->SetResultingComponent4Units(0);
  this->ResultingComponentsAreIndependent = -1;
   
  // Get the paintbrush label image.
  vtkImageData *inLabelImage = this->GetInputLabelImage();
  pds.inLabelData = inLabelImage ? inLabelImage->GetScalarPointer() : NULL;

  // set the second input for those that use it
  if (this->RequiresSecondInput && 
      !this->SecondInputIsUnstructuredGrid && 
      this->SecondInputOpenWizard && 
      this->SecondInputOpenWizard->GetOutput(0))
    {
    pds.inData2 = this->SecondInputOpenWizard->GetOutput(0)->GetScalarPointer();
    } 

  if (this->RequiresSecondInput && 
      this->SecondInputOpenWizard && 
      this->SecondInputIsUnstructuredGrid && 
      this->SecondInputOpenWizard->GetUnstructuredGridOutput(0))
    {
    pds.inData2 = (void *)(this->SecondInputOpenWizard->GetUnstructuredGridOutput(0));
    }
  
//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
  // set the series input for those that use it
  if (this->RequiresSeriesInput && 
      this->SeriesInputOpenWizard && 
      this->SeriesInputOpenWizard->GetLastReader() &&
     !this->SupportProcessingSeriesByVolumes )
    {
    // Replace this with an allocation for N x DataSet size buffer,
    // memcpying the buffer of all the input datasets in the serie.
    int numberOfSeries = this->PluginInfo.InputVolumeSeriesNumberOfVolumes;
    int volumeSizeInBytes = this->PluginInfo.InputVolumeSeriesScalarSize;
    for(int k=0; k<3; k++)
      {
      volumeSizeInBytes *= this->PluginInfo.InputVolumeSeriesDimensions[k];
      }
    pds.inDataSeries = new char [ numberOfSeries * volumeSizeInBytes ];
    for(int s=0; s<numberOfSeries; s++)
      {
      char * destination = (char *)(pds.inDataSeries)+(s*volumeSizeInBytes);
      vtkImageData * volume = this->SeriesInputOpenWizard->GetSeriesOutput(s);
      if( volume && volume->GetScalarPointer() )
        {
        memcpy(destination, volume->GetScalarPointer(), volumeSizeInBytes);
        }
      else
        {
        // if any of the datasets is not available, series data cannot be
        // passed to the pluging.
        if( pds.inDataSeries )
          {
          delete [] (char *)(pds.inDataSeries);
          }
        pds.inDataSeries = 0;
        break;
        }
      }
    }
#endif
 
#ifdef KWVolView_PLUGINS_USE_SPLINE
  // if it only does a mesh handle that
  if (this->ProducesMeshOnly)
    {
    pds.inData = input->GetScalarPointer();
    pds.outData = input->GetScalarPointer();
    pds.StartSlice = 0;
#ifdef KWVolView_PLUGINS_USE_SERIES
    pds.CurrentVolumeFromSeries = 0;
#endif
    pds.NumberOfSlicesToProcess = input->GetDimensions()[2];
    this->PluginInfo.ProcessData(&this->PluginInfo, &pds);
    // update the GUI
    plugins->Update();
    return;
    }
#endif

#ifdef KWVolView_PLUGINS_USE_SERIES
  // if it produces a volume series as output
  if (this->ProducesSeriesOutput)
    {
    pds.inData = input->GetScalarPointer();
    pds.outData = input->GetScalarPointer();
    pds.StartSlice = 0;
    pds.CurrentVolumeFromSeries = 0;
    pds.NumberOfSlicesToProcess = 0;

    int outVolScalarSize = 1;
    switch (this->PluginInfo.OutputVolumeSeriesScalarType)
      {
      case VTK_FLOAT:
        outVolScalarSize = sizeof(float);
        break;
      case VTK_DOUBLE:
        outVolScalarSize = sizeof(double);
        break;
      case VTK_INT:
      case VTK_UNSIGNED_INT:
        outVolScalarSize = sizeof(int);
        break;
      case VTK_LONG:
      case VTK_UNSIGNED_LONG:
        outVolScalarSize = sizeof(long);
        break;
      case VTK_SHORT:
      case VTK_UNSIGNED_SHORT:
        outVolScalarSize = sizeof(short);
        break;
      }
    this->PluginInfo.OutputVolumeSeriesScalarSize = outVolScalarSize;

    // Allocate memory for receiving the output series in a single block.
    const int outputSeriesSize = 
      this->PluginInfo.OutputVolumeSeriesNumberOfVolumes *
      this->PluginInfo.OutputVolumeSeriesScalarSize *
      this->PluginInfo.OutputVolumeSeriesNumberOfComponents *
      this->PluginInfo.OutputVolumeSeriesDimensions[0] *
      this->PluginInfo.OutputVolumeSeriesDimensions[1] *
      this->PluginInfo.OutputVolumeSeriesDimensions[2];
    
    if( outputSeriesSize )
      {
      pds.outDataSeries = new char[ outputSeriesSize ];
      }
    }
#endif
//ETX

  // otherwise check first for memory issues
  int memCheck = this->CheckMemory(input);
  if (!memCheck)
    {
    return;
    }

  // For plugins that produce as output array data to be plotted in 2D.
  // Here we allocate the memory needed for returning the data array.
  // We assume that the memory size of this array is negligeable compared
  // to the volume data itself, therefore we don't count this array on the
  // CheckMemory() function above.
  if( this->ProducesPlottingOutput )
    {
    const int arraySize = 
      this->PluginInfo.OutputPlottingNumberOfRows *
      this->PluginInfo.OutputPlottingNumberOfColumns;
    if( arraySize )
      {
      pds.outDataPlotting = new double[arraySize];
      }
    }

//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
  if( !this->RequiresSeriesInput && !this->ProducesSeriesOutput )
#endif
//ETX
    {
    // if we have the memory, and the plugin is not expecting a series
    // on a volume-by-volume basis, then just pass it in in one piece
    if (memCheck == 2)
      {
      this->ProcessInOnePiece(input, memCheck, &pds, plugins);
      this->DisplayPlot(&pds);
      return;
      }
    
    // if it supports in place processing then that is the easiest
    // but if we have the memory we might as well keep the input
    // around 
    if (this->SupportInPlaceProcessing)
      {
      // make sure the other values are consistent with in place 
      // processing as well
      if (this->PluginInfo.OutputVolumeDimensions[0] !=
          input->GetDimensions()[0] ||
          this->PluginInfo.OutputVolumeDimensions[1] !=
          input->GetDimensions()[1] ||
          this->PluginInfo.OutputVolumeDimensions[2] !=
          input->GetDimensions()[2] ||
          this->PluginInfo.OutputVolumeScalarType !=
          input->GetScalarType())
        {
        vtkErrorMacro("A plugin specified incorrectly that it could perform in place processing!");
        return;
        }
      pds.inData = input->GetScalarPointer();
      pds.outData = input->GetScalarPointer();
      pds.StartSlice = 0;
      pds.CurrentVolumeFromSeries = 0;
      pds.NumberOfSlicesToProcess = input->GetDimensions()[2];
      this->PluginInfo.ProcessData(&this->PluginInfo, &pds);
      input->Modified();
      this->PushNewProperties();
      this->DisplayPlot(&pds);
      return;
      }

    // handle the next case which is that the output type and extent are the
    // same as the input, but it cannot process in place
    int *dim = input->GetDimensions();
    if (this->SupportProcessingPieces && 
        this->PluginInfo.OutputVolumeDimensions[0] == dim[0] &&
        this->PluginInfo.OutputVolumeDimensions[1] == dim[1] &&
        this->PluginInfo.OutputVolumeDimensions[2] == dim[2] &&
        this->PluginInfo.OutputVolumeScalarType == input->GetScalarType() &&
        this->PluginInfo.OutputVolumeNumberOfComponents == 
        input->GetNumberOfScalarComponents() 
//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
        && !this->RequiresSeriesInput 
#endif
//ETX
      )
      {
      this->ProcessInPieces(input,memCheck, &pds);
      }
    }
//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
  else
    {
    // if we have the memory, and the plugin is not expecting a series
    // on a volume-by-volume basis, then just pass it in in one piece
    if (memCheck == 2 && !this->SupportProcessingSeriesByVolumes)
      {
      this->ProcessInOnePiece(input, memCheck, &pds, plugins);
      }
    else
      {
      if( this->RequiresSeriesInput && this->SupportProcessingSeriesByVolumes )
        {
        // this plugin can process an input series, volume by volume.
        this->ProcessSeriesByVolumes(input,memCheck, &pds, plugins);
        }
      else
        {
        // if all else failed, process in one piece and discard the input
        this->ProcessInOnePiece(input, memCheck, &pds, plugins);
        }
      }

    // Release intermediate memory that may have been used for passing the series
    // input data.
    if( pds.inDataSeries )
      {
      delete [] (char *)(pds.inDataSeries);
      pds.inDataSeries = 0;
      }

    // If the plugin produced a volume series as output we deal here with
    // saving those volumes.
    if( this->ProducesSeriesOutput )
      {
      this->SaveVolumeSeries(&pds);
      
      // Release intermediate memory that may have been used for passing the series
      // output data.
      if( pds.outDataSeries )
        {
        delete [] (char *)(pds.outDataSeries);
        pds.outDataSeries = 0;
        }
      }
    }
#endif
//ETX
  this->DisplayPlot(&pds);
  
  // The paintbrush may have been modified. Update timestamp
  if (inLabelImage && this->RequiresLabelInput)
    {
    inLabelImage->Modified();

    // Collapse the history. Since the plugins modify the paintbrush data under
    // the hood, we can't manage undo/redo here.
    if (vtkKWEPaintbrushDrawing *drawing = this->GetPaintbrushDrawing())
      {
      drawing->CollapseHistory();
      }
    }
}

//----------------------------------------------------------------------------
vtkKWEPaintbrushDrawing * vtkVVPlugin::GetPaintbrushDrawing()
{
  if (vtkVVWindow *win = vtkVVWindow::SafeDownCast(this->Window))
    {
    if (vtkVVWidgetInterface *wi = win->GetWidgetInterface())
      {
      vtkVVInteractorWidgetSelector *iws = wi->GetInteractorWidgetSelector();

      // If we have selected a paintbrush widget, use it

      int id = iws->GetIdOfSelectedPreset();
      if (id != -1)
        {
        if (vtkKWEPaintbrushWidget *paintbrush =
              vtkKWEPaintbrushWidget::SafeDownCast(
                iws->GetPresetInteractorWidget(id)))
          {
          vtkKWEPaintbrushRepresentation * rep =
            vtkKWEPaintbrushRepresentation::SafeDownCast(paintbrush->GetRepresentation());
          vtkKWEPaintbrushDrawing *drawing = rep->GetPaintbrushDrawing();        return drawing;
          }
        }
      }
    }

  return NULL;
}
  
//----------------------------------------------------------------------------
void vtkVVPlugin::ProcessInPieces(vtkImageData *input, 
                                  int vtkNotUsed(memCheck),
                                  vtkVVProcessDataStruct *pds)
{
  // break the input volume into pieces and pass them into the plugin
  // allocate a temp buffers to store the output
  int *dim = input->GetDimensions();
  int bufferSlices = dim[2]/10;
  if (bufferSlices < this->RequiredZOverlap)
    {
    bufferSlices =this->RequiredZOverlap;
    }
  unsigned char *buffer1 = new unsigned char [
    input->GetScalarSize()*bufferSlices*
    input->GetNumberOfScalarComponents()*dim[0]*dim[1]];
  unsigned char *buffer2 = new unsigned char [
    input->GetScalarSize()*bufferSlices*
    input->GetNumberOfScalarComponents()*dim[0]*dim[1]];
  unsigned char *bufferPtr;
  int buffer1Size;
  int buffer2Size = 0;
  int buffer1Slice = 0;
  int buffer2Slice = 0;
  int numSlicesToProcess;
  int abort = 0;
  while (!this->AbortProcessing && !abort && buffer1Slice < dim[2])
    {
    // run the plugin
    numSlicesToProcess = bufferSlices;
    if (buffer1Slice + numSlicesToProcess > dim[2])
      {
      numSlicesToProcess = dim[2] - buffer1Slice;
      }
    this->ProgressMinimum = (float)buffer1Slice/dim[2];
    this->ProgressMaximum = 
      (float)(buffer1Slice + numSlicesToProcess)/dim[2];
    buffer1Size = dim[0]*dim[1]*numSlicesToProcess*input->GetScalarSize()
      *input->GetNumberOfScalarComponents();
    pds->inData = input->GetScalarPointer();
    pds->outData = buffer1;
    pds->StartSlice = buffer1Slice;
    pds->NumberOfSlicesToProcess = numSlicesToProcess;
    if (this->PluginInfo.ProcessData(&this->PluginInfo, pds))
      {
      abort = 1;
      }
    
    // if we are past the first run copy results
    if (buffer1Slice > 0)
      {
      // copy results back into the buffer
      memcpy(input->GetScalarPointer(0,0,buffer2Slice),
             buffer2, buffer2Size);
      }
    
    // swap the buffers
    buffer2Slice = buffer1Slice;      
    buffer2Size = buffer1Size;
    bufferPtr = buffer1;
    buffer1 = buffer2;
    buffer2 = bufferPtr;
    
    // move lastSlice
    buffer1Slice += numSlicesToProcess;
    }
  // copy the last group
  memcpy(input->GetScalarPointer(0,0,buffer2Slice),
         buffer2, buffer2Size);
  delete [] buffer1;
  delete [] buffer2;
  input->Modified();  
  if (!this->AbortProcessing && !abort)
    {
    this->PushNewProperties();
    }
  return;
}
  
//----------------------------------------------------------------------------
void vtkVVPlugin::ProcessInOnePiece(vtkImageData *input, int memCheck,
                                    vtkVVProcessDataStruct *pds, 
                                    vtkVVPluginSelector *plugins)
{
  vtkImageData *output;
  // if we have the memory then keep the input and use a new output
  if (memCheck == 2)
    {
    output = vtkImageData::New();
    output->ShallowCopy(input);
    // since the current data is always the input ptr
    // we must swap here
    vtkImageData *tempID = input;
    input = output;
    output = tempID;
    // now the input is really a copy of the input ImageData
    }
  else
    {
    // otherwise we have to allocate the output
    output = input;
    }
  
  int outVolScalarSize = 1;
  switch (this->PluginInfo.OutputVolumeScalarType)
    {
    case VTK_FLOAT:
      outVolScalarSize = sizeof(float);
      break;
    case VTK_DOUBLE:
      outVolScalarSize = sizeof(double);
      break;
    case VTK_INT:
    case VTK_UNSIGNED_INT:
      outVolScalarSize = sizeof(int);
      break;
    case VTK_LONG:
    case VTK_UNSIGNED_LONG:
      outVolScalarSize = sizeof(long);
      break;
    case VTK_SHORT:
    case VTK_UNSIGNED_SHORT:
      outVolScalarSize = sizeof(short);
      break;
    }
  int *outDim = this->PluginInfo.OutputVolumeDimensions;
  int size = outVolScalarSize *
    this->PluginInfo.OutputVolumeNumberOfComponents *
    outDim[0]*outDim[1]*outDim[2];
  unsigned char *buffer1 = new unsigned char [size];
  pds->inData = input->GetScalarPointer();
  pds->outData = buffer1;
  // for in place plugins copy the input to the output
  if (this->SupportInPlaceProcessing)
    {
    memcpy(pds->outData, pds->inData, size);
    }
  pds->StartSlice = 0;
  pds->CurrentVolumeFromSeries = 0;
  pds->NumberOfSlicesToProcess = outDim[2];

  int failed = this->PluginInfo.ProcessData(&this->PluginInfo, pds);
  
  if (!failed && !this->AbortProcessing)
    {
    // now copy the output data to the ImageData instance
    // also adjust all the properties to match the results of the plugin
    output->SetScalarType(this->PluginInfo.OutputVolumeScalarType);
    output->SetSpacing(this->PluginInfo.OutputVolumeSpacing[0],
                       this->PluginInfo.OutputVolumeSpacing[1],
                       this->PluginInfo.OutputVolumeSpacing[2]);
    output->SetNumberOfScalarComponents(
      this->PluginInfo.OutputVolumeNumberOfComponents);
    output->SetExtent(0, 0, 0, 0, 0, 0);
    output->AllocateScalars();
    output->SetExtent(0, outDim[0] - 1, 0, outDim[1] - 1, 0, outDim[2] - 1);
    output->SetWholeExtent(output->GetExtent());
    output->GetPointData()->GetScalars()->SetVoidArray(
      buffer1, size/outVolScalarSize,0);

    // if we have kept the input, then let the user undo
    if (memCheck == 2)
      {
      // recall that the input here is not the original input anymore. Also, 
      // if a label map is processed, we don't bother to maintain the old 
      // label map. We don't want that kind of memory bloat. So just disable
      // undo in that case.
      output->Modified();
      plugins->SetUndoData(this->RequiresLabelInput == 0 ? input : 0);
      input->Delete();
      }
    // this must come after the SetUndoData
    this->PushNewProperties();
    }
  else
    {
    // if we have kept the input, then restore it
    if (memCheck == 2)
      {
      int *inDim = this->PluginInfo.InputVolumeDimensions;
      int inSize = this->PluginInfo.InputVolumeScalarSize*
        this->PluginInfo.InputVolumeNumberOfComponents *
        inDim[0]*inDim[1]*inDim[2];
      // take the smaller of the inSize and size.
      if (size > inSize)
        {
        size = inSize;
        }
      memcpy(pds->outData, pds->inData, size);
      input->Delete();
      }
    }
}
 
//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
//----------------------------------------------------------------------------
void vtkVVPlugin::ProcessSeriesByVolumes(vtkImageData *input, 
                                          int memCheck,
                                          vtkVVProcessDataStruct *pds,
                                          vtkVVPluginSelector *plugins)
{
  // Release memory, if it that was allocated in ExecuteData().
  if( pds->inDataSeries )
    {
    delete [] (char *)(pds->inDataSeries);
    pds->inDataSeries = 0;
    }

  vtkImageData *output;
  // if we have the memory then keep the input and use a new output
  if (memCheck == 2)
    {
    output = vtkImageData::New();
    output->ShallowCopy(input);
    // since the current data is always the input ptr
    // we must swap here
    vtkImageData *tempID = input;
    input = output;
    output = tempID;
    // now the input is really a copy of the input ImageData
    }
  else
    {
    // otherwise we have to allocate the output
    output = input;
    }
   
  int outVolScalarSize = 1;
  switch (this->PluginInfo.OutputVolumeScalarType)
    {
    case VTK_FLOAT:
      outVolScalarSize = sizeof(float);
      break;
    case VTK_DOUBLE:
      outVolScalarSize = sizeof(double);
      break;
    case VTK_INT:
    case VTK_UNSIGNED_INT:
      outVolScalarSize = sizeof(int);
      break;
    case VTK_LONG:
    case VTK_UNSIGNED_LONG:
      outVolScalarSize = sizeof(long);
      break;
    case VTK_SHORT:
    case VTK_UNSIGNED_SHORT:
      outVolScalarSize = sizeof(short);
      break;
    }
  int *outDim = this->PluginInfo.OutputVolumeDimensions;
  int size = outVolScalarSize *
    this->PluginInfo.OutputVolumeNumberOfComponents *
    outDim[0]*outDim[1]*outDim[2];
  unsigned char *buffer1 = new unsigned char [size];
  pds->inData = input->GetScalarPointer();
  pds->outData = buffer1;

  int abort = 0;
  int volumeToProcess = 0;

  while (!this->AbortProcessing && !abort && 
         volumeToProcess < this->PluginInfo.InputVolumeSeriesNumberOfVolumes )
    {
    vtkImageData * volume = this->SeriesInputOpenWizard->GetSeriesOutput(volumeToProcess);
    if( volume && volume->GetScalarPointer() )
      {
      pds->inDataSeries = volume->GetScalarPointer();
      pds->CurrentVolumeFromSeries = volumeToProcess;
      // run the plugin
      if (this->PluginInfo.ProcessData(&this->PluginInfo, pds))
        {
        abort = 1;
        }
      // set it to NULL to prevent others from attempting to release this
      // memory. The memory actually belongs to the reader.
      pds->inDataSeries = 0; 
      }
    else
      {
      vtkErrorMacro("Problem getting access to Volume " << volumeToProcess << " from the series" );
      pds->inDataSeries = 0;
      return;
      }
    volumeToProcess++;
    }

  if (!abort && !this->AbortProcessing)
    {
    // now copy the output data to the ImageData instance
    // also adjust all the properties to match the results of the plugin
    output->SetScalarType(this->PluginInfo.OutputVolumeScalarType);
    output->SetSpacing(this->PluginInfo.OutputVolumeSpacing[0],
                       this->PluginInfo.OutputVolumeSpacing[1],
                       this->PluginInfo.OutputVolumeSpacing[2]);
    output->SetNumberOfScalarComponents(
      this->PluginInfo.OutputVolumeNumberOfComponents);
    output->SetExtent(0, 0, 0, 0, 0, 0);
    output->AllocateScalars();
    output->SetExtent(0, outDim[0] - 1, 0, outDim[1] - 1, 0, outDim[2] - 1);
    output->SetWholeExtent(output->GetExtent());
    output->GetPointData()->GetScalars()->SetVoidArray(
      buffer1, size/outVolScalarSize,0);
    
    // if we have kept the input, then let the user undo
    if (memCheck == 2)
      {
      // recall that the input here is not the original input anymore. Also, 
      // if a label map is processed, we don't bother to maintain the old 
      // label map. We don't want that kind of memory bloat. So just disable
      // undo in that case.
      output->Modified();
      plugins->SetUndoData(this->RequiresLabelInput == 0 ? input : 0);
      input->Delete();
      }
    // this must come after the SetUndoData
    this->PushNewProperties();
    }
  else
    {
    // if we have kept the input, then restore it
    if (memCheck == 2)
      {
      int *inDim = this->PluginInfo.InputVolumeDimensions;
      int inSize = this->PluginInfo.InputVolumeScalarSize*
        this->PluginInfo.InputVolumeNumberOfComponents *
        inDim[0]*inDim[1]*inDim[2];
      // take the smaller of the inSize and size.
      if (size > inSize)
        {
        size = inSize;
        }
      memcpy(pds->outData, pds->inData, size);
      input->Delete();
      }
    }
}
#endif
//ETX

//----------------------------------------------------------------------------
void vtkVVPlugin::PushNewProperties()
{
  // if any properties were set then push them out
  int nb_rw = this->Window->GetNumberOfRenderWidgetsUsingSelectedDataItem();
  for (int i = 0; i < nb_rw; i++)
    {
    vtkKWRenderWidgetPro *rwp = vtkKWRenderWidgetPro::SafeDownCast(
      this->Window->GetNthRenderWidgetUsingSelectedDataItem(i));
    if (!rwp)
      {
      continue;
      }
    if (this->ResultingComponentsAreIndependent >= 0)
      {
      rwp->SetIndependentComponents(
        this->ResultingComponentsAreIndependent);      
      }
    if (this->ResultingDistanceUnits)
      {
      rwp->SetDistanceUnits(this->ResultingDistanceUnits);
      }
    if (this->ResultingComponent1Units)
      {
      rwp->SetScalarUnits(0,this->ResultingComponent1Units);
      }
    if (this->ResultingComponent2Units)
      {
      rwp->SetScalarUnits(1,this->ResultingComponent2Units);
      }
    if (this->ResultingComponent3Units)
      {
      rwp->SetScalarUnits(2,this->ResultingComponent3Units);
      }
    if (this->ResultingComponent4Units)
      {
      rwp->SetScalarUnits(3,this->ResultingComponent4Units);
      }
    }  
}

//BTX
#ifdef KWVolView_PLUGINS_USE_SERIES
//----------------------------------------------------------------------------
int vtkVVPlugin::SaveVolumeSeries(vtkVVProcessDataStruct *pds)
{
  // We must have a volume series from a plugin 

  if (!this->ProducesSeriesOutput || !pds)
    {
    return 0;
    }

  if (!pds->outDataSeries)
    {
    vtkKWMessageDialog::PopupMessage( 
      this->GetApplication(), this->Window, "Save Error", 
      "The plugin did not returned any series data.", vtkKWMessageDialog::ErrorIcon);
    return 0;
    }

  this->Window->DisableRenderStates();

  vtkVV4DSaveDialog *dlg = vtkVV4DSaveDialog::New();
  dlg->SetParent(this);
  dlg->Create();
  dlg->RetrieveLastPathFromRegistry("SavePath");

  int success = 0;

  if (dlg->Invoke() && this->SaveVolumeSeries(pds,dlg->GetFileName()))
    {
    dlg->SaveLastPathToRegistry("SavePath");
    success = 1;
    }

  dlg->Delete();

  this->Window->UpdateAccordingToImageData();
  this->Window->RestoreRenderStates();
  
  return success;
}

//----------------------------------------------------------------------------
int vtkVVPlugin::SaveVolumeSeries(vtkVVProcessDataStruct *pds,const char *fname) 
{
  // We must have a filename and a volume series from a plugin 
  if (!fname || !this->ProducesSeriesOutput || !pds || !pds->outDataSeries)
    {
    return 0;
    }

  vtkImageImport * imageImporter = vtkImageImport::New();
  imageImporter->SetDataScalarType( this->PluginInfo.InputVolumeSeriesScalarType );
  imageImporter->SetNumberOfScalarComponents( 
      this->PluginInfo.InputVolumeSeriesNumberOfComponents );
    
  int wExt[6];
  wExt[0] = 0;
  wExt[1] = this->PluginInfo.InputVolumeSeriesDimensions[0]-1;
  wExt[2] = 0;
  wExt[3] = this->PluginInfo.InputVolumeSeriesDimensions[1]-1;
  wExt[4] = 0;
  wExt[5] = this->PluginInfo.InputVolumeSeriesDimensions[2]-1;

  imageImporter->SetWholeExtent( wExt );
  imageImporter->SetDataExtentToWholeExtent();

  int i;
  double spacing[3];
  for (i = 0; i < 3; i++)
    {
    spacing[i] = this->PluginInfo.InputVolumeSeriesSpacing[i]; 
    }
  imageImporter->SetDataSpacing( spacing );
 
  vtkVV4DSaveVolume *sv = vtkVV4DSaveVolume::New();
  sv->Create();
  sv->SetWindow(this->Window);
  sv->SetInput(imageImporter->GetOutput());  

  vtkLargeInteger seriesOneVolumeSize = this->PluginInfo.OutputVolumeSeriesScalarSize;
  seriesOneVolumeSize = 
    seriesOneVolumeSize * this->PluginInfo.OutputVolumeSeriesNumberOfComponents;
  for(int k=0; k<3; k++)
    {
    seriesOneVolumeSize = 
      seriesOneVolumeSize * this->PluginInfo.OutputVolumeSeriesDimensions[k];
    }

  vtkstd::string fullfilename = fname;

  vtkstd::string filenamePath =
    vtksys::SystemTools::GetFilenamePath(fullfilename);
  vtkstd::string filenameExtension =
    vtksys::SystemTools::GetFilenameLastExtension(fullfilename);
  vtkstd::string filenameWithOutExtension = 
    vtksys::SystemTools::GetFilenameWithoutLastExtension(fullfilename);

  char * filename = new char[filenamePath.size() +
                             filenameWithOutExtension.size() +
                             filenameExtension.size() + 10 ];
  int si=0;
  int writinWasOK = 1;
  while(si < this->PluginInfo.OutputVolumeSeriesNumberOfVolumes && writinWasOK)
    {
    unsigned long offset = seriesOneVolumeSize.CastToUnsignedLong() * si;
    char * initialPointer = (char *)(pds->outDataSeries)+offset;

    imageImporter->SetImportVoidPointer(initialPointer,1);
    imageImporter->Update();

    sprintf( filename, "%s/%s%03i%s", filenamePath.c_str(), 
                                      filenameWithOutExtension.c_str(),
                                      si,filenameExtension.c_str());

    sv->SetFileName(filename);
    writinWasOK = sv->Write();

    if ( !writinWasOK )
      {
      vtkKWMessageDialog::PopupMessage(
        this->GetApplication(), this->Window, "VolView Write Error",
        "There was a problem writing the volume series.\n"
        "Please check the location and make sure you have write\n"
        "permissions and enough disk space.",
        vtkKWMessageDialog::ErrorIcon);
      }
    
    ++si;
    }

  sv->SetInput(0);
  sv->Delete();
  imageImporter->Delete();
  delete [] filename;
  
  return writinWasOK;
}
#endif
//ETX


//----------------------------------------------------------------------------
void vtkVVPlugin::DisplayPlot(vtkVVProcessDataStruct *pds)
{
  if( !pds->outDataPlotting )
    {
    return;
    }

  vtkKWXYPlotDialog * plottingDialog = vtkKWXYPlotDialog::New();

  // Pass the plugin data into the Plotter
  vtkXYPlotActor * plotActor = plottingDialog->GetXYPlotActor();

  vtkPoints   * points = vtkPoints::New();
  
  const vtkIdType numberOfPoints = this->PluginInfo.OutputPlottingNumberOfRows;
  points->SetNumberOfPoints(numberOfPoints);

  vtkIdType pointId=0;
  double pcoord[3];
  pcoord[0] = pcoord[1] = pcoord[2] = 0.0;

  double *px = static_cast<double *>(pds->outDataPlotting);
  while(pointId < numberOfPoints)
    {
    pcoord[0] = *px; // set the independent values as X coordinate of the points.
    points->SetPoint(pointId,pcoord);
    ++pointId;
    ++px;
    }

  const int numberOfSeries= this->PluginInfo.OutputPlottingNumberOfColumns-1;
  int nc=0;
  double *py = static_cast<double *>(pds->outDataPlotting) + numberOfPoints;
  while(nc < numberOfSeries )
    {
    vtkPolyData * dataObject = vtkPolyData::New();
    vtkFieldData * fieldData = vtkFieldData::New();
    vtkDoubleArray * dataSeries = vtkDoubleArray::New();
    dataSeries->SetNumberOfComponents(1);
    dataSeries->SetNumberOfTuples(numberOfPoints);
    vtkIdType dataId=0;
    while(dataId < numberOfPoints)
      {
      dataSeries->InsertValue(dataId,*py);
      ++dataId;
      ++py;
      }
    fieldData->AddArray(dataSeries);
    dataSeries->Delete();
    dataObject->SetFieldData(fieldData);
    fieldData->Delete();
    dataObject->SetPoints(points);
    plotActor->AddDataObjectInput(dataObject);
    dataObject->Delete();
    ++nc;
    }
 
  points->Delete();
  
  if( this->GetPlottingXAxisTitle() )
    {
    plotActor->SetXTitle( this->GetPlottingXAxisTitle() );
    }
  if( this->GetPlottingYAxisTitle() )
    {
    plotActor->SetYTitle( this->GetPlottingYAxisTitle() );
    }

  plottingDialog->Create();
  plottingDialog->SetParent(this->Window);
  
  plottingDialog->Invoke(); // Display as a modal window
  plottingDialog->Delete();
  
  // Release the memory buffer used for data transfer.
  delete [] (double *)(pds->outDataPlotting);
  pds->outDataPlotting = 0;
}
 
//----------------------------------------------------------------------------
void vtkVVPlugin::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Window: " << this->Window << endl;
  os << indent << "Name: ";
  if (this->Name)
    {
    os << this->Name << endl;
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "Group: ";
  if (this->Group)
    {
    os << this->Group << endl;
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "TerseDocumentation: ";
  if (this->TerseDocumentation)
    {
    os << this->TerseDocumentation << endl;
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "Full Documentation: ";
  if (this->FullDocumentation)
    {
    os << indent << this->FullDocumentation << endl;
    }
  else
    {
    os << indent << "(none)" << endl;
    }
  if (this->ResultingComponent1Units)
    {
    os << indent << this->ResultingComponent1Units << endl;
    }
  else
    {
    os << indent << "(none)" << endl;
    }
  if (this->ResultingComponent2Units)
    {
    os << indent << this->ResultingComponent2Units << endl;
    }
  else
    {
    os << indent << "(none)" << endl;
    }
  if (this->ResultingComponent3Units)
    {
    os << indent << this->ResultingComponent3Units << endl;
    }
  else
    {
    os << indent << "(none)" << endl;
    }
  if (this->ResultingComponent4Units)
    {
    os << indent << this->ResultingComponent4Units << endl;
    }
  else
    {
    os << indent << "(none)" << endl;
    }
  if (this->ResultingDistanceUnits)
    {
    os << indent << this->ResultingDistanceUnits << endl;
    }
  else
    {
    os << indent << "(none)" << endl;
    }
  os << indent << "NumberOfGUIItems: " << this->NumberOfGUIItems << endl;
  os << indent << "RequiresSecondInput: " << this->RequiresSecondInput << endl;
  os << indent << "SecondInputOptional: " << this->SecondInputOptional << endl;  
  os << indent << "RequiresLabelInput: " << this->RequiresLabelInput << endl;
  os << indent << "SecondInputOpenWizard: " << this->SecondInputOpenWizard << endl;
}

