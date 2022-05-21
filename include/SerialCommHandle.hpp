
#ifndef SERIAL_COMM_HANDLE_HPP
#define SERIAL_COMM_HANDLE_HPP

#include "SerialControl.hpp"
#include "CommandFrame.hpp"

#include <thread>
#include <mutex>

#define func auto
#define EMPTY_STATEMENT { }

template <typename T>
using Ref = std::shared_ptr<T>;

template <typename Signature>
using Function = std::function<Signature>;

template <typename T>
using Option = std::optional<T>;

using Mutex  = std::mutex;
using Thread = std::thread;

class SerialCommHandle
{
  private:

    SerialControl serialPort {};
    byte_t sof = 0x05;

    struct SubscriberBase
    {
        virtual uint16_t cmd() = 0;
        virtual void receive(uint8_t* data) = 0;
    };

    using SubscriberPtr = Ref<SubscriberBase>;
    std::unordered_map<uint16_t, SubscriberPtr> subscribers;

    Mutex serialPortMutex;
    Thread receivingDaemonThread;

    Function<void()> receivingDaemon();

  public:

    template <uint16_t Cmd, typename CmdData>
    class Publisher
    {
      private:

        SerialControl* serialPort;
        Mutex* serialPortMutex;
        byte_t sof;

        func cmd() -> uint16_t
        {
            return Cmd;
        }

        friend class SerialCommHandle;

      public:

        Publisher() = default;

        Publisher(const Publisher & another)
            : serialPort(another.port), serialPortMutex(another.mutex), sof(sof)
        { EMPTY_STATEMENT }

        explicit Publisher(SerialControl* port, Mutex* mutex, byte_t sof = 0x05)
            : serialPort(port), serialPortMutex(mutex), sof(sof)
        { EMPTY_STATEMENT }

        func publish(const CmdData & data) -> bool
        {
            CommandFrame<CmdData> commandFrame = CommandFrame<CmdData>(this->cmd(), data, this->sof);
            this->serialPortMutex->lock();
            int sent = this->serialPort->send(commandFrame.toBytes());
            this->serialPortMutex->unlock();
            return sent == sizeof(CmdData);
        }
    };

    template <typename CmdData>
    using Callback = Function<void(const CmdData&)>;

  private:

    template <uint16_t Cmd, typename CmdData>
    class Subscriber : public SubscriberBase
    {
      private:

        Callback<CmdData> callback;

        func receive(uint8_t* data) -> void override
        {
            auto* cmdData = reinterpret_cast<CmdData*>(data);
            this->callback(*cmdData);
        }

      public:

        func cmd() -> uint16_t override
        {
            return Cmd;
        }

        explicit Subscriber(Callback<CmdData> callback) : callback(callback) { }

    };

  public:

    explicit SerialCommHandle(const SerialControl & serialPortControl, byte_t sof = 0x05);

    explicit SerialCommHandle(const String & serialDevice, int baudRate = B115200, byte_t sof = 0x05);

    explicit SerialCommHandle(int baudRate = B115200, byte_t sof = 0x05);

    template <uint16_t Cmd, typename CmdData>
    Publisher<Cmd, CmdData> advertise()
    {
        return SerialCommHandle::Publisher<Cmd, CmdData>(&this->serialPort, &this->serialPortMutex);
    }

    template <uint16_t Cmd, typename CmdData>
    func subscribe(Callback<CmdData> callback) -> void
    {
        SubscriberPtr subscriber = std::make_shared<Subscriber<Cmd, CmdData>>(callback);
        subscribers[Cmd] = subscriber;
    }

    bool startReceiving();

    Thread & getReceivingDaemonThread();
};

#undef func

#endif // SERIAL_COMM_HANDLE_HPP
