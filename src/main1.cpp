#include <atomic_bitvector.hpp>
#include <iostream>
#include <string>
#include <bitset>
using namespace std;

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		cerr << "usage: " << argv[0] << " <bitstring>" << endl;
		return 1;
	}

	string xs{argv[1]};
	atomicbitvector::atomic_bv_t x(xs.c_str(), xs.size());
	cout << x << endl;
	return 0;
}
