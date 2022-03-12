#pragma once

#include "CRC.hpp"
#include "Common.hpp"
#include "Option.hpp"

#include <memory>
#include <vector>
#include <functional>
#include <cstdint>

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
| Field | Offset   | Length (bytes) | Description                                          |
| ----- | -------- | -------------- | ---------------------------------------------------- |
| SOF   | 0        | 1              | Start of Frame, fixed to 0x05                        |
| DLEN  | 1        | 2              | Length of DATA, little-endian uint16_t               |
| SEQ   | 3        | 1              | Sequence number                                      |
| CRC8  | 4        | 1              | p = 0x31, init = 0xFF, reflect data &  remainder     |
| CMD   | 5        | 2              | Command, little-endian uint16_t                      |
| DATA  | 7        | DLEN           | Data                                                 |
| CRC16 | 7 + DLEN | 2              | p = 0x1021, init = 0xFFFF, reflect data  & remainder |
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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

    using Self = CommandFrame<DataType>;

    RawCommandFrame<DataType> rawFrame;
    byte_t sof;

    inline uint8_t crc8() const
    {
        return CommandFrameUtils::Crc8::compute((byte_t *) &rawFrame, 4);
    }

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

    static Option<DataType> parse(const Vec<byte_t> & data, byte_t sof = 0x05)
    {
        if (data.size() != frameSize() || data[0] != sof) {
            return Option<DataType>::None();
        } else {
            CommandFrame<DataType> frame;
            frame.sof = sof;
            frame.rawFrame = *reinterpret_cast<const RawCommandFrame<DataType>*>(data.data());
            if (frame.validate()) {
                return Option<DataType>(frame.getData());
            } else {
                return Option<DataType>::None();
            }
        }
    }

    RawCommandFrame<DataType> getFrame() const
    {
        return rawFrame;
    }

    bool validate() const
    {
        return (
            rawFrame.sof  == this->sof &&
            this->crc8()  == rawFrame.crc8Value &&
            this->crc16() == rawFrame.crc16Value
        );
    }

    Option<DataType> getData() const
    {
        if (this->validate()) {
            return Option<DataType>(rawFrame.data);
        } else {
            return Option<DataType>::None();
        }
    }

    Vec<byte_t> toBytes() const
    {
        Vec<byte_t> bytes(sizeof(Self));
        const auto* ptr = reinterpret_cast<const byte_t*>(&rawFrame);
        for (size_t i = 0; i < bytes.size(); i++, ptr++) {
            bytes[i] = *ptr;
        }
        return std::move(bytes);
    }
};
