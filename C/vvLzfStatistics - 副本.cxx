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
#include <map>
#include <set>
using namespace std;
struct Coordinate{
	int x, y, z;
	Coordinate(int zz, int yy, int xx){
		x = xx;
		y = yy;
		z = zz;
	}
	bool operator < (const Coordinate &co) const{
		return ((z < co.z) || (z == co.z && y < co.y) || (z == co.z && y == co.y && x <co.x));
	}
};

template <class IT>
void generateStatisticsOrPly(int* dim, int inc, vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds,  IT* ptr, int filetype){
	FILE *out;
	int abort;

	int nonzero = 0;
	int Xd = (int)dim[0];
	int Yd = (int)dim[1]; 
	int Zd = (int)dim[2];
	switch(filetype){
		case 0:
			if(out = fopen("coronary_skeleton.txt", "w")){
				fprintf(out, "%d x %d x %d voxels, InputVolumeNumberOfComponents = %d\n", Xd, Yd, Zd, inc);
			}
			for (int i = 0; i < Zd; i++){
				info->UpdateProgress(info,(float)1.0*i/Zd,"Processing..."); 
				abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
				for (int j = 0;  !abort && j < Yd; j++){
					for (int k = 0; k < Xd; k++) {
						//grayScalar[i][j][k] = *readVol;
						if(*ptr){
							fprintf(out, "%d %d %d\n", i, j, k);
						}
						ptr++;
					}
				}
			}

			break;
		case 1:
			if(out = fopen("coronary_data.ply", "w")){
				fprintf(out, "ply\nformat ascii 1.0\ncomment VCGLIB generated\nelement vertex          \nproperty float x\nproperty float y\nproperty float z\nelement face 0\nproperty list uchar int vertex_indices\nend_header\n");
			}
			for (int i = 0; i < Zd; i++){
				info->UpdateProgress(info,(float)1.0*i/Zd,"Processing..."); 
				abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
				for (int j = 0;  !abort && j < Yd; j++){
					for (int k = 0; k < Xd; k++) {
						//grayScalar[i][j][k] = *readVol;
						if(*ptr){
							fprintf(out, "%f %f %f\n", 100.0*k/Xd, 100.0*j/Yd, 100.0*i/Zd);
							//fprintf(out, "%d %d %d\n", i, j, k);
							nonzero++;
						}
						ptr++;
					}
				}
			}
			fseek(out, 64, SEEK_SET); //文件指针重新指向文件头
			fprintf(out, "%d", nonzero); //写点数（vertex）
			break;
			
		default:break;
	}

	/*
	short *** grayScalar = new short**[Zd];  //Never write "new (double**)[n]"
	for(int i = 0; i < Zd; i++){
		grayScalar[i] = new short*[Yd];
		for(int j = 0; j < Yd; j++){
			grayScalar[i][j] =  new short[Xd];
		}
	}*/
  



	//output the statistics of HU
	/*
	std::map<int, int> stat;
	fprintf(out, "-------statistics-------\n");
	fprintf(out, "Non-zero pixel:%d\n", nonzpix);
	for(std::map<int, int>::const_iterator it = stat.begin(); it != stat.end(); it++){
	  fprintf(out, "HU[%d]=%d\n", it->first, it->second);
	}*/
  

	/*
	for(int i = 0; i < Zd; i++){
		for(int j = 0; j < Yd; j++){
			delete[] grayScalar[i][j];
		}
		delete[] grayScalar[i];
	}
	delete[] grayScalar;*/
  
	fclose(out);
}

