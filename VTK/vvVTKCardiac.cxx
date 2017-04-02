/*=========================================================================

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/VolViewCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
<<<<<<< HEAD
#include "vtkVVPluginAPI.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#ifdef _MSC_VER
#pragma warning ( disable : 4710 )
#endif

class Data
{
public:
	unsigned short *image;
	unsigned short *xray;
	int num;
	int slices;
	int res;
	char *name;
	float scalex,scaley,scalez;
};

class Patient : public Data
{
public:
  Patient() {};
	//Patient(Data *pD,int threshold);
	void segment(Data *pD);
	void filter(int s,int d, int radius,Data *pD);
	void remove_blood(Data *pD);
	int count(Data *pD);
	void connect(int *seed,Data *pD);
	unsigned char* seg;
	int radius;
	int s;
	int d;
	int value;
};

int Patient::count(Data *pD)
{
	int i;
	int n=0;
	for(i=0;i<pD->slices*pD->res*pD->res;i++){
		if(seg[i]==1)n++;
	}
	return n;
}
void Patient::segment(Data *pD)
{
	
	int i,z0;
	int res=pD->res;
	int pixels=pD->res*pD->res;
	int voxels;
	int x0=256;
	int y0=256;
	int seed[3];
	int pix;
	unsigned char zero=0;
	unsigned char two=2;
	voxels=pD->slices*pixels;
  std::cout<<"number of voxels = "<<voxels<<std::endl;
	
	std::cout<<"                  segment compising:"<<std::endl;
	std::cout<<"threshold"<<std::endl;
	for(i=0;i<voxels;i++){
		if(pD->image[i]>value)seg[i]=1;
		else seg[i] = 0;
	}
	std::cout<<"number of elements = " <<count(pD)<<std::endl;
	std::cout<<"erode with radius = 5"<<std::endl;
	filter(1,0,5,pD);
	std::cout<<"number of elements = " <<count(pD)<<std::endl;
	z0=pD->slices/2;
	while(*(seg+x0+x0*res+z0*pixels)==0){
		z0 += 10;
	}
	std::cout<<"seed ="<<x0<<", "<<y0<<", "<<z0<<std::endl;
	seed[0]=x0;
	seed[1]=y0;
	seed[2]=z0;

	std::cout<<"connect"<<std::endl;
	connect(seed,pD);
	std::cout<<"number of elements = " <<count(pD)<<std::endl;
	
	std::cout<<"remove blood"<<std::endl;
	remove_blood(pD);
	std::cout<<"number of elements = " <<count(pD)<<std::endl;

	std::cout<<"dilate with radius = 20"<<std::endl;
	filter(0,1,20,pD);
	std::cout<<"number of elements = " <<count(pD)<<std::endl;
	
	std::cout<<"erode blood with radius = 6"<<std::endl;
	filter(2,1,6,pD);
	std::cout<<"number of elements = " <<count(pD)<<std::endl;

	std::cout<<"dilate blood with radius = 9"<<std::endl;
	filter(1,2,9,pD);
	std::cout<<"number of elements = " <<count(pD)<<std::endl;

	std::cout<<"mask images"<<std::endl;
	pix=0;
	for(i=0;i<voxels;i++){
				//if(seg[i]==1)std::cout<<pD->image[i];
				if(seg[i]==zero)pD->image[i]=0;              //remove mask with 0 or 2
				if(seg[i]==two)pD->image[i]=0;
	}
}
void Patient::connect(int *seed, Data *pD)
{
	int x0,y0,z0;
	x0=seed[0];
	y0=seed[1];
	z0=seed[2];
	unsigned char *segptr,*buf,*bufptr;
	int pixels;
	int *stack1,*stack2;  //two seed stacks
	int count;
	int x,y,z;
	int *stkp1,*stkp2;
	int seeds,i,n,iterate;
	int total;
	int res=512;

	total=0;
	slices=pD->slices;
	pixels=res*res;
	iterate=100000;

	stack1 = new int[pixels*16];
	stack2 = new int[pixels*16];
	buf    = new unsigned char[pixels*slices];

	for(i=0;i<pixels*slices;i++)buf[i]=0;
	//zero first and last slice
	for(i=0;i<pixels;i++)seg[i]=0;
	for(i=0;i<pixels;i++)seg[i+pixels*(slices-1)]=0;

	
	x0=seed[0];
	y0=seed[1];
	z0=seed[2];

	stkp1=stack1;
	stkp2=stack2;
	//start seed in heart
	*stkp1++ = x0;
	*stkp1++ = y0;
	*stkp1++ = z0;
	seeds=1;
	count=0;
	for(n=0;n<iterate;n++){
		//std::cout<<z<<" ";
		stkp1=stack1;
		stkp2=stack2;
		while(seeds--){
			x= *stkp1++;
			y= *stkp1++;
			z= *stkp1++;
			
			segptr=seg+x+y*res+z*pixels;
			bufptr=buf+x+y*res+z*pixels;
			if(x<res-1){
				if(*(segptr+1)==1){
					if(*(bufptr+1)==0){
						count++;
						*(bufptr+1) = 1;
						*stkp2++ = x+1;
						*stkp2++ = y;
						*stkp2++ = z;
					}
				}
			}
			if(x>0){
				if(*(segptr-1)==1){
					if(*(bufptr-1)==0){
						count++;
						*(bufptr-1) = 1;
						*stkp2++ = x-1;
						*stkp2++ = y;
						*stkp2++ = z;
					}
				}
			}
			if(y<res-1){
				if(*(segptr+res)==1){
					if(*(bufptr+res)==0){
						count++;
						*(bufptr+res) = 1;
						*stkp2++ = x;
						*stkp2++ = y+1;
						*stkp2++ = z;
					}
				}
			}
			if(y>0){
				if(*(segptr-res)==1){
					if(*(bufptr-res)==0){
						count++;
						*(bufptr-res) = 1;
						*stkp2++ = x;
						*stkp2++ = y-1;
						*stkp2++ = z;
					}
				}
			}
		
			if(z<slices-1){
				if(*(segptr+pixels)==1){
					if(*(bufptr+pixels)==0){
						count++;
						*(bufptr+pixels) = 1;
						*stkp2++ = x;
						*stkp2++ = y;
						*stkp2++ = z+1;
					}
				}
			}
			if(z>0){
				if(*(segptr-pixels)==1){
					if(*(bufptr-pixels)==0){
						count++;
						*(bufptr-pixels) = 1;
						*stkp2++ = x;
						*stkp2++ = y;
						*stkp2++ = z-1;	
					}
				}
			}
		}
		seeds=count;
		total += count;
		stkp1=stack1;
		stkp2=stack2;
		
		if(count>16*pixels/3)std::cout<<"overflow"<<std::endl;
		for(i=0;i<3*seeds;i++){
			*stkp1++ = *stkp2++;
		}
		
		if(count==0)break;
		count=0;
	}
	//std::cout<<"total="<<total<<" ";
	segptr=seg;
	bufptr=buf;
	for(i=0;i<pixels*slices;i++){
		*segptr++ = *bufptr++;
	}
	delete [] buf;
	delete [] stack1;
	delete [] stack2;
	buf=0;
	//return total;
}
void Patient::remove_blood(Data *pD)
{
	int x,y,z,i;
	unsigned char *ptr, *bufptr, *buf;
	int res=pD->res;
	int slices=pD->slices;
	int pixels = res*res;
	int P,E,N,W,S,U,D;

	buf=new unsigned char[pixels*slices];
	for(i=0;i<pixels*slices;i++)buf[i]=0;
	for(z=1;z<slices-1;z++){
		//std::cout<<".";
		for(y=1;y<res-1;y++){
			for(x=1;x<res-1;x++){
				ptr=seg+x+res*y+pixels*z;
				bufptr=buf+x+res*y+pixels*z;
				P=*ptr;
				if(P==1){ //in object
					E=*(ptr+1);
					W=*(ptr-1);
					N=*(ptr+res);
					S=*(ptr-res);
					U=*(ptr+pixels);
					D=*(ptr-pixels);
					if(E*W*N*S*U*D==1){
						*bufptr=2;
					}
					else{
						*bufptr=1;
					}
				}		
			}
		}
	}
	for(i=0;i<pixels*slices;i++)seg[i]=buf[i];
	delete [] buf;
}

void Patient::filter(int s, int d, int radius,Data *pD)
{
	int i,j,x,y,z;
	int E,W,S,N,U,D,P;
	int pixels=512*512;
	int res=512;
	int slices=pD->slices;

	unsigned char *segptr,*buf,*bufptr;

	buf=new unsigned char[pixels*slices];
	segptr=seg;
	for(i=0;i<pixels*slices;i++)buf[i]=seg[i];
	for(i=0;i<radius;i++){
		//scan mask and mark surface voxels
		for(z=1;z <slices-1;z++){
			for(y=1;y <res-1;y++){
				for(x=1;x < res-1;x++){
					segptr=seg+x+y*res+z*res*res;
					bufptr=buf+x+y*res+z*res*res;
					P=*segptr;
					E=*(segptr+1);
					W=*(segptr-1);
					N=*(segptr+res);
					S=*(segptr-res);
					U=*(segptr+pixels);
					D=*(segptr-pixels);
					if(P==d){
						if(E==s)*(bufptr+1)=d;
						if(W==s)*(bufptr-1)=d;
						if(N==s)*(bufptr+res)=d;
						if(S==s)*(bufptr-res)=d;
						if(U==s)*(bufptr+pixels)=d;
						if(D==s)*(bufptr-pixels)=d;
					}
				}		
			}
		}
		for(j=0;j<pixels*slices;j++) seg[j]=buf[j];
		std::cout<<i<<" ";
	}
	delete [] buf;
}



//-----------------------------------------------------------
template <class IT>
void vvVTKCardiacTemplate(vtkVVPluginInfo *info,
                         vtkVVProcessDataStruct *pds, 
                         IT )
{
  IT *ptr = (IT *)pds->outData;
  int *dim = info->InputVolumeDimensions;
  double threshold = atof(info->GetGUIProperty(info, 0, VVP_GUI_VALUE));
  int abort = 0;

  Data pD;
  Patient pP;
  pD.image = (unsigned short *)(pds->outData);
  pD.res = info->InputVolumeDimensions[0];
	pP.seg	= new unsigned char[info->InputVolumeDimensions[0] * 
    info->InputVolumeDimensions[1] * info->InputVolumeDimensions[1]];
  pD.scalex = 1.0;
  pD.scaley = 1.0;
  pD.scalez = this->InputVolumeSpacing[2]/this->InputVolumeSpacing[0];
  pP.value = (int)threshold;
  pD.slices = info->InputVolumeDimensions[2];

  // The bulk of the work is done here
  pP.segment(&pD);
}

static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  switch (info->InputVolumeScalarType)
    {
    // invoke the appropriate templated function
    vtkTemplateMacro3(vvVTKCardiacTemplate, info, pds, 
                      static_cast<short>(0));
    }
  return 0;
}


static int UpdateGUI(void *inf)
{
  char tmp[1024];
  double stepSize = 1.0;
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "Threshold Value");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT , "0");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP, "Threshold");
    
  /* set the range of the sliders */
  if (info->InputVolumeScalarType == VTK_FLOAT || 
      info->InputVolumeScalarType == VTK_DOUBLE) 
    { 
    /* for float and double use a step size of 1/200 th the range */
    stepSize = info->InputVolumeScalarRange[1]*0.005 - 
      info->InputVolumeScalarRange[0]*0.005; 
    }
  sprintf(tmp,"%g %g %g", 
          info->InputVolumeScalarRange[0],
          info->InputVolumeScalarRange[1],
          stepSize);
  info->SetGUIProperty(info, 0, VVP_GUI_HINTS , tmp);

  /* auto range for scalar type */
  vvPluginSetGUIScaleRange(2);

  info->OutputVolumeScalarType = info->InputVolumeScalarType;
  info->OutputVolumeNumberOfComponents = info->InputVolumeNumberOfComponents;
  for (int i = 0; i < 3; i++)
    {
    info->OutputVolumeDimensions[i] = info->InputVolumeDimensions[i];
    info->OutputVolumeSpacing[i] = info->InputVolumeSpacing[i];
    info->OutputVolumeOrigin[i] = info->InputVolumeOrigin[i];
    }

  return 1;
}

