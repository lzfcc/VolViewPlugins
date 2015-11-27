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
void generateStatisticsOrPly(vtkVVPluginInfo *info,  IT* ptr, int filetype){
	FILE *out;
	int abort;
	int* dim = info->InputVolumeDimensions;
	int nonzero = 0;
	int Xd = (int)dim[0];
	int Yd = (int)dim[1]; 
	int Zd = (int)dim[2];
	switch(filetype){
		case 0:
			if(out = fopen("coronary_skeleton.txt", "w")){
				fprintf(out, "%d x %d x %d voxels, InputVolumeNumberOfComponents = %d\n", Xd, Yd, Zd, info->InputVolumeNumberOfComponents);
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
							//fprintf(out, "%f %f %f\n", 100.0*k/Xd, 100.0*j/Yd, 100.0*i/Zd);
							fprintf(out, "%d %d %d\n", i, j, k);
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
void reserveSurfacePoints(vtkVVPluginInfo *info,  IT* ptr){
	//保留二值“实心”图像的表面体素
	int* dim = info->InputVolumeDimensions;
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
				  if(countBackground >= 9)  //26邻域里有多少是背景点算作边界，可调
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
					fprintf(out, "(%d, %d, %d): %d\n", i, j, k, *ptr);
				ptr++;
				ptr2++;
			}
		}
	}

	fclose(out);
}

template <class IT, class I2T>
void coronaryBw2Gray(vtkVVPluginInfo *info,
								 vtkVVProcessDataStruct *pds, 
								 IT *, I2T *)
{
	IT *ptr = (IT *)pds->outData;
	I2T *ptr2 = (I2T *)pds->inData2;

	int *dim = info->InputVolumeDimensions;

	int abort = 0;
	int inNumCom = info->InputVolumeNumberOfComponents;
	int inNumCom2 = info->InputVolume2NumberOfComponents;
	
	//将分割好的二值图像匹配到原灰度图像

	int Xd = (int)dim[0];
	int Yd = (int)dim[1]; 
	int Zd = (int)dim[2];

	/*
	FILE *in;
	int x, y, z;
	set<Coordinate> pointSet;
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
	*/
	
	/*FILE *console;
	console = fopen("console.log", "w");
	fprintf(console, "%s", volInfo);
	if(!pointSet.empty())
		fprintf(console, "size of set: %d\n", (int)pointSet.size());
	fclose(console);*/
	
	for (int i = 0; i < Zd; i++){
		info->UpdateProgress(info,(float)1.0*i/Zd,"Processing..."); 
		abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
		for (int j = 0; !abort && j < Yd; j++){
			for (int k = 0; k < Xd; k++) {
				if(!*ptr2) 
					*ptr = -1024;
				ptr++;
				ptr2++;
			}
		}
	}
}


template <class IT>
/* TODO 1: Rename vvSampleTemplate to vv<your_plugin>Template */
void vvLzfStatisticsTemplate(vtkVVPluginInfo *info,
                      vtkVVProcessDataStruct *pds, 
                      IT *)
{
	IT *inPtr1 = (IT *)pds->inData;
	IT *outPtr1 = (IT *)pds->outData;
	int *dim1 = info->InputVolumeDimensions;
	int inNumComp1 = info->InputVolumeNumberOfComponents;

	//下面这样直接认定第二图像是某类型的写法不太好！但数据位长不统一，容易出错（都设为IT*（第一个图像的类型），指针自增的时候，位长短的图像提前访问到尾部）。先这样吧。。。
	unsigned char *inPtr2 = (unsigned char*)pds->inData2;  
	//对于coronaryBw2Gray，打开的是原始的灰度图像（2字节），图像2是分割好的血管二值图像（如果是心脏区域的提取也可以）（1字节）

	//对于coronaryStenosis，打开的是fdt后的图像（2字节），图像2是血管骨架线二值图像（1字节）
	int *dim2 = info->InputVolume2Dimensions;
	int inNumComp2 = info->InputVolume2NumberOfComponents;

	int Xd = (int)dim1[0];
	int Yd = (int)dim1[1]; 
	int Zd = (int)dim1[2];
  /*
  *The following code is writen by lzf on May 22, 2015
  *for save as a .ply file
  */
	
	reserveSurfacePoints(info, outPtr1);
	
	//generateStatisticsOrPly(info, outPtr1, 1); //0:txt 1:ply

	//以下方法解决了两幅图像位长不相同的问题。
	/*switch (info->InputVolume2ScalarType)
	{
		// invoke the appropriate templated function
		vtkTemplateMacro4(coronaryBw2Gray, info, pds, 
			static_cast<IT *>(0), static_cast<VTK_TT *>(0));
		//vtkTemplateMacro4(coronaryStenosis, info, pds, static_cast<IT *>(0), static_cast<VTK_TT *>(0));
	}*/
                                                    
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
                      "This plugin includes several functions...");
    info->SetProperty(info, VVP_FULL_DOCUMENTATION,
                      "This plugin is created on May 22nd, 2015, and being expanded. You only need to import the second image when 'coronaryBw2Gray'.");

    
    /* TODO 9: set these two values to "0" or "1" based on how your plugin
     * handles data all possible combinations of 0 and 1 are valid. */
    info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "1");
    info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "1");

    /* TODO 7: set the number of GUI items used by this plugin */
    info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "0");
		info->SetProperty(info, VVP_REQUIRES_SECOND_INPUT,        "0");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
  }
}
