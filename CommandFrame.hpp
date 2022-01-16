#pragma once

#include "CRC.hpp"

#include <memory>
#include <vector>
#include <functional>
#include <cstdint>

#ifndef mov
  #define mov(_x) std::move(_x)
#endif

template <typename Tp, typename Alloc = std::allocator<Tp>>
using Vec = std::vector<Tp, Alloc>;

template <typename Tp>
class Option
{
  private:

    using Consumer = std::function<void(Tp)>;

    bool isPresent = false;
    Tp object;

  public:

    explicit Option(Tp obj) : object(mov(obj)), isPresent(true) { }

    inline Tp except() const
    {
        if (this->isPresent) {
            return object;
        } else {
            throw std::exception();
        }
    }

    inline void ifPresent(const Consumer & consumer) const
    {
        if (this->isPresent) {
            consumer(object);
        }
    }

    inline static Option<Tp> empty()
    {
        return Option<Tp>();
    }
};

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

template <typename DataType>
class CommandFrame
{
  protected:

    using Self = CommandFrame<DataType>;

    const static CRC8<0x31,    0xFF,   0x00>   crc8;
    const static CRC16<0x1021, 0xFFFF, 0x0000> crc16;

    static byte_t sequence;
    RawCommandFrame<DataType> commandFrame;

  public:

    explicit CommandFrame(int commandId, const DataType & data)
    {
        this->commandFrame.sof = 0x05;
        this->commandFrame.dataLength = sizeof(data);
        this->commandFrame.sequence = sequence++;
        this->commandFrame.crc8Value = crc8.compute(&commandFrame, 4);
        this->commandFrame.commandId = commandId;
        this->commandFrame.data = data;
        this->commandFrame.crc16Value = crc16.compute(7 + commandFrame.dataLength);
    }

    static constexpr size_t dataSize()
    {
        return sizeof(DataType);
    }

    static Option<DataType> parse(const Vec<byte_t> & data)
    {
        if (data.size() != dataSize()) {
            return Option<DataType>::empty();
        } else {
            CommandFrame<DataType> frame;
            frame.commandFrame = *reinterpret_cast<RawCommandFrame<DataType>*>(data.data());
            if (frame.isValid()) {
                return Option<DataType>(frame);
            } else {
                return Option<DataType>::empty();
            }
        }
    }

    RawCommandFrame<DataType> getFrame() const
    {
        return commandFrame;
    }

    bool isValid()
    {
        return commandFrame.sof == 0x05;
    }

    Option<DataType> getData() const
    {
        if (!this->isValid()) {
            return commandFrame.data;
        } else {
            return Option<DataType>::empty();
        }
    }

    Vec<byte_t> toBytes() const
    {
        Vec<byte_t> bytes(sizeof(Self));
        auto* ptr = reinterpret_cast<byte_t*>(&commandFrame);
        for (size_t i = 0; i < bytes.size(); i++, ptr++) {
            bytes[i] = *ptr;
        }
        return std::forward<Vec<byte_t>>(bytes);
    }
};