template <class IT>
void reserveSurfacePoints(int* dim, int inc, vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds,  IT* ptr){
	//保留二值“实心”图像的表面体素
	int Xd = (int)dim[0];
	int Yd = (int)dim[1]; 
	int Zd = (int)dim[2];
	int abort;
	IT* readVol = ptr;

	/*FILE* out;
	if(out = fopen("coronary_surface.txt", "w")){
		fprintf(out, "%d x %d x %d voxels, InputVolumeNumberOfComponents = %d\n", Xd, Yd, Zd, inc);
	}*/

	short *** grayScalar = new short**[Zd];  //Never write "new (double**)[n]"
	for(int i = 0; i < Zd; i++){
		grayScalar[i] = new short*[Yd];
		for(int j = 0; j < Yd; j++){
			grayScalar[i][j] =  new short[Xd];
		}
	}

	bool*** border = new bool**[Zd];   //标记是否为边界点
	for(int i = 0; i < Zd; i++){
		border[i] = new bool*[Yd];
		for(int j = 0; j < Yd; j++){
			border[i][j] =  new bool[Xd];
			for(int k = 0; k < Xd; k++)
				border[i][j][k] = false;
		}
	}
	
	for (int i = 0; i < Zd; i++){
		info->UpdateProgress(info,(float)1.0*i/Zd,"Processing..."); 
		abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
		for (int j = 0;  !abort && j < Yd; j++){
			for (int k = 0; k < Xd; k++) {
				grayScalar[i][j][k] = *readVol;
				readVol++;
			}
		}
	}

	const int neighborDirection[26][3] = {
		{1, 0, 0},{0, 1, 0},{0, 0, 1},{-1, 0, 0},{0, -1, 0},{0, 0, -1},
		{1, 1, 0},{1, -1, 0},{-1, 1, 0},{-1, -1, 0},
		{0, 1, 1},{0, 1, -1},{0, -1, 1},{0, -1, -1},
		{1, 0, 1},{1, 0, -1},{-1, 0, 1},{-1, 0, -1},
		{1, 1, 1},{1, 1, -1},{1, -1, 1},{1, -1, -1},{-1, 1, 1},{-1, 1, -1},{-1, -1, 1},{-1,-1,-1}
	};
	for (int i = 0; i < Zd; i++){
		info->UpdateProgress(info,(float)1.0*i/Zd,"Processing..."); 
		abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
		for (int j = 0; !abort && j < Yd; j++){
			for (int k = 0; k < Xd; k++) {
				if(grayScalar[i][j][k]){
				  int countBackground = 0;
				  for(int h = 0; h < 26; h++){
					  if(!grayScalar[i + neighborDirection[h][0]][j + neighborDirection[h][1]][k + neighborDirection[h][2]])
						  countBackground++;
				  }
				  if(countBackground > 10)  //26邻域里有多少是背景点算作边界，可调
					  border[i][j][k] = true;
				}
			}
		}
	}
  
	for (int i = 0; i < Zd; i++){
		info->UpdateProgress(info,(float)1.0*i/Zd,"Processing..."); 
		abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
		for (int j = 0; !abort && j < Yd; j++){
			for (int k = 0; k < Xd; k++) {
				if(!border[i][j][k]){ 
					*ptr = 0;   //Min of gray scalar，可调
				}
				//输出功能放到generateStatisticsOrPly里去实现
				/*else{
					fprintf(out, "%.3f %.3f %.3f\n", 10.0 * i / Xd, 10.0 * j / Yd, 10.0 * k / Zd);
				}*/
				ptr++;
			}
		}
	}

	for(int i = 0; i < Zd; i++){
		for(int j = 0; j < Yd; j++){
			delete[] grayScalar[i][j];
		}
		delete[] grayScalar[i];
	}
	delete[] grayScalar;

	for(int i = 0; i < Zd; i++){
		for(int j = 0; j < Yd; j++){
			delete[] border[i][j];
		}
		delete[] border[i];
	}
	delete[] border;
	
	//fclose(out);
}

template <class IT>
void coronaryBw2Gray(int* dim, int inc, vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds,  IT* ptr){
	//将分割好的二值图像匹配到原灰度图像

	int abort;
	int Xd = (int)dim[0];
	int Yd = (int)dim[1]; 
	int Zd = (int)dim[2];


	FILE *in;
	set<Coordinate> pointSet;
	int x, y, z;
	char volInfo[102];
	if(in = fopen("coronary_data.txt", "r")){
		//fscanf(in, "%s", volInfo); // 错误！一旦第一行有空格，就读到空格为止！
		fgets(volInfo, 100, in);  //or scanf("%[^\n]", str);
		while(3 == fscanf(in, "%d %d %d", &z, &y, &x)){
			Coordinate co(z, y, x);
			pointSet.insert(co);
		}
		fclose(in);
	}
	else{
		perror("coronary_data.txt");  //print "coronary_data.txt: No such file or directory"
	}
	
	
	FILE *console;
	console = fopen("console.log", "w");
	fprintf(console, "%s", volInfo);
	if(!pointSet.empty())
		fprintf(console, "size of set: %d\n", (int)pointSet.size());
	fclose(console);
	
	for (int i = 0; i < Zd; i++){
	  info->UpdateProgress(info,(float)1.0*i/Zd,"Processing..."); 
	  abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
	  for (int j = 0;  !abort && j < Yd; j++){
		  for (int k = 0; k < Xd; k++) {
			  Coordinate co(i, j, k);
			  if(pointSet.find(co) == pointSet.end()) 
				  *ptr = -1024;
			  ptr++;
		  }
	  }
  }
}


