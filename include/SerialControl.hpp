#ifndef SERIAL_CONTROL
#define SERIAL_CONTROL

#include <string>
#include <vector>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include <termios.h> /* POSIX terminal control definitions */

#define EXPORT

using String = std::string;

EXPORT class SerialControl
{

  private:

    int fileDescriptor {};

  public:

    SerialControl() = default;

    explicit SerialControl(const String & tty, int baudRate, int flags = 0x00);

    /**
     * Open a serial port
     * @param tty tty pathname like "/dev/ttyUSB0"
     * @param baudRate
     * @param cflag control mode flags
     * @param iflag input mode flags
     * @param oflag output mode flags
     * @param oflag local mode flags
     * @return true if the serial port is
     *     successfully opened else return false
     */
    bool open(const String & tty, int baudRate, int cflag = CS8 | CLOCAL | CREAD, int iflag = 0, int oflag = 0, int lflag = 0);

    /**
     * Check if the port is open
     * @return
     */
    bool isOpen() const;

    /**
     * Close the port
     */
    void close() const;

    /**
     * Set baud rate
     * @param baud baud rate like `9600` or
     *     baud rate flag like `B9600`
     */
    void setBaudRate(int baud) const;

    /**
     * Add tty flag
     * @param flag
     */
    void addFlag(int flag) const;

    /**
     * Remove tty flag
     * @param flag
     */
    void removeFlag(int flag) const;

    /**
     * Send bytes to serial port
     * @param data data ptr
     * @param size length of data
     * @return number of successfully sent bytes
     */
    int send(void* data, size_t size) const;

    /**
     * Send a data struct to serial port
     * @tparam T struct type
     * @param data struct ptr
     * @return number of successfully sent bytes
     */
    template <typename T>
    int send(T* data) const;

    /**
     * Send a data struct to serial port
     * @param data data vector
     * @return number of successfully sent bytes
     */
    int send(const std::vector<unsigned char> & data) const;

    /**
     * Send a data struct to serial port
     * @tparam T struct type
     * @param data data struct itself
     * @return number of successfully sent bytes
     */
    template <typename T>
    int send(const T & data) const;

    /**
     * Receive bytes from serial port
     * @param data dst ptr
     * @param size length of data
     * @return number of successfully received bytes
     */
    int receive(void* data, size_t size) const;

    /**
     * Receive bytes from serial port
     * @param size length of data
     * @return data vector
     */
    std::vector<unsigned char> receive(size_t size) const;
};

#endif // SERIA_CONTROL
