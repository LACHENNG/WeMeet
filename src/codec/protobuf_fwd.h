#ifndef PROTOBUF_FWD_H
#define PROTOBUF_FWD_H

#include <memory>
#include <QSharedPointer>

// forward declearation
namespace google{
namespace protobuf{
class Message;
namespace stringpiece_internal {
class StringPiece;
}
}
}

using StringPiece = google::protobuf::stringpiece_internal::StringPiece;
using ProtoMessage = google::protobuf::Message;
using ProtoMessagePtr = QSharedPointer<ProtoMessage>;

#endif // PROTOBUF_FWD_H
