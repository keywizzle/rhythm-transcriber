#include "BPMAnalyzer.h"
#include "Config.h"

namespace RhythmTranscriber
{
    void BPMAnalyzer::get_score_data()
    {
        unsigned int bpmIndex = 0;

        float maxDuration;
        float endBeatTime;
        float distToBeat;
        float nextDistToBeat;
        float durationDiff;

        /// Allocate
        scoreData = std::vector<std::vector<float>>(
            (maxBeatDuration - minBeatDuration) / beatDurationStep + 1,
            std::vector<float>(notesLen - 1, 0.f));

        for (float duration = minBeatDuration; duration <= maxBeatDuration;
             duration += beatDurationStep)
        {
            maxDuration = duration * RhythmTranscriber::beatThresholdMultiplier;

            for (unsigned int i = 0; i < notesLen - 1; i++)
            {
                if ((notes + i)->duration > maxDuration)
                {
                    continue;
                }

                scoreData.at(bpmIndex).at(i) = calc_beat_score(duration, i);

                /* endBeatTime = (notes + i)->timestamp + duration;

                distToBeat = duration;

                /// Shouldn't this be j < notesLen - 1 if we're checking notes + j + 1? TODO
                for (unsigned int j = i; j < notesLen; j++)
                {
                    nextDistToBeat = std::abs(endBeatTime - (notes + j + 1)->timestamp);

                    if (nextDistToBeat > distToBeat)
                    {
                        durationDiff =
                            std::abs((notes + j)->timestamp - (notes + i)->timestamp - duration);

                        scoreData.at(bpmIndex).at(i) =
                            0.0009f / (durationDiff * durationDiff + 0.0009f);

                        break;
                    }
                    distToBeat = nextDistToBeat;
                } */
            }

            bpmIndex++;
        }
    }

    float BPMAnalyzer::calc_beat_score(float duration, unsigned int noteIndex)
    {
        float nextDistToBeat, durationDiff;

        float endBeatTime = (notes + noteIndex)->timestamp + duration;

        float distToBeat = duration;

        /// Shouldn't this be j < notesLen - 1 if we're checking notes + j + 1? TODO
        for (unsigned int j = noteIndex; j < notesLen; j++)
        {
            nextDistToBeat = std::abs(endBeatTime - (notes + j + 1)->timestamp);

            if (nextDistToBeat > distToBeat)
            {
                durationDiff =
                    std::abs((notes + j)->timestamp - (notes + noteIndex)->timestamp - duration);

                return 0.0009f / (durationDiff * durationDiff + 0.0009f);
            }
            distToBeat = nextDistToBeat;
        }
        return 0.f;
    }

    float BPMAnalyzer::calc_avg_beat_score(float duration)
    {
        float maxDuration = duration * RhythmTranscriber::beatThresholdMultiplier;

        float scoreSum = 0.f;
        for (unsigned int i = 0; i < notesLen - 1; i++)
        {
            if ((notes + i)->duration > maxDuration)
            {
                continue;
            }

            scoreSum += calc_beat_score(duration, i);
        }

        /// If we continue at any point in the above loop, this might be skewed.
        /// Maybe keep track of how many times a score is actually added..?
        return scoreSum / (notesLen - 1);
    }

    float BPMAnalyzer::calc_full_beat_score_at(unsigned int durationIndex)
    {
        float scoreSum = 0.f;
        float score = 0.f;
        for (unsigned int i = 0; i < notesLen - 1; i++)
        {
            /* float maxScore = scoreData.at(durationIndex).at(i); */
            float maxScore = 0.f;

            score = get_beat_score_mult(durationIndex, 2, i);
            if (score > maxScore)
                maxScore = score;

            /* score = get_beat_score_mult(durationIndex, 0.5, i);
            if (score > maxScore)
                maxScore = score; */

            scoreSum += maxScore;
        }
        return scoreSum / (notesLen - 1);
    }
}