extern "C" 
{
void VV_PLUGIN_EXPORT vvVTKCardiacInit(vtkVVPluginInfo *info)
{
  /* always check the version */
  vvPluginVersionCheck();

  /* setup information that never changes */
  info->ProcessData = ProcessData;
  info->UpdateGUI = UpdateGUI;

  /* set the properties this plugin uses */
  info->SetProperty(info, VVP_NAME, "Cardiac Segmentation");
  info->SetProperty(info, VVP_GROUP, "Cardiac");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION, "Cardiac Segmentation");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION, "Cardiac segmentation.");

  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "1");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "1");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "1");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}
  
}




=======
#include "vtkVVPluginAPI.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#ifdef _MSC_VER
#pragma warning ( disable : 4710 )
#endif

class Data
{
public:
	unsigned short *image;
	unsigned short *xray;
	int num;
	int slices;
	int res;
	char *name;
	float scalex,scaley,scalez;
};

class Patient : public Data
{
public:
  Patient() {};
	//Patient(Data *pD,int threshold);
	void segment(Data *pD);
	void filter(int s,int d, int radius,Data *pD);
	void remove_blood(Data *pD);
	int count(Data *pD);
	void connect(int *seed,Data *pD);
	unsigned char* seg;
	int radius;
	int s;
	int d;
	int value;
};

