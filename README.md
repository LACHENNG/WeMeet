### 基于LanceNet v1.1.0 构建的视频会议系统

QT 15.2 + MSVC 2019

protobuf：3.20.0

**技术概览**

1. 前端使用Qt 5.14.2 构建的UI，和QT内置的网络类 Qt Network

2. 后端使用 WSL2 ubuntu18.04 + LanceNet v.1.1.0（本人另外一个项目，参见） 开发

    1. 消息编码：使用Google protobuf 序列化和反射消息，使用相同protobuf序列化协议进行通信，前后端充分解耦

    2. 基于**gRPC** 高性能远程过程调用（RPC）框架 中的 流失RPC 实现视频通话

       > 流式PRC允许你在一个连接中发送或者接受多个消息，而不只是发送或接受一个消息

**主要功能**

1. 在会议中**文字聊天**，可以私发消息和给全体发送消息
2. 聊天发送表情，图片等
3. 聊天框**上传文件**
4. 支持主持人开启**视频**
5. 







提示词

前面我提到我想使用protobuf设计消息格式，用于实现一个视频会议软件，类似于腾讯会议，其中可以发送文本，emoji到图片文件到聊天框，并支持视频等，你设计了如下的.proto消息格式：

```protobuf
syntax = "proto3";
package MeetChat;
// 导入一些公共的 protobuf 类型
import "google/protobuf/any.proto";
import "google/protobuf/timestamp.proto";

// 定义一个通用的消息类型，可以包含不同类型的数据
message Message {
  // 消息的发送者
  string sender_id = 1;
  // 消息的接收者
  string receiver_id = 2; // -1 表示所有人接受
  // 消息的时间戳
  google.protobuf.Timestamp timestamp = 3;
  // 消息的数据，可以是任何类型，使用 Any 类型来封装
  google.protobuf.Any data = 4;
}

// 定义一个视频帧类型，包含帧序号、时间戳和图像数据
message VideoFrame {
  // 帧的序号
  int32 frame_number = 1;
  // 帧的时间戳
  google.protobuf.Timestamp timestamp = 2;
  // 帧的图像数据，可以是任何编码格式，例如 JPEG 或 H.264
  bytes image_data = 3;
}

// 定义一个文字消息类型，包含文本内容
message TextMessage {
  // 文本内容
  string text = 1;
}

// 定义一个表情类型，包含表情符号或图片
message Emoji {
  // 表情符号，例如 😊 或 😂
  string symbol = 1;
  // 表情图片，如果有的话，可以是任何编码格式，例如 PNG 或 GIF
  bytes image = 2;
}

// 定义一个图片类型，包含图片名称、大小和数据
message Image {
  // 图片名称，例如 "cat.jpg"
  string name = 1;
  // 图片大小，以字节为单位
  int32 size = 2;
  // 图片数据，可以是任何编码格式，例如 JPEG 或 PNG
  bytes data = 3;
}

// 定义一个文件类型，包含文件名称、大小和数据
message File {
  // 文件名称，例如 "report.pdf"
  string name = 1;
  // 文件大小，以字节为单位
  int32 size = 2;
  // 文件数据，可以是任何格式
  bytes data = 3;
}

```



假设使用该消息格式发送一个文本消息，发送端和接收端代码


```c++
########### sender ########################
#include <google/protobuf/any.pb.h>
#include "message.pb.h" // 假设你的 .proto 文件生成的头文件是这个

// 创建一个 TextMessage 对象，并设置文本内容
TextMessage text_message;
text_message.set_text("你好，这是一个文本消息");

// 创建一个 Message 对象，并设置发送者、接收者和时间戳
Message message;
message.set_sender_id("user1");
message.set_receiver_id("user2");
message.mutable_timestamp()->set_seconds(time(NULL));

// 使用 Any 类型来封装 TextMessage 对象，并设置到 Message 对象的 data 字段
google::protobuf::Any any;
any.PackFrom(text_message);
message.set_allocated_data(&any);

// 序列化 Message 对象到一个字符串
std::string output;
message.SerializeToString(&output);

// 发送 output 字符串到输出流



```

