#include "Transcriber.h"
#include "utils.h"
#include <algorithm>
#include <iostream>
#include <math.h>
#include <numeric>
#include <vector>

#include "tests/tests.h"
#include "transcription/TranscriptionFile.h"

float testTimestamps[] = {
    0.056,       0.1493333333, 0.2426666667, 0.3386666667, 0.432,        0.5253333333, 0.6186666667,
    0.68,        0.744,        0.8133333333, 0.8693333333, 0.9306666667, 0.9946666667, 1.181333333,
    1.242666667, 1.306666667,  1.370666667,  1.434666667,  1.493333333,  1.557333333,  1.744,
    1.805333333, 1.869333333,  1.930666667,  1.994666667,  2.056,        2.12,         2.213333333,
    2.554666667, 2.68,         2.805333333,  2.930666667,  3.056,        3.306666667,  3.368,
    3.432,       3.493333333,  3.557333333,  3.808,        3.869333333,  3.930666667,  3.994666667,
    4.056,       4.181333333,  4.432,        4.496,        4.562666667,  4.618666667,  4.682666667,
    4.744,       4.808,        4.869333333,  4.930666667,  4.994666667,  5.056,        5.12,
    5.181333333, 5.245333333,  5.306666667,  5.370666667,  5.432,        5.493333333,  5.557333333,
    5.618666667, 5.68,         5.746666667,  5.808,        5.869333333,  5.930666667,  5.994666667,
    6.056,       6.12,         6.181333333,  6.248,        6.306666667,  6.368,        6.432,
    6.68,        6.930666667,  7.181333333,  7.245333333,  7.306666667,  7.370666667,  7.432,
    7.501333333, 7.557333333,  7.618666667,  7.682666667,  7.744,        7.808,        7.869333333,
    7.930666667, 7.994666667,  8.056,        8.12,         8.181333333,  8.245333333,  8.312,
    8.68,        8.805333333,  8.930666667};
int testTimestampsLen = 101;

const float uniformDeltaThreshold = 1.65f;

inline bool is_uniform(float div1, float div2)
{
    return (div1 > div2 ? (div1 / div2) : (div2 / div1)) < uniformDeltaThreshold;
}

struct DivisionString
{
    float *pTimestamp = nullptr;

    float duration = 0.f;

    unsigned int notesLen = 0;
};

struct NoteStringInterpretation
{
    struct BeatRatio
    {
        float antecedent = 0.f;
        float consequent = 0.f;
        float quotient = 0.f;
    };

    /// @brief How much of a beat the interpretation represents
    BeatRatio ratio = BeatRatio{0.f, 0.f, 0.f};

    /// @brief How much of a beat each note represents
    BeatRatio noteRatio = BeatRatio{0.f, 0.f, 0.f};

    /// @brief Beats-per-minute of the interpretation
    float bpm = 0.f;
};

int main()
{
    using namespace RhythmTranscriber::Transcription;

    RhythmTranscriber::run_tests();

    return 0;

    std::string fileName = "computer\\3_5 beat pause offbeat";
    float bpm = 120.f;

    TranscriptionFile transcriptionFile = TranscriptionFile();
    auto transcription = transcriptionFile.read(".\\test data\\" + fileName + ".json");
    transcriptionFile.write(".\\test data\\" + fileName + ".json", transcription,
                            TranscriptionFile::WriteOptions{
                                TranscriptionFile::WriteOptions::Type::JSON, 1,
                                TranscriptionFile::WriteOptions::Compression::OMIT_EMPTY_FIELDS});
    transcriptionFile.write(".\\test data\\" + fileName + ".transcription", transcription);

    return 0;

    std::vector<float> timestamps;
    for (unsigned int i = 0; i < transcription.notes.size(); i++)
    {
        if (transcription.notes.at(i).flam)
            continue;
        timestamps.push_back(transcription.notes.at(i).timestamp);
    }

    RhythmTranscriber::Transcriber transcriber =
        RhythmTranscriber::Transcriber(&(timestamps[0]), timestamps.size());
    transcriber.transcribe(bpm, 1);

    Sleep(5000);

    return 0;
}