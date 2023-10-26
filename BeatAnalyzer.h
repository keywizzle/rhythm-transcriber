#pragma once

#include "Beat.h"
#include "BeatBranch.h"
#include "NoteString.h"

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
        /// level
        std::vector<unsigned int> dataLens;

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

        void create_beat_branch2(BaseNote *startNote, bool startsOffbeat, unsigned int depth);
        void create_beat_branch(BaseNote *startNote, unsigned int depth);
        void create_offbeat_branch(BaseNote *startNote, unsigned int depth);
    };
}