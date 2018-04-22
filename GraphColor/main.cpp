#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<sstream>
#include<utility>
#include<tuple>
#include<ctime>
using namespace std;
const int max_iter_time = 10000000;
const int bigNum = 10000000;
using step = tuple<int, int, int, int>;//0 is vertex 1 is old color 2 is new color 3 is delta

pair<int,int> getData(std::fstream& file,vector<vector<int>>&);
bool isTabu(int vertex, int newColor, int** tabuList, int iterTimes);
void iniSolution(vector<int>&colorSolution, int nColor);
int iniRivalList(vector<vector<int>>&rivalList,const vector<vector<int>>&graph,const vector<int> & colorSolution);
void showConflict(const vector<vector<int>>& graph, const vector<vector<int>>& rivalList, const vector<int>& colorSolution, int target);
pair<bool,long> tabuSearch(const vector<vector<int>>&graph, vector<int>&colorSolution, vector<vector<int>>& rivalList, int ** tabuList,int fs);
int calDelta(const vector<vector<int>>& rivalList, int vertex, int oldColor, int newColor);
step findMove(const vector<vector<int>>&graph,vector<int>&colorSolution, vector<vector<int>>&rivalList,int** tabuList, int iterTimes,int fs,int bestfs);
bool updateRivalList(const vector<vector<int>> & graph,vector<vector<int>>& rivalList,step move);
bool makeMove(const vector<vector<int>>& graph,vector<int>&colorSolution,vector<vector<int>>&rivalList,int ** tabuList,int iterTimes,step move,int & fs);

int main()
{
	vector<string> testFile{
		"DSJC125.1.col",
		"DSJC250.1.col",
		"DSJC250.5.col",
		"DSJC250.9.col",
		"DSJC500.1.col",
		"DSJC500.5.col",
		"DSJC500.9.col",
		"DSJC1000.1.col",
		"DSJC1000.5.col",
		"DSJC1000.9.col"
	};
	vector<int> optimalSolution{5,8,28,72,12,47,126,20,82,222};
	string logFileName = "output.csv";
	fstream logFile;
	logFile.open(logFileName, ios::out);
	logFile << "Instance,Algorithm,RandSeed,Duration,IterCount,Optima,Solution" << endl;
	const auto seed = time(NULL);
	srand(seed);
	for (decltype(testFile.size())i = 0;i < 7;++i) {
		string filename = testFile[i];
		fstream file;
		file.open(filename, ios::in);
		if (!file.is_open()) { cerr << "can't open file" << filename << endl; return -1; }
		logFile << filename<<","<<"TabuSearch"<<","<<seed<<",";
		vector<vector<int>>graph;
		//设置随机数种子，用于概率计算确保随机性
		//读取数据
		auto VNE = getData(file, graph);
		file.close();
		int nColor = optimalSolution[i] + 5;

		//解空间
		vector<int> colorSolution(VNE.first, 0);
		//仇恨表
		vector<int> bestSolution(colorSolution.begin(), colorSolution.end());
		vector<vector<int>> rivalList(VNE.first);
		int**tabuList = new int*[VNE.first];
		for (int i = 0; i < VNE.first; ++i) {
			tabuList[i] = new int[nColor];
			for (int j = 0; j < nColor; ++j) {
				tabuList[i][j] = 0;
			}
		}

		while (true)
		{
			//仇恨表伸缩大小为nColor。
			for (auto & x : rivalList) {
				x.resize(nColor);
			}
			//初始化解空间
			iniSolution(colorSolution, nColor);
			//仇恨表初始化,计算仇恨值
			int fs = iniRivalList(rivalList, graph, colorSolution);
			int best = fs;
			//构造禁忌表
			//之前都为初始化数据结构与一些必要的信息
			auto beginTime = clock();
			auto result = tabuSearch(graph, colorSolution, rivalList, tabuList, fs);
			auto endTime = clock();
			for (int i = 0; i < VNE.first; ++i) {
				for (int j = 0; j < nColor; ++j) {
					tabuList[i][j] = 0;
				}
			}
			if (true != result.first) {
				cout << nColor << "\t is false" << endl; 
				logFile << endTime - beginTime << ","<<result.second<<","<<"("<<nColor + 1<<"),";
				for (const auto & x : bestSolution) {
					logFile << x << " ";
				}
				logFile << endl;
				break;
			}
			else if (nColor == optimalSolution[i]) {
				logFile << endTime - beginTime << "," << result.second << "," << "(" << nColor<< ")"<<",";
				for (const auto & x : colorSolution) {
					logFile << x << " ";
				}
				logFile << endl;
				break;
			}
			else {
				cout << nColor << "is OK" << endl;
				bestSolution.assign(colorSolution.begin(), colorSolution.end());
				--nColor;
			}
		}
		for (int i = 0; i < VNE.first; ++i) {
			delete[] tabuList[i];
		}
		delete[]tabuList;
	}
	logFile.close();
	system("pause");
	return 0;
}

