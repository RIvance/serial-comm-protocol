#pragma once

#include "CRC.hpp"

#include <memory>
#include <vector>
#include <functional>
#include <cstdint>

#if __cplusplus >= 201703L
  #include <optional>
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
| Field | Offset   | Length (bytes) | Description                                           |
| ----- | -------- | -------------- | ----------------------------------------------------- |
| SOF   | 0        | 1              | Start of Frame, fixed to 0x05                         |
| DLEN  | 1        | 2              | Length of DATA, little-endian uint16_t                |
| SEQ   | 3        | 1              | Sequence number                                       |
| CRC8  | 4        | 1              | p = 0x31, init = 0xFF, reflect data and remainder     |
| CMD   | 5        | 2              | Command, little-endian uint16_t                       |
| DATA  | 7        | DLEN           | Raw data                                              |
| CRC16 | 7 + DLEN | 2              | p = 0x1021, init = 0xFFFF, reflect data and remainder |
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if __cplusplus >= 201703L
  namespace serial::command
#else
  namespace serial { namespace command
#endif
{
    #pragma pack(push, 1)

    template <typename DataType>
    struct RawCommandFrame
    {
        byte_t   sof;
        uint16_t dataLength;
        byte_t   sequence;
        byte_t   crc8Value;
        uint16_t commandId;
        DataType data;
        uint16_t crc16Value;
    };

    #pragma pack(pop)

    namespace CommandFrameUtils
    {
        using Crc8  = CRC8<0x31,    0xFF,   0x00>;
        using Crc16 = CRC16<0x1021, 0xFFFF, 0x0000>;
        static byte_t sequence;
    }

    template <typename DataType>
    class CommandFrame
    {
      protected:

        using RawFrame = RawCommandFrame<DataType>;

        RawFrame rawFrame;
        byte_t sof = 0x00;

        [[nodiscard]]
        inline uint8_t crc8() const
        {
            return CommandFrameUtils::Crc8::compute((byte_t *) &rawFrame, 4);
        }

        [[nodiscard]]
        inline uint16_t crc16() const
        {
            return CommandFrameUtils::Crc16::compute((byte_t *) &rawFrame, 7 + rawFrame.dataLength);
        }

        CommandFrame() = default;

      public:

        explicit CommandFrame(int commandId, const DataType & data, byte_t sof = 0x05)
        {
            this->sof = sof;
            this->rawFrame.sof = sof;
            this->rawFrame.dataLength = sizeof(data);
            this->rawFrame.sequence = CommandFrameUtils::sequence++;
            this->rawFrame.crc8Value = crc8();
            this->rawFrame.commandId = commandId;
            this->rawFrame.data = data;
            this->rawFrame.crc16Value = crc16();
        }

        static constexpr size_t dataSize()
        {
            return sizeof(DataType);
        }

        static constexpr size_t frameSize()
        {
            return sizeof(RawCommandFrame<DataType>);
        }

      #if __cplusplus >= 201703L

        static std::optional<DataType> parse(const std::vector<byte_t> & frameData, byte_t sof = 0x05)
        {
            if (frameData.size() != frameSize() || frameData[0] != sof) {
                return std::nullopt;
            } else {
                CommandFrame<DataType> frame;
                frame.sof = sof;
                frame.rawFrame = *reinterpret_cast<const RawCommandFrame<DataType>*>(frameData.data());
                if (frame.validate()) {
                    return std::optional<DataType>(frame.getData());
                } else {
                    return std::nullopt;
                }
            }
        }

      #else

        static bool parse(const std::vector<byte_t> & frameData, DataType & output, byte_t sof = 0x05)
        {
            if (frameData.size() != frameSize() || frameData[0] != sof) {
                return false;
            } else {
                CommandFrame<DataType> frame;
                frame.sof = sof;
                frame.rawFrame = *reinterpret_cast<const RawCommandFrame<DataType>*>(frameData.data());
                if (frame.validate()) {
                    output = frame.getData();
                    return true;
                } else {
                    return false;
                }
            }
        }

      #endif

        RawCommandFrame<DataType> getFrame() const
        {
            return rawFrame;
        }

        [[nodiscard]]
        bool validate() const
        {
            return (
                rawFrame.sof  == this->sof &&
                this->crc8()  == rawFrame.crc8Value &&
                this->crc16() == rawFrame.crc16Value
            );
        }

      #if __cplusplus >= 201703L

        std::optional<DataType> getData() const
        {
            if (this->validate()) {
                return std::optional<DataType>(rawFrame.data);
            } else {
                return std::nullopt;
            }
        }

      #endif

        bool getData(DataType & output) const
        {
            if (this->validate()) {
                output = rawFrame.data;
                return true;
            } else {
                return false;
            }
        }

        [[nodiscard]]
        std::vector<byte_t> toBytes() const
        {
            std::vector<byte_t> bytes(sizeof(RawFrame));
            const auto* ptr = reinterpret_cast<const byte_t*>(&rawFrame);
            for (size_t i = 0; i < bytes.size(); i++, ptr++) {
                bytes[i] = *ptr;
            }
            return bytes;
        }
    };
}

#if __cplusplus < 201703L
}
#endif
