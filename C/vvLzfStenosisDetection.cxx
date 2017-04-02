<<<<<<< HEAD
/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVVPluginAPI.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

template <class IT, class I2T>
void coronaryStenosis(vtkVVPluginInfo *info,
					  vtkVVProcessDataStruct *pds, 
					  IT *, I2T *){
	//计算中心线上的模糊距离
	IT *ptr = (IT *)pds->outData;
	I2T *ptr2 = (I2T *)pds->inData2;
	IT* readVol = ptr;
	int* dim = info->InputVolumeDimensions;
	int abort = 0;
	int Xd = (int)dim[0];
	int Yd = (int)dim[1]; 
	int Zd = (int)dim[2];
	/*short *** fuzzyDis = new short**[Zd];  //Never write "new (double**)[n]"
	for(int i = 0; i < Zd; i++){
		fuzzyDis[i] = new short*[Yd];
		for(int j = 0; j < Yd; j++){
			fuzzyDis[i][j] =  new short[Xd];
		}
	}
	for (int i = 0; i < Zd; i++){
		info->UpdateProgress(info,(float)1.0*i/Zd,"Processing..."); 
		abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
		for (int j = 0;  !abort && j < Yd; j++){
			for (int k = 0; k < Xd; k++) {
				fuzzyDis[i][j][k] = *readVol;
				readVol++;
			}
		}
	}*/


	FILE *in, *out;
	out = fopen("skeleton_diameter.txt", "w");
	/*int x, y, z;
	char volInfo[102];
	if(in = fopen("coronary_skeleton.txt", "r")){
		//fscanf(in, "%s", volInfo); // 错误！一旦第一行有空格，就读到空格为止！
		fgets(volInfo, 100, in);  //or scanf("%[^\n]", str);
		while(3 == fscanf(in, "%d %d %d", &z, &y, &x)){
			fprintf(out, "(%d, %d, %d): %d\n", z, y, x, fuzzyDis[z][y][x]);
			fuzzyDis[z][y][x] = 1024;
		}
		fclose(in);
	}
	else{
		perror("coronary_skeleton.txt");  //print "coronary_data.txt: No such file or directory"
	}*/

	for (int i = 0; i < Zd; i++){
		info->UpdateProgress(info,(float)1.0*i/Zd,"Processing..."); 
		abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
		for (int j = 0; !abort && j < Yd; j++){
			for (int k = 0; k < Xd; k++) {
				//*ptr = fuzzyDis[i][j][k];
				if(*ptr2)
					fprintf(out, "%d %d %d %d\n", k, j, i, *ptr);
				ptr++;
				ptr2++;
			}
		}
	}

	fclose(out);

	//ShellExecute(NULL,"open","python","\"Z:\\Python GUI\\fdt2\\branch_stenosis.py\"","",SW_HIDE);
}


template <class IT>
/* TODO 1: Rename vvSampleTemplate to vv<your_plugin>Template */
void vvLzfStenosisDetectionTemplate(vtkVVPluginInfo *info,
                      vtkVVProcessDataStruct *pds, 
                      IT *)
{
	//以下方法解决了两幅图像位长不相同的问题。
	switch (info->InputVolume2ScalarType)
	{
		// invoke the appropriate templated function
		vtkTemplateMacro4(coronaryStenosis, info, pds, static_cast<IT *>(0), static_cast<VTK_TT *>(0));
	}
                                                    
	info->UpdateProgress(info,(float)1.0,"Processing Complete");
}

static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  switch (info->InputVolumeScalarType)
    {
    /* TODO 2: Rename vvSampleTemplate to vv<your_plugin>Template */
    vtkTemplateMacro3(vvLzfStenosisDetectionTemplate, info, pds, 
                      static_cast<VTK_TT *>(0));
    }
  return 0;
}

