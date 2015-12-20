//oggpnson 
//hkhr 

#include<iostream>
#include<stdint.h>
#include<list>
#include<vector>
#include<map>
#include<stack>
#include<algorithm>
#include <fstream>

//#define ll long long int

using namespace std; 

typedef map<int, pair<int, list<int> > > Graph;
typedef vector< vector<int> > Matrix;
typedef stack<pair<int, int > > Stack;
typedef list<pair<int, int> > Lop;
typedef list<int> AdjList;
typedef map<int , bool> BoolDict;
typedef stack<int> IntStack;
typedef map<int, int> IntMap;

//state variable that can be configured to experiment 
string ORIENTATION[] = {"xy+", "xy-", "yz+", "yz-", "xz+", "xz-"};
int NOOFORIENTATION = 6;
int MAXIMUM_SIZE_OF_GENERATION = 10;
double P = .5;
int N = 6;


int calculateMaxZ(vector<vector<int> > heightmap){
		int i, j, m= heightmap.size();
		int globalMaximum=0, localMaximum=0;		
		for(i=0; i<m; i++){
			localMaximum = *max_element(heightmap[i].begin(), heightmap[i].end());
			if(localMaximum > globalMaximum){
				globalMaximum = localMaximum;
			}  
		}
		return globalMaximum;
}


//compares volume of pair, used for sorting 
bool compareVolume(pair<int, string> v1, pair<int, string> v2){
	return v1.first > v2.first;
}



//genetic algorithm implementation for toolpath sequence generation 
list<string> generateSequence(VolumetricModel &model){
	/*
		input: model of the object to be made 
		purpose: to generate the optimal sequence of operation that can carve the given model using 3 axis cnc 
		output: optimal sequence of orientation that can build model using a 3-axis cnc machine 
	*/

	vector<pair<int, string> > volumes(NOOFORIENTATION);
	int i;
	


	//generating intital generation of chromosome with six possible degrees of rotation 
	for(i=0; i<NOOFORIENTATION; i++){
		volumes[i] = make_pair(model.calculateMachinableVolume(ORIENTATION[i]), ORIENTATION[i]);
	}

	sort(volumes.begin(), volumes.end(), compareVolume );

	list<string> optimalSet;

	for(i=0; i<NOOFORIENTATION; i++){
		optimalSet.push_back(volumes[i].second);
	}

	return optimalSet;

}

//generate toolpath for sequence 
pair<list<string>, list<string> > toolpathGeneratorForSequence(list<string> sequence, VolumetricModel &model, int TOOL_DIA, int lMax, int bMax, int hMax, string folderName, bool printVolume, int feedrate, int depthPerPass, int toolRadius, int toolLength, long long int objectsVolume){
	
	int xmax = model.xmax, ymax = model.ymax, zmax = model.zmax;

	//iterator to access sequence of orientation 
	list<string>::iterator it;

	list<string> machiningSequence;

	list<string> toolpaths;

	int machinedVolume=0; 
	
		
		
	for(it = sequence.begin(); it != sequence.end(); it++){
		//printing the model for debugging purpose 
		//converting the model to heightmap in a given orientation 
		// cout<<*it<<"\n";
		int toBeMachinedVolume = model.calculateMachinableVolume(*it);
		int machinedVolumeInOrientation =0;
		if(toBeMachinedVolume != 0){
			Matrix heightmap = model.toHeightmap(*it);
			// cout<<*it<<"\n";
			int i, j;
			int N=heightmap.size(), M = heightmap[0].size();
			// for(i=0; i<N; i++){
			// 	for(j=0; j<M; j++){
			// 		cout<<heightmap[i][j]<<" ";
			// 	}
			// 	cout<<"\n";
			// }
			// cout<<"\n";
			//converting heightmap to graph and regionmap for toolpath generation
			pair<Graph, Matrix> graphRegionmap = toGraph(heightmap, toolRadius, toolLength);
			Graph graph = graphRegionmap.first;
			Matrix regionmap = graphRegionmap.second;

			// for(i=0; i<N; i++){
			// 	cout<<"m";
			// 	for(j=0; j<M; j++){
			// 		cout<<heightmap[i][j]<<" ";
			// 	}
			// 	cout<<"\n";
			// }
			// cout<<"\n";

			//calculating toolpath for the given model and orientation
			
			string toolpath;// = toToolpath(model, *it, graph, regionmap, safeHeight, maxHeight, TOOL_DIA);
			if(*it == "xy+" || *it == "xy-"){
				int safeHeight = hMax +2;
				toolpath = toToolpath(model, *it, graph, regionmap, safeHeight, hMax, heightmap, TOOL_DIA, depthPerPass, feedrate);
			}
			else if(*it == "yz+" || *it == "yz-"){
				int safeHeight = lMax +2;
				toolpath = toToolpath(model, *it, graph, regionmap, safeHeight, lMax, heightmap, TOOL_DIA, depthPerPass, feedrate);	
			}
			else if(*it == "xz+" || *it == "xz-"){
				int safeHeight = bMax +2;
				toolpath = toToolpath(model, *it, graph, regionmap, safeHeight, bMax, heightmap, TOOL_DIA, depthPerPass, feedrate);		
			}	
			toolpaths.push_back(toolpath);

			//filling machined volume so that it would be outta consideration in next orientation
			machinedVolumeInOrientation = model.fillMachinableVolume(*it, heightmap);
			if(machinedVolumeInOrientation){
				if(printVolume){
					ofstream myfile;
			 		myfile.open ("./" + folderName + "/" + *it + ".interim.raw", ios::binary );
			 		model.print(myfile);
			 		// const char *voxelsStream = voxels.c_str();
			 		// myfile.write(voxelsStream, lMax*hMax*bMax);
				}
				machiningSequence.push_back(*it);
			}

			machinedVolume += machinedVolumeInOrientation;
			// cout<<*it<<":"<<toBeMachinedVolume<<", "<<machinedVolumeInOrientation<<"\n";
		}		
	}
	if(machinedVolume != ((xmax+1)*(ymax+1)*(zmax+1) - objectsVolume)){
	cout<<"xmax:"<<xmax<<"\n";
	cout<<"ymax:"<<ymax<<"\n";
	cout<<"zmax:"<<zmax<<"\n";
	cout<<"Volume to be machined:"<<(xmax+1)*(ymax+1)*(zmax+1) - objectsVolume<<"\n";
	cout<<"Machined Volume: "<< machinedVolume<<"\n";
	cout<<"Sorry , the object cannot be machined\n";
	//exit(-1);
	}

	
	// int x,y,z;
	// for(x=0; x<=xmax; x++){
	// 	for(y=0; y<=ymax; y++){
	// 		for(z=0; z<=zmax; z++){
	// 			if(model.space[x][y][z] == false)
	// 				cout<<x<<", "<<y<<", "<<z<<"\n";
	// 		}
	// 	}
	// }
	

	return make_pair(machiningSequence, toolpaths);
}




void writeSequenceToFile(string folderName, list<string> sequence){
	list<string>::iterator it;

	ofstream myfile;

	myfile.open("./" + folderName + "/sequence.txt");

	for(it = sequence.begin(); it != sequence.end(); it++){
		myfile << (*it + "->");
	}

	myfile.close();
}

