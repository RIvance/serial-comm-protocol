
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
  private:

    CRC() = default;

  public:

    class CrcIterator
    {
      private:

        uint64_t value = initialXOR;

        static byte_t reflectByte(byte_t dataByte)
        {
            if (doReflectData) {
                byte_t reflection = 0x0;
                for (byte_t bit = 0; bit < 8; bit++) {
                    if ((dataByte & 1) == 1) {
                        reflection |= (byte_t) (1 << (7 - bit));
                    }
                    dataByte >>= 1;
                }
                return reflection;
            }
            return dataByte;
        }

        static uint64_t reflectRemainder(uint64_t data)
        {
            if (doReflectRemainder) {
                uint64_t reflection = 0;
                auto nbits = (byte_t) (width < 8 ? 8 : width);

                for (byte_t bit = 0; bit < nbits; bit++) {
                    if ((data & 1) == 1) {
                        reflection |= 1ul << ((nbits - 1) - bit);
                    }
                    data >>= 1;
                }
                return reflection;
            }
            return data;
        }

        static inline uint64_t getTopBit()
        {
            return (width < 8) ? 1ul << 7 : 1ul << (width - 1);
        }

        static inline uint64_t getCrcMask()
        {
            return (width < 8) ? (1ul << 8) - 1 : (1ul << width) - 1;
        }

      public:

        CrcIterator() = default;

        void computeNext(byte_t byte)
        {
            if (width < 8) {
                byte_t dataByte = reflectByte(byte);
                for (byte_t bit = 8; bit > 0; bit--) {
                    value <<= 1;
                    if (((dataByte ^ value) & getTopBit()) > 0) {
                        value ^= polynomial;
                    }
                    dataByte <<= 1;
                }
            } else {
                byte_t dataByte = reflectByte(byte);
                value ^= (uint64_t) (dataByte << (width - 8));
                for (byte_t bit = 8; bit > 0; bit--) {
                    if ((value & getTopBit()) > 0) {
                        value = (value << 1) ^ polynomial;
                    } else {
                        value <<= 1;
                    }
                }
            }
        }

        uint64_t getValue()
        {
            value &= getCrcMask();

            if (width < 8) {
                value = (value << (8 - width));
            }

            value = (reflectRemainder(value) ^ finalXOR) & getCrcMask();
            return value;
        }

    };

    static CrcIterator iterator()
    {
        return CrcIterator();
    }

    static uint64_t compute(const byte_t* data, size_t length)
    {
        CrcIterator iterator;
        for (size_t i = 0; i < length; i++) {
            iterator.computeNext(data[i]);
        }
        return iterator.getValue();
    }

    static uint64_t compute(const std::vector<byte_t> & bytes)
    {
        return compute(bytes.data(), bytes.size());
    }
};

template <
    uint32_t polynomial,
    uint32_t initialXOR, uint32_t finalXOR,
    bool doReflectData = true, bool doReflectRemainder = true
> using CRC8 = CRC <
    0x08, polynomial,
    initialXOR, finalXOR,
    doReflectData, doReflectRemainder
>;

template <
    uint32_t polynomial,
    uint32_t initialXOR, uint32_t finalXOR,
    bool doReflectData = true, bool doReflectRemainder = true
> using CRC16 = CRC <
    0x10, polynomial,
    initialXOR, finalXOR,
    doReflectData, doReflectRemainder
>;

#endif