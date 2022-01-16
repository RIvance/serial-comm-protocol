
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

    explicit CommHandle(const SerialPort & serialPortControl);

    explicit CommHandle(const String & tty, int baudRate = B115200);

    template <typename T>
    int send(int commandId, T data);

    template <typename T>
    Option<T> receive();
};

#endif //COMMHANDLE_HPP
