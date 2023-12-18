#include "BeatAnalyzer.h"
#include "Config.h"

#include "utils.h"
#include <iomanip>
#include <iostream>
#include <math.h>

inline float divisionScore(float num1, float num2)
{
    float div = num1 > num2 ? (num1 / num2) : (num2 / num1);

    /* std::cout << "roundDiv: " << std::round(div) << '\n'; */

    /// Scale value
    return 1 - (std::abs(div - std::round(div)) * 2);
}

inline float divisionScoreBetween(RhythmTranscriber::BaseNote *startNote, unsigned int length)
{
    if (length < 2)
        return 1.f;
    float scoreSum = 0.f;
    for (unsigned int i = 0; i < length; i++)
    {
        float dur1 = (startNote + i)->duration;

        float noteSum = 0.f;
        for (unsigned int j = 0; j < length; j++)
        {
            float dur2 = (startNote + j)->duration;

            float div = dur1 > dur2 ? (dur1 / dur2) : (dur2 / dur1);

            /* std::cout << "duration 1: " << dur1 << ", duration 2: " << dur2
                      << ", score: " << std::to_string(1 - (std::abs(div - std::round(div)) * 2))
                      << '\n'; */

            noteSum += 1 - (std::abs(div - std::round(div)) * 2);
        }
        scoreSum += (noteSum - 1) / (length - 1);
    }

    // std::cout << "scoreSum: " << scoreSum << ", length: " << length << '\n';

    return scoreSum / length;
}

namespace RhythmTranscriber
{
    void BeatAnalyzer::set_bpm(float bpm)
    {
        expectedBeatDuration = 60.f / bpm;

        branch.expectedBeatDuration = expectedBeatDuration;

        minBeatDuration = 60.f / (bpm / beatThresholdMultiplier);

        maxBeatDuration = 60.f / (bpm * beatThresholdMultiplier);

        /// Set last note to quarter note (essentially). Maybe this changes in the future.
        if (notesLen > 0)
        {
            notes[notesLen - 1].duration = expectedBeatDuration;
        }
    }
    void BeatAnalyzer::set_depth(unsigned int depth)
    {
        maxDepth = depth;

        branch.length = depth;
    }

    void BeatAnalyzer::get_best_branch_at(unsigned int noteIndex)
    {
        bestBranchScore = 0.f;

        /// In case we internally modified beat branch previously.
        branch.length = maxDepth;

        create_beat_branch(notes + noteIndex, false, 0);

        recentBestBeat = bestBranch.beatBuffer[0];
    }