int Patient::count(Data *pD)
{
	int i;
	int n=0;
	for(i=0;i<pD->slices*pD->res*pD->res;i++){
		if(seg[i]==1)n++;
	}
	return n;
}
void Patient::segment(Data *pD)
{
	
	int i,z0;
	int res=pD->res;
	int pixels=pD->res*pD->res;
	int voxels;
	int x0=256;
	int y0=256;
	int seed[3];
	int pix;
	unsigned char zero=0;
	unsigned char two=2;
	voxels=pD->slices*pixels;
  std::cout<<"number of voxels = "<<voxels<<std::endl;
	
	std::cout<<"                  segment compising:"<<std::endl;
	std::cout<<"threshold"<<std::endl;
	for(i=0;i<voxels;i++){
		if(pD->image[i]>value)seg[i]=1;
		else seg[i] = 0;
	}
	std::cout<<"number of elements = " <<count(pD)<<std::endl;
	std::cout<<"erode with radius = 5"<<std::endl;
	filter(1,0,5,pD);
	std::cout<<"number of elements = " <<count(pD)<<std::endl;
	z0=pD->slices/2;
	while(*(seg+x0+x0*res+z0*pixels)==0){
		z0 += 10;
	}
	std::cout<<"seed ="<<x0<<", "<<y0<<", "<<z0<<std::endl;
	seed[0]=x0;
	seed[1]=y0;
	seed[2]=z0;

	std::cout<<"connect"<<std::endl;
	connect(seed,pD);
	std::cout<<"number of elements = " <<count(pD)<<std::endl;
	
	std::cout<<"remove blood"<<std::endl;
	remove_blood(pD);
	std::cout<<"number of elements = " <<count(pD)<<std::endl;

	std::cout<<"dilate with radius = 20"<<std::endl;
	filter(0,1,20,pD);
	std::cout<<"number of elements = " <<count(pD)<<std::endl;
	
	std::cout<<"erode blood with radius = 6"<<std::endl;
	filter(2,1,6,pD);
	std::cout<<"number of elements = " <<count(pD)<<std::endl;

	std::cout<<"dilate blood with radius = 9"<<std::endl;
	filter(1,2,9,pD);
	std::cout<<"number of elements = " <<count(pD)<<std::endl;

	std::cout<<"mask images"<<std::endl;
	pix=0;
	for(i=0;i<voxels;i++){
				//if(seg[i]==1)std::cout<<pD->image[i];
				if(seg[i]==zero)pD->image[i]=0;              //remove mask with 0 or 2
				if(seg[i]==two)pD->image[i]=0;
	}
}
void Patient::connect(int *seed, Data *pD)
{
	int x0,y0,z0;
	x0=seed[0];
	y0=seed[1];
	z0=seed[2];
	unsigned char *segptr,*buf,*bufptr;
	int pixels;
	int *stack1,*stack2;  //two seed stacks
	int count;
	int x,y,z;
	int *stkp1,*stkp2;
	int seeds,i,n,iterate;
	int total;
	int res=512;

	total=0;
	slices=pD->slices;
	pixels=res*res;
	iterate=100000;

	stack1 = new int[pixels*16];
	stack2 = new int[pixels*16];
	buf    = new unsigned char[pixels*slices];

	for(i=0;i<pixels*slices;i++)buf[i]=0;
	//zero first and last slice
	for(i=0;i<pixels;i++)seg[i]=0;
	for(i=0;i<pixels;i++)seg[i+pixels*(slices-1)]=0;

	
	x0=seed[0];
	y0=seed[1];
	z0=seed[2];

	stkp1=stack1;
	stkp2=stack2;
	//start seed in heart
	*stkp1++ = x0;
	*stkp1++ = y0;
	*stkp1++ = z0;
	seeds=1;
	count=0;
	for(n=0;n<iterate;n++){
		//std::cout<<z<<" ";
		stkp1=stack1;
		stkp2=stack2;
		while(seeds--){
			x= *stkp1++;
			y= *stkp1++;
			z= *stkp1++;
			
			segptr=seg+x+y*res+z*pixels;
			bufptr=buf+x+y*res+z*pixels;
			if(x<res-1){
				if(*(segptr+1)==1){
					if(*(bufptr+1)==0){
						count++;
						*(bufptr+1) = 1;
						*stkp2++ = x+1;
						*stkp2++ = y;
						*stkp2++ = z;
					}
				}
			}
			if(x>0){
				if(*(segptr-1)==1){
					if(*(bufptr-1)==0){
						count++;
						*(bufptr-1) = 1;
						*stkp2++ = x-1;
						*stkp2++ = y;
						*stkp2++ = z;
					}
				}
			}
			if(y<res-1){
				if(*(segptr+res)==1){
					if(*(bufptr+res)==0){
						count++;
						*(bufptr+res) = 1;
						*stkp2++ = x;
						*stkp2++ = y+1;
						*stkp2++ = z;
					}
				}
			}
			if(y>0){
				if(*(segptr-res)==1){
					if(*(bufptr-res)==0){
						count++;
						*(bufptr-res) = 1;
						*stkp2++ = x;
						*stkp2++ = y-1;
						*stkp2++ = z;
					}
				}
			}
		
			if(z<slices-1){
				if(*(segptr+pixels)==1){
					if(*(bufptr+pixels)==0){
						count++;
						*(bufptr+pixels) = 1;
						*stkp2++ = x;
						*stkp2++ = y;
						*stkp2++ = z+1;
					}
				}
			}
			if(z>0){
				if(*(segptr-pixels)==1){
					if(*(bufptr-pixels)==0){
						count++;
						*(bufptr-pixels) = 1;
						*stkp2++ = x;
						*stkp2++ = y;
						*stkp2++ = z-1;	
					}
				}
			}
		}
		seeds=count;
		total += count;
		stkp1=stack1;
		stkp2=stack2;
		
		if(count>16*pixels/3)std::cout<<"overflow"<<std::endl;
		for(i=0;i<3*seeds;i++){
			*stkp1++ = *stkp2++;
		}
		
		if(count==0)break;
		count=0;
	}
	//std::cout<<"total="<<total<<" ";
	segptr=seg;
	bufptr=buf;
	for(i=0;i<pixels*slices;i++){
		*segptr++ = *bufptr++;
	}
	delete [] buf;
	delete [] stack1;
	delete [] stack2;
	buf=0;
	//return total;
}
void Patient::remove_blood(Data *pD)
{
	int x,y,z,i;
	unsigned char *ptr, *bufptr, *buf;
	int res=pD->res;
	int slices=pD->slices;
	int pixels = res*res;
	int P,E,N,W,S,U,D;

	buf=new unsigned char[pixels*slices];
	for(i=0;i<pixels*slices;i++)buf[i]=0;
	for(z=1;z<slices-1;z++){
		//std::cout<<".";
		for(y=1;y<res-1;y++){
			for(x=1;x<res-1;x++){
				ptr=seg+x+res*y+pixels*z;
				bufptr=buf+x+res*y+pixels*z;
				P=*ptr;
				if(P==1){ //in object
					E=*(ptr+1);
					W=*(ptr-1);
					N=*(ptr+res);
					S=*(ptr-res);
					U=*(ptr+pixels);
					D=*(ptr-pixels);
					if(E*W*N*S*U*D==1){
						*bufptr=2;
					}
					else{
						*bufptr=1;
					}
				}		
			}
		}
	}
	for(i=0;i<pixels*slices;i++)seg[i]=buf[i];
	delete [] buf;
}

