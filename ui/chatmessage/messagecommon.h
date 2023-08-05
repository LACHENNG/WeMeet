// Author : lang @ nwpu
// All rights reserved

#ifndef MESSAGECOMMON_H
#define MESSAGECOMMON_H

#include <QObject>

class MessageCommon{

public:
    enum TYPE{
        ME,
        OTHER
    };
    void setMsgId(int id) { m_msgId = id; }
    void setMsgTime(qint64 timeSecsSinceEpoch) { m_msgTimeSecsSinceEpoch = timeSecsSinceEpoch;}

    int msgId() const { return m_msgId; }
    qint64 msgTimeSecs() const { return m_msgTimeSecsSinceEpoch; }

private:
    int m_msgId; // to distinguish from different Messages
    qint64 m_msgTimeSecsSinceEpoch; // message time
};

#endif