/* this function updates the GUI elements to accomidate new data */
/* it will always get called prior to the plugin executing. */
static int UpdateGUI(void *inf)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  /* TODO 8: create your required GUI elements here */


  /* TODO 6: modify the following code as required. By default the output
  *  image's properties match those of the input depending on what your
  *  filter does it may need to change some of these values
  */
  info->OutputVolumeScalarType = info->InputVolumeScalarType;
  info->OutputVolumeNumberOfComponents = 
    info->InputVolumeNumberOfComponents;
  int i;
  for (i = 0; i < 3; i++)
    {
    info->OutputVolumeDimensions[i] = info->InputVolumeDimensions[i];
    info->OutputVolumeSpacing[i] = info->InputVolumeSpacing[i];
    info->OutputVolumeOrigin[i] = info->InputVolumeOrigin[i];
    }

  return 1;
}

extern "C" 
{
  /* TODO 3: Rename vvSampleInit to vv<your_plugin>Init */
  void VV_PLUGIN_EXPORT vvLzfStenosisDetectionInit(vtkVVPluginInfo *info)
  {
    /* always check the version */
    vvPluginVersionCheck();
    
    /* setup information that never changes */
    info->ProcessData = ProcessData;
    info->UpdateGUI = UpdateGUI;
    
    /* set the properties this plugin uses */
    /* TODO 4: Rename "Sample" to "<your_plugin>" */
    info->SetProperty(info, VVP_NAME, "LzfStenosisDetection");
    info->SetProperty(info, VVP_GROUP, "Utility");

    /* TODO 5: update the terse and full documentation for your filter */
    info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
                      "This plugin includes several functions...");
    info->SetProperty(info, VVP_FULL_DOCUMENTATION,
                      "This plugin is created on Nov 28nd, 2015, for calculating the FDT value on the skeleton(centerline). You should import the second volume of skeleton.");

    
    /* TODO 9: set these two values to "0" or "1" based on how your plugin
     * handles data all possible combinations of 0 and 1 are valid. */
    info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "1");
    info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "1");

    /* TODO 7: set the number of GUI items used by this plugin */
    info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "0");
		info->SetProperty(info, VVP_REQUIRES_SECOND_INPUT,        "1");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
  }
}
=======
/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVVPluginAPI.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
#include <ShellAPI.h>

template <class IT, class I2T>
void coronaryStenosis(vtkVVPluginInfo *info,
					  vtkVVProcessDataStruct *pds, 
					  IT *, I2T *){
	//计算中心线上的模糊距离
	IT *ptr = (IT *)pds->outData;
	I2T *ptr2 = (I2T *)pds->inData2;
	IT* readVol = ptr;
	int* dim = info->InputVolumeDimensions;
	int abort = 0;
	int Xd = (int)dim[0];
	int Yd = (int)dim[1]; 
	int Zd = (int)dim[2];
	/*short *** fuzzyDis = new short**[Zd];  //Never write "new (double**)[n]"
	for(int i = 0; i < Zd; i++){
		fuzzyDis[i] = new short*[Yd];
		for(int j = 0; j < Yd; j++){
			fuzzyDis[i][j] =  new short[Xd];
		}
	}
	for (int i = 0; i < Zd; i++){
		info->UpdateProgress(info,(float)1.0*i/Zd,"Processing..."); 
		abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
		for (int j = 0;  !abort && j < Yd; j++){
			for (int k = 0; k < Xd; k++) {
				fuzzyDis[i][j][k] = *readVol;
				readVol++;
			}
		}
	}*/


	FILE *in, *out;
	out = fopen("skeleton_diameter.txt", "w");
	int x, y, z;
	char volInfo[102];
	/*if(in = fopen("coronary_skeleton.txt", "r")){
		//fscanf(in, "%s", volInfo); // 错误！一旦第一行有空格，就读到空格为止！
		fgets(volInfo, 100, in);  //or scanf("%[^\n]", str);
		while(3 == fscanf(in, "%d %d %d", &z, &y, &x)){
			fprintf(out, "(%d, %d, %d): %d\n", z, y, x, fuzzyDis[z][y][x]);
			fuzzyDis[z][y][x] = 1024;
		}
		fclose(in);
	}
	else{
		perror("coronary_skeleton.txt");  //print "coronary_data.txt: No such file or directory"
	}*/

	for (int i = 0; i < Zd; i++){
		info->UpdateProgress(info,(float)1.0*i/Zd,"Processing..."); 
		abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
		for (int j = 0; !abort && j < Yd; j++){
			for (int k = 0; k < Xd; k++) {
				//*ptr = fuzzyDis[i][j][k];
				if(*ptr2)
					fprintf(out, "%d %d %d %d\n", i, j, k, *ptr);
				ptr++;
				ptr2++;
			}
		}
	}

	fclose(out);

	ShellExecute(NULL,"open","python","\"Z:\\Python GUI\\fdt2\\branch_stenosis.py\"","",SW_HIDE);
}


