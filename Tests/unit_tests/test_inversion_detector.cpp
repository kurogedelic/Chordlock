#include <gtest/gtest.h>
#include "Analysis/InversionDetector.h"

using namespace ChordLock;

class InversionDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        detector = std::make_unique<InversionDetector>();
    }

    std::unique_ptr<InversionDetector> detector;
};

TEST_F(InversionDetectorTest, RootPosition) {
    std::vector<int> root_pos = {0, 4, 7}; // C major root position
    auto info = detector->detectInversion(root_pos);
    
    EXPECT_EQ(info.type, InversionType::ROOT_POSITION);
    EXPECT_EQ(info.bass_interval, 0);
}

TEST_F(InversionDetectorTest, FirstInversion) {
    std::vector<int> first_inv = {0, 3, 8}; // C major first inversion (E bass)
    auto info = detector->detectInversion(first_inv);
    
    EXPECT_EQ(info.type, InversionType::FIRST_INVERSION);
    EXPECT_GT(info.confidence, 0.8f);
}

TEST_F(InversionDetectorTest, ConvertToRootPosition) {
    std::vector<int> first_inv = {0, 3, 8};
    auto root_pos = detector->convertToRootPosition(first_inv);
    
    std::vector<int> expected = {0, 4, 7};
    EXPECT_EQ(root_pos, expected);
}