void Patient::filter(int s, int d, int radius,Data *pD)
{
	int i,j,x,y,z;
	int E,W,S,N,U,D,P;
	int pixels=512*512;
	int res=512;
	int slices=pD->slices;

	unsigned char *segptr,*buf,*bufptr;

	buf=new unsigned char[pixels*slices];
	segptr=seg;
	for(i=0;i<pixels*slices;i++)buf[i]=seg[i];
	for(i=0;i<radius;i++){
		//scan mask and mark surface voxels
		for(z=1;z <slices-1;z++){
			for(y=1;y <res-1;y++){
				for(x=1;x < res-1;x++){
					segptr=seg+x+y*res+z*res*res;
					bufptr=buf+x+y*res+z*res*res;
					P=*segptr;
					E=*(segptr+1);
					W=*(segptr-1);
					N=*(segptr+res);
					S=*(segptr-res);
					U=*(segptr+pixels);
					D=*(segptr-pixels);
					if(P==d){
						if(E==s)*(bufptr+1)=d;
						if(W==s)*(bufptr-1)=d;
						if(N==s)*(bufptr+res)=d;
						if(S==s)*(bufptr-res)=d;
						if(U==s)*(bufptr+pixels)=d;
						if(D==s)*(bufptr-pixels)=d;
					}
				}		
			}
		}
		for(j=0;j<pixels*slices;j++) seg[j]=buf[j];
		std::cout<<i<<" ";
	}
	delete [] buf;
}



