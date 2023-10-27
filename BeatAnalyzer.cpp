#include "BeatAnalyzer.h"
#include "Config.h"

#include "utils.h"
#include <iomanip>
#include <iostream>
#include <math.h>

const unsigned int expectedInterps = 16;

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
    void BeatAnalyzer::analyze(BaseNote *notes, unsigned int notesLen)
    {
        this->notes = notes;
        this->notesLen = notesLen;

        try_bpm(bpm);
    }

    void BeatAnalyzer::try_bpm(float bpm)
    {
        /// For every note, check how close a note is to the note timestamp + beat duration.
        /// Exclude any notes where the closest note is outside minBpm/maxBpm range (> 1 quarter
        /// note).

        this->bpm = bpm;

        expectedBeatDuration = 60.f / bpm;
        minBeatDuration = 60.f / (bpm / beatThresholdMultiplier);
        maxBeatDuration = 60.f / (bpm * beatThresholdMultiplier);

        dataLens = std::vector<unsigned int>(maxDepth, 0);

        branch.length = maxDepth;

        create_beat_branch2(notes, false, maxDepth);

        /* std::cout << "best branch: \n";
        for (unsigned int i = 0; i < bestBranch.length; i++)
        {
            std::cout << bestBranch.beatBuffer[i].str() << '\n';
        } */

        /* for (unsigned int i = 0; i < maxDepth; i++)
        {
            std::cout << "nodes at depth " << i << ": " << dataLens[i] << '\n';
        } */

        return;

        /// BD 2017
        /* branch.dataBuffer[0] = BeatData{notes, 2, false, true};
        branch.dataBuffer[1] = BeatData{notes + 2, 2, true, true};
        branch.dataBuffer[2] = BeatData{notes + 4, 3, true, true};
        branch.dataBuffer[3] = BeatData{notes + 2, 3, true, false};
        branch.length = 4; */

        /// Rhythm X 2022

        /* int offset = 19;
        branch.dataBuffer[0] = BeatData{notes + offset, 4, false, false};
        branch.dataBuffer[1] = BeatData{notes + offset + 4, 5, false, true};
        branch.dataBuffer[2] = BeatData{notes + offset + 9, 1, true, false};
        branch.length = 3; */
        /* int offset = 19;
        branch.dataBuffer[0] = BeatData{notes + offset, 4, false, false};
        branch.dataBuffer[1] = BeatData{notes + offset + 4, 5, false, true};
        branch.dataBuffer[2] = BeatData{notes + offset + 9, 1, true, false};
        branch.dataBuffer[3] = BeatData{notes + offset + 10, 3, false, false};
        branch.length = 4; */
        /* int offset = 19;
        branch.dataBuffer[0] = BeatData{notes + offset, 4, false, false};
        branch.dataBuffer[1] = BeatData{notes + offset + 4, 4, false, false};
        branch.dataBuffer[2] = BeatData{notes + offset + 8, 2, false, false};
        branch.dataBuffer[3] = BeatData{notes + offset + 10, 3, false, false};
        branch.length = 4; */

        /* int offset = 29;
        branch.dataBuffer[0] = BeatData{notes + offset, 3, false, false};
        branch.dataBuffer[1] = BeatData{notes + offset + 3, 3, false, false};
        branch.dataBuffer[2] = BeatData{notes + offset + 6, 3, false, false};
        branch.dataBuffer[3] = BeatData{notes + offset + 9, 5, false, false};
        branch.length = 4; */

        /* branch.dataBuffer[0] = BeatData{notes, 4, false, false};
        branch.dataBuffer[1] = BeatData{notes + 4, 5, false, false};
        branch.dataBuffer[2] = BeatData{notes + 9, 5, false, false};
        branch.dataBuffer[3] = BeatData{notes + 14, 6, false, false};
        branch.length = 4; */

        /* branch.dataBuffer[0] = BeatData{notes, 6, false, true};
        branch.dataBuffer[1] = BeatData{notes + 6, 3, true, false};
        branch.length = 2; */

        StartTimer();
        for (unsigned int i = 0; i < 1; i++)
        {
            branch.create_beats();
        }
        StopTimer();

        for (unsigned int i = 0; i < branch.length; i++)
        {
            std::cout << "beat: " << branch.beatBuffer[i].str() << '\n';
        }
    }

    void BeatAnalyzer::get_best_branch_at(unsigned int noteIndex)
    {
        expectedBeatDuration = 60.f / bpm;
        minBeatDuration = 60.f / (bpm / beatThresholdMultiplier);
        maxBeatDuration = 60.f / (bpm * beatThresholdMultiplier);

        dataLens = std::vector<unsigned int>(maxDepth, 0);

        branch.length = maxDepth;

        create_beat_branch2(notes + noteIndex, false, maxDepth);

        /* for (unsigned int i = 0; i < maxDepth; i++)
        {
            std::cout << "nodes at depth " << i << ": " << dataLens[i] << '\n';
        } */
    }

    void BeatAnalyzer::create_beat_branch(BaseNote *startNote, unsigned int depth)
    {
        /// TODO:
        /// - Maybe try using one function for onbeat/offbeat but with more args to specify shit.
        ///   Maybe we can pass a reference to some BeatData which gives enough information to the
        ///   function.
        /// - Consider using iterators, using indexes makes it a bit weird when you have to convert
        ///   from index->iterator and back all the time
        /// - Optimize!!! I think this is already pretty fast for what it does but it needs to be
        ///   optimized regardless.
        ///     - Remove dataLens/siblings bullshit
        ///     - Avoid extra conditional checks for offbeat branching (see comment)
        ///     - Use stack-allocated branch

        float minTime = startNote->timestamp + minBeatDuration;
        float maxTime = startNote->timestamp + maxBeatDuration;

        /* std::cout << "(onbeat) startNote: " << startNote->timestamp << ", minTime: " << minTime
                  << ", maxTime: " << maxTime << '\n'; */

        int startNoteIndex = startNote - notes;

        unsigned int siblings = 0;

        for (unsigned int i = startNoteIndex; i < notesLen - 1; i++)
        {
            float timestamp = (notes + i)->timestamp;

            if (timestamp > maxTime)
            {
                break;
            }

            if (timestamp >= minTime)
            {
                branch.dataBuffer[this->maxDepth - depth] =
                    BeatData{startNote, i - startNoteIndex, false, false};

                siblings++;

                if (depth > 1)
                {
                    create_beat_branch(notes + i, depth - 1);
                }
                else
                {
                    branch.create_beats();
                }

                /// if (nextTimestamp <= maxTime)
                ///     create offbeat branch
            }
            /// else if (nextTimestamp >= minTime && nextTimestamp <= maxTime)
            ///     create offbeat branch
            /// We can also probably store data for next loop iteration that nextTimestamp is
            /// valid/invalid

            /// TODO: If duration of note is really short, skip the in-between possibility
            /// I think there could be a way to know exactly what duration is too short based on
            /// config variables
            if ((notes + i)->duration < minBeatBranchOffsetTime)
                continue;

            if (timestamp >= minTime && timestamp <= maxTime)
            {
                branch.dataBuffer[this->maxDepth - depth] =
                    BeatData{startNote, i - startNoteIndex + 1, false, true};

                siblings++;

                if (depth > 1)
                {
                    create_offbeat_branch(notes + i + 1, depth - 1);
                }
                else
                {
                    branch.create_beats();
                }
            }
        }

        if (siblings == 0)
        {
            /// This must mean that the second note was outside timestamp bounds, meaning the
            /// next note definitely lasts longer than a quarter note.
            // beats between timestamps: (second.timestamp - first.timestamp) / beatDuration
            // Only count towards beatLen when the next note is reached
        }

        dataLens.at(this->maxDepth - depth) += siblings;
    }

    void BeatAnalyzer::create_offbeat_branch(BaseNote *startNote, unsigned int depth)
    {
        float minTime = (startNote - 1)->timestamp + minBeatDuration;
        float maxTime = startNote->timestamp + maxBeatDuration;

        /* std::cout << "(offbeat) startNote: " << startNote->timestamp << ", minTime: " << minTime
                  << ", maxTime: " << maxTime << '\n'; */

        int startNoteIndex = startNote - notes;

        unsigned int siblings = 0;

        for (unsigned int i = startNoteIndex; i < notesLen - 1; i++)
        {
            float timestamp = (notes + i)->timestamp;

            if (timestamp > maxTime)
            {
                break;
            }

            if (timestamp >= minTime)
            {
                branch.dataBuffer[this->maxDepth - depth] =
                    BeatData{startNote, i - startNoteIndex, true, false};

                siblings++;

                if (depth > 1)
                {
                    create_beat_branch(notes + i, depth - 1);
                }
                else
                {
                    branch.create_beats();
                }

                /// if (nextTimestamp <= maxTime)
                ///     create offbeat branch
            }
            /// else if (nextTimestamp >= minTime && nextTimestamp <= maxTime)
            ///     create offbeat branch
            /// We can also probably store data for next loop iteration that nextTimestamp is
            /// valid/invalid

            /// TODO: If duration of note is really short, skip the in-between possibility
            /// I think there could be a way to know exactly what duration is too short based on
            /// config variables
            if ((notes + i)->duration < minBeatBranchOffsetTime)
                continue;

            if (timestamp >= minTime && timestamp <= maxTime)
            {
                /// The chances the next floating point value greater than `timestamp` is out of
                /// range of `minTime` and `maxTime` is basically impossible, so we create offbeat
                /// branch.

                branch.dataBuffer[this->maxDepth - depth] =
                    BeatData{startNote, i - startNoteIndex + 1, true, true};

                siblings++;

                if (depth > 1)
                {
                    create_offbeat_branch(notes + i + 1, depth - 1);
                }
                else
                {
                    branch.create_beats();
                }
            }
        }

        if (siblings == 0)
        {
            /// This must mean that the second note was outside timestamp bounds, meaning the
            /// next note definitely lasts longer than a quarter note.
            // beats between timestamps: (second.timestamp - first.timestamp) / beatDuration
            // Only count towards beatLen when the next note is reached
        }

        dataLens.at(this->maxDepth - depth) += siblings;
    }

    void BeatAnalyzer::create_beat_branch2(BaseNote *startNote, bool startsOffbeat,
                                           unsigned int depth)
    {
        float minTime = (startNote - startsOffbeat)->timestamp + minBeatDuration;
        float maxTime = startNote->timestamp + maxBeatDuration;

        int startNoteIndex = startNote - notes;

        unsigned int siblings = 0;

        for (unsigned int i = startNoteIndex; i < notesLen - 1; i++)
        {
            float timestamp = (notes + i)->timestamp;

            if (timestamp > maxTime)
            {
                break;
            }

            if (timestamp >= minTime)
            {
                branch.dataBuffer[this->maxDepth - depth] =
                    BeatData{startNote, i - startNoteIndex, startsOffbeat, false};

                siblings++;

                if (depth > 1)
                {
                    create_beat_branch2(notes + i, false, depth - 1);
                }
                else
                {
                    branch.create_beats();

                    float branchScore = branch.calc_score(expectedBeatDuration);

                    if (branchScore > bestBranchScore)
                    {
                        /* std::cout << "new best branch: " << branch.str() << '\n';
                        std::cout << "score: " << branchScore << '\n'; */
                        bestBranchScore = branchScore;
                        bestBranch = branch;
                    }
                }

                /// if (nextTimestamp <= maxTime)
                ///     create offbeat branch
            }
            /// else if (nextTimestamp >= minTime && nextTimestamp <= maxTime)
            ///     create offbeat branch
            /// We can also probably store data for next loop iteration that nextTimestamp is
            /// valid/invalid

            /// TODO: If duration of note is really short, skip the in-between possibility
            /// I think there could be a way to know exactly what duration is too short based on
            /// config variables
            /// NOTE: This won't always be a good idea. For example, in BD 2017 a 5let starting
            /// mid-beat crosses over the beat at some division of 10.
            /// It's a good optimization for non-super-crazy rhythms, though.

            if ((notes + i)->duration < minBeatBranchOffsetTime)
                continue;

            float nextTimestamp = (notes + i + 1)->timestamp;

            /// Already checking if timestamp > maxTime above
            if (timestamp >= minTime || nextTimestamp > maxTime)
            {
                /// It's possible this timestamp is less than `minTime` with the next timestamp
                /// being over `maxTime`, in which case there is some point between this note and
                /// the next in which a beat is between `minTime` and `maxTime`.

                branch.dataBuffer[this->maxDepth - depth] =
                    BeatData{startNote, i - startNoteIndex + 1, startsOffbeat, true};

                siblings++;

                if (depth > 1)
                {
                    create_beat_branch2(notes + i + 1, true, depth - 1);
                }
                else
                {
                    branch.create_beats();

                    float branchScore = branch.calc_score(expectedBeatDuration);

                    if (branchScore > bestBranchScore)
                    {
                        bestBranchScore = branchScore;
                        bestBranch = branch;
                    }
                }
            }

            /* if (timestamp >= minTime && timestamp <= maxTime)
            {
                branch.dataBuffer[this->maxDepth - depth] =
                    BeatData{startNote, i - startNoteIndex + 1, startsOffbeat, true};

                siblings++;

                if (depth > 1)
                {
                    create_beat_branch2(notes + i + 1, true, depth - 1);
                }
                else
                {
                    branch.create_beats();

                    float branchScore = branch.calc_score(expectedBeatDuration);

                    if (branchScore > bestBranchScore)
                    {
                        bestBranchScore = branchScore;
                        bestBranch = branch;
                    }
                }
            } */
        }

        if (siblings == 0)
        {
            /// This must mean that the second note was outside timestamp bounds, meaning the
            /// next note definitely lasts longer than a quarter note.
            // beats between timestamps: (second.timestamp - first.timestamp) / beatDuration
            // Only count towards beatLen when the next note is reached
        }

        dataLens.at(this->maxDepth - depth) += siblings;
    }

}