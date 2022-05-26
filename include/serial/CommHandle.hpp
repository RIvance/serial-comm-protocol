
#ifndef SERIAL_COMM_HANDLE_HPP
#define SERIAL_COMM_HANDLE_HPP

#include "serial/SerialControl.hpp"
#include "serial/command/CommandFrame.hpp"
#include "serial/utils/Logger.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>

namespace serial
{
    #define func auto

    using namespace command;

    class CommHandle
    {
      private:

        template <typename T>
        using Ref = std::shared_ptr<T>;

        template <typename Signature>
        using Function = std::function<Signature>;

        template <typename K, typename V>
        using HashMap = std::unordered_map<K, V>;

        using Mutex = std::mutex;
        using Thread = std::thread;
        using AtomicBool = std::atomic_bool;

        SerialControl serialPort {};
        byte_t sof = 0x05;

        int baudRate;
        String serialDevice;

        AtomicBool doReconnect = false;
        AtomicBool receivingStateFlag = false;

        Mutex reconnectionMutex;

        struct SubscriberBase
        {
            virtual uint16_t cmd() = 0;
            virtual void receive(uint8_t* data) = 0;
        };

        using SubscriberPtr = Ref<SubscriberBase>;
        HashMap<uint16_t, SubscriberPtr> subscribers;

        Mutex sendMutex;
        Mutex recvMutex;
        Thread receivingDaemonThread;

        Function<void()> receivingDaemon();

      public:

        template <uint16_t Cmd, typename CmdData>
        class Publisher
        {
          private:

            CommHandle* handle;

            func cmd() -> uint16_t
            {
                return Cmd;
            }

            friend class CommHandle;

          public:

            Publisher() = default;

            Publisher(const Publisher & another) : handle(another.handle) {}

            explicit Publisher(CommHandle* handle) : handle(handle) {}

            func publish(const CmdData & data) -> bool
            {
                CommandFrame<CmdData> commandFrame = CommandFrame<CmdData>(this->cmd(), data, handle->sof);
                handle->sendMutex.lock();
                int sent = 0;
                try {
                    sent = handle->serialPort.send(commandFrame.toBytes());
                } catch (serial::SerialClosedException & exception) {
                    logger::error("Serial device connection closed");
                    if (handle->doReconnect) {
                        this->handle->reconnect();
                    } else {
                        throw serial::SerialClosedException();
                    }
                }
                handle->sendMutex.unlock();
                return sent == sizeof(CmdData);
            }
        };

        template <typename CmdData>
        using Callback = Function<void(const CmdData &)>;

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

            explicit Subscriber(Callback<CmdData> callback) : callback(callback) {}

            func cmd() -> uint16_t override
            {
                return Cmd;
            }
        };

        void openSerialDevice(const String & device, int baud, byte_t sof = 0x05);

        void reconnect();

      public:

        explicit CommHandle(const SerialControl & serialPortControl, byte_t sof = 0x05);

        explicit CommHandle(const String & serialDevice, int baudRate = B115200, byte_t sof = 0x05);

        explicit CommHandle(int baudRate = B115200, byte_t sof = 0x05);

        ~CommHandle();

        void connect(const String & device, int baud = B115200);

        void autoConnect(int baud = B115200);

        template <uint16_t Cmd, typename CmdData>
        Publisher<Cmd, CmdData> advertise()
        {
            return CommHandle::Publisher<Cmd, CmdData>(this);
        }

        template <uint16_t Cmd, typename CmdData>
        func subscribe(Callback<CmdData> callback) -> void
        {
            SubscriberPtr subscriber = std::make_shared<Subscriber<Cmd, CmdData>>(callback);
            subscribers[Cmd] = subscriber;
        }

        bool startReceiving();
        bool startReceivingAsync();

        inline void stopReceiving()
        {
            this->receivingStateFlag = false;
        }

        Thread & getReceivingDaemonThread();

        inline bool isReceiving() const
        {
            return this->receivingStateFlag;
        }

        inline void setReconnect(bool value)
        {
            this->doReconnect = value;
        }
    };

    #undef func
}

#endif // SERIAL_COMM_HANDLE_HPP