template <class IT>
void coronaryStenosis(int* dim, int inc, vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds,  IT* ptr){
	//计算中心线上的模糊距离
	int abort;
	int Xd = (int)dim[0];
	int Yd = (int)dim[1]; 
	int Zd = (int)dim[2];
	short *** fuzzyDis = new short**[Zd];  //Never write "new (double**)[n]"
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
				fuzzyDis[i][j][k] = *ptr;
				ptr++;
			}
		}
	}

	FILE *in, *out;
	out = fopen("skeleton_diameter.txt", "w");
	int x, y, z;
	char volInfo[102];
	if(in = fopen("coronary_skeleton.txt", "r")){
		//fscanf(in, "%s", volInfo); // 错误！一旦第一行有空格，就读到空格为止！
		fgets(volInfo, 100, in);  //or scanf("%[^\n]", str);
		while(3 == fscanf(in, "%d %d %d", &z, &y, &x)){
			fprintf(out, "(%d, %d, %d): %d\n", z, y, x, fuzzyDis[z][y][x]);
		}
		fclose(in);
	}
	else{
		perror("coronary_skeleton.txt");  //print "coronary_data.txt: No such file or directory"
	}
	fclose(out);

}
template <class IT>
/* TODO 1: Rename vvSampleTemplate to vv<your_plugin>Template */
void vvLzfStatisticsTemplate(vtkVVPluginInfo *info,
                      vtkVVProcessDataStruct *pds, 
                      IT *)
{
	IT *inPtr = (IT *)pds->inData;
	IT *outPtr = (IT *)pds->outData;
	int *dim = info->InputVolumeDimensions;
	int inNumComp = info->InputVolumeNumberOfComponents;
 
  
  /*
  *The following code is writen by lzf on May 22, 2015
  *for save as a .ply file
  */
	
	//reserveSurfacePoints(dim, inNumComp, info, pds, outPtr);
	
	//generateStatisticsOrPly(dim, inNumComp, info, pds, outPtr, 0); //0:txt 1:ply

	//coronaryBw2Gray(dim, inNumComp, info, pds, outPtr);

	coronaryStenosis(dim, inNumComp, info, pds, outPtr);
                                                    
	info->UpdateProgress(info,(float)1.0,"Processing Complete");
}

static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  switch (info->InputVolumeScalarType)
    {
    /* TODO 2: Rename vvSampleTemplate to vv<your_plugin>Template */
    vtkTemplateMacro3(vvLzfStatisticsTemplate, info, pds, 
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
  void VV_PLUGIN_EXPORT vvLzfStatisticsInit(vtkVVPluginInfo *info)
  {
    /* always check the version */
    vvPluginVersionCheck();
    
    /* setup information that never changes */
    info->ProcessData = ProcessData;
    info->UpdateGUI = UpdateGUI;
    
    /* set the properties this plugin uses */
    /* TODO 4: Rename "Sample" to "<your_plugin>" */
    info->SetProperty(info, VVP_NAME, "LzfStatistics");
    info->SetProperty(info, VVP_GROUP, "Utility");

    /* TODO 5: update the terse and full documentation for your filter */
    info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
                      "Replace voxels above/at/below the threshold value");
    info->SetProperty(info, VVP_FULL_DOCUMENTATION,
                      "This filter performs a pixel replacement, replacing pixel in the original data by a specified replacement value. The pixels to be replaced are determine by comparing the original pixel value to a threshold value with a user specified comparison operation. This filter operates in place, and does not change the dimensions, data type, or spacing of the volume.");

    
    /* TODO 9: set these two values to "0" or "1" based on how your plugin
     * handles data all possible combinations of 0 and 1 are valid. */
    info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "1");
    info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "1");

    /* TODO 7: set the number of GUI items used by this plugin */
    info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "0");
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
#include <map>
#include <set>
using namespace std;
struct Coordinate{
	int x, y, z;
	Coordinate(int zz, int yy, int xx){
		x = xx;
		y = yy;
		z = zz;
	}
	bool operator < (const Coordinate &co) const{
		return ((z < co.z) || (z == co.z && y < co.y) || (z == co.z && y == co.y && x <co.x));
	}
};

