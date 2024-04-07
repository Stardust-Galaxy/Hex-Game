#define _CRT_SECURE_NO_WARNINGS
#define SIZE 11
#define AI 1
#define RIVAL -1
#include <iostream>
#include <random>
#include <vector>
using namespace std;

double epilson = 1e-6;

/*蒙特卡洛搜索树*/
struct MCSTNode {
	MCSTNode* parent = nullptr;	//父节点
	MCSTNode* next[121] = { nullptr };	//121个子节点，每个节点代表棋盘上一个点
	int N = 0;	//当前节点随即下了多少局棋了
	int win = 0;	//赢了多少局
	int move;
	int** clonedBoard;	//克隆的棋盘
	bool isLeaf = true;
	bool is_leaf()  {
		for(int i = 0;i < 121;i += 1) {
			if(next[i] != nullptr) 
				isLeaf = false;
		}
	}
	~MCSTNode() {
		for(int i = 0;i < 121;i += 1) {
			delete next[i];
		}
	}
};
class MCTS {
public:
	MCSTNode* root;

	MCTS(int** board) {
		root = new MCSTNode();
		root->clonedBoard = new int*[11];
		for(int i = 0;i < 11;i += 1) {
			root->clonedBoard[i] = new int[11];
		}
		for(int i = 0;i < 11;i += 1) {
			for(int j = 0;j < 11;j += 1) {
				root->clonedBoard[i][j] = board[i][j];
			}
		}
	}

	MCSTNode* select(MCSTNode* node) {
		while(!node->isLeaf) {
			node = bestChild(node);
		}
		return node;
	}

	void expand(MCSTNode* node) {
		for(int i = 0;i < 121;i += 1) {
			if(node->next[i] == nullptr) {
				node->isLeaf = false;
				node->next[i] = new MCSTNode();
				node->next[i]->parent = node;
				node->next[i]->move = i;
				break;
			}
		}
	}

	MCSTNode* bestChild(MCSTNode* node) {
		double max = -1;
		MCSTNode* best = nullptr;
		for(int i = 0;i < 121;i += 1) {
			if(node->next[i] != nullptr) {
				double UCB = node->next[i]->win / (node->next[i]->N + epilson) + sqrt(2 * log(node->N) / (node->next[i]->N + epilson)) + (rand() % 100) / 100.0 * epilson; // Add a random number between 0 and 1
				if(UCB > max) {
					max = UCB;
					best = node->next[i];
				}
			}
		}
		return best;
	}
	/*反向传播*/
	void backpropagation(MCSTNode* node, int result) {
		while(node != nullptr) {
			node->N += 1;
			node->win += result;
			node = node->parent;
		}
	}
	/*模拟*/
	int simulation(MCSTNode* node) {
		node->clonedBoard = new int*[11];
		for(int i = 0;i < 11;i += 1) {
			node->clonedBoard[i] = new int[11];
		}
		for(int i = 0;i < 11;i += 1) {
			for(int j = 0;j < 11;j += 1) {
				node->clonedBoard[i][j] = root->clonedBoard[i][j];
			}
		}
		//随机下棋
		vector<pair<int,int>> emptyGrids;
		
		//判断是否结束
		//返回结果
	}

	void run(int iterations) {
		srand(time(0));
		while(iterations--) {
			MCSTNode* node = select(root);
			expand(node);
			int win = simulation(node);
			backpropagation(node, win);
		}
	}
};


class DisjointSet {
public:
	DisjointSet() {}
	int father[130];	//并查集数组
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
};

/*int值转变为有序对，一一对应的*/
/*
void toPos(int a, int& x, int& y) {
	if (a == 120) {
		x = 10; y = 10;
		return;
	}
	int units = a % 10;	//个位数
	int tens = a / 10;		//十位数（可能是两位）
	if (units >= tens) {
		y = units - tens;
		x = (a - y) / 11;
	}
	else {
		int b = tens - units - 1;
		x = units + b;
		y = a - 11 * x;
	}
}
*/










class HexGame {
public:
	DisjointSet set;
	/* board=1为红方/先手的棋，board=-1为蓝方/后手的棋 */
	int steps = 0;	//双方已下棋回合数
	int board[SIZE + 4][SIZE + 4];	//棋盘状态，默认均为0（未落子）
public:
	void BuildBoard() {
		set.BuildFather(SIZE * SIZE);
		scanf_s("%d", &steps);
		int x, y;
		for (int i = 0; i < steps - 1; i++) {
			scanf_s("%d%d", &x, &y);   //对方落子
			if (x == -1) {
				//输入的第一棋是（-1，-1），说明AI己方是先手，不用管
			}
			else if (x != -1) {
				board[x][y] = RIVAL;
				UnionStones(x, y);
			}
			scanf_s("%d%d", &x, &y);  //这里输入的绝对不可能是（-1，-1）  //己方落子
			board[x][y] = AI;
			UnionStones(x, y);
		}
		scanf_s("%d%d", &x, &y);		//对方落子
		board[x][y] = RIVAL;
		UnionStones(x, y);
	}
	
	
	/*判断当前随机棋局是否结束，fa是那个时候的并查集，(x,y)是新下的棋的坐标
	* who指示新下的棋是先手方（红）还是后手方（蓝），who=1是先手红，who=0是后手蓝 */
	bool isOver(int fa[], int x, int y, int who) {
		int I = toIndex(x, y);
		int root = set.Find(I);
		if (set.father[root] <= -11) {		//若新下的棋连成一片的超过了11颗
			bool frontier1 = false, frontier2 = false;
			for (int i = 0; i < 121; i++) {
				if (set.Find(i) == root || i == root) {
					int xx, yy;
					toPos(i, xx, yy);
					if (who == 1) {
						if (xx == 0)
							frontier1 = true;
						else if (xx == 10)
							frontier2 = true;
					}
					else {
						if (yy == 0)
							frontier1 = true;
						else if (yy == 10)
							frontier2 = true;
					}
					if (frontier1 && frontier2)
						return true;
				}
			}
		}
		return false;
	}

	/*检查当前坐标是否越界*/  //应该还要写个检查坐标是否合法的函数
	bool isValid(int x, int y) {
		if (x < 0 || x > 10 || y < 0 || y > 10)
			return false;
		return true;
	}
	/*合并相连的棋子（并查集的合并）*/
	void UnionStones(int x, int y) {
		int I = toIndex(x, y);	//记录(x,y)对应的int值
		int state = board[x][y];	//记录(x,y)的状态，方便检测合并
		int xx = x - 1, yy = y;
		if (isValid(xx, yy) && board[xx][yy] == state)
			set.Union(toIndex(xx, yy), I);
		xx = x - 1; yy = y + 1;
		if (isValid(xx, yy) && board[xx][yy] == state)
			set.Union(toIndex(xx, yy), I);
		xx = x; yy = y + 1;
		if (isValid(xx, yy) && board[xx][yy] == state)
			set.Union(toIndex(xx, yy), I);
		xx = x + 1; yy = y;
		if (isValid(xx, yy) && board[xx][yy] == state)
			set.Union(toIndex(xx, yy), I);
		xx = x + 1; yy = y - 1;
		if (isValid(xx, yy) && board[xx][yy] == state)
			set.Union(toIndex(xx, yy), I);
		xx = x; yy = y - 1;
		if (isValid(xx, yy) && board[xx][yy] == state)
			set.Union(toIndex(xx, yy), I);
	}

	void toPos(int a, int& x, int& y) {
		x = a / 11;
		y = a % 11;
	}
	/*有序对转变为int，一一对应的*/
	int toIndex(int x, int y) {
		return x * 11 + y;
	}
};