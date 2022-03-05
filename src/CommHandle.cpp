
#include "CommHandle.hpp"
#include "CommandFrame.hpp"

#include <iostream>
#include <thread>

#define func auto

using namespace std::literals::chrono_literals;

CommHandle::CommHandle(const String & tty, int baudRate)
{
    while (!this->serialPort.open(tty, baudRate)) {
        std::cout << "Cannot open serial port " << tty << ", retrying..." << std::endl;
        std::this_thread::sleep_for(1000ms);
    }
}

CommHandle::CommHandle(const SerialPort & serialPortControl)
{
    this->serialPort = serialPortControl;
}

func CommHandle::startReceiving() -> bool
{
    this->receivingDaemonThread = Thread(receivingDaemon());
    if (receivingDaemonThread.joinable()) {
        receivingDaemonThread.join();
        return true;
    } else {
        return false;
    }
}

func CommHandle::getReceivingDaemonThread() -> Thread&
{
    return this->receivingDaemonThread;
}

func CommHandle::receivingDaemon() -> Function<void()>
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
        uint16_t crc16Value;
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

            for (int i = 0; i < received; i++) {

                byte_t currentByte = buffer[i];

                switch (state) {

                    case SOF:
                    {
                        if (currentByte == 0x05) {
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
                        if (currentByte == sequence) {
                            abandonFrame = true;
                        } else {
                            sequence = currentByte;
                        }
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
                        if (offset++ < dataLength) {
                            dataBuffer.emplace_back(currentByte);
                        } else {
                            offset = 0;
                            state = CRC16;
                        }
                    }
                    break;

                    case CRC16:
                    {
                        readToInt16(&crc16Value, currentByte);
                        if (offset == 2) {
                            offset = 0;
                            state = SOF;
                            if (crc16Iter.getValue() == crc16Value) {
                                subscribers[command]->receive(dataBuffer.data());
                            }
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

template <uint16_t Cmd, typename CmdData>
func CommHandle::Publisher<Cmd, CmdData>::cmd() -> uint8_t
{
    return Cmd;
}

template <uint16_t Cmd, typename CmdData>
func CommHandle::Publisher<Cmd, CmdData>::publish(const CmdData & data) -> bool
{
    CommandFrame<CmdData> commandFrame = CommandFrame<CmdData>(this->cmd(), data);
    this->serialPortMutex->lock();
    int sent = this->serialPort->send(commandFrame.toBytes());
    this->serialPortMutex->unlock();
    return sent == sizeof(CmdData);
}