template <class IT>
void generateStatisticsOrPly(int* dim, int inc, vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds,  IT* ptr, int filetype){
	FILE *out;
	int abort;

	int nonzero = 0;
	int Xd = (int)dim[0];
	int Yd = (int)dim[1]; 
	int Zd = (int)dim[2];
	switch(filetype){
		case 0:
			if(out = fopen("coronary_skeleton.txt", "w")){
				fprintf(out, "%d x %d x %d voxels, InputVolumeNumberOfComponents = %d\n", Xd, Yd, Zd, inc);
			}
			for (int i = 0; i < Zd; i++){
				info->UpdateProgress(info,(float)1.0*i/Zd,"Processing..."); 
				abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
				for (int j = 0;  !abort && j < Yd; j++){
					for (int k = 0; k < Xd; k++) {
						//grayScalar[i][j][k] = *readVol;
						if(*ptr){
							fprintf(out, "%d %d %d\n", i, j, k);
						}
						ptr++;
					}
				}
			}

			break;
		case 1:
			if(out = fopen("coronary_data.ply", "w")){
				fprintf(out, "ply\nformat ascii 1.0\ncomment VCGLIB generated\nelement vertex          \nproperty float x\nproperty float y\nproperty float z\nelement face 0\nproperty list uchar int vertex_indices\nend_header\n");
			}
			for (int i = 0; i < Zd; i++){
				info->UpdateProgress(info,(float)1.0*i/Zd,"Processing..."); 
				abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
				for (int j = 0;  !abort && j < Yd; j++){
					for (int k = 0; k < Xd; k++) {
						//grayScalar[i][j][k] = *readVol;
						if(*ptr){
							fprintf(out, "%f %f %f\n", 100.0*k/Xd, 100.0*j/Yd, 100.0*i/Zd);
							//fprintf(out, "%d %d %d\n", i, j, k);
							nonzero++;
						}
						ptr++;
					}
				}
			}
			fseek(out, 64, SEEK_SET); //文件指针重新指向文件头
			fprintf(out, "%d", nonzero); //写点数（vertex）
			break;
			
		default:break;
	}

	/*
	short *** grayScalar = new short**[Zd];  //Never write "new (double**)[n]"
	for(int i = 0; i < Zd; i++){
		grayScalar[i] = new short*[Yd];
		for(int j = 0; j < Yd; j++){
			grayScalar[i][j] =  new short[Xd];
		}
	}*/
  



	//output the statistics of HU
	/*
	std::map<int, int> stat;
	fprintf(out, "-------statistics-------\n");
	fprintf(out, "Non-zero pixel:%d\n", nonzpix);
	for(std::map<int, int>::const_iterator it = stat.begin(); it != stat.end(); it++){
	  fprintf(out, "HU[%d]=%d\n", it->first, it->second);
	}*/
  

	/*
	for(int i = 0; i < Zd; i++){
		for(int j = 0; j < Yd; j++){
			delete[] grayScalar[i][j];
		}
		delete[] grayScalar[i];
	}
	delete[] grayScalar;*/
  
	fclose(out);
}

