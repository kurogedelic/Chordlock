#include <gtest/gtest.h>
#include "Utils/NoteConverter.h"

using namespace ChordLock;

class NoteConverterTest : public ::testing::Test {
protected:
    void SetUp() override {
        converter = std::make_unique<NoteConverter>();
    }

    std::unique_ptr<NoteConverter> converter;
};

TEST_F(NoteConverterTest, MidiToNoteName) {
    // Test middle C
    EXPECT_EQ(converter->midiToNoteName(60), "C4");
    
    // Test other notes
    EXPECT_EQ(converter->midiToNoteName(61), "C#4");
    EXPECT_EQ(converter->midiToNoteName(64), "E4");
}

TEST_F(NoteConverterTest, NoteNameToMidi) {
    // Test conversion back
    EXPECT_EQ(converter->noteNameToMidi("C4"), 60);
    EXPECT_EQ(converter->noteNameToMidi("C#4"), 61);
    EXPECT_EQ(converter->noteNameToMidi("E4"), 64);
}

TEST_F(NoteConverterTest, AccidentalStyles) {
    // Test flat style
    converter->setDefaultAccidentalStyle(AccidentalStyle::FLATS);
    EXPECT_EQ(converter->midiToNoteName(61), "Db4");
    
    // Test sharp style
    converter->setDefaultAccidentalStyle(AccidentalStyle::SHARPS);
    EXPECT_EQ(converter->midiToNoteName(61), "C#4");
}

TEST_F(NoteConverterTest, NoteClass) {
    EXPECT_EQ(converter->getNoteClass(60), 0);  // C
    EXPECT_EQ(converter->getNoteClass(61), 1);  // C#
    EXPECT_EQ(converter->getNoteClass(72), 0);  // C (octave higher)
}