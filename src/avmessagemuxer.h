#ifndef AVDECODERMUXER_H
#define AVDECODERMUXER_H

#include <string>
#include "src/protoc/message.pb.h"
#include <google/protobuf/message.h>
#include <google/protobuf/timestamp.pb.h>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

}
#include <QSharedPointer>

class MediaCodec;
// useful to diff from different decoded AVFrames from AvPacket
// because AVFrames or AvPacket didn`t carry info who send or received this packet
class MessageContext{
public:
    std::string getSender_id() const;
    void setSender_id(const std::string &newSender_id);

    std::string getReceiver_id() const;
    void setReceiver_id(const std::string &newReceiver_id);

    google::protobuf::Timestamp getTimestamp() const;
    void setTimestamp(const google::protobuf::Timestamp &newTimestamp);

private:
    // a subset of MeetChat::Message
    std::string sender_id ;
    std::string receiver_id ;
    google::protobuf::Timestamp timestamp;
};

// A wrapper to MediaCodec
// which provide more info(MessageContext exactly) pass to onDecodedCallback
// to konw whose(identified by MessageContext) packet is decoded
class AVDecoderMuxer{
public:
    using OnAVPacketDecodedCallback = std::function<void (const MessageContext&, const AVFrame*)>;

    AVDecoderMuxer();
    ~AVDecoderMuxer();

    // decode one MeetChat::AVPacket ( encoded by encodeFrame)
    // Note: setOnPacketDecodedCallback to received decoded data
    int decodeAVPacket(const QSharedPointer<MeetChat::Message>& av_message);

    void setOnPacketDecodedCallback(const OnAVPacketDecodedCallback& cb) { m_onAvPacketDecoded = cb; }

private:
    using sender_t = std::string;
    using sender_avPacket_decodedCb_t =  std::function<void (const AVFrame*)>;

    std::unordered_map<sender_t, sender_avPacket_decodedCb_t> m_idToOnDecodedCbMap;
    // the on does the real decoding work
    std::unique_ptr<MediaCodec> m_mediaDecoder;
    // user registered callback
    OnAVPacketDecodedCallback m_onAvPacketDecoded;
};
#endif // AVDECODERMUXER_H
