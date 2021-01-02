#include "Neighborhood.h"
#include <algorithm>

namespace {

struct PointWithIndex {
    Point pos;
    Neighbor::indexType index;
};

void RandomlyPermutePointSet(PointWithIndex array[], size_t n) {
    std::random_shuffle(array, array + n, [](uint32_t k) {return RandomUInt(k);});
}

PointWithIndex* GenerateDataSet(PointWithIndex array[], size_t M, size_t E) {
    PointWithIndex* end = array;
    for (size_t k=0; k < M+E; ++k) {
        float r, theta;
        if (k<M) {
            r = 10.0f;
            theta = float(2*3.1415926535*k/M);
        } else {
            r = 12.0f + RandomFloat(2.0f);
            theta = RandomAngle();
        }
        end->pos = Polar(r, theta);
        end->index = k;
        ++end;
    }
    return end;
}

} // (anonymous)

void TestNeighborhood() {
    const size_t bufferMax = 100;
    Neighbor buffer[bufferMax];
    Neighborhood n(buffer, bufferMax);
    // Number of expected sides for the cell
    const int M = 6;
    // Number of extra points that will not be sides
    const int E = 12;
    PointWithIndex pointSet[M+E];
    PointWithIndex* end = GenerateDataSet(pointSet, M, E);
    for (int trial=0; trial<10000; ++trial) {
        RandomlyPermutePointSet(pointSet, end-pointSet);
        Point rotate = Polar(1.0f, RandomAngle());
        n.start();
        for (const PointWithIndex* p=pointSet; p!=end; ++p)
            n.addPoint(Multiply(rotate, p->pos), p->index);

        size_t m = n.finish();
        Assert(m==M);
        bool found[M];
        std::fill_n(found, M, false);
        for (size_t i=0; i<m; ++i) {
            size_t k = buffer[i].index;
            Assert(k<M);
            Assert(!found[k]);
            found[k] = true;
        }
    }
}