template <class IT>
void reserveSurfacePoints(int* dim, int inc, vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds,  IT* ptr){
	//保留二值“实心”图像的表面体素
	int Xd = (int)dim[0];
	int Yd = (int)dim[1]; 
	int Zd = (int)dim[2];
	int abort;
	IT* readVol = ptr;

	/*FILE* out;
	if(out = fopen("coronary_surface.txt", "w")){
		fprintf(out, "%d x %d x %d voxels, InputVolumeNumberOfComponents = %d\n", Xd, Yd, Zd, inc);
	}*/

	short *** grayScalar = new short**[Zd];  //Never write "new (double**)[n]"
	for(int i = 0; i < Zd; i++){
		grayScalar[i] = new short*[Yd];
		for(int j = 0; j < Yd; j++){
			grayScalar[i][j] =  new short[Xd];
		}
	}

	bool*** border = new bool**[Zd];   //标记是否为边界点
	for(int i = 0; i < Zd; i++){
		border[i] = new bool*[Yd];
		for(int j = 0; j < Yd; j++){
			border[i][j] =  new bool[Xd];
			for(int k = 0; k < Xd; k++)
				border[i][j][k] = false;
		}
	}
	
	for (int i = 0; i < Zd; i++){
		info->UpdateProgress(info,(float)1.0*i/Zd,"Processing..."); 
		abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
		for (int j = 0;  !abort && j < Yd; j++){
			for (int k = 0; k < Xd; k++) {
				grayScalar[i][j][k] = *readVol;
				readVol++;
			}
		}
	}

	const int neighborDirection[26][3] = {
		{1, 0, 0},{0, 1, 0},{0, 0, 1},{-1, 0, 0},{0, -1, 0},{0, 0, -1},
		{1, 1, 0},{1, -1, 0},{-1, 1, 0},{-1, -1, 0},
		{0, 1, 1},{0, 1, -1},{0, -1, 1},{0, -1, -1},
		{1, 0, 1},{1, 0, -1},{-1, 0, 1},{-1, 0, -1},
		{1, 1, 1},{1, 1, -1},{1, -1, 1},{1, -1, -1},{-1, 1, 1},{-1, 1, -1},{-1, -1, 1},{-1,-1,-1}
	};
	for (int i = 0; i < Zd; i++){
		info->UpdateProgress(info,(float)1.0*i/Zd,"Processing..."); 
		abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
		for (int j = 0; !abort && j < Yd; j++){
			for (int k = 0; k < Xd; k++) {
				if(grayScalar[i][j][k]){
				  int countBackground = 0;
				  for(int h = 0; h < 26; h++){
					  if(!grayScalar[i + neighborDirection[h][0]][j + neighborDirection[h][1]][k + neighborDirection[h][2]])
						  countBackground++;
				  }
				  if(countBackground > 10)  //26邻域里有多少是背景点算作边界，可调
					  border[i][j][k] = true;
				}
			}
		}
	}
  
	for (int i = 0; i < Zd; i++){
		info->UpdateProgress(info,(float)1.0*i/Zd,"Processing..."); 
		abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
		for (int j = 0; !abort && j < Yd; j++){
			for (int k = 0; k < Xd; k++) {
				if(!border[i][j][k]){ 
					*ptr = 0;   //Min of gray scalar，可调
				}
				//输出功能放到generateStatisticsOrPly里去实现
				/*else{
					fprintf(out, "%.3f %.3f %.3f\n", 10.0 * i / Xd, 10.0 * j / Yd, 10.0 * k / Zd);
				}*/
				ptr++;
			}
		}
	}

	for(int i = 0; i < Zd; i++){
		for(int j = 0; j < Yd; j++){
			delete[] grayScalar[i][j];
		}
		delete[] grayScalar[i];
	}
	delete[] grayScalar;

	for(int i = 0; i < Zd; i++){
		for(int j = 0; j < Yd; j++){
			delete[] border[i][j];
		}
		delete[] border[i];
	}
	delete[] border;
	
	//fclose(out);
}

template <class IT>
void coronaryBw2Gray(int* dim, int inc, vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds,  IT* ptr){
	//将分割好的二值图像匹配到原灰度图像

	int abort;
	int Xd = (int)dim[0];
	int Yd = (int)dim[1]; 
	int Zd = (int)dim[2];


	FILE *in;
	set<Coordinate> pointSet;
	int x, y, z;
	char volInfo[102];
	if(in = fopen("coronary_data.txt", "r")){
		//fscanf(in, "%s", volInfo); // 错误！一旦第一行有空格，就读到空格为止！
		fgets(volInfo, 100, in);  //or scanf("%[^\n]", str);
		while(3 == fscanf(in, "%d %d %d", &z, &y, &x)){
			Coordinate co(z, y, x);
			pointSet.insert(co);
		}
		fclose(in);
	}
	else{
		perror("coronary_data.txt");  //print "coronary_data.txt: No such file or directory"
	}
	
	
	FILE *console;
	console = fopen("console.log", "w");
	fprintf(console, "%s", volInfo);
	if(!pointSet.empty())
		fprintf(console, "size of set: %d\n", (int)pointSet.size());
	fclose(console);
	
	for (int i = 0; i < Zd; i++){
	  info->UpdateProgress(info,(float)1.0*i/Zd,"Processing..."); 
	  abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
	  for (int j = 0;  !abort && j < Yd; j++){
		  for (int k = 0; k < Xd; k++) {
			  Coordinate co(i, j, k);
			  if(pointSet.find(co) == pointSet.end()) 
				  *ptr = -1024;
			  ptr++;
		  }
	  }
  }
}


