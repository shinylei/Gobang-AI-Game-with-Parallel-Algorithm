#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iomanip>
#include <mpi.h>
#include <math.h>
//#include <ext/hash_set>
#include <iterator>
#include <vector>

using namespace std;
//using __gnu_cxx::hash_map;

/*
实现用的参数
*/
class CONFIG {
public:
    static const int BOARD_SIZE = 15;
    static const int EMPTY = 0;
    static const int USER_1 = 1;
    static const int USER_2 = 2;
    static const int AI_EMPTY = 0; // 无子
    static const int AI_MY = 1; // 待评价子
    static const int AI_OP = 2; // 对方子或不能下子
    static const int MAX_NODE = 2;
    static const int MIN_NODE = 1;
    static const int INF = 106666666;
    static const int ERROR_INDEX = -1;
    //估价值
    static const int AI_ZERO = 0;
    static const int AI_ONE = 10;
    static const int AI_ONE_S = 1;
    static const int AI_TWO = 100;
    static const int AI_TWO_S = 10;
    static const int AI_THREE = 1000;
    static const int AI_THREE_S = 100;
    static const int AI_FOUR = 10000;
    static const int AI_FOUR_S = 1000;
    static const int AI_FIVE = 100000;
};

/*
棋盘格子
*/
class Grid : CONFIG {
public:
    int type; //类型

    Grid() {
        type = EMPTY;
    }

    Grid(int t) {
        type = t;
    }

    void grid(int t = EMPTY) {
        type = t;
    }

    int isEmpty() {
        return type == EMPTY ? true : false;
    }
};

/*
棋盘
*/
class ChessBoard :CONFIG {
public:
    Grid chessBoard[BOARD_SIZE][BOARD_SIZE];

    ChessBoard() {
        for (int i = 0; i < BOARD_SIZE; ++i)
            for (int j = 0; j < BOARD_SIZE; ++j)
                chessBoard[i][j].grid();
    }

    ChessBoard(const ChessBoard &othr) {
        for (int i = 0; i < BOARD_SIZE; ++i)
            for (int j = 0; j < BOARD_SIZE; ++j)
                chessBoard[i][j].grid(othr.chessBoard[i][j].type);
    }

    /*
    放置棋子
    返回放置棋子是否成功
    */
    bool placePiece(int x, int y, int type) {
        if (chessBoard[x][y].isEmpty()) {
            chessBoard[x][y].type = type;
            return true;
        }
        return false;
    }

	int gridType(int x, int y) {
	    return chessBoard[x][y].type;
	}
};

/*
煞笔AI
*/
class Game :CONFIG {
public:
    ChessBoard curState; // 当前棋盘
    bool isStart; // 是否进行中
    int curUser; // 当前行棋人
    int MAX_DEPTH; // 最大搜索层数

                   /*
                   开始并设定难度
                   */
    void startGame(int nd = 2) {
        MAX_DEPTH = nd;
        isStart = true;
        curUser = USER_1;
    }

	void show() {
		cout << setw(2) << '-'<< ' ';
		for (int i = 0; i < 15; i++) {
		    cout << setw(2) << i <<' ';
		}
		cout << '\n';
	    for (int i = 0; i < BOARD_SIZE; i++) {
			cout << setw(2) << i << ' ';
		    for (int j = 0; j < BOARD_SIZE; j++) {
			    cout << setw(2) << curState.gridType(i, j) << ' ';
			}
			cout<< '\n';
		}
	}

    /*
    转换行棋人
    */
    void changeUser() {
        curUser = curUser == USER_1 ? USER_2 : USER_1;
    }

    /*
    根据给定type
    A:待判断棋子的类型
    type:我方棋子的类型
    返回A是待判断棋子 无棋子 对方棋子
    */
    int getPieceType(int A, int type) {
        return A == type ? AI_MY : (A == EMPTY ? AI_EMPTY : AI_OP);
    }

    int getPieceType(const ChessBoard &board, int x, int y, int type) {
        if (x < 0 || y < 0 || x >= BOARD_SIZE || y >= BOARD_SIZE)// 超出边界按对方棋子算
            return AI_OP;
        else
            return getPieceType(board.chessBoard[x][y].type, type);
    }