//从文件中获取图的基本内容并且返回节点数与边数。
pair<int,int> getData(std::fstream & file, vector<vector<int>>& graph)
{
	string linetext;
	int nvertex = 0;
	int nedge = 0;
	int begin;
	int end;
	string flag;
	while (file.good()) {
		getline(file, linetext);
		stringstream  textstream(linetext);
		textstream >> flag;
		if (flag == "c") continue;
		else if (flag == "p") { 
			textstream >> flag >> nvertex >> nedge; 
			graph.resize(nvertex);
		}
		else {
			textstream >> begin >> end;
			graph[begin -1].push_back(end - 1);
			graph[end - 1].push_back(begin - 1);
		}
	}
	return make_pair(nvertex, nedge);
}

//依据nColor的大小初始化解。
void iniSolution(vector<int>& colorSolution, int nColor)
{
	auto size = colorSolution.size();
	for (decltype(size) i = 0; i < size; ++i) {
		colorSolution[i] = (rand())%nColor;
	}
}

//将之前的仇恨表清空，并且根据colorsolution的解决方案计算当前仇恨值
  int iniRivalList(vector<vector<int>>& rivalList, const vector<vector<int>>& graph, const vector<int>& colorSolution)
{
	for (decltype(rivalList.size()) i = 0; i < rivalList.size(); ++i) {
		for (auto & x : rivalList[i]) {
			x = 0;
		}
	}
	int fs = 0;
	for (decltype(graph.size()) i = 0; i < graph.size(); ++i) {
		for (const auto & y : graph[i]) {
			++rivalList[i][colorSolution[y]];
			if (colorSolution[i] == colorSolution[y]) {
				++fs;
			}
		}
	}
	
	return fs;
}


pair<bool,long> tabuSearch(const vector<vector<int>>& graph, vector<int>& colorSolution, vector<vector<int>>& rivalList, int ** tabuList,int fs)
{
	int bestfs = fs;
	int iter = 0;
	if (tabuList == NULL) {
		cout << "here" << endl;
	}
	while (iter < max_iter_time&& fs != 0) {
		auto move = findMove(graph, colorSolution, rivalList, tabuList,iter,fs,bestfs);

		makeMove(graph,colorSolution, rivalList, tabuList,iter, move,fs);
		if (bestfs > fs) {
			bestfs = fs;
		}
		++iter;
		if (fs < 0) {
			throw _SYSTEM_ERROR_;
		}
	}
	if (fs == 0) {
	
		return pair<bool,long>(true,iter);
	}
	return pair<bool,long>(false,iter);
}

int calDelta(const vector<vector<int>>& rivalList, int vertex, int oldColor, int newColor)
{
	return 2 * (rivalList[vertex][newColor] - rivalList[vertex][oldColor]);
}

