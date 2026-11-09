#pragma once

#include <QDataStream>
#include "rec/rpc/serialization/Complex.h"

class Reading : public rec::rpc::serialization::Complex
{
public:
  explicit Reading(const QString &version = "reading_1.0") : Complex(version)
  {
    addChild(&position);
    addChild(&values);
  }
  rec::rpc::serialization::Primitive<int> position;
  rec::rpc::serialization::Primitive<QVector<float>> values;
};

inline QByteArray encode(const Reading &reading)
{
  QByteArray bytes;
  QDataStream stream(&bytes, QIODevice::WriteOnly);
  stream.setVersion(QDataStream::Qt_4_6);
  stream << reading;
  return bytes;
}