```c++
########## receiver #####################
#include <google/protobuf/any.pb.h>
#include "message.pb.h" // 假设你的 .proto 文件生成的头文件是这个

// 从输入流接收一个字符串
std::string input;

// 反序列化字符串到一个 Message 对象
Message message;
message.ParseFromString(input);

// 获取 Message 对象的 data 字段，它是一个 Any 类型
google::protobuf::Any any = message.data();

// 创建一个 TextMessage 对象，用来存放解析出来的文本消息
TextMessage text_message;

// 使用 Any 类型的 UnpackTo 方法来解析出 TextMessage 对象
if (any.UnpackTo(&text_message)) {
  // 成功解析出 TextMessage 对象
  // 获取文本内容
  std::string text = text_message.text();
  // 处理文本内容
} else {
  // 无法解析出 TextMessage 对象，可能是其他类型的数据
  // 输出错误信息
  std::cerr << "Unknown data type\n";
}
```

我突然感觉如果在Message中增加一下消息类型字段，可以方便接收端判断接收端的消息具体是Text还是Image还是Emoji等等，具体怎么修改我的proto消息格式？接收端发送端的代码如何修改？

你的想法很有道理，增加一个消息类型字段可以让接收端更容易识别不同类型的数据。你可以这样修改你的 .proto 消息格式：

```protobuf
syntax = "proto3";
package MeetChat;
// 导入一些公共的 protobuf 类型
import "google/protobuf/any.proto";
import "google/protobuf/timestamp.proto";

// 定义一个枚举类型，表示不同的消息类型
enum MessageType {
  TEXT = 0; // 文本消息
  EMOJI = 1; // 表情消息
  IMAGE = 2; // 图片消息
  FILE = 3; // 文件消息
  VIDEO = 4; // 视频帧消息
}

// 定义一个通用的消息类型，可以包含不同类型的数据
message Message {
  // 消息的发送者
  string sender_id = 1;
  // 消息的接收者
  string receiver_id = 2; // -1 表示所有人接受
  // 消息的时间戳
  google.protobuf.Timestamp timestamp = 3;
  // 消息的类型，使用枚举类型来表示
  MessageType type = 4;
  // 消息的数据，可以是任何类型，使用 Any 类型来封装
  google.protobuf.Any data = 5;
}

// 定义一个视频帧类型，包含帧序号、时间戳和图像数据
message VideoFrame {
  // 帧的序号
  int32 frame_number = 1;
  // 帧的时间戳
  google.protobuf.Timestamp timestamp = 2;
  // 帧的图像数据，可以是任何编码格式，例如 JPEG 或 H.264
  bytes image_data = 3;
}

// 定义一个文字消息类型，包含文本内容
message TextMessage {
  // 文本内容
  string text = 1;
}

// 定义一个表情类型，包含表情符号或图片
message Emoji {
  // 表情符号，例如 😊 或 😂
  string symbol = 1;
  // 表情图片，如果有的话，可以是任何编码格式，例如 PNG 或 GIF
  bytes image = 2;
}

// 定义一个图片类型，包含图片名称、大小和数据
message Image {
  // 图片名称，例如 "cat.jpg"
  string name = 1;
  // 图片大小，以字节为单位
  int32 size = 2;
  // 图片数据，可以是任何编码格式，例如 JPEG 或 PNG
  bytes data = 3;
}

// 定义一个文件类型，包含文件名称、大小和数据
message File {
  // 文件名称，例如 "report.pdf"
  string name = 1;
  // 文件大小，以字节为单位
  int32 size = 2;
  // 文件数据，可以是任何格式
  bytes data = 3;
}
```

发送端和接收端的代码也需要相应地修改一下：

```c++
########### sender ########################
#include <google/protobuf/any.pb.h>
#include "message.pb.h" // 假设你的 .proto 文件生成的头文件是这个


// 创建一个 TextMessage 对象，并设置文本内容

MeetChat::Message message;
message.set_sender_id("user1");
message.set_receiver_id("user2");
message.mutable_timestamp()->set_seconds(time(NULL));

// 必须new TextMessage对象，set_allocated_text_message后，会在
// message析构的时候自动释放该text_message，若重复
MeetChat::TextMessage *text_message = new MeetChat::TextMessage;
text_message->set_text("You is dian yi si");


// 使用 set_allocated_text_message 方法来设置 Message 对象的 data 字段为 TextMessage 类型
message.set_allocated_text_message(text_message);

send(message);

// 序列化 Message 对象到一个字符串
std::string output;
message.SerializeToString(&output);

// 发送 output 字符串到输出流
```