template <class IT>
void coronaryStenosis(int* dim, int inc, vtkVVPluginInfo *info, vtkVVProcessDataStruct *pds,  IT* ptr){
	//计算中心线上的模糊距离
	int abort;
	int Xd = (int)dim[0];
	int Yd = (int)dim[1]; 
	int Zd = (int)dim[2];
	short *** fuzzyDis = new short**[Zd];  //Never write "new (double**)[n]"
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
				fuzzyDis[i][j][k] = *ptr;
				ptr++;
			}
		}
	}

	FILE *in, *out;
	out = fopen("skeleton_diameter.txt", "w");
	int x, y, z;
	char volInfo[102];
	if(in = fopen("coronary_skeleton.txt", "r")){
		//fscanf(in, "%s", volInfo); // 错误！一旦第一行有空格，就读到空格为止！
		fgets(volInfo, 100, in);  //or scanf("%[^\n]", str);
		while(3 == fscanf(in, "%d %d %d", &z, &y, &x)){
			fprintf(out, "(%d, %d, %d): %d\n", z, y, x, fuzzyDis[z][y][x]);
		}
		fclose(in);
	}
	else{
		perror("coronary_skeleton.txt");  //print "coronary_data.txt: No such file or directory"
	}
	fclose(out);

}
template <class IT>
/* TODO 1: Rename vvSampleTemplate to vv<your_plugin>Template */
void vvLzfStatisticsTemplate(vtkVVPluginInfo *info,
                      vtkVVProcessDataStruct *pds, 
                      IT *)
{
	IT *inPtr = (IT *)pds->inData;
	IT *outPtr = (IT *)pds->outData;
	int *dim = info->InputVolumeDimensions;
	int inNumComp = info->InputVolumeNumberOfComponents;
 
  
  /*
  *The following code is writen by lzf on May 22, 2015
  *for save as a .ply file
  */
	
	//reserveSurfacePoints(dim, inNumComp, info, pds, outPtr);
	
	//generateStatisticsOrPly(dim, inNumComp, info, pds, outPtr, 0); //0:txt 1:ply

	//coronaryBw2Gray(dim, inNumComp, info, pds, outPtr);

	coronaryStenosis(dim, inNumComp, info, pds, outPtr);
                                                    
	info->UpdateProgress(info,(float)1.0,"Processing Complete");
}

static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  switch (info->InputVolumeScalarType)
    {
    /* TODO 2: Rename vvSampleTemplate to vv<your_plugin>Template */
    vtkTemplateMacro3(vvLzfStatisticsTemplate, info, pds, 
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
  void VV_PLUGIN_EXPORT vvLzfStatisticsInit(vtkVVPluginInfo *info)
  {
    /* always check the version */
    vvPluginVersionCheck();
    
    /* setup information that never changes */
    info->ProcessData = ProcessData;
    info->UpdateGUI = UpdateGUI;
    
    /* set the properties this plugin uses */
    /* TODO 4: Rename "Sample" to "<your_plugin>" */
    info->SetProperty(info, VVP_NAME, "LzfStatistics");
    info->SetProperty(info, VVP_GROUP, "Utility");

    /* TODO 5: update the terse and full documentation for your filter */
    info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
                      "Replace voxels above/at/below the threshold value");
    info->SetProperty(info, VVP_FULL_DOCUMENTATION,
                      "This filter performs a pixel replacement, replacing pixel in the original data by a specified replacement value. The pixels to be replaced are determine by comparing the original pixel value to a threshold value with a user specified comparison operation. This filter operates in place, and does not change the dimensions, data type, or spacing of the volume.");

    
    /* TODO 9: set these two values to "0" or "1" based on how your plugin
     * handles data all possible combinations of 0 and 1 are valid. */
    info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "1");
    info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "1");

    /* TODO 7: set the number of GUI items used by this plugin */
    info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "0");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
  }
}
>>>>>>> 582480cd14e067db3b1fed95c429f1724f683c48
