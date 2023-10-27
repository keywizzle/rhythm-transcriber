#include "Transcriber.h"
#include "Beat.h"
#include "utils.h"
#include <algorithm>
#include <math.h>

#include "BeatAnalyzer.h"

float score_test(unsigned int consequent)
{
    float divisionFreq = 0.f;
    for (unsigned int i = 1; i < consequent + 1; i++)
    {
        if (consequent % i == 0)
        {
            divisionFreq++;
        }
    }
    return divisionFreq / consequent;
}

namespace RhythmTranscriber
{
    void Transcriber::create_notes(float *timestamps, unsigned int length)
    {
        notes.reserve(length);
        for (unsigned int i = 0; i < length - 1; i++)
        {
            notes.push_back(
                RhythmTranscriber::BaseNote{timestamps[i], timestamps[i + 1] - timestamps[i]});
        }
        notes.push_back(RhythmTranscriber::BaseNote{timestamps[length - 1], -1.f});
    }

    void Transcriber::transcribe(float *timestamps, unsigned int length)
    {
        create_notes(timestamps, length);

        BeatAnalyzer analyzer;
        analyzer.analyze(&(notes[0]), notes.size());
    }
}