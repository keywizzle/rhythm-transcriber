/// TODO Rename this file to `Note` when finished

#include "NoteString.h"
#include <iostream>
#include <math.h>
#include <numeric>

namespace RhythmTranscriber
{
    /// UniformNoteString

    float UniformNoteString::get_sd()
    {
        /// Get average
        float durationAvg = 0.f;
        float durationSum = 0.f;
        for (unsigned int i = 0; i < length; i++)
        {
            durationSum += (notes + i)->duration;
        }
        durationAvg = durationSum / length;

        /// Get standard deviation
        float sumDiffMean = 0.f;
        for (unsigned int i = 0; i < length; i++)
        {
            auto diff = (notes + i)->duration - durationAvg;
            sumDiffMean += diff * diff;
        }

        return sd = std::sqrt(sumDiffMean / length);
    }

    float UniformNoteString::get_score() { return get_partial_score(0, length); }

    float UniformNoteString::get_partial_score(unsigned int offset, unsigned int len)
    {
        if (len < 2)
            return 1.f;

        float totalSum = 0.f;

        unsigned int stop = len + offset;

        for (unsigned int i = offset; i < stop; i++)
        {
            auto num1 = (notes + i)->duration;
            float divSum = 0.f;

            for (unsigned int j = offset; j < stop; j++)
            {
                auto num2 = (notes + j)->duration;
                float div = num1 > num2 ? (num1 / num2) : (num2 / num1);

                if (div >= 2)
                {
                    /// This implicitly adds a value of 0
                    continue;
                }

                divSum += -div + 2;
            }

            /// Since each note eventually checks with itself, adjust for this padding of score by
            /// excluding it from the sum and length.
            totalSum += (divSum - 1) / (len - 1);
        }

        return totalSum / len;
    }

    void UniformNoteString::calc_duration()
    {
        duration = (notes + length)->timestamp - notes->timestamp;
    }

    void UniformNoteString::create_interpretations()
    {
        float beatDuration = 60 / duration;

        auto interp = Interpretation{Interpretation::BeatRatio{1.f, 1.f, 1.f}};

        for (float consequent = 1.f; consequent <= RhythmTranscriber::maxNoteRatio; consequent++)
        {
            for (float antecedent = 1.f;; antecedent++)
            {
                float quotient = antecedent / consequent;

                float bpm = beatDuration * quotient;

                /// In each iteration of this loop specifically, quotient and BPM are strictly
                /// increasing. If we reach a BPM higher than maxBPM, any BPM after that would also
                /// be too high.
                if (bpm > RhythmTranscriber::maxBpm)
                {
                    break;
                }
                /// Skip possible early iterations of this loop that may have too low of a BPM.
                if (bpm < RhythmTranscriber::minBpm)
                {
                    continue;
                }
                /// Don't add interpretation if an interpretation already exists with the same
                /// quotient because it'll be identical. This method is faster than using
                /// `std::unordered_map` to keep track of which quotients have already been added.
                if (std::gcd((int)antecedent, (int)consequent) != 1)
                {
                    continue;
                }

                interp.bpm = bpm;
                interp.ratio = Interpretation::BeatRatio{antecedent, consequent, quotient};

                float noteGCD =
                    std::gcd((int)interp.ratio.antecedent, (int)interp.ratio.consequent * length);

                /* interp.noteRatio = Interpretation::BeatRatio{
                    interp.ratio.antecedent / noteGCD, (interp.ratio.consequent * length) / noteGCD,
                    interp.ratio.antecedent / (interp.ratio.consequent * length)}; */
                interp.noteRatio =
                    Interpretation::BeatRatio{antecedent / noteGCD, (consequent * length) / noteGCD,
                                              antecedent / (consequent * length)};

                /// Don't add if consequent of the note's ratio is too high
                if (interp.noteRatio.consequent > RhythmTranscriber::maxNoteRatio)
                    continue;

                interpretations.push_back(interp);
            }
        }
    }

    std::pair<UniformNoteString, UniformNoteString> UniformNoteString::split()
    {
        /// There's a more efficient way to do this, instead of using get_partial_score we can
        /// probably O(n) search and keep track of the score sum to reuse

        float bestAvgScore = 0.f;
        unsigned int bestSplitPos = 0;

        for (unsigned int splitPos = 1; splitPos < length; splitPos++)
        {
            float avgScore =
                (get_partial_score(0, splitPos) + get_partial_score(splitPos, length - splitPos)) /
                2;

            if (avgScore > bestAvgScore)
            {
                bestAvgScore = avgScore;
                bestSplitPos = splitPos;
            }
        }

        return splitAt(bestSplitPos);
    }

    std::string UniformNoteString::str()
    {
        std::string str = "timestamp: " + std::to_string(notes[0].timestamp) +
                          ", note length: " + std::to_string(length) +
                          ", score: " + std::to_string(get_score()) + '\n';
        for (unsigned int i = 0; i < interpretations.size(); i++)
        {
            auto interp = interpretations[i];

            str += "bpm: " + std::to_string(interp.bpm) +
                   ", ratio: " + std::to_string(interp.ratio.quotient) + " (" +
                   std::to_string(interp.ratio.antecedent) + '/' +
                   std::to_string(interp.ratio.consequent) +
                   "), note: " + std::to_string(interp.noteRatio.antecedent) + '/' +
                   std::to_string(interp.noteRatio.consequent) +
                   ", score: " + std::to_string(interp.score) + '\n';
        }
        return str;
    }

    /// Interpretation

    float UniformNoteString::Interpretation::get_beat_ratio_score()
    {
        /* return RhythmTranscriber::divisibilityScores[ratio.consequent]; */
        return 0.f;
    }
    float UniformNoteString::Interpretation::get_note_ratio_score()
    {
        /// Note antecedent is unbound, so clamp it (it will get a low score anyways)
        /* return (RhythmTranscriber::divisibilityScores[std::min(
                    (unsigned int)noteRatio.antecedent, RhythmTranscriber::ratioScoreCount)] +
                RhythmTranscriber::divisibilityScores[noteRatio.consequent]) /
               2; */
        return 0.f;
    }
}