    void BeatAnalyzer::create_beat_branch(BaseNote *startNote, bool startsOffbeat,
                                          unsigned int depth)
    {
        float minTime = (startNote - startsOffbeat)->timestamp + minBeatDuration;
        float maxTime = startNote->timestamp + maxBeatDuration;

        int startNoteIndex = startNote - notes;

        for (unsigned int i = startNoteIndex; i < notesLen; i++)
        {
            float timestamp = notes[i].timestamp;

            if (timestamp > maxTime)
            {
                break;
            }

            if (i == notesLen - 1)
            {
                notes[i].duration = expectedBeatDuration;

                /// Do we need to check for minTime here?

                /// Avoid length of 0 for BeatData.
                if (i != startNoteIndex)
                {
                    /// Prepare the normal beat data as well as an additional quarter note to
                    /// represent the last note.
                    branch.length = depth + 2;

                    branch.dataBuffer[depth] =
                        BeatData{startNote, i - startNoteIndex, startsOffbeat, false};
                    branch.dataBuffer[depth + 1] = BeatData{notes + notesLen - 1, 1, false, false};

                    eval_branch();
                }

                /// Try again but assuming the last note is not on its own beat.
                branch.length = depth + 1;

                branch.dataBuffer[depth] =
                    BeatData{startNote, i - startNoteIndex + 1, startsOffbeat, true};

                eval_branch();

                branch.length = maxDepth;

                return;
            }

            if (timestamp >= minTime && i != startNoteIndex)
            {
                branch.dataBuffer[depth] =
                    BeatData{startNote, i - startNoteIndex, startsOffbeat, false};

                if (depth + 1 < maxDepth)
                {
                    create_beat_branch(notes + i, false, depth + 1);
                }
                else
                {
                    eval_branch();
                }
            }

            /// TODO: If duration of note is really short, skip the in-between possibility
            /// I think there could be a way to know exactly what duration is too short based on
            /// config variables
            /// NOTE: This won't always be a good idea. For example, in BD 2017 a 5let starting
            /// mid-beat crosses over the beat at some division of 10.
            /// It's a good optimization for non-super-crazy rhythms, though.

            if (notes[i].duration < minBeatBranchOffbeatTime)
                continue;

            if (timestamp >= minTime || notes[i + 1].timestamp >= minTime)
            {
                branch.dataBuffer[depth] =
                    BeatData{startNote, i - startNoteIndex + 1, startsOffbeat, true};

                if (depth + 1 < maxDepth)
                {
                    create_beat_branch(notes + i + 1, true, depth + 1);

                    if (notes[i].duration > maxBeatDuration)
                    {
                        /// Consider the possibility this note makes its beat end offbeat, with the
                        /// next beat starting on the downbeat. This only works because the next
                        /// "beat" is actually multiple beats away, in terms of music.

                        create_beat_branch(notes + i + 1, false, depth + 1);
                    }
                }
                else
                {
                    eval_branch();
                }
            }
        }
    }

