#include "avmessagemuxer.h"
#include "mediacodec.h"


std::string MessageContext::getSender_id() const
{
    return sender_id;
}

void MessageContext::setSender_id(const std::string &newSender_id)
{
    sender_id = newSender_id;
}

std::string MessageContext::getReceiver_id() const
{
    return receiver_id;
}

void MessageContext::setReceiver_id(const std::string &newReceiver_id)
{
    receiver_id = newReceiver_id;
}

google::protobuf::Timestamp MessageContext::getTimestamp() const
{
    return timestamp;
}

void MessageContext::setTimestamp(const google::protobuf::Timestamp &newTimestamp)
{
    timestamp = newTimestamp;
}

AVDecoderMuxer::AVDecoderMuxer()
    : m_mediaDecoder(new MediaCodec(MediaCodec::USE_AS_DECODER))
{
}

AVDecoderMuxer::~AVDecoderMuxer()
{

}

int AVDecoderMuxer::decodeAVPacket(const QSharedPointer<MeetChat::Message> &av_message)
{
    auto sender_id = av_message->sender_id();
    // the first time to receive a MeetChat::Message of type AV
    // register an internal  ondecoded callback
    if(this->m_idToOnDecodedCbMap.find(sender_id) == m_idToOnDecodedCbMap.end()){
        MessageContext ctx;
        ctx.setSender_id(av_message->sender_id());
        ctx.setReceiver_id(av_message->receiver_id());
        ctx.setTimestamp(av_message->timestamp());
        auto onDecodedcb = [ctx, this](const AVFrame* avframe){
            this->m_onAvPacketDecoded(ctx, avframe); // call user provided callback
        };
        m_idToOnDecodedCbMap[sender_id] = onDecodedcb;
        // register on decoded callback
        m_mediaDecoder->setOnPacketDecodedCallback(onDecodedcb);
    }

    // do the actual decode
    google::protobuf::Any any = av_message->data();
    MeetChat::AVPacket av_packet;
    if (!any.UnpackTo(&av_packet)) {
        std::cerr << "Can`t parse data to AvPacket type\n";
        return -1;
    }
    return m_mediaDecoder->decodeAVPacket(av_packet);
}

const SwsContext *AVDecoderMuxer::getSwsCtx() const
{
    return m_mediaDecoder->getSwsCtx();
}
