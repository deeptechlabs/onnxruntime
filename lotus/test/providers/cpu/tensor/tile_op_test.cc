#include "gtest/gtest.h"
#include "test/providers/provider_test_utils.h"

namespace Lotus {
namespace Test {

#if 0  // These changes depent on changes to the Onnx defs.cc file (that the tiles & axis inputs are int64, not float)
TEST(MathOpTest, Tile1D) {
  OpTester test("Tile");

  test.AddInput<float>("input", {3}, {1.0f, 2.0f, 3.0f});
  test.AddInput<int64_t>("tiles", {}, {3});
  test.AddInput<int64_t>("axis", {}, {0});
  test.AddOutput<float>("output", {9}, {1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f});
  test.Run();
}

TEST(MathOpTest, Tile2D) {
  OpTester test("Tile");

  std::vector<int64_t> dims{1, 4};
  test.AddInput<float>("input", {2, 2},
                       {11.0f, 12.0f,
                        21.0f, 22.0f});
  test.AddInput<int64_t>("tiles", {}, {2});
  test.AddInput<int64_t>("axis", {}, {0});
  test.AddOutput<float>("output", {4, 2},
                        {11.0f, 12.0f,
                         21.0f, 22.0f,
                         11.0f, 12.0f,
                         21.0f, 22.0f});

  test.Run();
}

TEST(MathOpTest, Tile3D) {
  OpTester test("Tile");

  test.AddInput<float>("input", {2, 1, 3},
                       {111.0f, 112.0f, 113.0f,
                        211.0f, 212.0f, 213.0f});
  test.AddInput<int64_t>("tiles", {}, {2});
  test.AddInput<int64_t>("axis", {}, {1});
  test.AddOutput<float>("output", {2, 2, 3},
                        {111.0f, 112.0f, 113.0f,
                         111.0f, 112.0f, 113.0f,

                         211.0f, 212.0f, 213.0f,
                         211.0f, 212.0f, 213.0f});
  test.Run();
}
#endif
}  // namespace Test
}  // namespace Lotus