template <class IT>
/* TODO 1: Rename vvSampleTemplate to vv<your_plugin>Template */
void vvLzfStenosisDetectionTemplate(vtkVVPluginInfo *info,
                      vtkVVProcessDataStruct *pds, 
                      IT *)
{
	//以下方法解决了两幅图像位长不相同的问题。
	switch (info->InputVolume2ScalarType)
	{
		// invoke the appropriate templated function
		vtkTemplateMacro4(coronaryStenosis, info, pds, static_cast<IT *>(0), static_cast<VTK_TT *>(0));
	}
                                                    
	info->UpdateProgress(info,(float)1.0,"Processing Complete");
}

static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  switch (info->InputVolumeScalarType)
    {
    /* TODO 2: Rename vvSampleTemplate to vv<your_plugin>Template */
    vtkTemplateMacro3(vvLzfStenosisDetectionTemplate, info, pds, 
                      static_cast<VTK_TT *>(0));
    }
  return 0;
}

/* this function updates the GUI elements to accomidate new data */
/* it will always get called prior to the plugin executing. */
static int UpdateGUI(void *inf)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  /* TODO 8: create your required GUI elements here */


  /* TODO 6: modify the following code as required. By default the output
  *  image's properties match those of the input depending on what your
  *  filter does it may need to change some of these values
  */
  info->OutputVolumeScalarType = info->InputVolumeScalarType;
  info->OutputVolumeNumberOfComponents = 
    info->InputVolumeNumberOfComponents;
  int i;
  for (i = 0; i < 3; i++)
    {
    info->OutputVolumeDimensions[i] = info->InputVolumeDimensions[i];
    info->OutputVolumeSpacing[i] = info->InputVolumeSpacing[i];
    info->OutputVolumeOrigin[i] = info->InputVolumeOrigin[i];
    }

  return 1;
}

extern "C" 
{
  /* TODO 3: Rename vvSampleInit to vv<your_plugin>Init */
  void VV_PLUGIN_EXPORT vvLzfStenosisDetectionInit(vtkVVPluginInfo *info)
  {
    /* always check the version */
    vvPluginVersionCheck();
    
    /* setup information that never changes */
    info->ProcessData = ProcessData;
    info->UpdateGUI = UpdateGUI;
    
    /* set the properties this plugin uses */
    /* TODO 4: Rename "Sample" to "<your_plugin>" */
    info->SetProperty(info, VVP_NAME, "LzfStenosisDetection");
    info->SetProperty(info, VVP_GROUP, "Utility");

    /* TODO 5: update the terse and full documentation for your filter */
    info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
                      "This plugin includes several functions...");
    info->SetProperty(info, VVP_FULL_DOCUMENTATION,
                      "This plugin is created on Nov 28nd, 2015, for calculating the FDT value on the skeleton(centerline). You should import the second volume of skeleton.");

    
    /* TODO 9: set these two values to "0" or "1" based on how your plugin
     * handles data all possible combinations of 0 and 1 are valid. */
    info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "1");
    info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "1");

    /* TODO 7: set the number of GUI items used by this plugin */
    info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "0");
		info->SetProperty(info, VVP_REQUIRES_SECOND_INPUT,        "1");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
  }
}
>>>>>>> 582480cd14e067db3b1fed95c429f1724f683c48
