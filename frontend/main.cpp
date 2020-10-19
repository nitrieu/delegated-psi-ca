
#include <iostream>
#include "Network/BtChannel.h"
#include "Network/BtEndpoint.h"

using namespace std;
#include "Common/Defines.h"
using namespace osuCrypto;

#include "OtBinMain.h"
#include "Hashing/BitPosition.h"

#include <numeric>
#include "Common/Log.h"
//int miraclTestMain();


void usage(const char* argv0)
{
	std::cout << "Please use:" << std::endl;
	std::cout << "\t -m : number of cloud servers. E.g. -m 2" << std::endl;
	std::cout << "\t -p : polynomial pack/unpack" << std::endl;
	std::cout << "\t -g: GBF pack/unpack" << std::endl;
	std::cout << "\t -c: client's performance" << std::endl;

}
int main(int argc, char** argv)
{
	u64 numberCloudServer = 32;

	if (argv[1][0] == '-' && argv[1][1] == 'm')
		numberCloudServer = atoi(argv[2]);

	if (argv[3][0] == '-' && argv[3][1] == 'p')
		Poly_BenchMark(numberCloudServer);

	else if (argv[3][0] == '-' && argv[3][1] == 'g')
		GBF_BenchMark(numberCloudServer);
	
	else if (argv[3][0] == '-' && argv[3][1] == 'c')
		Client_Impl_BenchMark(numberCloudServer);
	
	else {
		usage(argv[0]);
	}

	return 0;	
	
}
