
#ifndef COMMHANDLE_HPP
#define COMMHANDLE_HPP

#include "SerialPort.hpp"
#include "CommandFrame.hpp"

#include <iostream>

class CommHandle
{
  private:

    SerialPort serialPort {};

  public:

    inline explicit CommHandle(const SerialPort & serialPortControl)
    {
        this->serialPort = serialPortControl;
    }

    inline explicit CommHandle(const String & tty, int baudRate = B115200)
    {
        while (!this->serialPort.open(tty, baudRate)) {
            std::cout << "Cannot open serial port " << tty << ", retrying..." << std::endl;
        }
    }

    template <typename T>
    int send(int commandId, T data)
    {
        CommandFrame<T> frame(commandId, data);
        return serialPort.send(frame.toBytes());
    }

    template <typename T>
    Option<T> receive()
    {
        auto data = serialPort.receive(CommandFrame<T>::dataSize());
        return CommandFrame<T>::parse(data);
    }
};


#endif //COMMHANDLE_HPP