```c++
########## receiver #####################
#include <google/protobuf/any.pb.h>
#include "message.pb.h" // 假设你的 .proto 文件生成的头文件是这个

// 创建一个 TextMessage 对象，用来存放解析出来的文本消息
TextMessage text_message;

// 创建一个 Emoji 对象，用来存放解析出来的表情消息
Emoji emoji;

// 创建一个 Image 对象，用来存放解析出来的图片消息
Image image;

// 创建一个 File 对象，用来存放解析出来的文件消息
File file;

// 创建一个 VideoFrame 对象，用来存放解析出来的视频帧消息
VideoFrame video_frame;

// 从输入流接收一个字符串
std::string input;

// 反序列化字符串到一个 Message 对象
Message message;
message.ParseFromString(input);

// 获取 Message 对象的 type 字段，它是一个枚举类型
MessageType type = message.type();

// 获取 Message 对象的 data 字段，它是一个 Any 类型
google::protobuf::Any any = message.data();

// 根据不同的消息类型，使用 Any 类型的 UnpackTo 方法来解析出对应的对象
switch (type) {
  case MessageType::TEXT:
    // 消息类型为 TEXT
    if (any.UnpackTo(&text_message)) {
      // 成功解析出 TextMessage 对象
      // 获取文本内容
      std::string text = text_message.text();
      // 处理文本内容
    } else {
      // 无法解析出 TextMessage 对象，输出错误信息
      std::cerr << "Invalid data for TEXT type\n";
    }
    break;
  case MessageType::EMOJI:
    // 消息类型为 EMOJI
    if (any.UnpackTo(&emoji)) {
      // 成功解析出 Emoji 对象
      // 获取表情符号或图片
      std::string symbol = emoji.symbol();
      bytes image = emoji.image();
      // 处理表情符号或图片
    } else {
      // 无法解析出 Emoji 对象，输出错误信息
      std::cerr << "Invalid data for EMOJI type\n";
    }
    break;
  case MessageType::IMAGE:
    // 消息类型为 IMAGE
    if (any.UnpackTo(&image)) {
      // 成功解析出 Image 对象
      // 获取图片名称、大小和数据
      std::string name = image.name();
      int32 size = image.size();
      bytes data = image.data();
      // 处理图片名称、大小和数据
    } else {
      // 无法解析出 Image 对象，输出错误信息
      std::cerr << "Invalid data for IMAGE type\n";
    }
    break;
  case MessageType::FILE:
    // 消息类型为 FILE
    if (any.UnpackTo(&file)) {
      // 成功解析出 File 对象
      // 获取文件名称、大小和数据
      std::string name = file.name();
      int32 size = file.size();
      bytes data = file.data();
      // 处理文件名称、大小和数据
    } else {
      // 无法解析出 File 对象，输出错误信息
      std::cerr << "Invalid data for FILE type\n";
    }
    break;
  case MessageType::VIDEO:
    // 消息类型为 VIDEO
    if (any.UnpackTo(&video_frame)) {
      // 成功解析出 VideoFrame 对象
      // 获取帧序号、时间戳和图像数据
      int32 frame_number = video_frame.frame_number();
      google.protobuf.Timestamp timestamp = video_frame.timestamp();
      bytes image_data = video_frame.image_data();
      // 处理帧序号、时间戳和图像数据
    } else {
      // 无法解析出 VideoFrame 对象，输出错误信息
      std::cerr << "Invalid data for VIDEO type\n";
    }
    break;
  default:
    // 未知的消息类型，输出错误信息
    std::cerr << "Unknown message type\n";
}
```

 复制

这样修改后，你的代码就可以根据消息类型来判断如何处理不同类型的数据了。😊



