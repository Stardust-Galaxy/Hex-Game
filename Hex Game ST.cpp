#define _CRT_SECURE_NO_WARNINGS
#define SIZE 11
#define RED 1
#define EMPTY 0
#define BLUE -1
#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include <ctime>
#include <queue>
#include <cstring>
#include <unordered_set>


using namespace std;

double epilson = 1e-2;
bool isFirst = false;	//AI是先手还是后手
int RIVAL = BLUE, AI = RED;

/*index值转有序对*/
void toPos(int a, int& x, int& y) {
	x = a / 11 + 1;
	y = a % 11 + 1;
}
/*有序对转变为int，一一对应的*/
int toIndex(int x, int y) {
	return (x - 1) * 11 + y - 1;
}

/*检查当前坐标是否越界*/
bool isValid(int x, int y) {
	if (x < 1 || x > 11 || y < 1 || y > 11)
		return false;
	return true;
}

bool isValidInWholeBoard(int x, int y) {
	if (x < 0 || x >= SIZE + 2 || y < 0 || y >= SIZE + 2)
		return false;
	return true;
}

class Cluster {
public:
	int color;
	unordered_set<int>* adjacentEmptyCells;
	unordered_set<int>* colorCells;
	Cluster(int color) {
		this->color = color;
		adjacentEmptyCells = new unordered_set<int>();
		colorCells = new unordered_set<int>();
	}
};

