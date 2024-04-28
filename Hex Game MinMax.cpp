#define _CRT_SECURE_NO_WARNINGS
#define SIZE 11
#define RED 1
#define EMPTY 0
#define BLUE -1
#define M 100
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
int threshold = 0.95 * (double)CLOCKS_PER_SEC;
int start_time, current_time;
int search_depth;
int queue_size;


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
	int player = AI;		//下一步该谁下了
	int value;
	int bluePotential = 100000;
	int redPotential = 100000;
	int blueMovability = 0;
	int redMovability = 0;
	int** clonedBoard;
	int** blueDistance;
	int** redDistance;
	int evalValue = -0x3f3f3f3f;		//评估值，同时应用于剪枝
	bool isLeaf = true;
	SearchTreeNode* parent = nullptr;
	vector<SearchTreeNode*> children;
	vector<int> bestChildren;	//字节点
	vector<Cluster> redClusters;
	vector<Cluster> blueClusters;
	int alpha = -0x3f3f3f3f;	//用于剪枝
	int beta = 0x3f3f3f3f;			//用于剪枝

	SearchTreeNode(int** board, int depth) {
		clonedBoard = board;
		this->depth = depth;
	}
	void getClusters(int** board) {
		int visited[SIZE + 2][SIZE + 2] = { 0 };
		for (int i = 0; i < SIZE + 2; i++) {
			for (int j = 0; j < SIZE + 2; j++) {
				if (board[i][j] == RED && visited[i][j] == 0) {
					Cluster cluster = Cluster(RED);
					//bfs
					queue<pair<int, int>> q;
					q.push(make_pair(i, j));
					visited[i][j] = 1;
					while (!q.empty()) {
						int x = q.front().first;
						int y = q.front().second;
						q.pop();
						cluster.colorCells->emplace(toIndex(x, y));
						int dx[6] = { -1, -1, 0, 1, 1, 0 };
						int dy[6] = { 0,   1, 1, 0,-1,-1 };
						for (int k = 0; k < 6; k++) {
							int xx = x + dx[k];
							int yy = y + dy[k];
							if (isValidInWholeBoard(xx, yy) && board[xx][yy] == RED && visited[xx][yy] == 0) {
								q.push(make_pair(xx, yy));
								visited[xx][yy] = 1;
							}
							else if (isValid(xx, yy) && board[xx][yy] == EMPTY && visited[xx][yy] == 0) {
								cluster.adjacentEmptyCells->emplace(toIndex(xx, yy));
							}
						}
					}
					redClusters.emplace_back(cluster);
				}
			}
		}
		memset(visited, 0, sizeof(visited));
		for (int i = 0; i < SIZE + 2; i++) {
			for (int j = 0; j < SIZE + 2; j++) {
				if (board[i][j] == BLUE && visited[i][j] == 0) {
					Cluster cluster = Cluster(BLUE);
					//bfs
					queue<pair<int, int>> q;
					q.push(make_pair(i, j));
					visited[i][j] = 1;
					while (!q.empty()) {
						int x = q.front().first;
						int y = q.front().second;
						q.pop();
						cluster.colorCells->emplace(toIndex(x, y));
						int dx[6] = { -1, -1, 0, 1, 1, 0 };
						int dy[6] = { 0,   1, 1, 0,-1,-1 };
						for (int k = 0; k < 6; k++) {
							int xx = x + dx[k];
							int yy = y + dy[k];
							if (isValidInWholeBoard(xx, yy) && board[xx][yy] == BLUE && visited[xx][yy] == 0) {
								q.push(make_pair(xx, yy));
								visited[xx][yy] = 1;
							}
							else if (isValid(xx, yy) && board[xx][yy] == EMPTY && visited[xx][yy] == 0) {
								cluster.adjacentEmptyCells->emplace(toIndex(xx, yy));
							}
						}
					}
					blueClusters.emplace_back(cluster);
				}
			}
		}
	}
	struct my_compare {
		bool operator()(const pair<int, int>& p1, const pair<int, int>& p2) {
			return p1.first < p2.first; // 这里使用 > 来实现降序排序，< 表示升序
		}
	};
	void CLA(int** board) {
		queue<pair<int, int>> q1;
		queue<pair<int, int>> q2;
		int lineNum = 1;
		int distanceToUpOrRight[SIZE + 2][SIZE + 2] = { 0 };
		int distanceToDownOrLeft[SIZE + 2][SIZE + 2] = { 0 };
		//std::fill(&distanceToUpOrRight[0][0], &distanceToUpOrRight[0][0] + (SIZE * 2) * (SIZE + 2), 100000);
		//std::fill(&distanceToDownOrLeft[0][0], &distanceToDownOrLeft[0][0] + (SIZE * 2) * (SIZE + 2), 100000);
		memset(distanceToUpOrRight, 0x3f3f3f3f, sizeof(distanceToUpOrRight));
		memset(distanceToDownOrLeft, 0x3f3f3f3f, sizeof(distanceToDownOrLeft));
		int color = RED;
		redDistance = new int* [SIZE + 2];
		int visited[SIZE + 2][SIZE + 2] = { 0 };
		for (int i = 0; i < SIZE + 2; i += 1) {
			redDistance[i] = new int[SIZE + 2];
		}
		blueDistance = new int* [SIZE + 2];
		for (int i = 0; i < SIZE + 2; i += 1) {
			blueDistance[i] = new int[SIZE + 2];
		}
		//RED，求红色双距离
		for (auto& iterator : *(redClusters[0].adjacentEmptyCells)) {
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
				int dx[6] = { -1, -1, 0, 1, 1, 0 };
				int dy[6] = { 0,   1, 1, 0,-1,-1 };
				for (int i = 0; i < 6; i += 1) {
					int xx = x + dx[i];
					int yy = y + dy[i];
					if (isValid(xx, yy) && board[xx][yy] == EMPTY) {
						if (visited[xx][yy] == 0) {
							visited[xx][yy] = 1;
						}
						else if (visited[xx][yy] == 1) {
							visited[xx][yy] = 2;
							distanceToUpOrRight[xx][yy] = lineNum;
							q2.push(make_pair(xx, yy));
						}
					}
					else if (visited[xx][yy] == 0 && isValid(xx, yy) && board[xx][yy] == color) {
						for (auto& iterator : redClusters) {
							if (iterator.colorCells->find(toIndex(xx, yy)) != iterator.colorCells->end()) {
								for (auto& iterator2 : *(iterator.adjacentEmptyCells)) {
									int x, y;
									toPos(iterator2, x, y);
									if (board[x][y] == EMPTY) {
										if (visited[x][y] == 0) {
											visited[x][y] = 1;
										}
										else if (visited[x][y] == 1) {
											visited[x][y] = 2;
											distanceToUpOrRight[x][y] = lineNum;
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
		int grid = toIndex(SIZE + 1, 1);
		for (int i = 0; i < redClusters.size(); i += 1) {
			if (redClusters[i].colorCells->find(grid) != redClusters[i].colorCells->end()) {
				for (auto& iterator : *(redClusters[i].adjacentEmptyCells)) {
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
				int dx[6] = { -1, -1, 0, 1, 1, 0 };
				int dy[6] = { 0,   1, 1, 0,-1,-1 };
				for (int i = 0; i < 6; i += 1) {
					int xx = x + dx[i];
					int yy = y + dy[i];
					if (isValid(xx, yy) && board[xx][yy] == EMPTY) {
						if (visited[xx][yy] == 0) {
							visited[xx][yy] = 1;
						}
						else if (visited[xx][yy] == 1) {
							visited[xx][yy] = 2;
							distanceToDownOrLeft[xx][yy] = lineNum;
							q2.push(make_pair(xx, yy));
						}
					}
					else if (visited[xx][yy] == 0 && isValid(xx, yy) && board[xx][yy] == color) {
						for (auto& iterator : redClusters) {
							if (iterator.colorCells->find(toIndex(xx, yy)) != iterator.colorCells->end()) {
								for (auto& iterator2 : *(iterator.adjacentEmptyCells)) {
									int x, y;
									toPos(iterator2, x, y);
									if (board[x][y] == EMPTY) {
										if (visited[x][y] == 0) {
											visited[x][y] = 1;
										}
										else if (visited[x][y] == 1) {
											visited[x][y] = 2;
											distanceToDownOrLeft[x][y] = lineNum;
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
		for (int i = 1; i <= SIZE; i += 1) {
			for (int j = 1; j <= SIZE; j += 1) {
				redDistance[i][j] = distanceToUpOrRight[i][j] + distanceToDownOrLeft[i][j];
			}
		}
		for (int i = 1; i <= SIZE; i += 1) {
			for (int j = 1; j <= SIZE; j += 1) {
				if (redDistance[i][j] == 0)
					continue;
				if (redDistance[i][j] < redPotential) {
					redPotential = redDistance[i][j];
					redMovability = 1;
				}
				else if (redDistance[i][j] == redPotential) {
					redMovability += 1;
				}
			}
		}
		//BLUE，求蓝色双距离
		lineNum = 1;
		/*std::fill(&distanceToDownOrLeft[0][0], &distanceToDownOrLeft[0][0] + (SIZE + 2) * (SIZE + 2), 100000);
		std::fill(&distanceToUpOrRight[0][0], &distanceToUpOrRight[0][0] + (SIZE + 2) * (SIZE + 2), 100000);*/
		memset(distanceToUpOrRight, 0x3f3f3f3f, sizeof(distanceToUpOrRight));
		memset(distanceToDownOrLeft, 0x3f3f3f3f, sizeof(distanceToDownOrLeft));
		memset(visited, 0, sizeof(visited));
		color = BLUE;
		for (auto& iterator : *(blueClusters[0].adjacentEmptyCells)) {
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
				int dx[6] = { -1, -1, 0, 1, 1, 0 };
				int dy[6] = { 0,   1, 1, 0,-1,-1 };
				for (int i = 0; i < 6; i += 1) {
					int xx = x + dx[i];
					int yy = y + dy[i];
					if (isValid(xx, yy) && board[xx][yy] == EMPTY) {
						if (visited[xx][yy] == 0) {
							visited[xx][yy] = 1;
						}
						else if (visited[xx][yy] == 1) {
							visited[xx][yy] = 2;
							distanceToUpOrRight[xx][yy] = lineNum;
							q2.push(make_pair(xx, yy));
						}
					}
					else if (visited[xx][yy] == 0 && isValid(xx, yy) && board[xx][yy] == color) {
						for (auto& iterator : blueClusters) {
							if (iterator.colorCells->find(toIndex(xx, yy)) != iterator.colorCells->end()) {
								for (auto& iterator2 : *(iterator.adjacentEmptyCells)) {
									int x, y;
									toPos(iterator2, x, y);
									if (board[x][y] == EMPTY) {
										if (visited[x][y] == 0) {
											visited[x][y] = 1;
										}
										else if (visited[x][y] == 1) {
											visited[x][y] = 2;
											distanceToUpOrRight[x][y] = lineNum;
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
		grid = toIndex(1, 0);
		for (int i = 0; i < blueClusters.size(); i += 1) {
			if (blueClusters[i].colorCells->find(grid) != blueClusters[i].colorCells->end()) {
				for (auto& iterator : *(blueClusters[i].adjacentEmptyCells)) {
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
				int dx[6] = { -1, -1, 0, 1, 1, 0 };
				int dy[6] = { 0,   1, 1, 0,-1,-1 };
				for (int i = 0; i < 6; i += 1) {
					int xx = x + dx[i];
					int yy = y + dy[i];
					if (isValid(xx, yy) && board[xx][yy] == EMPTY) {
						if (visited[xx][yy] == 0) {
							visited[xx][yy] = 1;
						}
						else if (visited[xx][yy] == 1) {
							visited[xx][yy] = 2;
							distanceToDownOrLeft[xx][yy] = lineNum;
							q2.push(make_pair(xx, yy));
						}
					}
					else if (visited[xx][yy] == 0 && isValid(xx, yy) && board[xx][yy] == color) {
						for (auto& iterator : blueClusters) {
							if (iterator.colorCells->find(toIndex(xx, yy)) != iterator.colorCells->end()) {
								for (auto& iterator2 : *(iterator.adjacentEmptyCells)) {
									int x, y;
									toPos(iterator2, x, y);
									if (board[x][y] == EMPTY) {
										if (visited[x][y] == 0) {
											visited[x][y] = 1;
										}
										else if (visited[x][y] == 1) {
											visited[x][y] = 2;
											distanceToDownOrLeft[x][y] = lineNum;
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
		for (int i = 1; i <= SIZE; i += 1) {
			for (int j = 1; j <= SIZE; j += 1) {
				blueDistance[i][j] = distanceToUpOrRight[i][j] + distanceToDownOrLeft[i][j];
			}
		}
		for (int i = 1; i <= SIZE; i += 1) {
			for (int j = 1; j <= SIZE; j += 1) {
				if (blueDistance[i][j] == 0)
					continue;
				if (blueDistance[i][j] < bluePotential) {
					bluePotential = blueDistance[i][j];
					blueMovability = 1;
				}
				else if (blueDistance[i][j] == bluePotential) {
					blueMovability += 1;
				}
			}
		}
		//merge
		int globalMin = 100;
		int movablity = 0;
		int distance[SIZE + 2][SIZE + 2] = { 0 };
		priority_queue<pair<int, int>, vector<pair<int, int>>, my_compare> q;
		for (int i = 1; i <= SIZE; i += 1) {
			for (int j = 1; j <= SIZE; j += 1) {
				if (board[i][j] == 0) {	//空格子才有可能被放入这个队列里面作为 将要进行扩展的子节点的候选人
					distance[i][j] = redDistance[i][j] + blueDistance[i][j];
					if (distance[i][j] != 0)
						q.push(make_pair(distance[i][j], toIndex(i, j)));
					if (q.size() > queue_size)		//每个节点扩展几个子节点
						q.pop();
				}
			}
		}
		/*确定这个节点状态的评估值*/
		if (AI == RED)
			evalValue = M * (bluePotential - redPotential) - (blueMovability - redMovability);
		else
			evalValue = M * (redPotential - bluePotential) - (redMovability - blueMovability);
		vector<int> res;
		while (q.empty() == false) {
			res.emplace_back(q.top().second);
			q.pop();
		}
		bestChildren = res;
	}


};

class SearchTree {
public:
	SearchTreeNode* root;
	SearchTree(int** board, vector<pair<int, int>> twoLastMoves) {
		root = new SearchTreeNode(board, 0);
		root->getClusters(board);
		root->CLA(board);
		root->isLeaf = false;
	}


	/*ab剪枝，以dfs为基础
	*	r是当前节点，layer是要搜索的层数，now_layer初始值为0*/
	int abCut(SearchTreeNode* r, int layer, int now_layer = 0) {		//layer：搜索层数
		if (now_layer == layer) {
			return r->evalValue;
		}
		if (r->parent != nullptr) {
			if (AI == RED) {
				if (r->parent->redDistance[r->x][r->y] == 2)		//若在CLA里面考虑了，可以换成r->bestchildren.size()==0
					return r->evalValue;
			}
			else if (AI == BLUE) {
				if (r->parent->blueDistance[r->x][r->y] == 2)
					return r->evalValue;
			}
		}
		current_time = clock();
		if (current_time - start_time > threshold) {
			return r->evalValue;
		}
		if (r->player == AI) {
			int max = -0x3f3f3f3f;
			for (auto iterator : r->bestChildren) {
				if (r->alpha > r->beta) {
					break;	//剪枝
				}
				/*扩展这个子节点*/
				int xx, yy;
				toPos(iterator, xx, yy);
				int** clonedBoardTemp = new int* [SIZE + 2];
				for (int i = 0; i < SIZE + 2; i++) {
					clonedBoardTemp[i] = new int[SIZE + 2];
					memcpy(clonedBoardTemp[i], r->clonedBoard[i], sizeof(int) * (SIZE + 2));
				}
				clonedBoardTemp[xx][yy] = r->player;
				SearchTreeNode* child = new SearchTreeNode(clonedBoardTemp, r->depth + 1);
				child->x = xx;
				child->y = yy;
				child->parent = r;
				child->player = -r->player;
				r->children.emplace_back(child);
				child->depth = r->depth + 1;		//这句话没用应该
				child->getClusters(clonedBoardTemp);
				child->CLA(clonedBoardTemp);
				child->alpha = r->alpha;
				child->beta = r->beta;
				int temp = abCut(child, layer, now_layer + 1);	//dfs子节点
				if (temp > max)
					max = temp;
				if (child->evalValue > r->alpha)
					r->alpha = child->evalValue;
			}
			r->evalValue = max;
		}
		else {		//r->player=RIVAL
			int min = 0x3f3f3f3f;
			for (auto iterator : r->bestChildren) {
				if (r->alpha > r->beta) {
					break;	//剪枝
				}
				/*扩展这个子节点*/
				int xx, yy;
				toPos(iterator, xx, yy);
				int** clonedBoardTemp = new int* [SIZE + 2];
				for (int i = 0; i < SIZE + 2; i++) {
					clonedBoardTemp[i] = new int[SIZE + 2];
					memcpy(clonedBoardTemp[i], r->clonedBoard[i], sizeof(int) * (SIZE + 2));
				}
				clonedBoardTemp[xx][yy] = r->player;
				SearchTreeNode* child = new SearchTreeNode(clonedBoardTemp, r->depth + 1);
				child->x = xx;
				child->y = yy;
				child->parent = r;
				child->player = -r->player;
				r->children.emplace_back(child);
				child->depth = r->depth + 1;		//这句话没用应该
				child->getClusters(clonedBoardTemp);
				child->CLA(clonedBoardTemp);
				child->alpha = r->alpha;
				child->beta = r->beta;
				int temp = abCut(child, layer, now_layer + 1);
				if (temp < min)
					min = temp;
				if (child->evalValue < r->beta)
					r->beta = child->evalValue;
			}
			r->evalValue = min;
		}
		return r->evalValue;
	}
};

class HexGame {
public:
	HexGame() {}
	int** board;
	int** blueDistance;
	int** redDistance;
	SearchTree* searchTree;
	vector<pair<int, int>> twoLastMoves;
	bool BuildBoard() {
		/*初始化棋盘*/
		board = new int* [SIZE + 2];
		for (int i = 0; i < SIZE + 2; i++) {
			board[i] = new int[SIZE + 2];
		}
		for (int i = 0; i <= SIZE + 1; i++)
			for (int j = 0; j <= SIZE + 1; j++)
				board[i][j] = EMPTY;
		for (int i = 0; i <= SIZE; i += 1) {
			board[0][i] = RED;
			board[SIZE + 1][i + 1] = RED;
			board[i + 1][0] = BLUE;
			board[i][SIZE + 1] = BLUE;
		}
		/*读入数据*/
		int steps;
		scanf("%d", &steps);
		int x, y;
		for (int i = 0; i < steps - 1; i++) {
			scanf("%d%d", &x, &y);   //对方落子
			if (i == 0 && x == -1) {
				//输入的第一棋是（-1，-1），说明AI己方是先手，不用管
				isFirst = true;
				search_depth = 4;
				queue_size = 6;
				AI = RED;
				RIVAL = BLUE;
			}
			else if (i == 0 && x != -1) {
				isFirst = false;
				search_depth = 3;
				queue_size = 20;
				RIVAL = RED;
				AI = BLUE;
				board[x + 1][y + 1] = RIVAL;
			}
			else if (x != -1) {
				board[x + 1][y + 1] = RIVAL;
			}
			scanf("%d%d", &x, &y);  //这里输入的绝对不可能是（-1，-1）  //己方落子
			board[x + 1][y + 1] = AI;
			if (i == steps - 2)
				twoLastMoves.emplace_back(make_pair(x + 1, y + 1));
		}
		scanf("%d%d", &x, &y);		//对方落子
		twoLastMoves.emplace_back(make_pair(x + 1, y + 1));
		if (x == -1) {
			printf("1 2");
			return false;
		}
		else if (steps == 1 && x == 1) {
			printf("7 3");
			return false;
		}
		else {
			board[x + 1][y + 1] = RIVAL;
			searchTree = new SearchTree(board, twoLastMoves);
			return true;
		}
	}

	void play() {
		bool flag = BuildBoard();
		if (flag == false)
			return;
		searchTree->abCut(searchTree->root, search_depth, 0);
		int max = -1 * (0x3f3f3f3f);
		int x, y;
		for (auto& iterator : searchTree->root->children) {
			if (iterator->evalValue > max) {
				max = iterator->evalValue;
				x = iterator->x;
				y = iterator->y;
			}
		}
		printf("%d %d", x - 1, y - 1);
	}
};

int main() {
	start_time = current_time = clock();
	HexGame hexGame;
	hexGame.play();
	return 0;
}