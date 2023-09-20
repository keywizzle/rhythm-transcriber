#pragma once

#include "Transcription.h"
#include <string>

namespace RhythmTranscriber
{
    namespace Transcription
    {
        class TranscriptionFile
        {
        public:
            /// @brief Options for writing a transcription file.
            struct WriteOptions
            {
                enum class Type
                {
                    BINARY = 0,
                    JSON,
                };

                enum class Compression
                {
                    /// @brief Every field is included, even default/blank ones
                    NONE = 0,
                    /// @brief Empty/default fields are omitted. See `NoteElement` for what the
                    /// default fields are.
                    OMIT_EMPTY_FIELDS,
                    /// @brief TODO Rename to something more specific
                    FULL,
                };

                /// @brief Type of the file, but this is usually inferred by the file extension.
                Type type = Type::BINARY;

                unsigned int version = 1;

                /// @brief Compression to use. Some options are only applicable to certain file
                /// types.
                Compression compression = Compression::OMIT_EMPTY_FIELDS;
            };

            std::string filePath = "";

            Transcription read(std::string filePath);

            /// @brief Writes the transcription to a file path.
            /// @param filePath Path to the destination file.
            /// @param transcription Transcription to write.
            /// @param options (optional) Options for writing.
            void write(std::string filePath, Transcription &transcription,
                       WriteOptions options = WriteOptions{
                           .type = WriteOptions::Type::BINARY,
                           .version = 1,
                           .compression = WriteOptions::Compression::OMIT_EMPTY_FIELDS,
                       });

        private:
            inline std::string get_extension(std::string filePath)
            {
                return filePath.substr(filePath.find_last_of('.') + 1);
            }
        };
    }

}