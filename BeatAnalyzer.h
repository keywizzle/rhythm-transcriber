#pragma once

#include "Beat.h"
#include "BeatBranch.h"
#include "Note.h"

#include <iostream>
#include <math.h>

namespace RhythmTranscriber
{
    extern unsigned int iters;

    class BeatAnalyzer
    {
    public:
        BaseNote *notes = nullptr;
        unsigned int notesLen = 0;

        float bpm = 0.f;

        unsigned int maxDepth = 3;

        /// @todo Move to private, it's public right now for testing purposes.
        BeatBranch branch;

        BeatBranch bestBranch;
        float bestBranchScore = 0.f;

        void set_bpm(float bpm);

        void set_depth(unsigned int depth);

        void get_best_branch_at(unsigned int noteIndex);

        float calc_branch_score();

    private:
        float expectedBeatDuration = 0.f;
        float minBeatDuration = 0.f;
        float maxBeatDuration = 0.f;

        /// @brief After evaluating the best branch, this will be set to the branches first beat.
        /// Further branch evaluations can use this as the preceding beat to the first beat of the
        /// branch for scoring.
        Beat recentBestBeat;

        void create_beat_branch(BaseNote *startNote, bool startsOffbeat, unsigned int depth);

        inline void eval_branch()
        {
            if (branch.create_beats())
            {
                float branchScore = calc_branch_score();

                if (branchScore > bestBranchScore)
                {
                    bestBranchScore = branchScore;
                    bestBranch = branch;
                }
            }
        }
    };
}