    /*
    当前行棋人放置棋子
    放置失败返回失败
    放置成功
    检察游戏是否结束
    转换游戏角色后返回成功
    */
    bool placePiece(int x, int y) {
        if (curState.placePiece(x, y, curUser)) {
            // 检察行棋人是否胜利
            if (isWin(x, y)) {
                isStart = false; // 游戏结束
                                 //  return true;
            }
            changeUser(); // 转换游戏角色
            return true;
        }
        return false;
    }

    bool isWin(int x, int y) {
        if (evaluatePiece(curState, x, y, curUser) >= AI_FIVE)
            return true;
        return false;
    }

    /*
    以center作为评估位置进行评价一个方向的棋子
    */
    int evaluateLine(int line[], bool ALL) {
        int value = 0; // 估值
        int cnt = 0; // 连子数
        int blk = 0; // 封闭数
        for (int i = 0; i < BOARD_SIZE; ++i) {
            if (line[i] == AI_MY) { // 找到第一个己方的棋子
                                    // 还原计数
                cnt = 1;
                blk = 0;
                // 看左侧是否封闭
                if (line[i - 1] == AI_OP)
                    ++blk;
                // 计算连子数
                for (i = i + 1; i < BOARD_SIZE && line[i] == AI_MY; ++i, ++cnt);
                // 看右侧是否封闭
                if (line[i] == AI_OP)
                    ++blk;
                // 计算评估值
                value += getValue(cnt, blk);
            }
        }
        return value;
    }

    /*
    以center作为评估位置进行评价一个方向的棋子（前后4格范围内）
    */
    int evaluateLine(int line[]) {
        int cnt = 1; // 连子数
        int blk = 0; // 封闭数
                     // 向左右扫
        for (int i = 3; i >= 0; --i) {
            if (line[i] == AI_MY) ++cnt;
            else if (line[i] == AI_OP) {
                ++blk;
                break;
            }
            else
                break;
        }
        for (int i = 5; i < 9; ++i) {
            if (line[i] == AI_MY) ++cnt;
            else if (line[i] == AI_OP) {
                ++blk;
                break;
            }
            else
                break;
        }
        return getValue(cnt, blk);
    }

    /*
    根据连字数和封堵数给出一个评价值
    */
    int getValue(int cnt, int blk) {
        if (blk == 0) {// 活棋
            switch (cnt) {
            case 1:
                return AI_ONE;
            case 2:
                return AI_TWO;
            case 3:
                return AI_THREE;
            case 4:
                return AI_FOUR;
            default:
                return AI_FIVE;
            }
        }
        else if (blk == 1) {// 单向封死
            switch (cnt) {
            case 1:
                return AI_ONE_S;
            case 2:
                return AI_TWO_S;
            case 3:
                return AI_THREE_S;
            case 4:
                return AI_FOUR_S;
            default:
                return AI_FIVE;
            }
        }
        else {// 双向堵死
            if (cnt >= 5)
                return AI_FIVE;
            else
                return AI_ZERO;
        }
    }

