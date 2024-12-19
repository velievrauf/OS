#pragma once

#define API _declspec(dllexport)

extern "C" {
	API float E(int x);
	API int PrimeCount(int A, int B);
}