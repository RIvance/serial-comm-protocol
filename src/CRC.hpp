/**
 *
 * @file CRC.hpp
 * @author UoN-Lancet
 * @date Aug 15, 2021
 *
 */

#ifndef  CRC_HPP
#define  CRC_HPP

#include <cstdint>
#include <vector>

typedef unsigned char byte_t;

template <
    byte_t width, uint32_t polynomial,
    uint32_t initialXOR, uint32_t finalXOR,
    bool doReflectData = true, bool doReflectRemainder = true
>
class CRC
{
  public:

    CRC() = default;
    
    byte_t reflectByte(byte_t dataByte) const
    {
        if (doReflectData) {
            byte_t reflection = 0x0;
            for (byte_t bit = 0; bit < 8; bit++) {
                if ((dataByte & 1) == 1)
                    reflection |= (byte_t) (1 << (7 - bit));
                dataByte >>= 1;
            }
            return reflection;
        }
        return dataByte;
    }

    uint64_t reflectRemainder(uint64_t data) const
    {
        if (doReflectRemainder) {
            uint64_t reflection = 0;
            auto nbits = (byte_t) (width < 8 ? 8 : width);

            for (byte_t bit = 0; bit < nbits; bit++) {
                if ((data & 1) == 1)
                    reflection |= 1ul << ((nbits - 1) - bit);
                data >>= 1;
            }
            return reflection;
        }
        return data;
    }

    inline uint64_t getTopBit() const
    {
        return (width < 8) ? 1ul << 7 : 1ul << (width - 1);
    }

    inline uint64_t getCrcMask() const
    {
        return (width < 8) ? (1ul << 8) - 1 : (1ul << width) - 1;
    }

    uint64_t compute(byte_t* data, int length) const
    {
        uint64_t crcValue = initialXOR;

        if (width < 8) {
            for (int i = 0; i < length; i++) {
                byte_t currentByte = *(data++);
                byte_t dataByte = reflectByte(currentByte);
                for (byte_t bit = 8; bit > 0; bit--) {
                    crcValue <<= 1;
                    if (((dataByte ^ crcValue) & getTopBit()) > 0)
                        crcValue ^= polynomial;
                    dataByte <<= 1;
                }
            }
        }
        else {
            for (int i = 0; i < length; i++) {
                byte_t currentByte = *(data++);
                byte_t dataByte = reflectByte(currentByte);
                crcValue ^= (uint64_t) (dataByte << (width - 8));

                for (byte_t bit = 8; bit > 0; bit--) {
                    if ((crcValue & getTopBit()) > 0)
                        crcValue = (crcValue << 1) ^ polynomial;
                    else
                        crcValue <<= 1;
                }
            }
        }

        crcValue &= getCrcMask();

        if (width < 8) {
            crcValue = (crcValue << (8 - width));
        }

        crcValue = (reflectRemainder(crcValue) ^ finalXOR) & getCrcMask();
        return crcValue;
    }

    uint64_t compute(const std::vector<byte_t> & bytes) const
    {
        return compute(bytes.data(), bytes.size());
    }
};

template <
    uint32_t polynomial,
    uint32_t initialXOR, uint32_t finalXOR,
    bool doReflectData = true, bool doReflectRemainder = true
> using CRC8 = CRC<
    0x08, polynomial,
    initialXOR, finalXOR,
    doReflectData, doReflectRemainder
>;

template <
    uint32_t polynomial,
    uint32_t initialXOR, uint32_t finalXOR,
    bool doReflectData = true, bool doReflectRemainder = true
> using CRC16 = CRC<
    0x08, polynomial,
    initialXOR, finalXOR,
    doReflectData, doReflectRemainder
>;

#endif