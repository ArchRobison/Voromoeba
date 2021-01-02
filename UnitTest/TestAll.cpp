#include "AssertLib.h"

void TestGeometry();
void TestNeighborhood();
void TestVoronoi();

int main() {
    TestGeometry();
    TestVoronoi();
    TestNeighborhood();
    return 0;
}
