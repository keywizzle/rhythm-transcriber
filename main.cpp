#include "Transcriber.h"
#include "utils.h"
#include <inttypes.h>
#include <iostream>
#include <math.h>
#include <unordered_set>
#include <vector>

#include "transcription/TranscriptionFile.h"

#include "rapidjson/document.h"

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

inline bool is_uniform(float div1, float div2)
{
    return (div1 > div2 ? (div1 / div2) : (div2 / div1)) < 1.8;
}

struct DivisionString
{
    float *pTimestamp = nullptr;

    float duration = 0.f;

    unsigned int notesLen = 0;
};

std::vector<float> get_partials(float maxDivision, float maxProduct)
{
    std::unordered_set<float> partialSet;
    std::vector<float> partialVec;

    for (unsigned int i = 1; i <= maxDivision; i++)
    {
        // complexities.emplace((float)notes / (float)beats).second
        float basePartial = 1.f / i;
        float partial = 0.f;
        for (unsigned int multiplier = 1; partial <= maxProduct; multiplier++)
        {
            // std::cout << "partial: " << std::to_string(partial) << '\n';
            partial = basePartial * multiplier;
            if (partial > maxProduct)
                break;
            if (partialSet.emplace(partial).second)
            {
                partialVec.push_back(partial);
            }
        }
    }

    return partialVec;
}

void bpm_test(std::vector<float> &timestamps)
{
    std::vector<DivisionString> divisions;

    float divisionLen = 0.f;
    unsigned int notesLen = 0;
    float *divisionTimestamp = &(timestamps.at(0));
    for (unsigned int i = 0; i < timestamps.size() - 2; i++)
    {
        float timestamp = timestamps.at(i);
        float nextTimestamp = timestamps.at(i + 1);

        float duration = nextTimestamp - timestamp;
        float nextDuration = timestamps.at(i + 2) - nextTimestamp;

        divisionLen += duration;
        notesLen++;

        if (!is_uniform(duration, nextDuration) || i == timestamps.size() - 3)
        {
            divisions.push_back(DivisionString{divisionTimestamp, divisionLen, notesLen});
            divisionTimestamp = &(timestamps.at(i + 1));
            divisionLen = 0.f;
            notesLen = 0;
        }
    }

    auto partials = get_partials(6, 5.5f);
    auto partialCount = partials.size();

    std::vector<std::vector<float>> partialBpms;

    for (unsigned int i = 0; i < divisions.size(); i++)
    {
        float divisionDuration = divisions.at(i).duration;
        std::vector<float> bpms;
        bpms.reserve(partialCount);
        for (unsigned int j = 0; j < partialCount; j++)
        {
            auto thing = divisionDuration / partials.at(j);
            bpms.push_back(60 / thing);
        }
        partialBpms.push_back(bpms);
    }

    float testBpm = 160.f;

    for (unsigned int i = 0; i < partialBpms.size(); i++)
    {
        auto bpms = partialBpms.at(i);

        float bestScore = 0.f;
        unsigned int bestIndex = -1;
        for (unsigned int j = 0; j < bpms.size(); j++)
        {
            float bpm = bpms.at(j);

            if (bpm > RhythmTranscriber::maxBPM || bpm < RhythmTranscriber::minBPM)
            {
                continue;
            }

            // Store top scores, then when you have all of them go back and pick from each top set
            // of scores the best one which also fits with surrounding divisions
            // Ex: If previous and next interpretation is 0.666 and top two scores are 0.25 and
            // 0.333, 0.333 would make more sense to fit the triplet-ish phrase

            float tempScore = 1 / (std::abs(bpm - testBpm) + j);
            std::cout << "\tbpm: " << std::to_string(bpm)
                      << ", partial: " << std::to_string(partials.at(j))
                      << ", tempScore: " << std::to_string(tempScore) << '\n';
            if (tempScore > bestScore)
            {
                bestScore = tempScore;
                bestIndex = j;
            }

            /// Don't score single-longish notes as heavily
            /// Humans suck at timing transition notes between phrases/note strings, and they also
            /// tend to have very strange divisions.
        }
        std::cout << "best match for " << std::to_string(testBpm) << " at "
                  << std::to_string(*divisions.at(i).pTimestamp) << ": "
                  << std::to_string(partialBpms.at(i).at(bestIndex)) << " BPM for "
                  << std::to_string(partials.at(bestIndex)) << '\n';
    }
}

int main()
{
    float bpm = 164.f;

    using namespace RhythmTranscriber::Transcription;

    std::string fileName = "bd 2017";

    StartTimer();
    TranscriptionFile transcriptionFile = TranscriptionFile();
    auto transcription = transcriptionFile.read(".\\test data\\" + fileName + ".json");
    /* transcriptionFile.write(".\\test data\\" + fileName + ".json", transcription,
                            TranscriptionFile::WriteOptions{
                                TranscriptionFile::WriteOptions::Type::JSON, 1,
                                TranscriptionFile::WriteOptions::Compression::OMIT_EMPTY_FIELDS});
    transcriptionFile.write(".\\test data\\" + fileName + ".transcription", transcription); */
    StopTimer();

    std::vector<float> timestamps;
    for (unsigned int i = 0; i < transcription.notes.size(); i++)
    {
        if (transcription.notes.at(i).flam)
            continue;
        timestamps.push_back(transcription.notes.at(i).timestamp);
    }

    /* for (unsigned int i = 0; i < testTimestampsLen; i++)
    {
        timestamps.push_back(testTimestamps[i]);
    } */
    bpm_test(timestamps);

    /* RhythmTranscriber::Transcriber transcriber;
    transcriber.transcribe(&(timestamps[0]), timestamps.size(), bpm); */

    Sleep(5000);

    return 0;
}