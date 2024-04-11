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
#include <unordered_set>

using namespace std;

double epilson = 1e-2;
bool isFirst = false;	//AI是先手还是后手
int RIVAL, AI;

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


class DisjointSet {
public:
	DisjointSet() {}
	DisjointSet(const DisjointSet& set) {
		for (int i = 0; i < 121; i += 1) {
			father[i] = set.father[i];
		}
	}
	int father[121];	//并查集数组
	/*并查集，father中数字>=0，指代该节点的父节点；father中数字<0，说明该节点为根节点，|father|为该节点所有子节点个数（包括自己）*/
	void BuildFather(int n) {
		for (int i = 0; i < n; i++)
			father[i] = -1;
	}
	int Find(int x) {	//路径压缩
		if (father[x] < 0)
			return x;
		father[x] = Find(father[x]);
		return father[x];
	}
	void Union(int x, int y) {
		int a = Find(x), b = Find(y);
		if (a != b) {
			father[b] += father[a];
			father[a] = b;
		}
	}

	/*合并相连的棋子（并查集的合并）*/
	void UnionStones(int x, int y, int** board) {
		int I = toIndex(x, y);	//记录(x,y)对应的int值
		int state = board[x][y];	//记录(x,y)的状态，方便检测合并
		int xx = x - 1, yy = y;
		if (isValid(xx, yy) && board[xx][yy] == state)
			Union(toIndex(xx, yy), I);
		xx = x - 1; yy = y + 1;
		if (isValid(xx, yy) && board[xx][yy] == state)
			Union(toIndex(xx, yy), I);
		xx = x; yy = y + 1;
		if (isValid(xx, yy) && board[xx][yy] == state)
			Union(toIndex(xx, yy), I);
		xx = x + 1; yy = y;
		if (isValid(xx, yy) && board[xx][yy] == state)
			Union(toIndex(xx, yy), I);
		xx = x + 1; yy = y - 1;
		if (isValid(xx, yy) && board[xx][yy] == state)
			Union(toIndex(xx, yy), I);
		xx = x; yy = y - 1;
		if (isValid(xx, yy) && board[xx][yy] == state)
			Union(toIndex(xx, yy), I);
	}
};

class Cluster {
public:
	int color;
	unordered_set<pair<int, int>> adjacentEmptyCells;
	unordered_set<pair<int, int>> colorCells;
	Cluster(int color) {
		this->color = color;
	}
	Cluster(const Cluster& cluster) {
		this->color = cluster.color;
		this->adjacentEmptyCells = cluster.adjacentEmptyCells;
		this->colorCells = cluster.colorCells;
	}
};





/*在棋盘下满之后，判断谁赢，return true说明AI赢，false说明RIVAL赢
	用于随机模拟对局中判断AI输赢*/
bool checkWinner(DisjointSet* set) {
	vector<int> frontierRoots;
	if (isFirst == false) {
		for (int i = 0; i < 121; i += 11) {
			int root = set->Find(i);
			frontierRoots.push_back(root);
		}
		sort(frontierRoots.begin(), frontierRoots.end());
		for (int i = 10; i < 121; i += 11) {
			int root = set->Find(i);
			if (binary_search(frontierRoots.begin(), frontierRoots.end(), root)) {
				return true;
			}
		}
		return false;
	}
	else {
		for (int i = 0; i < 11; i += 1) {
			int root = set->Find(i);
			frontierRoots.push_back(root);
		}
		sort(frontierRoots.begin(), frontierRoots.end());
		for (int i = 110; i < 121; i += 1) {
			int root = set->Find(i);
			if (binary_search(frontierRoots.begin(), frontierRoots.end(), root)) {
				return true;
			}
		}
		return false;
	}


}
/*蒙特卡洛搜索树中的每个节点*/
struct MCSTNode {
	MCSTNode* parent = nullptr;	//父节点
	MCSTNode* next[121] = { nullptr };	//121个子节点，每个节点代表棋盘上一个点
	int N = 0;	//当前节点随即下了多少局棋了
	int win = 0;	//赢了多少局
	int move;		//当前节点是父节点得到哪一个子节点
	DisjointSet* set;		//当前节点的并查集
	int player = RED;	//当前节点是哪个玩家下的，初始化为RIVAL（根节点是对手下的最后一颗棋）
	int** clonedBoard;	//克隆的棋盘
	bool isLeaf = true;		//是否是叶节点
	int evalValue = 0;
	vector<pair<int, int>> emptyGrids;
	vector<Cluster> clusters;
	int** blueDistance;
	int** redDistance;
	int bluePotential;
	int redPotential;
	int blueMovability;
	int redMovability;