//-----------------------------------------------------------
template <class IT>
void vvVTKCardiacTemplate(vtkVVPluginInfo *info,
                         vtkVVProcessDataStruct *pds, 
                         IT )
{
  IT *ptr = (IT *)pds->outData;
  int *dim = info->InputVolumeDimensions;
  double threshold = atof(info->GetGUIProperty(info, 0, VVP_GUI_VALUE));
  int abort = 0;

  Data pD;
  Patient pP;
  pD.image = (unsigned short *)(pds->outData);
  pD.res = info->InputVolumeDimensions[0];
	pP.seg	= new unsigned char[info->InputVolumeDimensions[0] * 
    info->InputVolumeDimensions[1] * info->InputVolumeDimensions[1]];
  pD.scalex = 1.0;
  pD.scaley = 1.0;
  pD.scalez = this->InputVolumeSpacing[2]/this->InputVolumeSpacing[0];
  pP.value = (int)threshold;
  pD.slices = info->InputVolumeDimensions[2];

  // The bulk of the work is done here
  pP.segment(&pD);
}

static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  switch (info->InputVolumeScalarType)
    {
    // invoke the appropriate templated function
    vtkTemplateMacro3(vvVTKCardiacTemplate, info, pds, 
                      static_cast<short>(0));
    }
  return 0;
}


