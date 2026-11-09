#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDataStream>
#include <QTcpSocket>

#include "rec/rpc/Server.h"
#include "rec/rpc/Client.h"
#include "RpcFixture.h"

namespace
{
class Application
{
public:
  Application() : app(argc, argv) {}
private:
  int argc = 1;
  char name[9] = "rpc_test";
  char *argv[2] = {name, nullptr};
public:
  QCoreApplication app;
};
}

TEST(RpcSerialization, ValuesAndSignedPositions)
{
  Reading source;
  source.position = -12345;
  source.values = QVector<float>{0.0F, -0.125F, 32.5F};
  auto bytes = encode(source);
  QDataStream stream(bytes);
  stream.setVersion(QDataStream::Qt_4_6);
  Reading result;
  stream >> result;
  ASSERT_EQ(stream.status(), QDataStream::Ok);
  EXPECT_EQ(result.position.value(), -12345);
  EXPECT_EQ(result.values.value(), source.values.value());
  EXPECT_TRUE(stream.atEnd());
}

TEST(RpcSerialization, IncorrectVersionIsRejected)
{
  Reading source;
  source.position = 9;
  source.values = QVector<float>{1.0F};
  QDataStream stream(encode(source));
  Reading result("reading_2.0");
  stream >> result;
  EXPECT_EQ(stream.status(), QDataStream::ReadCorruptData);
}

TEST(RpcSerialization, EveryTruncatedPrefixFails)
{
  Reading source;
  source.position = -1;
  source.values = QVector<float>{2.5F, -3.0F};
  const auto bytes = encode(source);
  for (int size = 0; size < bytes.size(); ++size) {
    QDataStream stream(bytes.left(size));
    stream.setVersion(QDataStream::Qt_4_6);
    Reading result;
    stream >> result;
    EXPECT_NE(stream.status(), QDataStream::Ok) << size;
  }
}

TEST(RpcLifecycle, ServerListensAndCloses)
{
  Application application;
  rec::rpc::Server server;
  server.setLocalIPCEnabled(false);
  server.setPort(0);
  server.listen(true);
  ASSERT_TRUE(server.isListening());
  ASSERT_GT(server.serverPort(), 0);
  QTcpSocket socket;
  socket.connectToHost("127.0.0.1", server.serverPort());
  ASSERT_TRUE(socket.waitForConnected(2000));
  socket.disconnectFromHost();
  server.close(true);
  EXPECT_FALSE(server.isListening());
}

TEST(RpcClientInfo, EqualEndpointsAreNotLessThanEachOther)
{
  const rec::rpc::ClientInfo first(QHostAddress::LocalHost, 1234, "first");
  const rec::rpc::ClientInfo second(QHostAddress::LocalHost, 1234, "second");
  EXPECT_FALSE(first < first);
  EXPECT_FALSE(first < second);
  EXPECT_FALSE(second < first);
  const rec::rpc::ClientInfo later(QHostAddress::LocalHost, 1235);
  EXPECT_TRUE(first < later);
  EXPECT_FALSE(later < first);
}
