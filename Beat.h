#pragma once

#include "NoteString.h"

#include <array>
#include <iostream>
#include <math.h>
#include <numeric>
#include <string>
#include <vector>

namespace RhythmTranscriber
{
    namespace
    {
        /// Imagine having constexpr sine in the standard library.

        constexpr double factorial(int n) { return (n == 0) ? 1.0 : n * factorial(n - 1); }
        constexpr double power(double base, int exponent)
        {
            return (exponent == 0) ? 1.0 : base * power(base, exponent - 1);
        }
        constexpr double constexpr_sin_approx(double x, int terms)
        {
            double result = 0.0;
            for (int n = 0; n < terms; ++n)
            {
                int exponent = 2 * n + 1;
                result += (n % 2 == 0 ? 1 : -1) * power(x, exponent) / factorial(exponent);
            }
            return result;
        }
        constexpr double constexpr_sine(double x)
        {
            x = std::fmod(x, 2 * M_PI);         // Normalize angle to [0, 2*pi]
            return constexpr_sin_approx(x, 10); // Adjust the number of terms for desired accuracy
        }

        const unsigned int beatDivisionScoresSize = 36;
        constexpr std::array<std::array<float, beatDivisionScoresSize + 1>,
                             beatDivisionScoresSize + 1>
        get_beat_division_scores()
        {
            std::array<std::array<float, beatDivisionScoresSize + 1>, beatDivisionScoresSize + 1>
                arr = {};
            for (unsigned int i = 1; i <= beatDivisionScoresSize; i++)
            {
                for (unsigned int j = 1; j <= beatDivisionScoresSize; j++)
                {
                    /// Only care about the simplified consequent of the ratio.

                    unsigned int ratioGCD = std::gcd(i, j);

                    unsigned int simplifiedDivision = j / ratioGCD;

                    /// Get divisibility score
                    double divisionFreq = 0;
                    for (unsigned int k = 1; k <= simplifiedDivision; k++)
                    {
                        if (simplifiedDivision % k == 0)
                        {
                            divisionFreq++;
                        }
                    }

                    /// Apply sinusoidal easing (increase values > 0.5, decrease values < 0.5)
                    /// The specific easing function (and whether or not to apply easing at all) is
                    /// still up for thought and may change depending on results.

                    /* arr[j][i] =
                        0.5 *
                        (constexpr_sin_approx(
                             3.14159265359 * ((divisionFreq / simplifiedDivision) - 0.5), 10) +
                         1); */

                    /* arr[j][i] = divisionFreq / simplifiedDivision; */

                    /// Exponential decay easing passing through the point (7, 0.5).

                    /// 0.35 for divisibility score, 0.65 for punishing larger
                    /// `simplifiedDivision`s. This is to essentially put 1/6 before 1/5, and
                    /// similar divisions.
                    arr[j][i] = 0.65 * (divisionFreq / simplifiedDivision) +
                                0.35 * (49.f / (simplifiedDivision * simplifiedDivision + 49));
                }
            }
            return arr;
        }

        const unsigned int noteDivisionScoresSize = 36;
        constexpr std::array<std::array<float, noteDivisionScoresSize + 1>,
                             noteDivisionScoresSize + 1>
        get_note_division_scores()
        {
            std::array<std::array<float, noteDivisionScoresSize + 1>, noteDivisionScoresSize + 1>
                arr = {};

            for (unsigned int i = 1; i <= noteDivisionScoresSize; i++)
            {
                for (unsigned int j = 1; j <= noteDivisionScoresSize; j++)
                {
                    /// Only care about the simplified consequent of the ratio.

                    unsigned int ratioGCD = std::gcd(i, j);

                    unsigned int simplifiedDivision = j / ratioGCD;

                    /// Get divisibility score
                    double divisionFreq = 0;
                    for (unsigned int k = 1; k <= simplifiedDivision; k++)
                    {
                        if (simplifiedDivision % k == 0)
                        {
                            divisionFreq++;
                        }
                    }
                    /// Exponential decay easing passing through the point (7, 0.6).

                    /* arr[j][i] = 0.5 * (divisionFreq / simplifiedDivision) +
                                0.5 * (384.f / (simplifiedDivision * simplifiedDivision + 384)); */
                    arr[j][i] = 0.3 * (divisionFreq / simplifiedDivision) +
                                0.7 * (196.f / (simplifiedDivision * simplifiedDivision + 196));
                    /* arr[j][i] =
                        0.5 * (-384.f / (std::pow(divisionFreq / simplifiedDivision, 1.1f) + 384) +
                               1) +
                        0.5 * (384.f / (simplifiedDivision * simplifiedDivision + 384)); */
                }
            }

            return arr;
        }

