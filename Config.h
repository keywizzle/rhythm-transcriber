#pragma once

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
    const float maxBPM = 300.f;
    /// @brief Minimum BPM to consider
    const float minBPM = 60.f;

    /// @brief Number of rhythmsCalculated by finding number of unique multiples of notes and beats;
    /// for 16 and 4 it's 43
    const unsigned int rhythmCount = 43;

    /// NoteStrings

    /// @brief Max level of recursion for UniformNoteString to use for determining a note string.
    const unsigned int maxRecursiveDepth = 5;
}