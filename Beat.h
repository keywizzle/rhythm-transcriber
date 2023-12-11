#pragma once

#include "Note.h"

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

        const unsigned int beatDivisionScoreTableSize = 36;
        constexpr std::array<std::array<float, beatDivisionScoreTableSize + 1>,
                             beatDivisionScoreTableSize + 1>
        get_beat_division_score_table()
        {
            std::array<std::array<float, beatDivisionScoreTableSize + 1>,
                       beatDivisionScoreTableSize + 1>
                arr = {};
            for (unsigned int i = 1; i <= beatDivisionScoreTableSize; i++)
            {
                for (unsigned int j = 1; j <= beatDivisionScoreTableSize; j++)
                {
                    /// Only care about the simplified consequent of the ratio.

                    unsigned int ratioGCD = std::gcd(i, j);

                    unsigned int simplifiedDivision = j / ratioGCD;

                    if (j > 1)
                    {
                        if (simplifiedDivision != j)
                        {
                            /// Use score from simplified ratio. It should already exist.
                            arr[i][j] = arr[i / ratioGCD][simplifiedDivision];
                        }
                        else
                        {
                            arr[i][j] = arr[i][1];
                        }
                    }

                    /// Get divisibility score
                    double divisionFreq = 0;
                    for (unsigned int k = 1; k <= simplifiedDivision; k++)
                    {
                        if (simplifiedDivision % k == 0)
                        {
                            divisionFreq++;
                        }
                    }

                    /// 0.35 for divisibility score, 0.65 for punishing larger
                    /// `simplifiedDivision`s. This is to essentially put 1/6 before 1/5, and
                    /// similar divisions.
                    /* arr[j][i] = 0.65 * (divisionFreq / simplifiedDivision) +
                                0.35 * (49.f / (simplifiedDivision * simplifiedDivision + 49)); */

                    /* arr[j][i] = 0.6 * (divisionFreq / simplifiedDivision) +
                                0.4 * (500.f / (simplifiedDivision * simplifiedDivision + 500)); */

                    float ratioScore = 1 - (divisionFreq / simplifiedDivision);

                    /* arr[j][i] =
                        0.65 * (0.75 / (ratioScore * ratioScore * ratioScore * ratioScore + 0.75)) +
                        0.35 * (500.0 / (simplifiedDivision * simplifiedDivision + 500)); */
                    arr[j][i] =
                        0.65 * (0.65 / (ratioScore * ratioScore * ratioScore * ratioScore + 0.65)) +
                        0.35 * (50.0 / (simplifiedDivision + 50));

                    /* arr[j][i] = (0.4f / (ratioScore * ratioScore * ratioScore + 0.4f)); */
                    /* arr[j][i] = 500.0 / (simplifiedDivision * simplifiedDivision + 500); */
                }
            }
            return arr;
        }

        const unsigned int noteDivisionScoreTableSize = 36;
        constexpr std::array<std::array<float, noteDivisionScoreTableSize + 1>,
                             noteDivisionScoreTableSize + 1>
        get_note_division_score_table()
        {
            std::array<std::array<float, noteDivisionScoreTableSize + 1>,
                       noteDivisionScoreTableSize + 1>
                arr = {};

            for (unsigned int i = 1; i <= noteDivisionScoreTableSize; i++)
            {
                for (unsigned int j = 1; j <= noteDivisionScoreTableSize; j++)
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

                    /* arr[j][i] = 0.3 * (divisionFreq / simplifiedDivision) +
                                0.7 * (196.f / (simplifiedDivision * simplifiedDivision + 196)); */

                    float ratioScore = 1 - (divisionFreq / simplifiedDivision);

                    arr[j][i] = 0.25 * (0.4 / (ratioScore * ratioScore + 0.4)) +
                                0.75 * (250.0 / (simplifiedDivision * simplifiedDivision + 250));
                    /* arr[j][i] = (0.4f / (ratioScore * ratioScore + 0.4f)); */
                    /* arr[j][i] = (196.f / (simplifiedDivision * simplifiedDivision + 196)); */
                }
            }

            return arr;
        }

        const unsigned int divisibilityScoreTableSize = 36;
        constexpr std::array<float, divisibilityScoreTableSize + 1> get_divisibility_score_table()
        {
            std::array<float, divisibilityScoreTableSize + 1> arr = {};
            for (unsigned int i = 1; i <= divisibilityScoreTableSize; i++)
            {
                float divisionFreq = 0.f;
                for (unsigned int j = 1; j <= divisibilityScoreTableSize; j++)
                {
                    if (i % j == 0)
                    {
                        divisionFreq++;
                    }
                }
                arr[i] = divisionFreq / i;
                /* arr[i] = (float)i / divisionFreq; */
            }
            return arr;
        }
    }

    /// @brief Contains score information stored in a matrix based around how divisible the beat's
    /// division is at the time a note with a significantly different division is added to the beat.
    constexpr auto beatDivisionScoreTable = get_beat_division_score_table();

    constexpr auto noteDivisionScoreTable = get_note_division_score_table();

    constexpr auto divisibilityScoreTable = get_divisibility_score_table();

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
        /// integer. Specifically, it's the note's duration divided by the beat's base duration.
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

        /* std::vector<NoteRatio> noteRatios = std::vector<NoteRatio>(32, NoteRatio{}); */
        std::vector<NoteRatio> noteRatios;
        /* std::array<NoteRatio, 32> noteRatios; */

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

        /// @brief Assumes `notesLen` is 0.
        /// @param notes
        Beat(BaseNote *notes);

        Beat(BaseNote *notes, unsigned int notesLen);

        /// @brief This is used to create a beat when the beat doesn't end on a downbeat note.
        /// @param notes
        /// @param notesLen
        /// @param beatDuration
        Beat(BaseNote *notes, unsigned int notesLen, float beatDuration);

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
        /// @return `true` if this beat now represents a valid beat, `false` otherwise.
        bool set_note_ratios(unsigned int division);

        /// @brief Determines the most-likely to occur note ratios based on `division`.
        /// @note This is for when the notes are known to not fit perfectly in the beat, and that
        /// the beat should have a tail.
        /// @param division
        /// @return `true` if this beat now represents a valid offbeat, `false` otherwise.
        bool set_offbeat_note_ratios(unsigned int division);

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
        }

        /// @brief Recalculates each `noteRatio`'s `partial` value. Since it's dependent on beat
        /// duration, this should be done after all notes have been added.
        void calc_note_partials();

        float calc_score();

        /// @brief Calculates the beat's `endTime` and `duration` properties based on `startTime`
        /// and the composition of `noteRatios`. Use this if using note timestamps won't work, ie
        /// the beat has a tail.
        void calc_time();

        inline float get_duration() const { return duration; }

        inline bool has_tail() { return division.antecedent > division.consequent; }

        /// @brief Checks if the last note should be within this beat.
        /// @return
        inline bool end_is_valid()
        {
            return (notesLen < 2 || division.antecedent - noteRatios[notesLen - 1].antecedent <
                                        division.consequent);
        }

        std::vector<Beat> get_trailing_beats();

        std::string str();

    private:
        float partialSum = 0.f;

        void transform_note_ratios(float ratio);
    };
}