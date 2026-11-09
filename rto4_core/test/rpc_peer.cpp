#include <QCoreApplication>
#include <QTimer>
#include <QSocketNotifier>
#include <unistd.h>
#include <iostream>

#include "rec/rpc/Client.h"
#include "rec/rpc/Server.h"
#include "rec/rpc/serialization/String.h"
#include "rec/rpc/serialization/Primitive.h"
#include "RpcFixture.h"

DEFINE_STRING_TOPICDATA(text);
DEFINE_PRIMITIVE_PARAM(doubleValue, int);
DEFINE_PRIMITIVE_RESULT(doubleValue, int);

class PeerServer : public rec::rpc::Server
{
public:
  using rec::rpc::Server::publishTopic;
  PeerServer()
  {
    addPermanentTopic("text", 0);
    REGISTER_FUNCTION(doubleValue);
    setGreeting("rto4 conformance peer");
  }
private:
  DECLARE_FUNCTION(doubleValue);
};

BEGIN_FUNCTION_DEFINITION(PeerServer, doubleValue)
  (void)client;
  result = param.value() * 2;
END_FUNCTION_DEFINITION

class PeerClient : public rec::rpc::Client
{
public:
  using rec::rpc::Client::invoke;
  PeerClient()
  {
    setExpectedGreeting("rto4 conformance peer");
    setAutoReconnectEnabled(false);
    QObject::connect(this, &rec::rpc::Client::connected, [this]() {
      std::cerr << "Connected" << std::endl;
      REGISTER_TOPICLISTENER(text);
    });
    QObject::connect(this, &rec::rpc::Client::error,
      [](QAbstractSocket::SocketError, const QString &message) {
        std::cerr << message.toStdString() << std::endl;
        QCoreApplication::exit(9);
      });
  }
  ~PeerClient() { UNREGISTER_TOPICLISTENER(text); }
  bool received = false;
  bool initial = false;
private:
  DECLARE_TOPICLISTENER(text);
};

BEGIN_TOPICLISTENER_DEFINITION(PeerClient, text)
  (void)client;
  std::cerr << "Topic received: " << data.ref().toStdString() << std::endl;
  if (errorCode != rec::rpc::NoError) {
    std::cerr << "Topic callback failed: " << errorCode << std::endl;
    QCoreApplication::exit(3);
    return;
  }
  if (data.ref() == "initial snapshot") {
    initial = true;
    PUBLISH_TOPIC_SIMPLE(text, QString("signed -12345, float -0.125"));
  } else if (data.ref() == "signed -12345, float -0.125") {
    received = true;
  }
END_TOPICLISTENER_DEFINITION

int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
  if (argc == 2 && QString(argv[1]) == "bytes") {
    Reading fixture;
    fixture.position = -12345;
    fixture.values = QVector<float>{0.0F, -0.125F, 32.5F};
    std::cout << encode(fixture).toHex().toStdString() << std::endl;
    return 0;
  }
  if (argc < 3) {
    std::cerr << "Usage: rpc_peer server|client PORT [ipc]" << std::endl;
    return 2;
  }
  bool valid = false;
  const int port = QString(argv[2]).toInt(&valid);
  if (!valid || port < 0 || port > 65535) {
    std::cerr << "Invalid port" << std::endl;
    return 2;
  }
  const bool ipc = argc == 4 && QString(argv[3]) == "ipc";
  QTimer deadline;
  deadline.setSingleShot(true);
  QObject::connect(&deadline, &QTimer::timeout, [&]() {
    std::cerr << "Peer deadline exceeded" << std::endl;
    app.exit(4);
  });
  deadline.start(10000);
  try {
    if (QString(argv[1]) == "server") {
      PeerServer server;
      server.setLocalIPCEnabled(ipc);
      server.setPort(port);
      server.listen(true);
      if (!server.isListening()) {
        std::cerr << "Server not listening" << std::endl;
        return 3;
      }
      QSocketNotifier shutdown(STDIN_FILENO, QSocketNotifier::Read);
      QObject::connect(&shutdown, &QSocketNotifier::activated, [&]() {
        char command[16];
        if (::read(STDIN_FILENO, command, sizeof(command)) >= 0) {
          app.quit();
        }
      });
      QTimer::singleShot(0, [&server]() {
        server.publishTopic("text",
          rec::rpc::detail::createSerializable<topictextData>(QString("initial snapshot")));
      });
      QTimer::singleShot(50, [&server]() {
        std::cout << "READY " << server.serverPort() << std::endl;
      });
      const int result = app.exec();
      server.close(true);
      return result;
    }
    if (QString(argv[1]) != "client") {
      std::cerr << "Invalid role" << std::endl;
      return 2;
    }
    PeerClient peer;
    peer.setLocalIPCEnabled(ipc);
    peer.setAddress(QString("127.0.0.1:%1").arg(port));
    peer.connectToServer(2000);
    QTimer poll;
    QObject::connect(&poll, &QTimer::timeout, [&]() {
      if (!peer.received || !peer.initial) {
        return;
      }
      try {
        const auto result = rec::rpc::detail::createSerializable<doubleValueResult>();
        peer.invoke("doubleValue",
          rec::rpc::detail::createSerializable<doubleValueParam>(-12345), result, true);
        if (result->value() != -24690) {
          std::cerr << "RPC result differs" << std::endl;
          app.exit(5);
          return;
        }
        bool rejected = false;
        try {
          peer.invoke("missing_method",
            rec::rpc::detail::createSerializable<doubleValueParam>(1),
            rec::rpc::detail::createSerializable<doubleValueResult>(), true);
        } catch (const rec::rpc::Exception &error) {
          rejected = error.errorCode() == rec::rpc::UnknownFunction;
        }
        if (!rejected) {
          std::cerr << "Missing method did not return UnknownFunction" << std::endl;
          app.exit(6);
          return;
        }
        std::cout << "PASS snapshot publish RPC error" << std::endl;
        app.exit(0);
      } catch (const rec::rpc::Exception &error) {
        std::cerr << error.what() << std::endl;
        app.exit(7);
      }
    });
    poll.start(20);
    return app.exec();
  } catch (const rec::rpc::Exception &error) {
    std::cerr << error.what() << std::endl;
    return 8;
  }
}
