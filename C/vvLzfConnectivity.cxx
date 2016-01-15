#include "vtkVVPluginAPI.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
using namespace std;

const int neighborDirection[26][3] = {
	{1, 0, 0},{0, 1, 0},{0, 0, 1},{-1, 0, 0},{0, -1, 0},{0, 0, -1},
	{1, 1, 0},{1, -1, 0},{-1, 1, 0},{-1, -1, 0},
	{0, 1, 1},{0, 1, -1},{0, -1, 1},{0, -1, -1},
	{1, 0, 1},{1, 0, -1},{-1, 0, 1},{-1, 0, -1},
	{1, 1, 1},{1, 1, -1},{1, -1, 1},{1, -1, -1},{-1, 1, 1},{-1, 1, -1},{-1, -1, 1},{-1,-1,-1}
};
int ***vol, ***idx;
int Xd, Yd, Zd;
map<int, int> component;
vector< pair<int, int> > sortComp;

int cmp(const pair<int, int> &a, const pair<int, int> &b){
	return a.second > b.second;
}

void dfs(int s, int r, int c, int  id){
	if(s < 0 || s >= Zd || r < 0 || r >= Yd || c < 0 || c >= Xd) return;
	if(idx[s][r][c] > 0 || vol[s][r][c] <= 0) return;
	idx[s][r][c] = id;
	component[id]++;
	for(int h = 0; h < 26; h++)
		dfs(s + neighborDirection[h][0], r + neighborDirection[h][1], c + neighborDirection[h][2], id);
}

template <class IT>
/* TODO 1: Rename vvSampleTemplate to vv<your_plugin>Template */
void vvLzfConnectivityTemplate(vtkVVPluginInfo *info,
                      vtkVVProcessDataStruct *pds, 
                      IT *)
{
	IT *inPtr = (IT *)pds->inData;
	IT *outPtr = (IT *)pds->outData;
	int *dim = info->InputVolumeDimensions;
	int inNumComp = info->InputVolumeNumberOfComponents;
	int i, j, k, l;
	int abort;

  	Xd = (int)dim[0]; //*dim
	Yd = (int)dim[1]; //*(dim+1)
	Zd = (int)dim[2];

	//存储每个点的连通分量编号, 初始0
	idx = new int**[Zd];  
	for(i = 0; i < Zd; i++){
		idx[i] = new int*[Yd];
		for(j = 0; j < Yd; j++){
			idx[i][j] =  new int[Xd]();
		}
	}

	//体数据
	vol = new int**[Zd];  
	for(i = 0; i < Zd; i++){
		vol[i] = new int*[Yd];
		for(j = 0; j < Yd; j++){
			vol[i][j] =  new int[Xd]();
		}
	}

	//连通分量数量
	int cnt = 0;
  
	for ( k = 0; k < Zd; k++ ){                       
		info->UpdateProgress(info,(float)1.0*k/Zd,"Reading into memory..."); 
		abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
		for ( j = 0; !abort && j < Yd; j++ ){
			for ( i = 0; i < Xd; i++ ){
				vol[k][j][i] = *inPtr;
				inPtr++;
			}
		}
	}
	
	for ( k = 0; k < Zd; k++ ){                       
		info->UpdateProgress(info,(float)1.0*k/Zd,"Computing connected components..."); 
		abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
		for ( j = 0; !abort && j < Yd; j++ ){
			for ( i = 0; i < Xd; i++ ){
				if(vol[k][j][i] > 0 && idx[k][j][i] == 0){  //背景点
					dfs(k, j, i, ++cnt);
				}
			}
		}
	}

	for(map<int, int>::iterator it = component.begin(); it != component.end(); it++){
		sortComp.push_back(make_pair(it->first, it->second));
	}
	//按照连通分量里体素数量排序。
	sort(sortComp.begin(), sortComp.end(), cmp);

	ofstream outfile;
	outfile.open(".\\log.txt", ofstream::out);
	
	//保留前n个
	//vector<int> reservedIdx;
	for(int i = 0; i < 10; i++){
		//reservedIdx.push_back(sortComp[i].first);
		outfile << "Component#" << sortComp[i].first << ": " << sortComp[i].second << endl;
	}

	for ( k = 0; k < Zd; k++ ){                       
		info->UpdateProgress(info,(float)1.0*k/Zd,"Reserving the vessels..."); 
		abort = atoi(info->GetProperty(info,VVP_ABORT_PROCESSING));
		for ( j = 0; !abort && j < Yd; j++ ){
			for ( i = 0; i < Xd; i++ ){
				if(vol[k][j][i] > 0){
					int h;
					for(h = 0; h < 10; h++){
						if(idx[k][j][i] == sortComp[h].first) break;
					}
					outfile << k << " " << j << " " << i << " #" << idx[k][j][i] << " (" << h << ")" << endl;
					if(h >= 10) *outPtr = -100;  //不要保留的体素
				} 
				outPtr++;
			}
		}
	}
		outfile.close();

	info->UpdateProgress(info,(float)1.0,"Processing Complete");
}

static int ProcessData(void *inf, vtkVVProcessDataStruct *pds)
{
  vtkVVPluginInfo *info = (vtkVVPluginInfo *)inf;

  switch (info->InputVolumeScalarType)
    {
    /* TODO 2: Rename vvSampleTemplate to vv<your_plugin>Template */
    vtkTemplateMacro3(vvLzfConnectivityTemplate, info, pds, 
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
  void VV_PLUGIN_EXPORT vvLzfConnectivityInit(vtkVVPluginInfo *info)
  {
    /* always check the version */
    vvPluginVersionCheck();
    
    /* setup information that never changes */
    info->ProcessData = ProcessData;
    info->UpdateGUI = UpdateGUI;
    
    /* set the properties this plugin uses */
    /* TODO 4: Rename "Sample" to "<your_plugin>" */
    info->SetProperty(info, VVP_NAME, "LzfConnectivity");
    info->SetProperty(info, VVP_GROUP, "Utility");

    /* TODO 5: update the terse and full documentation for your filter */
    info->SetProperty(info, VVP_TERSE_DOCUMENTATION,
                      "Find all connected components.");
    info->SetProperty(info, VVP_FULL_DOCUMENTATION,
                      "This filter is used to eliminate noise in a volume containing segmented vessels. Noise is those blob- and line-like particle. We only keep the components with larger numbers of voxels.");

    
    /* TODO 9: set these two values to "0" or "1" based on how your plugin
     * handles data all possible combinations of 0 and 1 are valid. */
    info->SetProperty(info, VVP_SUPPORTS_IN_PLACE_PROCESSING, "1");
    info->SetProperty(info, VVP_SUPPORTS_PROCESSING_PIECES,   "0");

    /* TODO 7: set the number of GUI items used by this plugin */
    info->SetProperty(info, VVP_NUMBER_OF_GUI_ITEMS,          "0");
  info->SetProperty(info, VVP_REQUIRES_SERIES_INPUT,        "0");
  info->SetProperty(info, VVP_SUPPORTS_PROCESSING_SERIES_BY_VOLUMES, "0");
  info->SetProperty(info, VVP_PRODUCES_OUTPUT_SERIES, "0");
  info->SetProperty(info, VVP_PRODUCES_PLOTTING_OUTPUT, "0");
  }
}