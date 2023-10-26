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
    void Transcriber::create_note_strs()
    {
        auto noteStr = UniformNoteString{&(notes.at(0)), notes.at(0).duration, 1};

        /// Exclude last note since it won't have a duration
        auto notesLen = notes.size() - 1;

        for (unsigned int i = 1; i < notesLen; i++)
        {
            auto note = notes.at(i);

            float duration = note.duration;

            if (!noteStr.is_uniform(duration))
            {
                noteStrs.push_back(noteStr);
                noteStrs.at(noteStrs.size() - 1).create_interpretations();

                noteStr.notes = &(notes[i]);
                noteStr.duration = duration;
                noteStr.length = 1;
            }
            else
            {
                noteStr.duration += duration;
                noteStr.length++;
            }
        }

        /// Add last division string
        noteStrs.push_back(noteStr);
        noteStrs.at(noteStrs.size() - 1).create_interpretations();
    }
    void Transcriber::score_interpretations()
    {
        std::vector<UniformNoteString> sortedStrs = noteStrs;
        std::sort(sortedStrs.begin(), sortedStrs.end(),
                  [](UniformNoteString &noteStr1, UniformNoteString &noteStr2)
                  {
                      return noteStr1.interpretations.size() > noteStr2.interpretations.size();
                      /* float score1 = noteStr1.get_score();
                      float score2 = noteStr2.get_score();

                      if (score1 == 1 && score2 != 1)
                          return true;
                      if (score1 != 1 && score2 == 1)
                          return false;

                      return score1 < score2; */
                  });

        auto noteStrLen = sortedStrs.size();
        for (unsigned int i = 0; i < noteStrLen; i++)
        {
            auto noteStr = sortedStrs.at(i);
            auto interpsLen = noteStr.interpretations.size();
            std::cout << "note length: " << std::to_string(noteStr.length)
                      << ", timestamp: " << noteStr.notes[0].timestamp
                      << ", score: " << noteStr.get_score() << '\n';
            for (unsigned int j = 0; j < interpsLen; j++)
            {
                /// Combined with the previous note string, this endBeatScore is a measure of how
                /// well this interpretation fits with

                auto interp = noteStr.interpretations.at(j);

                auto beatScore = interp.get_beat_ratio_score();
                auto noteScore = interp.get_note_ratio_score();

                std::cout << "bpm: " + std::to_string(interp.bpm) +
                                 ", ratio: " + std::to_string(interp.ratio.quotient) + " (" +
                                 std::to_string(interp.ratio.antecedent) + '/' +
                                 std::to_string(interp.ratio.consequent) +
                                 "), note: " + std::to_string(interp.noteRatio.antecedent) + '/' +
                                 std::to_string(interp.noteRatio.consequent) +
                                 ", beat score: " + std::to_string(beatScore) +
                                 ", note score: " + std::to_string(noteScore) + ", " +
                                 std::to_string((beatScore + noteScore) / 2)
                          << '\n';
            }
        }
    }

    void Transcriber::try_bpm(float bpm)
    {
        std::vector<Beat> beats;
        float localBpm = 0.f;
        for (unsigned int i = 0; i < noteStrs.size(); i++)
        {
            auto noteStr = noteStrs.at(i);
            Beat beat;

            auto interpretations = noteStr.interpretations;

            std::sort(
                interpretations.begin(), interpretations.end(),
                [localBpm](const RhythmTranscriber::UniformNoteString::Interpretation &interp1,
                           const RhythmTranscriber::UniformNoteString::Interpretation &interp2)
                { return std::abs(localBpm - interp1.bpm) < std::abs(localBpm - interp2.bpm); });
        }
    }

    void Transcriber::transcribe(float *timestamps, unsigned int length)
    {
        create_notes(timestamps, length);

        /// It might be useful to detect and replace diddles to use with note strings

        // create_note_strs();

        // score_interpretations();

        BeatAnalyzer analyzer;
        analyzer.analyze(&(notes[0]), notes.size());

        /* std::vector<Beat> beats;
float localBpm = 0.f;
for (unsigned int i = 0; i < noteStrs.size(); i++)
{
    auto noteStr = noteStrs.at(i);
    Beat beat;

    auto interpretations = noteStr.interpretations;

    std::sort(
        interpretations.begin(), interpretations.end(),
        [localBpm](const RhythmTranscriber::UniformNoteString::Interpretation &interp1,
                   const RhythmTranscriber::UniformNoteString::Interpretation &interp2)
        { return std::abs(localBpm - interp1.bpm) < std::abs(localBpm - interp2.bpm); });
} */

        /* for (unsigned int i = 0; i < noteStrs.size(); i++)
        {
            auto noteStr = noteStrs.at(i);
            std::cout << "division str at " << std::to_string(noteStr.notes->timestamp)
                      << " with note length " << std::to_string(noteStr.length)
                      << " (score: " << noteStr.get_score() << "): \n";
            for (unsigned int j = 0; j < noteStr.interpretations.size(); j++)
            {
                auto interp = noteStr.interpretations.at(j);

                if (interp.noteRatio.consequent > RhythmTranscriber::maxNoteRatio)
                    continue;

                std::cout << "bpm: " << std::to_string(interp.bpm)
                          << ", ratio: " << std::to_string(interp.ratio.quotient) << " ("
                          << interp.ratio.antecedent << '/' << interp.ratio.consequent
                          << "), note: " << interp.noteRatio.antecedent << '/'
                          << interp.noteRatio.consequent << ", score: " << interp.score << '\n';
            }
        } */
    }
}