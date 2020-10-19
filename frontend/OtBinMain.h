#pragma once

#include "Crypto/PRNG.h"




void GBF_Test_Impl(u64 senderSetSize, u64 recvSetSize);
void Poly_Test_Impl(u64 senderSetSize, u64 recvSetSize);
void Client_Impl_BenchMark(u64 numberServer);
void Poly_BenchMark(u64 numberCloudServer);
void GBF_BenchMark(u64 numberCloudServer);