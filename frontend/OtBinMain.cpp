#include <fstream>
#include <cassert> 
#include <numeric>
#include <iostream>
#include "Network/BtEndpoint.h"
#include "Common/Defines.h"
#include "Common/ByteStream.h"
#include "Common/Log.h"
#include "Common/Log1.h"
#include "Common/Timer.h"
#include "Crypto/PRNG.h"
#include "polyFFT.h"
#include "Hashing/BitPosition.h"
#include "Hashing/binSet.h"

using namespace osuCrypto;


void GBF_Test_Impl(u64 senderSetSize, u64 recvSetSize)
{
	//InitDebugPrinting("../testout.txt");
	u64 numHashFunctions = 31;
	u64 mBfBitCount = 58 * senderSetSize;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

	std::vector<block> mSetX(senderSetSize), mSetY(senderSetSize), mSetEval(recvSetSize), garbledBF(mBfBitCount); //h(x) and value y

	for (u64 i = 0; i < senderSetSize; ++i)
	{
		mSetX[i] = prng.get<block>();
		mSetY[i] = prng.get<block>();
	}

	for (u64 i = 0; i < recvSetSize; ++i)
		mSetEval[i] = prng.get<block>();

	mSetEval[0] = mSetX[0];
	//create hash

	std::vector<AES> mBFHasher(numHashFunctions);
	for (u64 i = 0; i < mBFHasher.size(); ++i)
		mBFHasher[i].setKey(_mm_set1_epi64x(i));


	Timer mTimer;
	double mTime = 0;
	auto start = mTimer.setTimePoint("GBF.insert");


	std::vector<std::set<u64>> idxs(senderSetSize);
	for (u64 i = 0; i < senderSetSize; ++i)
	{
		u64 firstFreeIdx(-1);
		block sum = ZeroBlock;

		//std::cout << "input[" << i << "] " << inputs[i] << std::endl;

		//idxs.clear();
		for (u64 hashIdx = 0; hashIdx < mBFHasher.size(); ++hashIdx)
		{

			block hashOut = mBFHasher[hashIdx].ecbEncBlock(mSetX[i]);
			u64& idx = *(u64*)&hashOut;
			idx %= mBfBitCount;
			idxs[i].emplace(idx);

			//std::cout << idx << " ";
		}
		//std::cout << "\n";

		for (auto idx : idxs[i])
		{
			if (eq(garbledBF[idx], ZeroBlock))
			{
				if (firstFreeIdx == u64(-1))
				{
					firstFreeIdx = idx;
					//std::cout << "firstFreeIdx: " << firstFreeIdx << std::endl;

				}
				else
				{
					garbledBF[idx] = _mm_set_epi64x(idx, idx);
					//	std::cout << garbledBF[idx] <<"\n";
					sum = sum ^ garbledBF[idx];
					//std::cout << idx << " " << garbledBF[idx] << std::endl;
				}
			}
			else
			{
				sum = sum ^ garbledBF[idx];
				//std::cout << idx << " " << garbledBF[idx] << std::endl;
			}
		}

		garbledBF[firstFreeIdx] = sum ^ mSetY[i];
		//std::cout << firstFreeIdx << " " << garbledBF[firstFreeIdx] << std::endl;
		//std::cout << test << "\n";
		//std::cout << "sender " << i << " *   " << garbledBF[firstFreeIdx] << "    " << firstFreeIdx << std::endl;
	}

	//test
	auto mid = mTimer.setTimePoint("GBF.decrypt");


	for (u64 i = 0; i < recvSetSize; ++i)
	{
		//std::cout << "mSetY[" << i << "]= " << mSetY[i] << std::endl;
		//	std::cout << mSetX[i] << std::endl;

		std::set<u64> idxs;

		for (u64 hashIdx = 0; hashIdx < mBFHasher.size(); ++hashIdx)
		{
			block hashOut = mBFHasher[hashIdx].ecbEncBlock(mSetEval[i]);
			u64& idx = *(u64*)&hashOut;
			idx %= mBfBitCount;
			idxs.emplace(idx);
		}

		block sum = ZeroBlock;
		for (auto idx : idxs)
		{
			///std::cout << idx << " " << garbledBF[idx] << std::endl;
			sum = sum ^ garbledBF[idx];
		}

		//if (i == 0) //for test
		//	std::cout << mSetY[0] << "\t vs \t" << sum << std::endl;
	}
	/*for (u64 i = 0; i < garbledBF.size(); ++i)
	{
		if (eq(garbledBF[i], ZeroBlock))
			garbledBF[i] = prng.get<block>();
	}*/

	auto end = mTimer.setTimePoint("GBF.done");
	//double time1 = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start).count();
	//double time2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - mid).count();
	//double time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

	//Log::out << "senderSetSizeperBin vs recvSetSizeperBin:  " << senderSetSizeperBin  << "\t vs \t" << recvSetSizeperBin << "\n";
	//Log::out << "time1= " << time1 << "\n";
	//Log::out << "time2= " << time2 << "\n";
	//Log::out << "total= " << time << "\n";

	Log::out << mTimer << "\n";
	//Log::out << senderSetSize << " \t vs \t" << recvSetSize << "\n";
	Log::out << Log::endl;

}