    /*
    对一个状态的一个位置放置一种类型的棋子的优劣进行估价
    */
    int evaluatePiece(ChessBoard state, int x, int y, int type) {
        int value = 0; // 估价值
        int line[17];  //线状态
        bool flagX[8];// 横向边界标志
        flagX[0] = x - 4 < 0;
        flagX[1] = x - 3 < 0;
        flagX[2] = x - 2 < 0;
        flagX[3] = x - 1 < 0;
        flagX[4] = x + 1 > 14;
        flagX[5] = x + 2 > 14;
        flagX[6] = x + 3 > 14;
        flagX[7] = x + 4 > 14;
        bool flagY[8];// 纵向边界标志
        flagY[0] = y - 4 < 0;
        flagY[1] = y - 3 < 0;
        flagY[2] = y - 2 < 0;
        flagY[3] = y - 1 < 0;
        flagY[4] = y + 1 > 14;
        flagY[5] = y + 2 > 14;
        flagY[6] = y + 3 > 14;
        flagY[7] = y + 4 > 14;

        line[4] = AI_MY; // 中心棋子
                         // 横
        line[0] = flagX[0] ? AI_OP : (getPieceType(state.chessBoard[x - 4][y].type, type));
        line[1] = flagX[1] ? AI_OP : (getPieceType(state.chessBoard[x - 3][y].type, type));
        line[2] = flagX[2] ? AI_OP : (getPieceType(state.chessBoard[x - 2][y].type, type));
        line[3] = flagX[3] ? AI_OP : (getPieceType(state.chessBoard[x - 1][y].type, type));

        line[5] = flagX[4] ? AI_OP : (getPieceType(state.chessBoard[x + 1][y].type, type));
        line[6] = flagX[5] ? AI_OP : (getPieceType(state.chessBoard[x + 2][y].type, type));
        line[7] = flagX[6] ? AI_OP : (getPieceType(state.chessBoard[x + 3][y].type, type));
        line[8] = flagX[7] ? AI_OP : (getPieceType(state.chessBoard[x + 4][y].type, type));

        value += evaluateLine(line);

        // 纵
        line[0] = flagY[0] ? AI_OP : getPieceType(state.chessBoard[x][y - 4].type, type);
        line[1] = flagY[1] ? AI_OP : getPieceType(state.chessBoard[x][y - 3].type, type);
        line[2] = flagY[2] ? AI_OP : getPieceType(state.chessBoard[x][y - 2].type, type);
        line[3] = flagY[3] ? AI_OP : getPieceType(state.chessBoard[x][y - 1].type, type);

        line[5] = flagY[4] ? AI_OP : getPieceType(state.chessBoard[x][y + 1].type, type);
        line[6] = flagY[5] ? AI_OP : getPieceType(state.chessBoard[x][y + 2].type, type);
        line[7] = flagY[6] ? AI_OP : getPieceType(state.chessBoard[x][y + 3].type, type);
        line[8] = flagY[7] ? AI_OP : getPieceType(state.chessBoard[x][y + 4].type, type);

        value += evaluateLine(line);

        // 左上-右下
        line[0] = flagX[0] || flagY[0] ? AI_OP : getPieceType(state.chessBoard[x - 4][y - 4].type, type);
        line[1] = flagX[1] || flagY[1] ? AI_OP : getPieceType(state.chessBoard[x - 3][y - 3].type, type);
        line[2] = flagX[2] || flagY[2] ? AI_OP : getPieceType(state.chessBoard[x - 2][y - 2].type, type);
        line[3] = flagX[3] || flagY[3] ? AI_OP : getPieceType(state.chessBoard[x - 1][y - 1].type, type);

        line[5] = flagX[4] || flagY[4] ? AI_OP : getPieceType(state.chessBoard[x + 1][y + 1].type, type);
        line[6] = flagX[5] || flagY[5] ? AI_OP : getPieceType(state.chessBoard[x + 2][y + 2].type, type);
        line[7] = flagX[6] || flagY[6] ? AI_OP : getPieceType(state.chessBoard[x + 3][y + 3].type, type);
        line[8] = flagX[7] || flagY[7] ? AI_OP : getPieceType(state.chessBoard[x + 4][y + 4].type, type);

        value += evaluateLine(line);

        // 右上-左下
        line[0] = flagX[7] || flagY[0] ? AI_OP : getPieceType(state.chessBoard[x + 4][y - 4].type, type);
        line[1] = flagX[6] || flagY[1] ? AI_OP : getPieceType(state.chessBoard[x + 3][y - 3].type, type);
        line[2] = flagX[5] || flagY[2] ? AI_OP : getPieceType(state.chessBoard[x + 2][y - 2].type, type);
        line[3] = flagX[4] || flagY[3] ? AI_OP : getPieceType(state.chessBoard[x + 1][y - 1].type, type);

        line[5] = flagX[3] || flagY[4] ? AI_OP : getPieceType(state.chessBoard[x - 1][y + 1].type, type);
        line[6] = flagX[2] || flagY[5] ? AI_OP : getPieceType(state.chessBoard[x - 2][y + 2].type, type);
        line[7] = flagX[1] || flagY[6] ? AI_OP : getPieceType(state.chessBoard[x - 3][y + 3].type, type);
        line[8] = flagX[0] || flagY[7] ? AI_OP : getPieceType(state.chessBoard[x - 4][y + 4].type, type);

        value += evaluateLine(line);

        return value;
    }

