#include "TranscriptionFile.h"

#include <fstream>
#include <inttypes.h>
#include <iostream>
#include <sstream>

namespace RhythmTranscriber
{
    namespace Transcription
    {
        Transcription TranscriptionFile::read(std::string filePath)
        {
            std::ifstream fs(filePath, std::ios::binary);

            if (!fs.is_open())
            {
                // TODO throw exception
                return Transcription{};
            }

            auto ext = get_extension(filePath);
            if (ext == "json")
            {
                /// Read as JSON

                std::ostringstream tmp;
                tmp << fs.rdbuf();

                return Transcription::fromJSON(tmp.str());
            }

            /// Read as binary

            Transcription transcription = Transcription();
            uint32_t version;
            fs.read((char *)&version, sizeof(uint32_t));

            uint32_t notesLen;
            fs.read((char *)&notesLen, sizeof(uint32_t));

            for (unsigned int i = 0; i < notesLen; i++)
            {
                NoteElement note = NoteElement();

                fs.read((char *)&note.timestamp, sizeof(float));

                uint8_t hand;
                fs.read((char *)&hand, sizeof(uint8_t));
                note.hand = (NoteHand)hand;

                uint8_t articulation;
                fs.read((char *)&articulation, sizeof(uint8_t));
                note.articulation = (NoteArticulation)articulation;

                uint8_t placement;
                fs.read((char *)&placement, sizeof(uint8_t));
                note.placement = (NotePlacement)placement;

                uint16_t rhythmNotes;
                uint16_t rhythmBeats;
                fs.read((char *)&rhythmNotes, sizeof(uint16_t));
                fs.read((char *)&rhythmBeats, sizeof(uint16_t));
                note.rhythm = NoteRhythm{rhythmNotes, rhythmBeats};

                uint8_t flam;
                fs.read((char *)&flam, sizeof(uint8_t));
                note.flam = flam;

                uint8_t diddle;
                fs.read((char *)&diddle, sizeof(uint8_t));
                note.diddle = diddle;

                uint8_t buzz;
                fs.read((char *)&buzz, sizeof(uint8_t));
                note.buzz = buzz;

                transcription.notes.push_back(note);
            }

            return transcription;
        }

        void TranscriptionFile::write(std::string filePath, Transcription &transcription,
                                      WriteOptions options)
        {
            /// Rhythm BPM and note duration are omitted from the file since they can be calculated

            std::ofstream fs;
            fs.open(filePath, std::ios::binary | std::ios::out);

            auto ext = get_extension(filePath);
            if (ext == "json" || options.type == WriteOptions::Type::JSON)
            {
                /// Write as JSON

                auto jsonStr = transcription.toJSON(
                    options.compression == WriteOptions::Compression::OMIT_EMPTY_FIELDS ||
                            options.compression == WriteOptions::Compression::FULL
                        ? true
                        : false);

                fs.write(jsonStr.c_str(), jsonStr.length());
            }
            else
            {
                /// Write as binary

                uint32_t version = options.version;
                fs.write((char *)&version, sizeof(uint32_t));

                uint32_t notesLen = transcription.notes.size();
                fs.write((char *)&notesLen, sizeof(uint32_t));

                for (unsigned int i = 0; i < notesLen; i++)
                {
                    auto note = transcription.notes.at(i);

                    fs.write((char *)&note.timestamp, sizeof(float));

                    uint8_t hand = note.hand;
                    fs.write((char *)&hand, sizeof(uint8_t));

                    uint8_t articulation = note.articulation;
                    fs.write((char *)&articulation, sizeof(uint8_t));

                    uint8_t placement = note.placement;
                    fs.write((char *)&placement, sizeof(uint8_t));

                    uint16_t rhythmNotes = note.rhythm.notes;
                    uint16_t rhythmBeats = note.rhythm.beats;
                    fs.write((char *)&rhythmNotes, sizeof(uint16_t));
                    fs.write((char *)&rhythmBeats, sizeof(uint16_t));

                    uint8_t flam = note.flam;
                    fs.write((char *)&flam, sizeof(uint8_t));

                    uint8_t diddle = note.diddle;
                    fs.write((char *)&diddle, sizeof(uint8_t));

                    uint8_t buzz = note.buzz;
                    fs.write((char *)&buzz, sizeof(uint8_t));
                }
            }

            fs.close();
        }
    }
}
