# chatroom

## 简介

这是一个简易的聊天室，用于练习linux网络编程，数据库基本操作，C++，Cmake...

聊天室的服务器基于TCP协议，epoll的ET模式构建，单线程单epoll，线程池管理多线程处理请求的reactor模型。

客户端比较简易，两个线程，一个接受消息，一个发送消息，采用不阻塞的方式与服务器连接。

数据通过mysql进行管理和操作, 使用json序列化和反序列化。

C++书写，通过Cmake管理多文件编译。

代码量5000行左右。

## 依赖

libesmtp-devel 用于发邮件

mysql-connector-c mysql的cAPI接口

openssl 邮件依赖

arch linux配置环境

```
yay libesmtp-devel mysql-connector-c openssl
```

## 功能

### 登录界面

实现了登录，注册，忘记密码，注销账号等基本操作。

实现了通过smtp协议在注册和忘记密码时向邮箱发送验证码。

### command界面

command界面是该聊天室一切的开始，输入command会出现command菜单：

1.私聊		2.群聊		3.发送文件		4.查看文件		5.好友操作

6.群操作	    7.接受文件        8.历史记录                9.退出

如不进入command，输入的信息所有人都能受到消息，相当于网游中的世界频道。

### 好友功能

1. 添加好友
2. 删除好友
3. 屏蔽好友
4. 解除屏蔽
5. 好友列表(显示在线情况)
6. 好友间的私聊
7. 发送和接受文件
8. 查看聊天记录

### 群聊操作

1. 创建群聊
2. 加入群聊
3. 退出群聊
4. 解散群聊
5. 群聊列表
6. 设置管理员
7. 取消管理员
8. 踢人
9. 群聊成员列表
10. 申请列表
11. 同意进群
12. 查看历史记录

## 遇到的困难

发送信息，如何让服务器进行不同的信息不同处理，后来我通过采用向信息头加入标志位，让服务器识别并处理。

文件发送和接收时，不知道该怎么办，后来我通过将发送的文件储存在服务器那边，将文件路径储存到数据库中，这样就可以实现随时接收文件了。

