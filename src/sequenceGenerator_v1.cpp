//oggpnson 
//hkhr 

#include<iostream>
#include<list>
#include<vector>
#include<map>
#include<stack>
#include<algorithm>
#include "toolpathGenerator.h"
#include <fstream>

using namespace std; 

#define ll long long int

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

	int machinedVolume = 0, reachableVolume =0; 

	for(it = sequence.begin(); it != sequence.end(); it++){
			//printing the model for debugging purpose 
			
			//converting the model to heightmap in a given orientation 
			int toBeMachinedVolume = model.calculateMachinableVolume(*it);
			int machinedVolumeInOrientation =0;
			if(toBeMachinedVolume != 0){
				if(printVolume){
					ofstream myfile;
			 		myfile.open ("./" + folderName + "/" + *it + ".interim.raw", ios::binary );
			 		model.print(myfile);
			 		// const char *voxelsStream = voxels.c_str();
			 		// myfile.write(voxelsStream, lMax*hMax*bMax);
				}
				
				Matrix heightmap = model.toHeightmap(*it);
				machiningSequence.push_back(*it);

				// int i, j;
				// int N=heightmap.size(), M = heightmap[0].size();
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
					toolpath = toToolpath(model, *it, graph, regionmap, safeHeight, hMax, TOOL_DIA, depthPerPass, feedrate);
				}
				else if(*it == "yz+" || *it == "yz-"){
					int safeHeight = lMax +2;
					toolpath = toToolpath(model, *it, graph, regionmap, safeHeight, lMax, TOOL_DIA, depthPerPass, feedrate);	
				}
				else if(*it == "xz+" || *it == "xz-"){
					int safeHeight = bMax +2;
					toolpath = toToolpath(model, *it, graph, regionmap, safeHeight, bMax, TOOL_DIA, depthPerPass, feedrate);		
				}	

				toolpaths.push_back(toolpath);

				//filling machined volume so that it would be outta consideration in next orientation
				machinedVolumeInOrientation = model.fillMachinableVolume(*it, heightmap);
				machinedVolume += machinedVolumeInOrientation;
				reachableVolume += toBeMachinedVolume;
				//cout<<*it<<":"<<toBeMachinedVolume<<", "<<machinedVolumeInOrientation<<"\n";
			}
		
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
	if(machinedVolume != ((xmax+1)*(ymax+1)*(zmax+1) - objectsVolume)){
		cout<<"xmax:"<<xmax<<"\n";
		cout<<"ymax:"<<ymax<<"\n";
		cout<<"zmax:"<<zmax<<"\n";
		cout<<"Volume to be machined:"<<(xmax+1)*(ymax+1)*(zmax+1) - objectsVolume<<"\n";
		cout<<"Machined Volume: "<< machinedVolume<<"\n";
		cout<<"Reachable Volume: "<< reachableVolume<<"\n";
		cout<<"Sorry , the object cannot be machined\n";
		exit(-1);
	}

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

int main(int argc, char *argv[]){

	//length, breadth, height of the model along with the corresponding iterator 
	int lMax, bMax, hMax, lIter, bIter, hIter;
	cin>>lMax>>bMax>>hMax; 
	string folderName = static_cast<string>(argv[1]);


	//data model for storing voxels 
	vector<vector<vector<int> > > voxels(lMax, vector<vector<int> >(bMax, vector<int>(hMax, false)));
	char boolVal; 
	int TOOL_DIA = 1;
	int feedrate = 500;
	int depthPerPass=1;
	bool printVolume = true;
	int toolRadius = 10;
	int toolLength = 10;
	long long int objectsVolume=0;

	//taking voxel input  
	for(hIter = 0; hIter<hMax; hIter++){
		for(lIter = 0; lIter<lMax; lIter++){
			for(bIter = 0; bIter<bMax; bIter++){
				cin>>boolVal;
				if(boolVal == '1'){
					voxels[lIter][bIter][hIter] = true;
					objectsVolume += 1;
				}
				else{
					voxels[lIter][bIter][hIter] = false;
				}
			}	
		}
	}
	//cout<<voxels[25][6][13]<<" *"<<voxels[23][6][13]<<"Yaha dekho!!\n";
	//internal representation of the model 
	VolumetricModel model(voxels, lMax, bMax, hMax);

	//generating sequence using genetic algorithm 
	list<string> sequence = generateSequence(model);
	list<string>::iterator it;

	
	//generating toolpath for the optimal sequence thats produced by sequence generator 
	pair<list<string>, list<string> > skk = toolpathGeneratorForSequence(sequence, model, TOOL_DIA,lMax,bMax, hMax,folderName, printVolume, feedrate, depthPerPass, toolRadius, toolLength, objectsVolume);
	list<string> sk = skk.second;
	sequence = skk.first;

	list<string>::iterator itt;
	itt = sequence.begin();

	//writing sequence to the test file 
	writeSequenceToFile(folderName, sequence);

	//saving toolpath in folder "OrientationOutput"
 	for(it = sk.begin(); it != sk.end(); it++){
 		ofstream myfile;
 		myfile.open ("./" + folderName + "/" + *itt + ".gcode" );
 		myfile<<*it;
 		itt++;
 		myfile.close();
 	}

	// // list<string> sk;
	// // sk.push_back("xz+");
	// // cout<< toolpathGeneratorForSequence(sk, model, TOOL_DIA, lMax, bMax, hMax);


	return 0;
}
