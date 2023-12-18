#include "Ratio.h"

#include <numeric>

namespace RhythmTranscriber
{
    void Ratio::simplify()
    {
        float gcd = std::gcd(antecedent, consequent);
        antecedent /= gcd;
        consequent /= gcd;
    }

    void Ratio::add(Ratio ratio)
    {
        if (consequent == 0)
        {
            this->antecedent = ratio.antecedent;
            this->consequent = ratio.consequent;
        }
        else if (ratio.consequent == consequent)
        {
            antecedent += ratio.antecedent;
        }
        else
        {
            /// I think `ratio` needs to be in simplest form for the below to work properly. Haven't
            /// had to deal with it yet, but if it becomes an issue `simplify` will have to be used.
            ratio.simplify();

            auto consequentLCM = std::lcm(consequent, ratio.consequent);

            antecedent = antecedent * (consequentLCM / consequent) +
                         ratio.antecedent * (consequentLCM / ratio.consequent);
            consequent = consequentLCM;
        }
    }

    Ratio Ratio::operator-(const Ratio &ratio)
    {
        if (ratio.consequent == consequent)
        {
            return Ratio{antecedent - ratio.antecedent, consequent};
        }
        else
        {
            /// It might be possible that one (or both) ratios need to be simplified for this to
            /// work properly.

            auto consequentLCM = std::lcm(consequent, ratio.consequent);

            return Ratio{antecedent * (consequentLCM / consequent) -
                             ratio.antecedent * (consequentLCM / ratio.consequent),
                         consequentLCM};
        }
    }

    Ratio Ratio::operator+(const Ratio &ratio)
    {
        if (ratio.consequent == consequent)
        {
            return Ratio{antecedent + ratio.antecedent, consequent};
        }
        else
        {
            /// It might be possible that one (or both) ratios need to be simplified for this to
            /// work properly.

            auto consequentLCM = std::lcm(consequent, ratio.consequent);

            return Ratio{antecedent * (consequentLCM / consequent) +
                             ratio.antecedent * (consequentLCM / ratio.consequent),
                         consequentLCM};
        }
    }

    std::string Ratio::to_string()
    {
        std::string str;
        str += std::to_string(antecedent);
        str += '/';
        return str += std::to_string(consequent);
    }
}