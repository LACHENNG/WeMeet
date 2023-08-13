### WeMeet ：基于LanceNet v1.1.0 构建的云会议（客户端）✨

#### 一、**技术概览**

1. 前端使用Win10 Qt 5.14.2 + MSVC2019 构建的UI，和QT内置的网络类 Qt Network
2. 后端使用 ubuntu18.04 + LanceNet v.1.1.0（本人另外一个项目，参见个人Github主页） 开发

   1. 消息打包：设计了一个Packet类型，自动处理封包和拆包、主要包含长度，typename和proto payload，用于封装protobuf序列化后的数据，typename实现**消息反射**
   2. 消息序列化和反序列：应用程序为每种类型的消息（FILE，Text，Video等）设计了一个**通用**的protobuf::Message 格式，其中使用类型字段type标识具体的payload的消息类型，实现**消息分发**
   3. 数据压缩
      1. 对于视频数据，使用**H.264算法压缩、解码**，大大节省了带宽
      2. 对于音频数据，使用[Opus](https://www.opus-codec.org/)库压缩、解码
   4. 简单封装了ffmpeg库，**提供了回调机制**：将ffmpeg中原本**异步**的编码和解码转换成了回调的方式**通知数据就绪**，简化了客户端代码
   5. 封装了[PortAudio](http://portaudio.com/docs/v19-doxydocs/index.html)库，提供了更加易用的接口来捕获和播放音频（PCM格式）
   6. 【TODO】有效的处理视频**音频帧乱序**和**音视频同步**的问题

#### 二、**主要功能**

1. 在会议**文字聊天**，可以私发消息和给全体发送消息

2. 聊天发送表情，图片等

3. 聊天框**上传、下载文件**

4. **视频聊天**

5. 语音聊天

   

🎯【注意】本项目仅为客户端代码，服务端见 [LanceNet](https://github.com/LACHENNG/LanceNet) 项目`apps/chat/server 目录`