void polynomial_Test_Impl(u64 senderSetSize, u64 recvSetSize)
{
	BaseOPPRF poly;
	u64 numBins = 1.5 * senderSetSize; //3 hash functions
	u64 binSize = 3 * (senderSetSize + numBins - 1) / numBins; //3 hash functions

	u64 maskSize = roundUpTo(40 + std::log2(binSize) - 1, 8) / 8;
	poly.poly_init(maskSize);

	std::vector<block> mSetX, mSetY, coeffs;
	block blkEval, blkValue;

	u64 setSize = 10;
	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	mSetX.resize(setSize);
	mSetY.resize(setSize);


	for (u64 i = 0; i < binSize; ++i)
	{
		mSetX[i] = prng.get<block>();
		mSetY[i] = prng.get<block>();
		//	std::cout << mSetY[i] << std::endl;
	}

	blkEval = mSetX[0];

	Timer mTimer;
	double mTime = 0;
	auto start = mTimer.setTimePoint("poly.interpolate");
	for (size_t i = 0; i < numBins; i++) //each poly per bin
	{
		poly.getBlkCoefficients(20, mSetX, mSetY, coeffs);
	}

	auto mid = mTimer.setTimePoint("poly.evaluate");

	for (size_t i = 0; i < numBins; i++) //each poly per bin
	{
		poly.evalPolynomial(coeffs, blkEval, blkValue);
	}

	//std::cout << blkValue << " -- " << mSetY[0] << std::endl;

	auto end = mTimer.setTimePoint("poly.done");
	double time1 = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start).count();
	double time2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - mid).count();
	double time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

	//Log::out << "senderSetSizeperBin vs recvSetSizeperBin:  " << senderSetSize << "\t vs \t" << recvSetSize << "\n";
	Log::out << "time1= " << time1 << "\n";
	Log::out << "time2= " << time2 << "\n";
	Log::out << "total= " << time << "\n";

	Log::out << mTimer;

	Log::out << Log::endl;

}