        const unsigned int divisibilityScoresSize = 36;
        constexpr std::array<float, divisibilityScoresSize + 1> get_divisibility_scores()
        {
            std::array<float, divisibilityScoresSize + 1> arr = {};
            for (unsigned int i = 1; i <= divisibilityScoresSize; i++)
            {
                float divisionFreq = 0.f;
                for (unsigned int j = 1; j <= divisibilityScoresSize; j++)
                {
                    if (i % j == 0)
                    {
                        divisionFreq++;
                    }
                }
                arr[i] = divisionFreq / i;
            }
            return arr;
        }
    }

    /// @brief Contains score information stored in a matrix based around how divisible the beat's
    /// division is at the time a note with a significantly different division is added to the beat.
    constexpr auto beatDivisionScores = get_beat_division_scores();

    constexpr auto noteDivisionScores = get_note_division_scores();

    constexpr auto divisibilityScores = get_divisibility_scores();

    /// Score weighting for determining beat score. Each of these should be a number between 0 and 1
    /// that all sum to 1.

    extern float divisionWeight;
    extern float noteWeight;
    extern float distWeight;

    class BaseRatio
    {
    public:
        unsigned int antecedent = 0;
        unsigned int consequent = 0;

        /// @brief Adds a ratio.
        /// @param ratio
        /// @return
        void add(const BaseRatio ratio);

        BaseRatio &operator+=(const BaseRatio ratio);

        void simplify();
    };

    class NoteRatio : public BaseRatio
    {
    public:
        /// @brief A context-dependent and more accurate form of `antecedent` if it wasn't an
        /// integer. Specifically, it's the note's duration divided by the beat duration.
        /// @note The reason such a simple calculation is stored here is because it's done
        /// automatically when note ratios are set, which can be reused when calculating score.
        /// @todo see if it's faster to just recalculate when getting score than having to set/get)
        float partial = 0.f;
    };

    class Beat
    {
    public:
        BaseRatio division = BaseRatio{0, 0};

        BaseNote *notes = nullptr;

        /// @brief Number of notes in the beat. This does not include the downbeat of the next beat.
        unsigned int notesLen = 0;

        /// TODO: Having this be stack allocated will provide a HUGE performance boost. Use
        /// something similar to SSO.
        std::vector<NoteRatio> noteRatios = std::vector<NoteRatio>(8, NoteRatio{});
        /* NoteRatio noteRatios[8] = {NoteRatio{}, NoteRatio{}, NoteRatio{}, NoteRatio{},
                                   NoteRatio{}, NoteRatio{}, NoteRatio{}, NoteRatio{}}; */

        /// @brief Offest at the beginning of the beat until the first note. This is non-zero when
        /// the previous beat has a note that extends past the full beat's division.
        BaseRatio offset = BaseRatio{0, 0};

        /// @brief Duration of the beat, total time taken up by notes.
        float duration = 0.f;

        /// @brief Timestamp of the start of the beat. This will not always necessarily be the first
        /// note's timestamp.
        float startTime = 0.f;

        /// @brief Timestamp of the end of the beat. This will not always necessarily be the last
        /// note's timestamp.
        float endTime = 0.f;

        /* /// @brief Tail at the end of the beat. This is usually just the difference between the
        /// beat's division consequent and antecedent.
        BaseRatio tail = BaseRatio{0, 0}; */

        /// @brief Average score of each note based on how far the note's chosen division is to the
        /// note's actual duration.
        float distScore = 0.f;

        /// @brief Represents how divisible the beat's division is at the time a note with a
        /// significantly different division is added.
        /// @note Example: 3/6 + 1/6 + 1/6 has a better score than 1/10 + 2/10 + 4/10 + 3/10.
        float divisionScore = 0.f;

        /// @brief Represents how divisible each simplified note's division is.
        float noteScore = 0.f;

        /// @brief A combination of the above scores weighted to give an overall score.
        float score = 0.f;

        Beat();

        Beat(BaseNote *notes, unsigned int notesLen);

        Beat(BaseNote *notes, unsigned int notesLen, unsigned int division);

        inline void init_notes(BaseNote *notes, unsigned int notesLen)
        {
            /// TODO: Maybe put timestamp and duration along with `NoteRatio` to maybe make better
            /// use of the cache.

            this->notes = notes;
            this->notesLen = notesLen;

            /// Set start/end time, assuming all notes fit perfectly within the beat.
            startTime = notes->timestamp;
            /// Hopefully this doesn't cause any errors.
            endTime = (notes + notesLen)->timestamp;

            duration = endTime - startTime;

            if (noteRatios.size() < notesLen)
            {
                noteRatios.resize(notesLen, NoteRatio{});
            }
        }

