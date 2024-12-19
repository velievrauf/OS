#include <Windows.h>
#include <string>
#include <iostream>

typedef float (*E)(int);
typedef int (*PrimeCount)(int, int);

int main() {
	HINSTANCE libHandle = nullptr;
	E eFunc = nullptr;
	PrimeCount primeCountFunc = nullptr;

	libHandle = LoadLibraryA("contract_native.dll");

	if (!libHandle) {
		std::cerr << "Failed to include lib file" << std::endl;
		return 1;
	}

	eFunc = (E)GetProcAddress(libHandle, "E");
	primeCountFunc = (PrimeCount)GetProcAddress(libHandle, "PrimeCount");

	if (!eFunc || !primeCountFunc) {
		std::cerr << "Failed to load the functions" << std::endl;
		FreeLibrary(libHandle);
		return 1;
	}

	std::cout << eFunc(2) << std::endl;

	std::cout << primeCountFunc(3, 100) << std::endl;

	return 0;
}