void Poly_Test_Impl(u64 senderSetSizeperServer, u64 recvSetSizeperServer) {

//	InitDebugPrinting("../testout.txt");

	const u64 numSuperBlocks = 4;
	PRNG prng0(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	//	u64 lastPolyMaskBytes = 20;


	u64 senderSetSizeperBin = 1024;
	u64 numBin = (senderSetSizeperServer*1.03)/senderSetSizeperBin; //2% dummy
	//u64 numBin = recvSetSizeperServer;
	//u64 senderSetSizeperBin = senderSetSizeperServer / numBin; //assuming random distribtion
	u64 recvSetSizeperBin = recvSetSizeperServer / numBin; //assuming random distribtion
	if (recvSetSizeperBin == 0)
		recvSetSizeperBin = 1;

	std::vector<std::array<block, numSuperBlocks>> inputs(senderSetSizeperBin), setEval(recvSetSizeperBin);
	std::vector<block> setValues(inputs.size());

	for (u64 i = 0; i < inputs.size(); ++i)
	{
		setValues[i] = prng0.get<block>();
		for (u64 j = 0; j < numSuperBlocks; ++j)
		{
			inputs[i][j] = prng0.get<block>();
		}
	}

	for (u64 i = 0; i < recvSetSizeperBin; ++i)
		for (u64 j = 0; j < numSuperBlocks; ++j)
			setEval[i][j] = prng0.get<block>();

	setEval[0] = inputs[0];

	Timer mTimer;

	ZZ_pX Polynomials;


	u64 lastPolyMaskBytes = 10;
	u64 numThreads = 1;
	ZZ mPrime = to_ZZ("1208925819614629174706189"); //nextprime(2^80)

	ZZ_p::init(ZZ(mPrime));


	u64 degree = inputs.size() - 1; //2%
	ZZ_p* zzX = new ZZ_p[inputs.size()];
	ZZ_p* zzY = new ZZ_p[inputs.size()];
	ZZ_p* zzEval = new ZZ_p[setEval.size()];

	ZZ zz;
	ZZ_pX* M = new ZZ_pX[degree * 2 + 1];;
	ZZ_p* a = new ZZ_p[degree + 1];;
	ZZ_pX* temp = new ZZ_pX[degree * 2 + 1];
	ZZ_p* zzY1 = new ZZ_p[inputs.size()];

	mTimer.reset();
	mTimer.setTimePoint("poly.interpolate");

	for (u64 idxBin = 0; idxBin < numBin; idxBin++)
	{
		//===================== = inter========== =
		for (u64 idx = 0; idx < inputs.size(); idx++)
		{
			ZZFromBytes(zz, (u8*)&inputs[idx], lastPolyMaskBytes );
			zzX[idx] = to_ZZ_p(zz);
		}

		for (u64 idx = 0; idx < inputs.size(); idx++)
		{
			ZZFromBytes(zz, (u8*)&setValues[idx], lastPolyMaskBytes);
			zzY[idx] = to_ZZ_p(zz);
		}

		prepareForInterpolate(zzX, degree, M, a, numThreads, mPrime);
		iterative_interpolate_zp(Polynomials, temp, zzY, a, M, degree * 2 + 1, numThreads, mPrime);
	}

	auto mid = mTimer.setTimePoint("poly.eval");
	for (u64 idxBin = 0; idxBin < numBin; idxBin++)
	{
		for (u64 idx = 0; idx < setEval.size(); idx++)
		{
			ZZFromBytes(zz, (u8*)&setEval[idx], lastPolyMaskBytes);
			zzEval[idx] = to_ZZ_p(zz);
		}

		for (u64 idx = 0; idx < setEval.size(); idx++)
		{
			eval(zzY1[idx], Polynomials, zzEval[idx]);
		}
	}

	mTimer.setTimePoint("poly.done");
	Log::out << mTimer << Log::endl;

	//std::cout << senderSetSizeperBin << "\t vs \t" << recvSetSizeperBin << std::endl;
	//std::cout << senderSetSizeperServer << "\t vs \t" << recvSetSizeperServer << std::endl;
	//std::cout << zzY1[0] << "\t vs \t" << zzY[0] << std::endl;
}

void GBF_BenchMark(u64 numberCloudServer) //GBF- based construction
{
	std::cout << " ===========GBF_BenchMark==========\n";

	//for (auto senderOrginalSetSize : { 1 << 20, 1 << 22,1 << 24 })
	for (auto senderOrginalSetSize : { 1 << 20 })
	{
		for (auto recvOrginalSetSize : { 1 << 10, 1 << 12 })
		{
			u64 senderSetSize =  3 * senderOrginalSetSize / numberCloudServer; //1.5 for client's hashing, 3 for bark-oprf
			u64 recvSetSize = (double)1.5 * recvOrginalSetSize / numberCloudServer; //
			
			std::cout << "NumberCloudServers         :" << numberCloudServer << std::endl;
			std::cout << "Set size (Orginial)        : " << senderOrginalSetSize << " vs " << recvOrginalSetSize << std::endl;
			std::cout << "Set size (per Cloud Server): " << senderSetSize << " vs " << recvSetSize << std::endl;
			GBF_Test_Impl(senderSetSize, recvSetSize);
			std::cout << " ========================\n ";

		}
	}
}

void Poly_BenchMark(u64 numberCloudServer) //Polynomial-based construction
{
	std::cout << " ===========Poly_BenchMark==========\n";

	//for (auto senderOrginalSetSize : { 1 << 22, 1 << 24,1 << 26 })
	for (auto senderOrginalSetSize : { 1 << 20 })
	{
		for (auto recvOrginalSetSize : { 1 << 10, 1 << 12 })
		{
			u64 senderSetSize = 3 * senderOrginalSetSize / numberCloudServer; //1.5 for client's hashing, 3 for bark-oprf, 8 for #cloud servers
			u64 recvSetSize = (double)1.5 * recvOrginalSetSize / numberCloudServer; //
			
			std::cout << "NumberCloudServers         :" << numberCloudServer << std::endl;
			std::cout << "Set size (Orginial)        : " << senderOrginalSetSize << " vs " << recvOrginalSetSize << std::endl;
			std::cout << "Set size (per Cloud Server): " << senderSetSize << " vs " << recvSetSize << std::endl;
			Poly_Test_Impl(senderSetSize, recvSetSize);
			std::cout << " ========================\n ";

		}
	}
}
void Client_Impl_BenchMark(u64 numberServer) //simuate client's implemementation
{
	//InitDebugPrinting("../testout.txt");

	std::cout << " ===========Client_Impl_BenchMark==========\n";

	Timer mTimer;

	for (auto setSize : { 1 << 10, 1 << 11,1 << 12 })
	{
		//for (auto numberServer : {16,8,2 })
		{
			std::cout << "NumberCloudServers: " << numberServer << std::endl;
			std::cout << "SetSize           : " << setSize << std::endl;

			u64 numHashFunctions = 3;
			std::vector<block> inputs(setSize);
			PRNG prng0(_mm_set_epi32(4253465, 3434565, 234435, 23987045));

			for (u64 i = 0; i < inputs.size(); ++i)
				inputs[i] = prng0.get<block>();

			std::vector<AES> mBFHasher(numHashFunctions);
			for (u64 i = 0; i < mBFHasher.size(); ++i)
				mBFHasher[i].setKey(_mm_set1_epi64x(i));

			mTimer.reset();
			mTimer.setTimePoint("start");

			binSet bins;
			bins.init(0, 2, setSize, 40, 1);
			bins.hashing2Bins(inputs, 1);
			//bins.mCuckooBins.print(0, 1, 0, 0);

			mTimer.setTimePoint("cuckoo:done");


			////////////
			std::vector<PRNG> seedsPRNG(numberServer - 1);
			for (int i = 0; i < numberServer - 1; i++)
				seedsPRNG[i].SetSeed(prng0.get<block>());

			for (int i = 0; i < bins.mCuckooBins.mBins.size(); i++)
			{
				block sum = inputs[0]; //[bins.mCuckooBins.mBins[i].idx()];
				for (int j = 0; j < numberServer - 1; j++)
				{
					///std::cout << idx << " " << garbledBF[idx] << std::endl;
					sum = sum ^ seedsPRNG[j].get<block>();;
				}

			}
			mTimer.setTimePoint("secretshare:done");

			std::vector<block> values(inputs.size());
			for (int i = 0; i < inputs.size(); i++)
			{
				values[i] = prng0.get<block>();
			}

			int intersection_count=0;
			std::vector<block> receivedValues(bins.mCuckooBins.mBins.size());
			for (int i = 0; i < receivedValues.size(); i++)
			{
				receivedValues[i] = prng0.get<block>();
				for (int j = 0; j < values.size(); j++)
				{
					if (!memcmp((u8*)&receivedValues[i], &values[j], 10))
					{
						intersection_count++;
					}
				}
			}
			mTimer.setTimePoint("intersection:done");
			Log::out << mTimer << Log::endl;
			std::cout << "===============" << std::endl;
		}
	}
}

