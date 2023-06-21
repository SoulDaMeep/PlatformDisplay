#include "pch.h"
#include "CppUnitTest.h"
#include "../PlatformDisplay/ScoreboardPositionInfo.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{
	TEST_CLASS(ScoreboardPositionTests)
	{
	public:
		
		TEST_METHOD(TestHDScreen)
		{
			Vector2F hd{ 1920, 1080 };

			/* Test at 100% ui scale. */
			SbPosInfo sbPosInfo = getSbPosInfo(hd,
				/* uiScale    = */ 1.0,
				/* mutators   = */ false,
				/* numBlues   = */ 3,
				/* numOranges = */ 3);

			Assert::AreEqual(sbPosInfo.blueLeaderPos.X, 429.92f, 0.01f);
			Assert::AreEqual(sbPosInfo.blueLeaderPos.Y, 307.19f, 0.01f);

			Assert::AreEqual(sbPosInfo.orangeLeaderPos.X, 429.92f, 0.01f);
			Assert::AreEqual(sbPosInfo.orangeLeaderPos.Y, 571.97f, 0.01f);

			Assert::AreEqual(sbPosInfo.playerSeparation, 56.95f, 0.01f);

			Assert::AreEqual(sbPosInfo.profileScale, 0.48f, 0.01f);

			/* Test at 50% ui scale. */
			sbPosInfo = getSbPosInfo(hd,
				/* uiScale    = */ 0.5,
				/* mutators   = */ false,
				/* numBlues   = */ 3,
				/* numOranges = */ 3);

			Assert::AreEqual(sbPosInfo.blueLeaderPos.X, 694.96f, 0.01f);
			Assert::AreEqual(sbPosInfo.blueLeaderPos.Y, 423.59f, 0.01f);

			Assert::AreEqual(sbPosInfo.orangeLeaderPos.X, 694.96f, 0.01f);
			Assert::AreEqual(sbPosInfo.orangeLeaderPos.Y, 555.99f, 0.01f);

			Assert::AreEqual(sbPosInfo.playerSeparation, 28.47f, 0.01f);

			Assert::AreEqual(sbPosInfo.profileScale, 0.24f, 0.01f);
		}

		TEST_METHOD(TestQuadHDScreen)
		{
			Vector2F qhd{ 2560, 1440 };

			/* Test at 100% ui scale. */
			SbPosInfo sbPosInfo = getSbPosInfo(qhd,
				/* uiScale    = */ 1.0,
				/* mutators   = */ false,
				/* numBlues   = */ 3,
				/* numOranges = */ 3);

			Assert::AreEqual(sbPosInfo.blueLeaderPos.X, 573.24f, 0.01f);
			Assert::AreEqual(sbPosInfo.blueLeaderPos.Y, 409.58f, 0.01f);

			Assert::AreEqual(sbPosInfo.orangeLeaderPos.X, 573.24f, 0.01f);
			Assert::AreEqual(sbPosInfo.orangeLeaderPos.Y, 762.63f, 0.01f);

			Assert::AreEqual(sbPosInfo.playerSeparation, 75.94f, 0.01f);

			Assert::AreEqual(sbPosInfo.profileScale, 0.64f, 0.01f);

			/* Test at 50% ui scale. */
			sbPosInfo = getSbPosInfo(qhd,
				/* uiScale    = */ 0.5,
				/* mutators   = */ false,
				/* numBlues   = */ 3,
				/* numOranges = */ 3);

			Assert::AreEqual(sbPosInfo.blueLeaderPos.X, 926.62f, 0.01f);
			Assert::AreEqual(sbPosInfo.blueLeaderPos.Y, 564.79f, 0.01f);

			Assert::AreEqual(sbPosInfo.orangeLeaderPos.X, 926.62f, 0.01f);
			Assert::AreEqual(sbPosInfo.orangeLeaderPos.Y, 741.31f, 0.01f);

			Assert::AreEqual(sbPosInfo.playerSeparation, 37.97f, 0.01f);

			Assert::AreEqual(sbPosInfo.profileScale, 0.32f, 0.01f);
		}
	};
}