static int UpdateGUI(void *inf)
{
  char tmp[1024];
  double stepSize = 1.0;
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  info->SetGUIProperty(info, 0, VVP_GUI_LABEL, "Threshold Value");
  info->SetGUIProperty(info, 0, VVP_GUI_TYPE, VVP_GUI_SCALE);
  info->SetGUIProperty(info, 0, VVP_GUI_DEFAULT , "0");
  info->SetGUIProperty(info, 0, VVP_GUI_HELP, "Threshold");
    
  /* set the range of the sliders */
  if (info->InputVolumeScalarType == VTK_FLOAT || 
      info->InputVolumeScalarType == VTK_DOUBLE) 
    { 
    /* for float and double use a step size of 1/200 th the range */
    stepSize = info->InputVolumeScalarRange[1]*0.005 - 
      info->InputVolumeScalarRange[0]*0.005; 
    }
  sprintf(tmp,"%g %g %g", 
          info->InputVolumeScalarRange[0],
          info->InputVolumeScalarRange[1],
          stepSize);
  info->SetGUIProperty(info, 0, VVP_GUI_HINTS , tmp);

  /* auto range for scalar type */
  vvPluginSetGUIScaleRange(2);

  info->OutputVolumeScalarType = info->InputVolumeScalarType;
  info->OutputVolumeNumberOfComponents = info->InputVolumeNumberOfComponents;
  for (int i = 0; i < 3; i++)
    {
    info->OutputVolumeDimensions[i] = info->InputVolumeDimensions[i];
    info->OutputVolumeSpacing[i] = info->InputVolumeSpacing[i];
    info->OutputVolumeOrigin[i] = info->InputVolumeOrigin[i];
    }

  return 1;
}

extern "C" 
{
void VV_PLUGIN_EXPORT vvVTKCardiacInit(vtkVVPluginInfo *info)
{
  /* always check the version */
  vvPluginVersionCheck();

  /* setup information that never changes */
  info->ProcessData = ProcessData;
  info->UpdateGUI = UpdateGUI;

  /* set the properties this plugin uses */
  info->SetProperty(info, VVP_NAME, "Cardiac Segmentation");
  info->SetProperty(info, VVP_GROUP, "Cardiac");
  info->SetProperty(info, VVP_TERSE_DOCUMENTATION, "Cardiac Segmentation");
  info->SetProperty(info, VVP_FULL_DOCUMENTATION, "Cardiac segmentation.");

  info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "1");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "1");
  info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "1");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
}
  
}




>>>>>>> 582480cd14e067db3b1fed95c429f1724f683c48