        inline void init_division(unsigned int division)
        {
            this->division = BaseRatio{0, division};

            for (unsigned int i = 0; i < notesLen; i++)
            {
                /* noteRatios.at(i) = NoteRatio{0, division, 0.f}; */
                noteRatios[i] = NoteRatio{0, division, 0.f};
            }
        }

        /// @brief Sets the offset of this beat.
        /// @note Start/end times are not adjusted. (maybe TODO..?)
        /// @param antecedent
        /// @param consequent
        inline void set_offset(unsigned int antecedent, unsigned int consequent)
        {
            offset.antecedent = antecedent;
            offset.consequent = consequent;

            division.antecedent = antecedent;
            division.consequent = consequent;
        }

        /// @brief Creates the next beat based on this beat's notes and division. Automatically sets
        /// notes and offset while initializing `notesLen` to 0.
        Beat create_next();

        /// @brief
        /// @param noteRatio
        void add_note(NoteRatio noteRatio);

        /// @brief Determines the most-likely to occur note ratios based on `division`.
        /// @note Only use this when constraints have been set (`notes` and `notesLen`). This
        /// assumes all notes should fit perfectly within the beat.
        /// @param division
        void set_note_ratios(unsigned int division);

        /// @brief Determines the most-likely to occur note ratios based on `division` and
        /// `baseDuration`.
        /// @note Only use this when constraints have been set (`notes` and `notesLen`).
        /// @param division Division of the beat.
        /// @param baseDuration Duration (in seconds) of `beat duration / division`.
        inline void set_note_ratios(unsigned int division, float baseDuration)
        {
            /// Reset division stuff
            this->division.antecedent = 0;
            partialSum = 0.f;

            for (unsigned int i = 0; i < notesLen; i++)
            {
                float noteDivision = (notes + i)->duration / baseDuration;

                /// Multiplier should be at least 1.
                unsigned int multiplier = std::round(noteDivision) + (noteDivision < 0.5f);

                noteRatios[i] = NoteRatio{multiplier, division, noteDivision};

                this->division.antecedent += multiplier;

                partialSum += noteDivision > 1.f ? noteDivision : 1.f;
            }
            /* for (unsigned int i = 0; i < notesLen; i++)
            {
                std::cout << "notes: " << noteRatios.at(i).antecedent << "/"
                          << noteRatios.at(i).consequent << ", ";
            }
            std::cout << '\n';  */
        }

        /// @brief Recalculates each `noteRatio`'s `partial` value. Since it's dependent on beat
        /// duration, this should be done after all notes have been added.
        void calc_note_partials();

        float calc_score();

        void calc_end_time();

        inline float get_duration() const { return duration; }

        std::vector<Beat> get_trailing_beats();

        std::string str();

    private:
        float partialSum = 0.f;

        void transform_note_ratios(float ratio);
    };
    /* struct BeatDivision
    {
        BeatDivisionValue numerator = 0;
        BeatDivisionValue denominator = 0;

        float duration = 0;

        operator bool() const;
    };

    class Beat
    {
    public:
        /// @brief Timestamp of the start of the beat.
        float timestamp;

        /// @brief Duration of the beat (TODO: Isn't this the same as `division.duration`?).
        float duration;

        /// @brief If set to nonzero, the beat will add notes that have a division denominator as a
        /// factor of `forceDivision`. If a note is added whose estimated rhythm does not follow
        /// this criteria, the note's rhythm interpretation will be set to the next matching rhythm
        /// that does follow the criteria.
        BeatDivisionValue forceDivision = 0;

        /// @brief Division of the beat. Basically, how much of the beat is occupied by notes. For
        /// example, a division of 1/4 means 1/4th of the beat (equivalent to one sixteenth note)
        /// from the start of the beat is taken.
        BeatDivision division;

        /// @brief Offest at the beginning of the beat until the first note. This is a
        BeatDivision offset;

        /// @brief Beat division of a note that extends outside the end of the beat. This can be
        /// used to define the next note's `offset`.
        BeatDivision tail;

        /// TODO: Maybe we change this to a stack array (std::array) with a size of like 16 (or
        /// maybe defined in config) if performance seems to be an issue.
        std::vector<NoteInterpretation> notes;

        Beat(float timestamp);
        Beat(float timestamp, BeatDivision offset);
        Beat(float timestamp, BeatDivisionValue forceDivision);
        Beat(float timestamp, BeatDivision offset, BeatDivisionValue forceDivision);


        void addNoteInterpretation(NoteInterpretation &note);

        void addNote(float timestamp, float duration, NoteRhythm rhythm);

        std::string str(bool compact = false);
    }; */
}