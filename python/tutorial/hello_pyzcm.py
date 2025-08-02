import sys
import os
import ctypes

# 替换为你的编译产物目录的绝对路径
pyzcm_path = "/mnt/d/ClionProjects/zcm_cmake/cmake-build-debug/lib/"  # Linux 示例

# 将路径添加到 Python 搜索路径
sys.path.insert(0, pyzcm_path)

# 手动预加载 libzcm.so
ctypes.CDLL("/mnt/d/ClionProjects/zcm_cmake/cmake-build-debug/lib/libzcm.so")


from pyzcm import ZCM, LogFile, LogEvent


def on_msg(channel, data, timestamp):
    print(f"[{timestamp}] {channel}: {data}")

# 实例化
zcm = ZCM("ipc")

# 订阅
sub = zcm.subscribe("chatter", on_msg)

# 发布原始数据
zcm.publish_raw("chatter", b"hello")

# 发布结构化消息（msg 需实现 encode()）
class DummyMsg:
    def encode(self):
        return b"structured-message"

zcm.publish("chatter", DummyMsg())

# 日志写入
log = LogFile("test.zcm", "w")
evt = LogEvent()
evt.channel = "chatter"
evt.timestamp = 123456789
evt.data = b"hello"
log.writeEvent(evt)
log.close()

# 日志读取
log = LogFile("test.zcm", "r")
e = log.readNextEvent()
print(e.channel, e.timestamp, e.data)