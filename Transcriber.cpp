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
    Transcriber::Transcriber(float *timestamps, unsigned int length)
    {
        create_notes(timestamps, length);
    }

    void Transcriber::create_notes(float *timestamps, unsigned int length)
    {
        notes.reserve(length);
        for (unsigned int i = 0; i < length - 1; i++)
        {
            notes.push_back(
                RhythmTranscriber::BaseNote{timestamps[i], timestamps[i + 1] - timestamps[i]});
        }
        notes.push_back(RhythmTranscriber::BaseNote{timestamps[length - 1], -1.f});

        beatAnalyzer.notes = &(notes[0]);
        beatAnalyzer.notesLen = notes.size();
    }

    void Transcriber::transcribe()
    {
        /// BPMAnalyzer stuff...

        BeatAnalyzer beatAnalyzer;
    }

    void Transcriber::transcribe(float bpm)
    {
        beatAnalyzer.maxDepth = 4;
        beatAnalyzer.bpm = bpm;
    }

    void Transcriber::transcribe(float bpm, unsigned int depth)
    {
        beatAnalyzer.set_bpm(bpm);
        beatAnalyzer.set_depth(depth);

        /// TODO: Have dynamic depth, basically take longer on trickier beats, maybe by looking at
        /// score or something.

        int beatCount = 0;

        for (unsigned int i = 0; i < notes.size();)
        {
            /* std::cout << "getting best branch starting at " << notes[i].timestamp << '\n'; */
            beatAnalyzer.get_best_branch_at(i);

            unsigned int beatIndex = 0;

            Beat beat = beatAnalyzer.bestBranch.beatBuffer[0];

            add_beat(beat);

            beatCount++;
            std::cout << beat.str() << '\n';

            i += beat.notesLen;

            while (beat.division.antecedent > beat.division.consequent)
            {
                beatIndex++;

                beat = beatAnalyzer.bestBranch.beatBuffer[beatIndex];
                /* std::cout << "offset beat: " << beat.str() << '\n'; */

                add_beat(beat);

                beatCount++;

                std::cout << beat.str() << '\n';

                i += beat.notesLen;
            }
            if (beatCount >= 8)
            {
                break;
            }
        }

        /// Add last note as a quarter note (at least for now).
        

        /* for (unsigned int i = 0; i < transcription.notes.size(); i++)
        {
            std::cout << "Transcription note: " << transcription.notes[i].rhythm.beats << "/"
                      << transcription.notes[i].rhythm.notes << '\n';
        } */
    }

    void Transcriber::transcribe(float *timestamps, unsigned int length)
    {
        create_notes(timestamps, length);

        BeatAnalyzer analyzer;
    }
}