```c++

syntax = "proto3";

package MeetChat;


// 导入一些公共的 protobuf 类型
// import "google/protobuf/any.proto";
import "google/protobuf/timestamp.proto";

// 定义一个通用的消息类型，可以包含不同类型的数据
message Message {
  // 消息的发送者
  string sender_id = 1;
  // 消息的接收者
  string receiver_id = 2; // -1 表示所有人接受
  // 消息的时间戳
  google.protobuf.Timestamp timestamp = 3;
  // 消息的数据，可以是以下类型之一 
  oneof data {   //  可以避免使用 Any 类型来封装造成的额外开销 google.protobuf.Any data = 4;
    TextMessage text_message = 4;
    Emoji emoji = 5;
    Image image = 6;
    File file = 7;
    VideoFrame video_frame = 8;
  }
}

// 定义一个视频帧类型，包含帧序号、时间戳和图像数据
message VideoFrame {
  // 帧的序号
  int32 frame_number = 1;
  // 帧的时间戳
  google.protobuf.Timestamp timestamp = 2;
  // 帧的图像数据，可以是任何编码格式，例如 JPEG 或 H.264
  bytes image_data = 3;
}

// 定义一个文字消息类型，包含文本内容
message TextMessage {
  // 文本内容
  string text = 1;
}

// 定义一个表情类型，包含表情符号或图片
message Emoji {
  // 表情符号，例如 😊 或 😂
  string symbol = 1;
  // 表情图片，如果有的话，可以是任何编码格式，例如 PNG 或 GIF
  bytes image = 2;
}

// 定义一个图片类型，包含图片名称、大小和数据
message Image {
  // 图片名称，例如 "cat.jpg"
  string name = 1;
  // 图片大小，以字节为单位
  int32 size = 2;
  // 图片数据，可以是任何编码格式，例如 JPEG 或 PNG
  bytes data = 3;
}

// 定义一个文件类型，包含文件名称、大小和数据
message File {
  // 文件名称，例如 "report.pdf"
  string name = 1;
  // 文件大小，以字节为单位
  int32 size = 2;
  // 文件数据，可以是任何格式
  bytes data = 3;
}

```

```c++
########### sender ########################
#include "message.pb.h" // 假设你的 .proto 文件生成的头文件是这个

// 创建一个 TextMessage 对象，并设置文本内容
TextMessage text_message;
text_message.set_text("你好，这是一个文本消息");

// 创建一个 Message 对象，并设置发送者、接收者、时间戳和数据
Message message;
message.set_sender_id("user1");
message.set_receiver_id("user2");
message.mutable_timestamp()->set_seconds(time(NULL));
message.set_allocated_text_message(&text_message); // 设置消息数据为 TextMessage 对象

// 序列化 Message 对象到一个字符串
std::string output;
message.SerializeToString(&output);

// 发送 output 字符串到输出流

```

```c++
########## receiver #####################
#include "message.pb.h" // 假设你的 .proto 文件生成的头文件是这个

// 创建一个 TextMessage 对象，用来存放解析出来的文本消息
TextMessage text_message;

// 创建一个 Emoji 对象，用来存放解析出来的表情消息
Emoji emoji;

// 创建一个 Image 对象，用来存放解析出来的图片消息
Image image;

// 创建一个 File 对象，用来存放解析出来的文件消息
File file;

// 创建一个 VideoFrame 对象，用来存放解析出来的视频帧消息
VideoFrame video_frame;

// 从输入流接收一个字符串
std::string input;

// 反序列化字符串到一个 Message 对象
Message message;
message.ParseFromString(input);

// 获取 Message 对象的 data 字段，它是一个 oneof 类型
// 使用 has_* 方法来判断具体是哪种类型的数据
if (message.has_video_frame()) {
  // 消息数据为 VideoFrame 对象
  // 获取 VideoFrame 对象
  video_frame = message.video_frame();
  // 获取帧序号、时间戳和图像数据
  int32 frame_number = video_frame.frame_number();
  google.protobuf.Timestamp timestamp = video_frame.timestamp();
  bytes image_data = video_frame.image_data();
  // 处理帧序号、时间戳和图像数据
} else if (message.has_text_message()) {
  // 消息数据为 TextMessage 对象
  // 获取 TextMessage 对象
  text_message = message.text_message();
  // 获取文本内容
  std::string text = text_message.text();
  // 处理文本内容
} else if (message.has_emoji()) {
  // 消息数据为 Emoji 对象
  // 获取 Emoji 对象
  emoji = message.emoji();
  // 获取表情符号或图片
  std::string symbol = emoji.symbol();
  bytes image = emoji.image();
  // 处理表情符号或图片
} else if (message.has_image()) {
  // 消息数据为 Image 对象
  // 获取 Image 对象
  image = message.image();
  // 获取图片名称、大小和数据
  std::string name = image.name();
  int32 size = image.size();
  bytes data = image.data();
  // 处理图片名称、大小和数据
} else if (message.has_file()) {
  // 消息数据为 File 对象
  // 获取 File 对象
  file = message.file();
  // 获取文件名称、大小和数据
  std::string name = file.name();
  int32 size = file.size();
  bytes data = file.data();
  // 处理文件名称、大小和数据
} else {
  // 没有消息数据，输出错误信息
  std::cerr << "No data in message\n";
}

```





