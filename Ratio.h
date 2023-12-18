#pragma once

#include <string>

namespace RhythmTranscriber
{
    class Ratio
    {
    public:
        /// @brief The numerator of the ratio.
        unsigned int antecedent = 0;

        /// @brief The denominator of the ratio.
        unsigned int consequent = 0;

        /// @brief Adds a ratio.
        /// @param ratio
        /// @return
        void add(Ratio ratio);

        void simplify();

        Ratio operator-(const Ratio &ratio);

        Ratio operator+(const Ratio &ratio);

        std::string to_string();
    };
}