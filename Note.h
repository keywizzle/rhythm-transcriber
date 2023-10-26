#pragma once

#include "Config.h"
#include "Rhythm.h"
#include <vector>

namespace RhythmTranscriber
{
    struct Note
    {
        float timestamp;

        /// Duration of the note, usually the difference between the next note's timestamp and the
        /// current note's timestamp. If there is no next note, it will be set to `-1`.
        float duration;

        NoteRhythm rhythms[rhythmCount];

        float neighbor_duration_ratios[2] = {-1.f, -1.f};

        Note(float timestamp, float duration);

        NoteRhythm rhythms_for_bpm(float bpm);

        std::string str();
    };

    /// @brief Contains data about an interpretation of a specific note, mainly its rhythm.
    struct NoteInterpretation
    {
        /// @brief Reference to the note.
        Note *note;

        /// @brief Interpreted rhythm for the note.
        NoteRhythm rhythm;

        NoteInterpretation();
        NoteInterpretation(Note *note);

        void assign_bpm(float bpm);
    };

    class NoteString
    {
    public:
        Note *notes = nullptr;

        unsigned int length = 0;

        NoteString();
        NoteString(Note *notes);
        NoteString(Note *notes, unsigned int length);
        NoteString(std::vector<Note> &notes);

        Note operator[](unsigned int index);
        NoteString operator=(const NoteString &noteStr);

        /// @brief Increase the note string by 1 note to the right
        /// @note Ex: `1 [2 3 4] 5` => `1 [2 3 4 5]`
        inline void increaseRight() { length++; }
        /// @brief Increase the note string by 1 note to the left
        /// @note Ex: `1 [2 3 4] 5` => `[1 2 3 4] 5`
        inline void increaseLeft()
        {
            notes--;
            length++;
        }
        /// @brief Decrease the note string by 1 note to the right
        /// @note Ex: `1 [2 3 4] 5` => `1 [2 3] 4 5`
        inline void decreaseRight() { length--; }
        /// @brief Decrease the note string by 1 note to the left
        /// @note Ex: `1 [2 3 4] 5` => `1 2 [3 4] 5`
        inline void decreaseLeft()
        {
            notes++;
            length--;
        }

        void calcStats();
    };

    /// @brief Represents a strict note string where every note should be approximately the same
    /// duration. This means diddles will not be counted.
    class UniformNoteString_ : public NoteString
    {
    public:
        UniformNoteString_ operator=(const UniformNoteString_ &noteStr);

        /// TODO Maybe make this inline
        /// @brief Checks how well a note would fit into this note string, based on duration.
        /// @param note
        /// @return Value between 0-1, 0 being poor fit and 1 being perfect fit
        float check_note(const Note &note);

        float get_score();

        /// @brief Compares the two note strings.
        /// @param noteStr Note string to compare with.
        /// @return
        float compare(const UniformNoteString_ &noteStr);

        /// @brief Searches for notes and increases length to include only notes that are judged to
        /// be the same division.
        /// @param maxLength
        void search(unsigned int maxLength, unsigned int recursiveDepth);
    };
}