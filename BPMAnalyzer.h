#pragma once

#include "Config.h"
#include "Note.h"
#include <math.h>
#include <vector>

namespace RhythmTranscriber
{
    const float minBeatDuration = 60.f / maxBpm;
    const float maxBeatDuration = 60.f / minBpm;

    class BPMAnalyzer
    {
    public:
        BaseNote *notes = nullptr;

        unsigned int notesLen = 0;

        std::vector<std::vector<float>> scoreData;

        float beatDurationStep = 0.0025;

        void get_score_data();

    private:
        float calc_beat_score(float duration, unsigned int noteIndex);
        float calc_avg_beat_score(float duration);
        float calc_full_beat_score_at(unsigned int durationIndex);

        /// @brief Given the index of a beat duration score, gets the score of that beat duration
        /// times `multiplier`.
        /// @param durationIndex
        /// @param multiplier
        /// @return
        inline float get_beat_score_mult(unsigned int durationIndex, float multiplier,
                                         unsigned int noteIndex)
        {
            float index = (minBeatDuration * (multiplier - 1)) / beatDurationStep +
                          multiplier * durationIndex;

            int floorIndex = index;

            /// Exclude last index since we don't have enough data to get it
            /// TODO: If outside range, manually calculate it..?
            if (index > scoreData.size() - 1 || index < 0.f)
            {
                return calc_beat_score(
                    (minBeatDuration + beatDurationStep * durationIndex) * multiplier, noteIndex);

                /* return 0.f; */
            }

            if (floorIndex != index)
            {
                /// Interp value
                return (scoreData.at(floorIndex + 1).at(noteIndex) -
                        scoreData.at(floorIndex).at(noteIndex)) *
                           (index - floorIndex) +
                       scoreData.at(floorIndex).at(noteIndex);
            }

            return scoreData.at(std::round(index)).at(noteIndex);
        }
    };
}