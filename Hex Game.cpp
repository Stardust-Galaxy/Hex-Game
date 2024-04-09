#define _CRT_SECURE_NO_WARNINGS
#define SIZE 11
#define AI 1
#define RIVAL -1
#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include <ctime>
using namespace std;

double epilson = 1e-2;
bool isFirst = false;	//AI是先手还是后手

/*index值转有序对*/
void toPos(int a, int& x, int& y) {
	x = a / 11;
	y = a % 11;
}
/*有序对转变为int，一一对应的*/
int toIndex(int x, int y) {
	return x * 11 + y;
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

	/*检查当前坐标是否越界*/  //应该还要写个检查坐标是否合法的函数
	bool isValid(int x, int y) {
		if (x < 0 || x > 10 || y < 0 || y > 10)
			return false;
		return true;
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
	int player = RIVAL;	//当前节点是哪个玩家下的，初始化为RIVAL（根节点是对手下的最后一颗棋）
	int** clonedBoard;	//克隆的棋盘
	bool isLeaf = true;		//是否是叶节点

	bool is_leaf() {
		for (int i = 0; i < 121; i += 1) {
			if (next[i] != nullptr)
				isLeaf = false;
		}
	}

	~MCSTNode() {
		for (int i = 0; i < 121; i += 1) {
			delete next[i];
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
		root->clonedBoard = new int* [SIZE];
		for (int i = 0; i < SIZE; i += 1) {
			root->clonedBoard[i] = new int[SIZE];
		}
		for (int i = 0; i < SIZE; i += 1) {
			for (int j = 0; j < SIZE; j += 1) {
				root->clonedBoard[i][j] = board[i][j];
			}
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
				double UCB = node->next[i]->win / (node->next[i]->N + epilson) + sqrt(2 * log(root->N + epilson) / (node->next[i]->N + epilson)) + (rand() % 100) / 100.0 * epilson; // Add a random number between 0 and 1
				if (UCB > max) {
					max = UCB;
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
		node->clonedBoard = new int* [SIZE];
		simulation_board = new int* [SIZE];
		for (int i = 0; i < SIZE; i++) {
			node->clonedBoard[i] = new int[SIZE];
			simulation_board[i] = new int[SIZE];
		}
		for (int i = 0; i < SIZE; i++) {
			for (int j = 0; j < SIZE; j++) {
				node->clonedBoard[i][j] = node->parent->clonedBoard[i][j];
				simulation_board[i][j] = node->clonedBoard[i][j];
			}
		}
		int x, y;
		toPos(node->move, x, y);
		node->clonedBoard[x][y] = -node->parent->player;		//= node->player
		simulation_board[x][y] = node->clonedBoard[x][y];
		/*设置node节点状态的并查集 和 模拟用set*/
		DisjointSet* simulation_set;
		node->set = new DisjointSet(*node->parent->set);
		node->set->UnionStones(x, y, node->clonedBoard);
		simulation_set = new DisjointSet(*node->set);

		/*纯随机对局，这里就是之后添加策略所要修改的地方*/
		vector<pair<int, int>> emptyGrids;
		for (int i = 0; i < 11; i += 1) {
			for (int j = 0; j < 11; j += 1) {
				if (simulation_board[i][j] == 0) {
					emptyGrids.push_back(make_pair(i, j));
				}
			}
		}
		int who = -node->player;	//在上面构建该节点board过程中，该节点对应的那颗棋已经下完，也就是说下一颗棋（正式随即对局的第一颗棋）是-node->player下
		while (emptyGrids.size() > 0) {
			int index = rand() % emptyGrids.size();
			int x = emptyGrids[index].first;
			int y = emptyGrids[index].second;
			emptyGrids.erase(emptyGrids.begin() + index);
			simulation_board[x][y] = who;
			simulation_set->UnionStones(x, y, simulation_board);
			who = -who;
		}
		//判断AI输赢
		bool win = checkWinner(simulation_set);
		//返回结果
		if (win) {
			return 1;
		}
		else {
			return 0;
		}
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
	void expand(MCSTNode* node) {
		node->isLeaf = false;
		for (int i = 0; i < 121; i++) {
			if (node->next[i] == nullptr) {		//应该是不需要
				node->next[i] = new MCSTNode();
				node->next[i]->parent = node;
				node->next[i]->move = i;
				node->next[i]->player = -(node->player);
			}
		}
	}

	/*蒙特卡洛自博弈开始，最终形成一棵可计算UCT的博弈树*/
	void run(int iterations) {
		srand(time(0));
		while (iterations--) {
			MCSTNode* node = select(root);
			int win = simulation(node);
			backpropagation(node, win);
			expand(node);
		}
	}

	/*决策*/
	int getBestMove() {
		double max = -1;
		int best = -1;
		for (int i = 0; i < 121; i += 1) {
			if (root->next[i] != nullptr) {
				double UCB = root->next[i]->win / (root->next[i]->N + epilson) + sqrt(2 * log(root->N + epilson) / (root->next[i]->N + epilson)) + (rand() % 100) / 100.0 * epilson; // Add a random number between 0 and 1
				if (UCB > max) {
					max = UCB;
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
	int **board;	//棋盘状态，默认均为0（未落子）
public:
	void BuildBoard() {
		/*初始化*/
		set = new DisjointSet();
		set->BuildFather(SIZE * SIZE);
		board = new int* [SIZE];
		for (int i = 0; i < SIZE; i++) {
			board[i] = new int[SIZE];
		}
		for (int i = 0; i < SIZE; i++)
			for (int j = 0; j < SIZE; j++)
				board[i][j] = 0;
		/*读入数据*/
		scanf("%d", &steps);
		int x, y;
		for (int i = 0; i < steps - 1; i++) {
			scanf("%d%d", &x, &y);   //对方落子
			if (i == 0 && x == -1) {
				//输入的第一棋是（-1，-1），说明AI己方是先手，不用管
				isFirst = true;
			}
			else if (x != -1) {
				board[x][y] = RIVAL;
				set->UnionStones(x, y, board);
			}
			scanf("%d%d", &x, &y);  //这里输入的绝对不可能是（-1，-1）  //己方落子
			board[x][y] = AI;
			set->UnionStones(x, y, board);
		}
		scanf("%d%d", &x, &y);		//对方落子
		board[x][y] = RIVAL;
		set->UnionStones(x, y, board);
		mcts = new MCTS(board, set);
	}

	void play() {
		BuildBoard();
		mcts->run(1000);
		int best = mcts->getBestMove();
		int x, y;
		toPos(best, x, y);
		printf("%d %d\n", x, y);
	}
};



int main(void) {
	HexGame game;
	game.play();
}