#include "Timestamp.h"
#include <string>
#include <vector>

using std::string;
using netlib::Timestamp;

void benchmark() {
    int testnum = 1000 * 1000;

    std::vector<Timestamp> Timestamps;
    for (int i = 0; i < testnum; i++) {
        Timestamps.push_back(Timestamp::now());
    }
    printf("frist timestamp : %s\n", Timestamps.front().toString().c_str());
    printf("last timestamp : %s\n", Timestamps.back().toString().c_str());
    printf("time difference : %ld \n", Timestamps.back().getUsSinceEpoch()-Timestamps.front().getUsSinceEpoch());
    printf("is equality : %s \n", Timestamps.front() == Timestamps.back() ? "equal" : "unequal");
    printf("frist is less than last : %s \n", Timestamps.front() < Timestamps.back() ? "<" : ">=");
    printf("last is bigger than last : %s \n", Timestamps.back() > Timestamps.front() ? ">" : "<=");

    int increments[100] = { 0 };
    int64_t start = Timestamps.front().getSencondsSinceEpoch();
    for (int i = 1; i < testnum; ++i) {
        int64_t next = Timestamps[i].getSencondsSinceEpoch();
        int64_t inc = next - start;
        start = next;
        if (inc < 0) {
          printf("reverse!\n");
        } else if (inc < 100) {
          ++increments[inc];
        }
        else {
          printf("big gap %d\n", static_cast<int>(inc));
        }
    }

    for (int i = 0; i < 100; ++i) {
        printf("%2d: %d\n", i, increments[i]);
    }
}

int main() {
    Timestamp now(Timestamp::now());
    printf("now : %s\n", now.toString().c_str());
    printf("now : %s\n", now.toFormattedString(true).c_str());
    printf("now : %s\n", now.toFormattedString(false).c_str());
    benchmark();
}