	~MCSTNode() {
		for (int i = 0; i < 121; i += 1) {
			delete next[i];
		}
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
						cluster.colorCells.emplace(x, y);
						int dx[6] = { 0, 1, 1, -1, 0, -1 };
						int dy[6] = { 1, 1, 0, 0, -1, -1 };
						for (int k = 0; k < 6; k++) {
							int xx = x + dx[k];
							int yy = y + dy[k];
							if (isValid(xx, yy) && board[xx][yy] == color && visited[xx][yy] == 0) {
								q.push(make_pair(xx, yy));
								visited[xx][yy] = 1;
							}
							else if (isValid(xx, yy) && board[xx][yy] == EMPTY && visited[xx][yy] == 0) {
								cluster.adjacentEmptyCells.emplace(xx, yy);
							}
						}
					}
					clusters.push_back(cluster);
				}
			}
		}
	}

	void CLA(int color) {
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
		for (auto& iterator : clusters[0].adjacentEmptyCells) {
			distanceToUpOrRight[iterator.first][iterator.second] = 1;
			q1.push(iterator);
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
					if (isValid(xx, yy) && clonedBoard[xx][yy] == EMPTY) {
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
					else if (isValid(xx, yy) && clonedBoard[xx][yy] == color) {
						for (auto& iterator : clusters) {
							if (iterator.colorCells.find({ xx,yy }) != iterator.colorCells.end()) {
								for (auto& iterator2 : iterator.adjacentEmptyCells) {
									if (clonedBoard[iterator2.first][iterator2.second] == EMPTY) {
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
								}
								break;
							}
						}
					}
				}
			}
			swap(q1, q2);
		}
		pair<int, int> grid;
		if (color == RED) {
			grid = make_pair(SIZE + 1, 1);
		}
		else {
			grid = make_pair(0, 1);
		}
		for (int i = 0; i < clusters.size(); i += 1) {
			if (clusters[i].adjacentEmptyCells.find(grid) != clusters[i].adjacentEmptyCells.end()) {
				for (auto& iterator : clusters[i].adjacentEmptyCells) {
					distanceToDownOrLeft[iterator.first][iterator.second] = 1;
					q1.push(iterator);
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
					if (isValid(xx, yy) && clonedBoard[xx][yy] == EMPTY) {
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
					else if (isValid(xx, yy) && clonedBoard[xx][yy] == color) {
						for (auto& iterator : clusters) {
							if (iterator.colorCells.find({xx,yy}) != iterator.colorCells.end()) {
								for (auto& iterator2 : iterator.adjacentEmptyCells) {
									if (clonedBoard[iterator2.first][iterator2.second] == EMPTY) {
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
		for (int i = 0; i < SIZE + 2; i += 1) {
			for (int j = 0; j < SIZE + 2; j += 1) {
				distance[i][j] = distanceToUpOrRight[i][j] + distanceToDownOrLeft[i][j];
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
	}

};
/*蒙特卡洛树搜索算法*/
class MCTS {
public:
	MCSTNode* root;	//这棵博弈树的根节点，即当前输入的棋盘状态

	/*构造MCTS，只会在HexGame.run中调用一次
		board是棋盘状态，set是此状态下的并查集，first为AI是否为先手*/
	MCTS(int** board, DisjointSet* set) {
		root = new MCSTNode();
		root->set = set;
		root->isLeaf = false;
		root->clonedBoard = new int* [SIZE + 2];
		root->player = isFirst ? RED : BLUE;
		for (int i = 0; i < SIZE + 2; i += 1) {
			root->clonedBoard[i] = new int[SIZE + 2];
		}
		for (int i = 0; i < SIZE + 2; i += 1) {
			for (int j = 0; j < SIZE + 2; j += 1) {
				root->clonedBoard[i][j] = board[i][j];
			}
		}
		for (int i = 1; i <= SIZE; i += 1) {
			for (int j = 1; j <= SIZE; j += 1) {
				if (root->clonedBoard[i][j] == 0) {
					root->emptyGrids.push_back(make_pair(i, j));
				}
			}
		}
		for (auto& grid : root->emptyGrids) {
			MCSTNode* child = new MCSTNode();
			child->parent = root;
			child->move = toIndex(grid.first, grid.second);
			child->player = -root->player;
			child->set = new DisjointSet(*root->set);
			child->set->UnionStones(grid.first, grid.second, root->clonedBoard);
			root->next[toIndex(grid.first, grid.second)] = child;
		}
	}

	/*选取进行模拟对局的节点*/
	MCSTNode* select(MCSTNode* node) {
		while (!node->isLeaf) {
			node = bestChild(node);
		}
		return node;
	}

	MCSTNode* bestChild(MCSTNode* node) {
		double max = -1;
		MCSTNode* best = nullptr;
		for (int i = 0; i < 121; i += 1) {
			if (node->next[i] != nullptr) {	//应当必不为空，在进行寻找bestchild时，节点应当121个子
				double UCB = node->next[i]->win / (node->next[i]->N + epilson) + sqrt(2 * log(root->N + epilson + 1) / (node->next[i]->N + epilson)) + (rand() % 100) / 100.0 * epilson; // Add a random number between 0 and 1
				if (UCB  + node->next[i]->evalValue > max) {
					max = UCB + node->next[i]->evalValue;
					best = node->next[i];
				}
			}
		}
		return best;
	}

	/*模拟，当前是纯随机对局，node是进行模拟对局的节点*/
	int simulation(MCSTNode* node) {
		/*设置node节点状态的棋盘 和 模拟用board*/
		int** simulation_board;
		simulation_board = new int* [SIZE + 2];
		for (int i = 0; i < SIZE + 2; i++) {
			simulation_board[i] = new int[SIZE + 2];
		}
		for (int i = 0; i < SIZE + 2; i++) {
			for (int j = 0; j < SIZE + 2; j++) {
				simulation_board[i][j] = node->parent->clonedBoard[i][j];
			}
		}
		int x, y;
		toPos(node->move, x, y);
		simulation_board[x][y] = node->parent->clonedBoard[x][y];
		simulation_board[x][y] = -node->parent->player;
		/*设置node节点状态的并查集 和 模拟用set*/
		DisjointSet* simulation_set;
		simulation_set = new DisjointSet(*node->set);

		/*纯随机对局，这里就是之后添加策略所要修改的地方*/

		for (int i = 1; i <= SIZE; i += 1) {
			for (int j = 1; j <= SIZE; j += 1) {
				if (simulation_board[i][j] == 0) {
					node->emptyGrids.push_back(make_pair(i, j));
				}
			}
		}
		//在上面构建该节点board过程中，该节点对应的那颗棋已经下完，也就是说下一颗棋（正式随即对局的第一颗棋）是-node->player下
		int who = -node->player;
		while (node->emptyGrids.size() > 0) {
			int index = rand() % node->emptyGrids.size();
			int x = node->emptyGrids[index].first;
			int y = node->emptyGrids[index].second;
			node->emptyGrids.erase(node->emptyGrids.begin() + index);
			simulation_board[x][y] = who;
			simulation_set->UnionStones(x, y, simulation_board);
			who = -who;
		}
		delete[] simulation_board;
		//判断AI输赢
		bool win = checkWinner(simulation_set);
		//返回结果
		if (win) {
			return 1;
		}
		else {
			return 0;
		}
		delete simulation_set;
	}

	/*反向传播*/
	void backpropagation(MCSTNode* node, int result) {
		while (node != nullptr) {
			node->N += 1;
			node->win += result;
			node = node->parent;
		}
	}

	/*对节点（以这一节点为起始模拟的对局一定已完成）扩展子节点*/
	MCSTNode* expand(MCSTNode* node) {
		node->clonedBoard = new int* [SIZE + 2];
		for (int i = 0; i < SIZE + 2; i++) {
			node->clonedBoard[i] = new int[SIZE + 2];
		}
		for (int i = 0; i < SIZE + 2; i++) {
			for (int j = 0; j < SIZE + 2; j++) {
				node->clonedBoard[i][j] = node->parent->clonedBoard[i][j];
			}
		}
		int x, y;
		toPos(node->move, x, y);
		node->clonedBoard[x][y] = -node->parent->player;
		node->player = -node->parent->player;
		node->isLeaf = false;
		for (int i = 1; i <= SIZE; i += 1) {
			for (int j = 1; j <= SIZE; j += 1) {
				if (node->clonedBoard[i][j] == 0) {
					node->emptyGrids.push_back(make_pair(i, j));
				}
			}
		}
		node->getClusters(node->clonedBoard, node->player);
		node->CLA(node->player);
		node->getClusters(node->clonedBoard, -node->player);
		node->CLA(-node->player);
		if (AI == BLUE) {
			node->evalValue = 100 * (node->redPotential - node->bluePotential) - (node->redMovability - node->blueMovability);
		}
		else if (AI == RED) {
			node->evalValue = 100 * (node->bluePotential - node->redPotential) - (node->blueMovability - node->redMovability);
		}
		for (auto& grid : node->emptyGrids) {
			MCSTNode* child = new MCSTNode();
			child->parent = node;
			child->move = toIndex(grid.first, grid.second);
			child->player = -node->player;
			child->set = new DisjointSet(*node->set);
			child->set->UnionStones(grid.first, grid.second, node->clonedBoard);
			node->next[toIndex(grid.first, grid.second)] = child;
		}
		return node->next[(node->emptyGrids[0].first - 1) * 11 + node->emptyGrids[0].second - 1];
	}

	/*蒙特卡洛自博弈开始，最终形成一棵可计算UCT的博弈树*/
	void run() {
		srand(time(0));
		int threshold = 0.95 * (double)CLOCKS_PER_SEC;
		int startTime, currentTime;
		startTime = currentTime = clock();
		while (currentTime - startTime < threshold) {
			MCSTNode* node = select(root);
			node = expand(node);
			int win = simulation(node);
			backpropagation(node, win);
			currentTime = clock();
		}
	}

	/*决策*/
	int getBestMove() {
		double max = -1;
		int best = -1;
		for (int i = 0; i < 121; i += 1) {
			if (root->next[i] != nullptr) {
				double UCB = root->next[i]->win / (root->next[i]->N + epilson); // Add a random number between 0 and 1
				if (UCB + root->next[i]->evalValue> max) {
					max = UCB + root->next[i]->evalValue;
					best = i;
				}
			}
		}
		return best;
	}
};



class HexGame {
public:
	DisjointSet* set;
	MCTS* mcts;

	/* board=1为红方/先手的棋，board=-1为蓝方/后手的棋 */
	int steps = 0;	//双方已下棋回合数
	int** board;	//棋盘状态，默认均为0（未落子）
public:
	bool BuildBoard() {
		/*初始化*/
		set = new DisjointSet();
		set->BuildFather(SIZE * SIZE);
		board = new int* [SIZE + 2];
		for (int i = 0; i < SIZE + 2; i++) {
			board[i] = new int[SIZE + 2];
		}
		for (int i = 1; i <= SIZE; i++)
			for (int j = 1; j <= SIZE; j++)
				board[i][j] = 0;
		for (int i = 0; i <= SIZE; i += 1) {
			board[0][i] = RED;
			board[SIZE + 1][i + 1] = RED;
			board[i + 1][0] = BLUE;
			board[i][SIZE + 1] = BLUE;
		}
		/*读入数据*/
		scanf("%d", &steps);
		int x, y;
		for (int i = 0; i < steps - 1; i++) {
			scanf("%d%d", &x, &y);   //对方落子
			if (i == 0 && x == -1) {
				//输入的第一棋是（-1，-1），说明AI己方是先手，不用管
				isFirst = true;
				AI = RED;
				RIVAL = BLUE;
			}
			else if (i == 0 && x != -1) {
				isFirst = false;
				RIVAL = RED;
				AI = BLUE;
			}
			else if (x != -1) {
				board[x + 1][y + 1] = RIVAL;
				set->UnionStones(x + 1, y + 1, board);
			}
			scanf("%d%d", &x, &y);  //这里输入的绝对不可能是（-1，-1）  //己方落子
			board[x + 1][y + 1] = AI;
			set->UnionStones(x + 1, y + 1, board);
		}
		scanf("%d%d", &x, &y);		//对方落子
		if (x == -1) {
			printf("1 2");
			return false;
		}
		else {
			board[x + 1][y + 1] = RIVAL;
			set->UnionStones(x + 1, y + 1, board);
			mcts = new MCTS(board, set);
			return true;
		}
	}

	void play() {
		bool flag = BuildBoard();
		if (flag == false) {
			return;
		}
		mcts->run();
		int best = mcts->getBestMove();
		int x, y;
		toPos(best, x, y);
		printf("%d %d", x, y);
	}
};



int main(void) {
	HexGame game;
	game.play();
}