step findMove(const vector<vector<int>>& graph, vector<int>& colorSolution, vector<vector<int>>& rivalList, int ** tabuList, int iterTimes,int fs, int bestfs)
{
	//I need to add variables as  possibilities to fulfil the ability of eaqual chose.
	int tabubest = bigNum;
	int normbest = bigNum;
	int tabuEqu = 1;
	int normEqu = 1;
	int tabuVertex = -1;
	int tabuNewColor = -1;
	int normVertex = -1;
	int normNewColor = -1;
	auto nColor = rivalList[0].size();
	for (decltype(graph.size())i = 0; i < graph.size(); ++i) {
		//examine whether the current vertex need to change color;
		if (rivalList[i][colorSolution[i]] != 0) {
			for (decltype(nColor)j = 0; j < nColor; ++j) {
				//if change doesn't make sense jump.
				if (j == colorSolution[i]) continue;
				//calculate the delta value to each colorchange 
				int currDelta = calDelta(rivalList, i, colorSolution[i], j);
				//according to the tabuList to divide those results to normalsolution and tabu solution then compare them with the best to update the best solution and construct them
				//remeber we are supposed to make each value-equal solution the equal possbility to choose  as the final solution 
				if (iterTimes < tabuList[i][j]) {
					//tabu case
					if (currDelta > tabubest) continue;
					else if (currDelta < tabubest) {
						//possbility = 1 then change to 1/2
						//solution = 
						tabuVertex = i;
						tabuNewColor = j;
						tabubest = currDelta;
						tabuEqu = 1;
					}
					else {
						if (rand()%tabuEqu == 0) {
							tabuVertex = i;
							tabuNewColor = j;
							tabubest = currDelta;
						}
						++tabuEqu;
					}
				}
				else {
					if (currDelta > normbest) continue;
					else if (currDelta < normbest) {
						//solution =
						normVertex = i;
						normNewColor = j;
						normbest = currDelta;
						normEqu = 1;
					}
					else {
						if (0 == rand()%normEqu) {
							//solution = 
							normVertex = i;
							normNewColor = j;
							normbest = currDelta;
						}
						++normEqu;
					}

					//normal case
				}

			}
		}
	}
	//examine whether the situation fulfill the aspiration type, if so then apply the tabu best solution otherwise choose the normal solution.
	int vertex = -1;
	int newColor = -1;
	int bestDelta = bigNum;
	//解禁策略
	if (tabubest < normbest&& tabubest + fs < bestfs) {
		vertex = tabuVertex;
		newColor = tabuNewColor;
		bestDelta = tabubest;
	}
	else {
		vertex = normVertex;
		newColor = normNewColor;
		bestDelta = normbest;
	}
	int conflictNum = 0;
	for (decltype(rivalList.size())i = 0; i < rivalList.size(); ++i) {
		if (rivalList[i][colorSolution[i]] != 0) ++conflictNum;
	}
	if (iterTimes > max_iter_time / 2 && iterTimes < max_iter_time/2 + 100) {
		if (isTabu(vertex, newColor, tabuList, iterTimes)){
			cout << "this is tabu";
			cout << "now iter is" << iterTimes << "\t Limit is" << tabuList[vertex][newColor]<<'\t';
			cout << "delta is" << bestDelta <<"\t conflict number is "<<conflictNum<< endl;
		}
		else {
			cout << "this is not tabu";
			cout << "now iter is" << iterTimes << "\t Limit is" << tabuList[vertex][newColor] << '\t';
			cout << "delta is" << bestDelta << "\t conflict number is " << conflictNum << endl;
		}
	}
	if (vertex < 0) {
		cout << iterTimes << endl;
		cout << colorSolution[vertex]<<endl;
		cout << bestDelta << endl;
	}
	return step(vertex,colorSolution[vertex],newColor,bestDelta);
}

bool updateRivalList(const vector<vector<int>>& graph, vector<vector<int>>& rivalList, step move)
{
	for (const auto& x : graph[get<0>(move)]) {
		--rivalList[x][get<1>(move)];
		++rivalList[x][get<2>(move)];
	}
	return true;
}

bool makeMove(const vector<vector<int>>& graph,vector<int>& colorSolution, vector<vector<int>>& rivalList, int ** tabuList,int iterTimes, step move, int & fs)
{
	colorSolution[get<0>(move)] = get<2>(move);
	updateRivalList(graph, rivalList, move);
	fs += get<3>(move);
	tabuList[get<0>(move)][get<1>(move)] = iterTimes + fs/2  + rand() % 10;
	return 0;
}

void showConflict(const vector<vector<int>>& graph, const vector<vector<int>>& rivalList,const vector<int>& colorSolution,int target)
{
	cout << target << "color is " << colorSolution[target] << endl;
	cout << "have rivals:";
	for (const auto & x : graph[target]) {
		cout << x << "color is" << colorSolution[x] << endl;
	}
	cout << endl << endl << endl;
}

bool isTabu(int vertex, int newColor, int** tabuList, int iterTimes)
{
	if (iterTimes >= tabuList[vertex][newColor]) return false;
	else return true;
}