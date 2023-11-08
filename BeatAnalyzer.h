#pragma once

#include "Beat.h"
#include "BeatBranch.h"
#include "Note.h"

#include <iostream>
#include <math.h>

namespace RhythmTranscriber
{
    class BeatAnalyzer
    {
    public:
        BaseNote *notes = nullptr;
        unsigned int notesLen = 0;

        float bpm = 0.f;

        unsigned int maxDepth = 3;

        /// Treating `beatData` as a linear tree, represents the number of sibling nodes per child
        /// level (TODO remove)
        std::vector<unsigned int> dataLens;

        std::vector<Beat> beats;

        BeatBranch bestBranch;
        float bestBranchScore = 0.f;

        void analyze(BaseNote *notes, unsigned int notesLen);

        void try_bpm(float bpm);

        void get_best_branch_at(unsigned int noteIndex);

    private:
        float expectedBeatDuration = 0.f;
        float minBeatDuration = 0.f;
        float maxBeatDuration = 0.f;

        BeatBranch branch;

        void create_beat_branch(BaseNote *startNote, bool startsOffbeat, unsigned int depth);
    };
}