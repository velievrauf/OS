#include "contract.h"
#include <cmath>

extern "C" {
    float E(int x) {
        float sum = 0.0f;
        int factorial = 1;
        for (int n = 0; n <= x; ++n) {
            if (n > 0) {
                factorial *= n;
            }
            sum += 1.0f / factorial;
        }
        return sum;
    }

    int PrimeCount(int A, int B)
    {
        if (A > B) return 0;

        int count = 0;
        for (int i = A; i <= B; ++i) {
            if (i < 2) {
                continue;
            }

            bool isPrime = true;;
            for (int j = 2; j * j <= i; ++j) {
                if (i % j == 0) {
                    isPrime = false;
                    break;
                }
            }
            if (isPrime) ++count;
        }

        return count;
    }
}
