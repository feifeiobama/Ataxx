#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>

#define RATE     2
#define DEAD     50
#define MAX_VAL  1000
#define LIMIT    0.88

using namespace std;

static int mask[4] = {0x15, 0, 0, 0x15};
static int delta[24][2] = {
        {1, 1},  {0, 1},   {-1, 1},  {-1, 0},  {-1, -1}, {0, -1}, {1, -1}, {1, 0},
        {2, 0},  {2, 1},   {2, 2},   {1, 2},   {0, 2},   {-1, 2}, {-2, 2}, {-2, 1},
        {-2, 0}, {-2, -1}, {-2, -2}, {-1, -2}, {0, -2},  {1, -2}, {2, -2}, {2, -1}};
        
static clock_t clk0 = clock() + LIMIT * CLOCKS_PER_SEC;

struct moveStep { int x0, y0, x1, y1; };

istream &operator>>(istream &is, moveStep &m) {
    return is >> m.x0 >> m.y0 >> m.x1 >> m.y1;
}
ostream &operator<<(ostream &os, moveStep &m) {
    return os << m.x0 << ' ' << m.y0 << ' ' << m.x1 << ' ' << m.y1;
}

class chessBoard {
    // 0/1-empty 2-black 3-white
    int info[7];
    int currColor;

 public:
    chessBoard() {}
    chessBoard(istream &is) {
        memset(info, 0, 7 * sizeof(int));
        info[0] = 0xC008;  // |B|W|
        info[6] = 0x800C;  // |W|B|
        currColor = 2;
        int turnID;
        is >> turnID;
        moveStep tmp = {-1, -1, -1, -1};
        for (int i = 1; i != turnID; ++i) {
            // enemy's turn to procstep
            is >> tmp;
            if (tmp.x1 >= 0) procStep(tmp);
            // our turn to procstep
            is >> tmp;
            procStep(tmp);
        }
        is >> tmp;
        if (tmp.x1 >= 0) procStep(tmp);
    }
    chessBoard(const chessBoard &B) {
        memcpy(info, B.info, 7 * sizeof(int));
        currColor = B.currColor;
    }
    bool inMap(int x, int y) {
        return x >= 0 && x <= 6 && y >= 0 && y <= 6;
    }
    int pos(int x, int y) { return (info[x] >> ((y + 1) << 1)) & 3; }
    bool avail(int x, int y) {
        return ((info[x] >> ((y + 1) << 1)) & 3) == currColor;
    }
    bool empty(int x, int y) { return !((info[x] >> ((y + 1) << 1)) & 2); }
    void procStep(const moveStep &m) {
        if (abs(m.x1 - m.x0) == 2 || abs(m.y1 - m.y0) == 2)
            info[m.x0] ^= (2 << ((m.y0 + 1) << 1));
        info[m.x1] &= ~(3 << ((m.y1 + 1) << 1));
        info[m.x1] |= (currColor << ((m.y1 + 1) << 1));
        for (int i = -1; i <= 1; ++i)
            if (m.x1 + i >= 0 && m.x1 + i <= 6) {
                info[m.x1 + i] &= ~(mask[0] << (m.y1 << 1));
                info[m.x1 + i] |= (mask[currColor] << (m.y1 << 1));
            }
        currColor = 5 - currColor;
    }
    int value() {
        int sum[4] = {0};
        for (int i = 0; i != 7; ++i) {
            int line = info[i];
            for (int j = 0; j != 7; ++j)
                ++sum[(line >> ((j + 1) << 1)) & 3];
        }
        return sum[currColor] - sum[5 - currColor];
    }
    // return won ? true : false
    int end() {
        int sum[4] = {0};
        for (int i = 0; i != 7; ++i) {
            int line = info[i];
            for (int j = 0; j != 7; ++j)
                ++sum[(line >> ((j + 1) << 1)) & 3];
        }
        return sum[currColor] <= 24 ? -DEAD : DEAD;
    }
    moveStep* calcAvail(int &num) {
        moveStep* av = new moveStep[600];
        for (int x0 = 0; x0 != 7; ++x0)
            for (int y0 = 0; y0 != 7; ++y0)
                if (empty(x0, y0))
                    for (int i = 0; i != 24; ++i) {
                        int x1 = x0 + delta[i][0], y1 = y0 + delta[i][1];
                        if (!inMap(x1, y1)) continue;
                        if (!avail(x1, y1)) continue;
                        moveStep tmp = {x1, y1, x0, y0};
                        av[num++] = tmp;
                        if (i < 8) i = 8;
                    }
        return av;
    }
};

class storedNode : public chessBoard {
    moveStep from;
    storedNode *chld = NULL;
    int num = 0, dead = 0, calcd = 0, sum = 0;

 public:
    storedNode() {}
    storedNode(istream &is) : chessBoard(is) {}
    storedNode(const chessBoard &B) : chessBoard(B) {}
    void build(moveStep *av) {
        chld = new storedNode[num];
        for (int i = 0; i != num; ++i) {
            chld[i] = chessBoard(*this);
            chld[i].procStep(av[i]);
            chld[i].from = av[i];
        }
        delete []av;
    }
    int valDead() {
        if (dead != 0) return dead;
        return dead = end();
    }
    double calcVal(int parentNum, double rate) {
        if (calcd == 0) return - MAX_VAL + rand() / double(RAND_MAX);
        return double(sum) / calcd - rate * sqrt(log(parentNum) / calcd);
    }
    storedNode* chooseBest(bool isEnd) {
        double rate = isEnd ? 0 : RATE;
        ++calcd;
        int best = 0;
        double bestVal = MAX_VAL;
        for (int i = 0; i != num; ++i) {
            double val = chld[i].calcVal(calcd, rate);
            if (val < bestVal) {
                bestVal = val;
                best = i;
            }
        }
        return chld + best;
    }
    int update(int x) {
        sum += x;
        return -x;
    }
    int choose_expand() {
        if (chld == NULL) {
            build(calcAvail(num));
            if (num == 0) return update(valDead());
            return update(value());
        } else if (num == 0) {
            return update(valDead());
        } else {
            storedNode *N = chooseBest(false);
            return update(N->choose_expand());
        }
    }
    void printAns() { cout << chooseBest(true)->from; }
};

int main() {
    istream::sync_with_stdio(false);
    srand(time(NULL));
    storedNode N(cin);
    while(clock() < clk0) N.choose_expand();
    N.printAns();
    return 0;
}