    /*
    评价一个棋面上的一方
    */
    int evaluateState(ChessBoard state, int type) {
        int value = 0;
        // 分解成线状态
        int line[6][17];
        int lineP;

        for (int p = 0; p < 6; ++p)
            line[p][0] = line[p][16] = AI_OP;

        // 从四个方向产生
        for (int i = 0; i < BOARD_SIZE; ++i) {
            // 产生线状态
            lineP = 1;

            for (int j = 0; j < BOARD_SIZE; ++j) {
                line[0][lineP] = getPieceType(state, i, j, type); /* | */
                line[1][lineP] = getPieceType(state, j, i, type); /* - */
                line[2][lineP] = getPieceType(state, i + j, j, type); /* \ */
                line[3][lineP] = getPieceType(state, i - j, j, type); /* / */
                line[4][lineP] = getPieceType(state, j, i + j, type); /* \ */
                line[5][lineP] = getPieceType(state, BOARD_SIZE - j - 1, i + j, type); /* / */
                ++lineP;
            }
            // 估计
            int special = i == 0 ? 4 : 6;
            for (int p = 0; p < special; ++p) {
                value += evaluateLine(line[p], true);
            }
        }
        return value;
    }

    /*
    若x, y位置周围1格内有棋子则搜索
    */
    bool canSearch(ChessBoard state, int x, int y) {

        int tmpx = x - 1;
        int tmpy = y - 1;
        for (int i = 0; tmpx < BOARD_SIZE && i < 3; ++tmpx, ++i) {
            int ty = tmpy;
            for (int j = 0; ty < BOARD_SIZE && j < 3; ++ty, ++j) {
                if (tmpx >= 0 && ty >= 0 && state.chessBoard[tmpx][ty].type != EMPTY)
                    return true;
                else
                    continue;
            }
        }
        return false;
    }

    /*
    给出后继节点的类型
    */
    int nextType(int type) {
        return type == MAX_NODE ? MIN_NODE : MAX_NODE;
    }

    /*
    state 待转换的状态
    type  当前层的标记：MAX MIN
    depth 当前层深
    alpha 父层alpha值
    beta  父层beta值
    */
    int minMax(ChessBoard state, int x, int y, int type, int depth, int alpha, int beta) {
        ChessBoard newState(state);
        newState.placePiece(x, y, nextType(type));

        int weight = 0;
        int max = -INF; // 下层权值上界
        int min = INF; // 下层权值下界

        if (depth < MAX_DEPTH) {
            // 已输或已胜则不继续搜索
            if (evaluatePiece(newState, x, y, nextType(type)) >= AI_FIVE) {
                if (type == MIN_NODE)
                    return AI_FIVE; // 我方胜
                else
                    return -AI_FIVE;
            }

            int i, j;
            for (i = 0; i < BOARD_SIZE; ++i) {
                for (j = 0; j < BOARD_SIZE; ++j) {
                    if (newState.chessBoard[i][j].type == EMPTY && canSearch(newState, i, j)) {
                        weight = minMax(newState, i, j, nextType(type), depth + 1, min, max);

                        if (weight > max)
                            max = weight; // 更新下层上界
                        if (weight < min)
                            min = weight; // 更新下层下界

                                          // alpha-beta
                        if (type == MAX_NODE) {
                            if (max >= alpha)
                                return max;
                        }
                        else {
                            if (min <= beta)
                                return min;
                        }
                    }
                    else
                        continue;
                }
            }

            if (type == MAX_NODE)
                return max; // 最大层给出最大值
            else
                return min; // 最小层给出最小值
        }
        else {
            weight = evaluateState(newState, MAX_NODE); // 评估我方局面
            weight -= type == MIN_NODE ? evaluateState(newState, MIN_NODE) * 10 : evaluateState(newState, MIN_NODE); // 评估对方局面
            return weight; // 搜索到限定层后给出权值
        }
    }


    /*
    AI 行棋
    */
    bool placePieceAI() {
        int weight;
        int max = -INF; // 本层的权值上界
        int x = 0, y = 0;
        for (int i = 0; i < BOARD_SIZE; ++i) {
            for (int j = 0; j < BOARD_SIZE; ++j) {
                if (curState.chessBoard[i][j].type == EMPTY && canSearch(curState, i, j)) {
                    weight = minMax(curState, i, j, nextType(MAX_NODE), 1, -INF, max);
                    if (weight > max) {
                        max = weight; // 更新下层上界
                        x = i;
                        y = j;
                    }
                }
                else
                    continue;
            }
        }
        return placePiece(x, y); // AI最优点
    }

 };