你的问题很有意思，我从网上搜索了一些相关的信息，希望能帮助你选择合适的方法。😊

根据我找到的资料[1](https://stackoverflow.com/questions/65491268/what-the-difference-between-google-protobuf-any-and-google-protobuf-value)[2](https://learn.microsoft.com/en-us/dotnet/architecture/grpc-for-wcf-developers/protobuf-any-oneof)，google.protobuf.Any 和 oneof 的区别主要有以下几点：

- google.protobuf.Any 可以表示任何已知的 Protobuf 消息类型，而 oneof 只能表示定义在同一个消息中的几种类型之一。
- google.protobuf.Any 需要导入 google/protobuf/any.proto 文件，而 oneof 是 Protobuf 的语言特性，不需要导入任何文件。
- google.protobuf.Any 需要使用 PackFrom 和 UnpackTo 方法来封装和解析消息对象，而 oneof 可以直接使用 has_* 和 set_* 方法来判断和设置字段。
- google.protobuf.Any 会增加额外的开销，因为它需要存储消息类型的 URL 和序列化后的消息数据，而 oneof 只需要存储一个枚举值和一个字段值。

另外，我还发现了一个关于 oneof 和 enum 的讨论[3](https://stackoverflow.com/questions/69185020/oneof-kind-vs-enum-in-protobuf)，其中提到了一些使用 oneof 的优点：

- oneof 可以让你在不同的字段之间传递不同类型的数据，例如字符串、整数、浮点数等，而 enum 只能传递整数值。
- oneof 可以让你在不同的字段之间传递更复杂的数据结构，例如嵌套的消息、列表、映射等，而 enum 只能传递简单的值。
- oneof 可以让你在运行时检查哪个字段被设置了，而 enum 需要你额外定义一个标志位来表示状态。

综上所述，我认为如果你需要传递不同类型或复杂度的数据，并且不太关心额外的开销和复杂度，那么 google.protobuf.Any 可能是一个好选择。如果你只需要传递几种简单的数据类型，并且想要更高的效率和可读性，那么 oneof 可能是一个好选择。如果你只需要传递几种整数值，并且想要最简单和最快速的方式，那么 enum 可能是一个好选择。

当然，这只是我的个人观点，你可以根据你自己的需求和偏好来决定哪种方法更适合你。😊

[1](https://stackoverflow.com/questions/65491268/what-the-difference-between-google-protobuf-any-and-google-protobuf-value): [What the difference between google.protobuf.Any and google.protobuf.Value? - Stack Overflow](https://stackoverflow.com/questions/65491268/what-the-difference-between-google-protobuf-any-and-google-protobuf-value) [2](https://learn.microsoft.com/en-us/dotnet/architecture/grpc-for-wcf-developers/protobuf-any-oneof): [Protobuf Any and Oneof fields for variant types - gRPC for WCF developers | Microsoft Learn](https://learn.microsoft.com/en-us/dotnet/architecture/grpc-for-wcf-developers/protobuf-any-oneof) [3](https://stackoverflow.com/questions/69185020/oneof-kind-vs-enum-in-protobuf): [Oneof kind vs Enum in protobuf - Stack Overflow](https://stackoverflow.com/questions/69185020/oneof-kind-vs-enum-in-protobuf)

