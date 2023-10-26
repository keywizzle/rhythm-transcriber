#pragma once

#include <array>

// Contains config values

namespace RhythmTranscriber
{
    /// TODO: Allow customization of these values. Right now they're declared const but there might
    /// be a way to calculate it compile time instead of hardcoding

    /// @brief Maximum number of notes to allow for a rhythm's division
    const unsigned char maxNotes = 16;
    /// @brief Maximum number of beats to allow for a rhythm's division
    const unsigned char maxBeats = 4;

    /// @brief Maximum BPM to consider
    const float maxBpm = 300.f;
    /// @brief Minimum BPM to consider
    const float minBpm = 60.f;

    /// @brief Number of rhythmsCalculated by finding number of unique multiples of notes and beats;
    /// for 16 and 4 it's 43
    const unsigned int rhythmCount = 43;

    ///

    /// NoteStrings

    /// @brief Maximum ratio between two note's durations to be considered uniform with each other.
    const float uniformRatioThreshold = 1.5f;

    const float maxNoteRatio = 9.f;

    /// @brief Max level of recursion for UniformNoteString to use for determining a note string.
    const unsigned int maxRecursiveDepth = 5;

    const float beatThresholdMultiplier = 0.7f;

    /// @brief When creating a new beat branch, this is the minimum amount of time that must pass
    /// since the previous beat for one to be created.
    const float minBeatBranchOffsetTime = 0.09f;
}