int main(int argc, char** argv)
{
    int rank, size;
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	
	Game G;
	G.startGame(4);

	int max;
    int	count = 0;//number of rounds
	int info[6];
	int lastpos[2];
	lastpos[0] = 7;
	lastpos[1] = 7;
	double t1,t2;
	double t3,t4;
	if(rank == 0) {
	    cout << "=== round" << cout << "===" <<endl;
		G.placePiece(lastpos[0], lastpos[1]);
		G.show();
	}
	if(rank ==0) t1 = MPI_Wtime();

	while(count < 20) {
		if (rank == 0) {
			t3 = MPI_Wtime();
            count ++;
            cout << "===   round" << count << " ===" << endl;            
			//package infomation to be sent
			max = -666666;
	        info[0] = lastpos[0]; //last_x
		    info[1] = lastpos[1]; //last_y
		    info[2] = G.curUser;
			info[3] = max;
		}

        // all the other processor receive the position of the last placement
		MPI_Bcast(info, 4, MPI_INT, 0, MPI_COMM_WORLD);
		
		//all the other processor update its own board
		if (rank > 0) {
		    int lastUser = info[2] == 1 ? 2 : 1;
			G.curState.placePiece (info[0], info[1], lastUser);
			//cout << "board: "<< rank << endl;
			//G.show();
		}
	
		//t3 = MPI_Wtime();
		vector< pair<int, int> > tasks;
		int task_size = 0;
		for (int i = 0; i < 15; i++) {
		    for (int j = 0; j < 15; j++) {
			    if (G.curState.chessBoard[i][j].type == 0 && G.canSearch(G.curState, i, j)) {
	   				tasks.push_back(make_pair(i, j));
	                task_size++;			
				}
			}
		}
		//t4 = MPI_Wtime();
		//if (rank == 0) cout<<"tasktime: "<< t4 -t3<<endl;
			
		if (rank > 0) {
			//cout << "my rank:  "<< rank << "   my task_size:  "<< task_size <<endl;
			int mytask_size = 0;
		    if (rank <= task_size % (size - 1)) {
			    mytask_size = task_size / (size - 1) + 1;
			} else {
			    mytask_size = task_size / (size - 1);
			}
			//cout << "my rank is:  "<< rank<<"  num my own tasks is:  "<< mytask_size <<endl;
            
			for (int i = 0; i < mytask_size; i++) {
			    pair<int, int> curtask =  tasks[(size - 1) * i + rank - 1];
				//cout<< "curtask: "<< curtask.first<<"  "<< curtask.second<<endl;
                int weight = G.minMax(G.curState, curtask.first, curtask.second, 1, 1, -66666, -666666);
				//cout<< "weight:  "<<weight<<endl;
			    info[3] = weight;
				info[4] = curtask.first;
				info[5] = curtask.second;
				MPI_Send(info, 6, MPI_INT, 0, 1, MPI_COMM_WORLD);
				//cout<< "my rank :  " << rank << "I have sent to 0 "<< endl;
			}
			
		}
        
		if (rank == 0) {
		    int num = 1;
			int times = task_size;
			cout<< "task_size: "<<times<<endl;
			while(times > 0) {
			    MPI_Recv(info, 6, MPI_INT, num, 1, MPI_COMM_WORLD, &status);
			    if (info[3] > max) {
				    lastpos[0] = info[4];
					lastpos[1] = info[5];
					max = info[3];
				}
				times--;
				//cout << "time of receive back "<< times<< endl;
				num = ++num < size ? num : 1;
			}
			G.placePiece(lastpos[0],lastpos[1]);
			cout<<"lastx  "<< lastpos[0] << endl;
			cout<<"lasty  "<< lastpos[1] << endl;
			t4 = MPI_Wtime();
			cout<<"itr time:"<<t4- t3<<endl; 
		}
       // MPI_Barrier(MPI_COMM_WORLD);
	    
        //if (rank == 0){
		  //  G.show();
		//}
	//	MPI_Barrier(MPI_COMM_WORLD);
    }

	if (rank == 0) {
	    t2 = MPI_Wtime();
		cout<<"Time:  "<< t2 - t1 << endl;
	}

    MPI_Finalize();
    return 0;
}
