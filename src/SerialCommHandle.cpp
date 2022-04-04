
#include "SerialCommHandle.hpp"
#include "CommandFrame.hpp"

#include <iostream>
#include <thread>
#include <filesystem>
#include <regex>

#define func auto

using namespace std::literals::chrono_literals;
using DirectoryIterator = std::filesystem::directory_iterator;
using Regex = std::regex;

func getSerialDevices() -> Vec<String>
{
    static const Regex SERIAL_DEV_PATTERN("\\/dev\\/tty(USB|ACM)[0-9]+");
    Vec<String> serialDevices;
    for (const auto & entry : DirectoryIterator("/dev")) {
        String path = entry.path();
        if (std::regex_match(path, SERIAL_DEV_PATTERN)) {
            serialDevices.emplace_back(path);
        }
    }
    std::sort(serialDevices.begin(), serialDevices.end());
    return serialDevices;
}

SerialCommHandle::SerialCommHandle(const String & serialDevice, int baudRate, byte_t sof)
{
    this->sof = sof;
    while (!this->serialPort.open(serialDevice, baudRate)) {
        std::cout << "Unable to open serial device " << serialDevice << ", retrying..." << std::endl;
        std::this_thread::sleep_for(1000ms);
    }
    std::cout << "Successfully connected to serial device " << serialDevice << std::endl;
}

SerialCommHandle::SerialCommHandle(int baudRate, byte_t sof)
{
    this->sof = sof;
    Vec<String> serialDevices = getSerialDevices();
    while (serialDevices.empty() || !this->serialPort.open(serialDevices.front(), baudRate)) {
        serialDevices = getSerialDevices();
        if (serialDevices.empty()) {
            std::cout << "No serial device found, retrying..." << std::endl;
            std::this_thread::sleep_for(1000ms);
            continue;
        }
        std::cout << "Unable to serial device " << serialDevices.front() << ", retrying..." << std::endl;
        std::this_thread::sleep_for(1000ms);
    }
    std::cout << "Successfully connected to serial device " << serialDevices.front() << std::endl;
}

SerialCommHandle::SerialCommHandle(const SerialControl & serialPortControl, byte_t sof)
{
    this->sof = sof;
    this->serialPort = serialPortControl;
}

func SerialCommHandle::startReceiving() -> bool
{
    this->receivingDaemonThread = Thread(receivingDaemon());
    if (receivingDaemonThread.joinable()) {
        receivingDaemonThread.join();
        return true;
    } else {
        return false;
    }
}

func SerialCommHandle::getReceivingDaemonThread() -> Thread&
{
    return this->receivingDaemonThread;
}

func SerialCommHandle::receivingDaemon() -> Function<void()>
{
    return [this]() __attribute__((__noreturn__)) -> void
    {
        const size_t BUFFER_SIZE = 1024;
        byte_t buffer[BUFFER_SIZE];

        using Crc8  = CRC8<0x31,    0xFF,   0x00>;
        using Crc16 = CRC16<0x1021, 0xFFFF, 0x0000>;

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

        enum { SOF, DLEN, SEQ, CRC8, CMD, DATA, CRC16 } state = SOF;

        uint16_t dataLength;
        uint8_t  sequence = -1;
        uint16_t crc16Value = 0;
        uint16_t command;

        Crc8::CrcIterator crc8Iter;
        Crc16::CrcIterator crc16Iter;

        Vec<byte_t> dataBuffer;

        bool abandonFrame = false;

        int offset = 0;

        auto readToInt16 = [&offset](uint16_t* dst, byte_t src)
        {
            reinterpret_cast<byte_t*>(dst)[offset++] = src;
        };

        while (true) {

            this->serialPortMutex.lock();
            size_t received = this->serialPort.receive(buffer, BUFFER_SIZE);
            this->serialPortMutex.unlock();

            if (received == -1) {
                continue;
            }

            for (int i = 0; i < received; i++) {

                byte_t currentByte = buffer[i];

                switch (state) {

                    case SOF:
                    {
                        if (currentByte == this->sof) {
                            state = DLEN;
                            offset = 0;
                            crc8Iter  = Crc8::iterator();
                            crc16Iter = Crc16::iterator();
                        }
                    }
                    break;

                    case DLEN:
                    {
                        readToInt16(&dataLength, currentByte);
                        if (offset == 2) {
                            offset = 0;
                            state = SEQ;
                        }
                    }
                    break;

                    case SEQ:
                    {
                      #ifdef ABANDON_SAME_FRAME
                        if (currentByte == sequence) {
                            abandonFrame = true;
                        } else {
                            sequence = currentByte;
                        }
                      #endif
                        state = CRC8;
                    }
                    break;

                    case CRC8:
                    {
                        if (crc8Iter.getValue() == currentByte) {
                            state = CMD;
                        } else {
                            state = SOF;
                        }
                    }
                    break;

                    case CMD:
                    {
                        readToInt16(&command, currentByte);
                        if (offset == 2) {
                            offset = 0;
                            state = DATA;
                        }
                    }
                    break;

                    case DATA:
                    {
                        if (++offset <= dataLength) {
                            dataBuffer.emplace_back(currentByte);
                        }
                        if (offset == dataLength){
                            offset = 0;
                            state = CRC16;
                            crc16Iter.computeNext(currentByte);
                        }
                    }
                    break;

                    case CRC16:
                    {
                        readToInt16(&crc16Value, currentByte);
                        if (offset == 2) {
                            offset = 0;
                            state = SOF;
                            if (!abandonFrame && crc16Iter.getValue() == crc16Value) {
                                subscribers[command]->receive(dataBuffer.data());
                            }
                            crc16Value = 0;
                            dataBuffer.clear();
                        }
                    }
                    break;

                }   // end switch

                if (state != SOF && state != CRC16) {
                    crc8Iter.computeNext(currentByte);
                    crc16Iter.computeNext(currentByte);
                }

            }   // end for

        }   // end while

    };  // end lambda
}
