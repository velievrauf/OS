#include "contract_native.h"
#include <cmath>
#include <vector>

extern "C" {
    float E(int x) {
        if (x == 0) return 1.0f;
        float result = pow(1.0f + 1.0f / x, x);
        return result;
    }

    int PrimeCount(int A, int B)
    {
        if (A > B || A < 2) return 0;

        std::vector<bool> primes(B + 1, true);
        primes[0] = primes[1] = false;

        for (int i = 2; i * i <= B; ++i) {
            if (primes[i]) {
                for (int j = i * i; j <= B; j += i) {
                    primes[j] = false;
                }
            }
        }

        int count = 0;
        for (int i = A; i <= B; ++i) {
            if (primes[i]) ++count;
        }

        return count;
    }
}