class SearchTreeNode {
public:
    int x, y;
    int depth;
    int player = RIVAL;
    int value;
    int bluePotential;
    int redPotential;
    int blueMovability;
    int redMovability;
    int** clonedBoard;
    int** blueDistance;
    int** redDistance;
    SearchTreeNode* parent;
    vector<SearchTreeNode*> children;
    vector<Cluster> clusters;
    vector<pair<int,int>> emptyGrids;
    SearchTreeNode(int** board,int depth) {
        clonedBoard = board;
        this->depth = depth;
    }
    void getClusters(int** board, int color) {
		clusters.clear();
		int visited[SIZE + 2][SIZE + 2] = { 0 };
		for (int i = 0; i < SIZE + 2; i++) {
			for (int j = 0; j < SIZE + 2; j++) {
				if (board[i][j] == color && visited[i][j] == 0) {
					Cluster cluster = Cluster(color);
					//bfs
					queue<pair<int, int>> q;
					q.push(make_pair(i, j));
					visited[i][j] = 1;
					while (!q.empty()) {
						int x = q.front().first;
						int y = q.front().second;
						q.pop();
						cluster.colorCells->emplace(toIndex(x, y));
						int dx[6] = { 0, 1, 1, -1, 0, -1 };
						int dy[6] = { 1, 1, 0, 0, -1, -1 };
						for (int k = 0; k < 6; k++) {
							int xx = x + dx[k];
							int yy = y + dy[k];
							if (isValidInWholeBoard(xx, yy) && board[xx][yy] == color && visited[xx][yy] == 0) {
								q.push(make_pair(xx, yy));
								visited[xx][yy] = 1;
							}
							else if (isValid(xx, yy) && board[xx][yy] == EMPTY && visited[xx][yy] == 0) {
								cluster.adjacentEmptyCells->emplace(toIndex(xx, yy));
							}
						}
					}
					clusters.emplace_back(cluster);
				}
			}
		}
	}
	struct my_compare {
  		bool operator()(const pair<int, int>& p1, const pair<int, int>& p2) {
    	return p1.first < p2.first; // 这里使用 > 来实现降序排序，< 表示升序
 		}
	};
	vector<int> CLA(int** board,int color) {
		queue<pair<int, int>> q1;
		queue<pair<int, int>> q2;
		int lineNum = 1;
		int distanceToUpOrRight[SIZE + 2][SIZE + 2] = { 0 };
		int distanceToDownOrLeft[SIZE + 2][SIZE + 2] = { 0 };
		int** distance = new int* [SIZE + 2];
		int visited[SIZE + 2][SIZE + 2] = { 0 };
		for (int i = 0; i < SIZE + 2; i += 1) {
			distance[i] = new int[SIZE + 2];
		}

		for (auto& iterator : *(clusters[0].adjacentEmptyCells)) {
			int x, y;
			toPos(iterator, x, y);
			distanceToUpOrRight[x][y] = 1;
			visited[x][y] = 2;
			q1.push({ x,y });
		}
		while (!q1.empty()) {
			lineNum += 1;
			while (q1.empty() == false) {
				int x = q1.front().first;
				int y = q1.front().second;
				q1.pop();
				int dx[6] = { 0, 1, 1, -1, 0, -1 };
				int dy[6] = { 1, 1, 0, 0, -1, -1 };
				for (int i = 0; i < 6; i += 1) {
					int xx = x + dx[i];
					int yy = y + dy[i];
					if (isValid(xx, yy) && board[xx][yy] == EMPTY) {
						if (visited[xx][yy] == 0) {
							visited[xx][yy] = 1;
							distanceToUpOrRight[xx][yy] = lineNum;
						}
						else if (visited[xx][yy] == 1) {
							visited[xx][yy] = 2;
							distanceToUpOrRight[xx][yy] = lineNum > distanceToUpOrRight[xx][yy] ? lineNum : distanceToUpOrRight[xx][yy];
							q2.push(make_pair(xx, yy));
						}
					}
					else if (isValid(xx, yy) && board[xx][yy] == color) {
						for (auto& iterator : clusters) {
							if (iterator.colorCells->find(toIndex(xx, yy)) != iterator.colorCells->end()) {
								for (auto& iterator2 : *(iterator.adjacentEmptyCells)) {
									int x, y;
									toPos(iterator2, x, y);
									if (board[x][y] == EMPTY) {
										if (visited[x][y] == 0) {
											visited[x][y] = 1;
											distanceToUpOrRight[x][y] = lineNum;
										}
										else if (visited[x][y] == 1) {
											visited[x][y] = 2;
											distanceToUpOrRight[x][y] = lineNum > distanceToUpOrRight[x][y] ? lineNum : distanceToUpOrRight[x][y];
											q2.push(make_pair(x, y));
										}
									}
								}
								break;
							}
						}
					}
				}
			}
			swap(q1, q2);
		}
		memset(visited, 0, sizeof(visited));
		int grid = color == RED ? toIndex(SIZE + 1,1) : toIndex(1, 0);
		for (int i = 0; i < clusters.size(); i += 1) {
			if (clusters[i].colorCells->find(grid) != clusters[i].colorCells->end()) {
				for (auto& iterator : *(clusters[i].adjacentEmptyCells)) {
					int x, y;
					toPos(iterator, x, y);
					distanceToDownOrLeft[x][y] = 1;
					visited[x][y] = 2;
					q1.push({ x,y });
				}
				break;
			}
		}
		lineNum = 1;
		while (!q1.empty()) {
			lineNum += 1;
			while (q1.empty() == false) {
				int x = q1.front().first;
				int y = q1.front().second;
				q1.pop();
				int dx[6] = { 0, 1, 1, -1, 0, -1 };
				int dy[6] = { 1, 1, 0, 0, -1, -1 };
				for (int i = 0; i < 6; i += 1) {
					int xx = x + dx[i];
					int yy = y + dy[i];
					if (isValid(xx, yy) && board[xx][yy] == EMPTY) {
						if (visited[xx][yy] == 0) {
							visited[xx][yy] = 1;
							distanceToDownOrLeft[xx][yy] = lineNum;
						}
						else if (visited[xx][yy] == 1) {
							visited[xx][yy] = 2;
							distanceToDownOrLeft[xx][yy] = lineNum > distanceToDownOrLeft[xx][yy] ? lineNum : distanceToDownOrLeft[xx][yy];
							q2.push(make_pair(xx, yy));
						}
					}
					else if (isValid(xx, yy) && board[xx][yy] == color) {
						for (auto& iterator : clusters) {
							if (iterator.colorCells->find(toIndex(xx, yy)) != iterator.colorCells->end()) {
								for (auto& iterator2 : *(iterator.adjacentEmptyCells)) {
									int x, y;
									toPos(iterator2, x, y);
									if (board[x][y] == EMPTY) {
										if (visited[x][y] == 0) {
											visited[x][y] = 1;
											distanceToDownOrLeft[x][y] = lineNum;
										}
										else if (visited[x][y] == 1) {
											visited[x][y] = 2;
											distanceToDownOrLeft[x][y] = lineNum > distanceToDownOrLeft[x][y] ? lineNum : distanceToDownOrLeft[x][y];
											q2.push(make_pair(x, y));
										}
									}
								}
								break;
							}
						}
					}
				}
			}
			swap(q1, q2);
		}
		int globalMin = 100;
		int movablity = 0;
		priority_queue<pair<int,int>,vector<pair<int,int>>,my_compare> q;
		
		for (int i = 1; i <= SIZE; i += 1) {
			for (int j = 1; j <= SIZE; j += 1) {
				distance[i][j] = distanceToUpOrRight[i][j] + distanceToDownOrLeft[i][j];
				q.push(make_pair(distance[i][j],toIndex(i,j)));
				if(q.size() > 5)
					q.pop();
				if (distance[i][j] == 0)
					continue;
				if (globalMin == distance[i][j]) {
					movablity += 1;
				}
				else if (globalMin > distance[i][j]) {
					globalMin = distance[i][j];
					movablity = 1;
				}
			}
		}
		if (color == RED) {
			redDistance = distance;
			redPotential = globalMin;
			redMovability = movablity;
		}
		else {
			blueDistance = distance;
			bluePotential = globalMin;
			blueMovability = movablity;
		}
		vector<int> res;
		while(q.empty() == false){
			res.emplace_back(q.top().second);
			q.pop();
		}
		return res;
	}

    void getEmptyGrids() {
        emptyGrids.clear();
        for (int i = 1; i <= SIZE; i++) {
            for (int j = 1; j <= SIZE; j++) {
                if (clonedBoard[i][j] == EMPTY) {
                    emptyGrids.emplace_back(make_pair(i, j));
                }
            }
        }
    }

};

class SearchTree {
public:
    SearchTreeNode* root;
    SearchTree(int** board) {
        root = new SearchTreeNode(board,0);
        root->getClusters(board,AI);
		vector<int> result = root->CLA(board,AI);
        for(auto& iterator : result){
			int x,y;
			toPos(iterator,x,y);
			
		}
    }
    
};
class HexGame { 

};