    float BeatAnalyzer::calc_branch_score()
    {
        float bpmDeltaScore = 0.f;
        float bpmDistScore = 0.f;
        float avgBeatScore = 0.f;

        float beatScoreSum = 0.f;
        float beatWeightSum = 0.f;

        /// TODO: Maybe have a local BPM score: how well each beat fits with the local BPM.
        /// This is similar to delta score, except look at further beats than only the neighboring
        /// one.

        for (unsigned int i = 0; i < branch.length; i++)
        {
            /// @todo This shouldn't be needed. We already check if the branch is valid before
            /// calling this method I think.
            if (!branch.beat_matches_data_at(i))
            {
                return 0.f;
            }

            float beatScore = 0.f;
            float beatWeight = 0.f;

            float beatDuration = branch.beatBuffer[i].get_duration();
            float baseDuration = beatDuration / branch.beatBuffer[i].division.antecedent;

            /// Dist score
            float durationDistScore =
                1 - (beatDuration < expectedBeatDuration ? beatDuration / expectedBeatDuration
                                                         : expectedBeatDuration / beatDuration);
            /* beatScore += 1.2f * (0.0625f / (durationDistScore * durationDistScore + 0.0625f)); */

            /* std::cout << "durationDistScore: "
                      << std::to_string(
                             (0.0625f / (durationDistScore * durationDistScore + 0.0625f)))
                      << '\n'; */

            beatScore += 1.f * (0.0625f / (durationDistScore * durationDistScore + 0.0625f));
            beatWeight += 1.f;

            /// Beat score
            /* std::cout << "beatScore: " << branch.beatBuffer[i].score << '\n'; */
            beatScore += 1.f * branch.beatBuffer[i].score;
            beatWeight += 1.f;

            if (i > 0)
            {
                /// Delta score
                float durationDeltaScore =
                    1 - (beatDuration < branch.beatBuffer[i - 1].get_duration()
                             ? beatDuration / branch.beatBuffer[i - 1].get_duration()
                             : branch.beatBuffer[i - 1].get_duration() / beatDuration);

                /* std::cout << "deltaScore: "
                          << std::to_string(
                                 (0.025f / (durationDeltaScore * durationDeltaScore + 0.025f)))
                          << '\n'; */

                /* beatScore += 0.7f * (0.01f / (durationDeltaScore * durationDeltaScore + 0.01f));
                beatWeight += 0.7f; */
                beatScore += 0.1f * (0.025f / (durationDeltaScore * durationDeltaScore + 0.025f));
                beatWeight += 0.1f;

                /// Uniformity score
                if ((branch.beatBuffer[i].division.consequent %
                     branch.beatBuffer[i - 1].division.consequent) != 0 &&
                    (branch.beatBuffer[i - 1].division.consequent %
                     branch.beatBuffer[i].division.consequent) != 0)
                {
                    /// If the true change in division should have a very small change in note
                    /// duration, then there should be a noticeable difference in base duration.

                    float prevBaseDuration = branch.beatBuffer[i - 1].get_duration() /
                                             branch.beatBuffer[i - 1].division.consequent;

                    float durationDiff = std::abs(baseDuration - prevBaseDuration);

                    /// Base duration if this beat's divison applied to the previous beat's
                    /// duration.
                    float translatedBaseDuration = branch.beatBuffer[i - 1].get_duration() /
                                                   branch.beatBuffer[i].division.consequent;

                    float expectedDurationDiff =
                        std::abs(translatedBaseDuration - prevBaseDuration);

                    /* beatScore += 0.75f * (1 - 0.0001f / (durationDiff * durationDiff + 0.0001f));
                    beatWeight += 0.75f; */
                    if (durationDiff < 0.01f)
                    {
                        /* std::cout << "neighbor beat duration score: "
                                  << std::to_string(
                                         (0.75f *
                                          (1 - 0.0001f / (durationDiff * durationDiff + 0.0001f))) /
                                         0.75f)
                                  << '\n'; */
                        beatScore +=
                            0.75f * (1 - 0.0001f / (durationDiff * durationDiff + 0.0001f));
                        beatWeight += 0.75f;
                    }
                }
            }
            else if (recentBestBeat.endTime == branch.beatBuffer[0].startTime)
            {
                /// @todo Cleanup, use a single function for this to avoid repetition.

                /// Delta score
                float durationDeltaScore = 1 - (beatDuration < recentBestBeat.get_duration()
                                                    ? beatDuration / recentBestBeat.get_duration()
                                                    : recentBestBeat.get_duration() / beatDuration);

                /* beatScore += 0.7f * (0.01f / (durationDeltaScore * durationDeltaScore + 0.01f));
                beatWeight += 0.7f; */
                beatScore += 0.1f * (0.025f / (durationDeltaScore * durationDeltaScore + 0.025f));
                beatWeight += 0.1f;

                /// Use the recent best beat as the previous beat to this branches first beat.
                if ((branch.beatBuffer[i].division.consequent %
                     recentBestBeat.division.consequent) != 0 &&
                    (recentBestBeat.division.consequent %
                     branch.beatBuffer[i].division.consequent) != 0)
                {
                    /// If the true change in division should have a very small change in note
                    /// duration, then there should be a noticeable difference in base duration.

                    float prevBaseDuration =
                        recentBestBeat.get_duration() / recentBestBeat.division.consequent;

                    float durationDiff = std::abs(baseDuration - prevBaseDuration);

                    /// Base duration if this beat's divison applied to the previous beat's
                    /// duration.
                    float translatedBaseDuration =
                        recentBestBeat.get_duration() / branch.beatBuffer[i].division.consequent;

                    float expectedDurationDiff =
                        std::abs(translatedBaseDuration - prevBaseDuration);

                    if (durationDiff < 0.01f)
                    {
                        beatScore +=
                            0.75f * (1 - 0.0001f / (durationDiff * durationDiff + 0.0001f));
                        beatWeight += 0.75f;
                    }
                }
            }

            beatScoreSum += beatScore;
            beatWeightSum += beatWeight;
        }

        return beatScoreSum / beatWeightSum;
    }
}