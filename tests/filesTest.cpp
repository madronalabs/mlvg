

#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <numeric>
#include <thread>
#include <unordered_map>
#include <vector>

#include "MLFiles.h"
#include "catch.hpp"
#include "madronalib.h"

using namespace ml;

TEST_CASE("mlvg/core/files", "[files]")
{
	bool OK{ false };
	const int vecSize{ 1000 };
	CharVector cv1;
	cv1.resize(vecSize);

	RandomScalarSource r;
	for (int i = 0; i < vecSize; ++i)
	{
		uint8_t c1 = r.getUInt32();
		cv1[i] = c1;
	}

	Path userPath = FileUtils::getUserDataPath();

	File testFile;
    if (userPath)
    {
        Path fullPath = Path(userPath, "mlvg-test1.bin");
		testFile = File(fullPath);
    }
	
	// test save and load
	if (testFile)
	{
		testFile.replaceWithData(cv1);
		CharVector cv2;
		testFile.load(cv2);
		REQUIRE(cv1 == cv2);
	}

	// test compressed save and load
	if (testFile)
	{
		testFile.replaceWithDataCompressed(cv1);
		CharVector cv2;
		testFile.loadCompressed(cv2);
		REQUIRE(cv1 == cv2);
	}



 //  REQUIRE(OK == true);
}
