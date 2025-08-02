```shell
pip install cython
```
在zerocm.pyx和zcm-python.h同目录下创建setup.py，用于配置 Cython 编译选项
```python
from setuptools import setup, Extension
from Cython.Build import cythonize
import os
import sys

# 配置ZCM库和头文件路径
# 方法1：自动检测（推荐，需ZCM安装时生成pkg-config文件）
try:
    import pkgconfig
    if not pkgconfig.exists("zcm"):
        raise Exception("ZCM库未找到，请检查安装")
    # 获取ZCM的编译和链接选项
    zcm_cflags = pkgconfig.cflags("zcm").split()
    zcm_libs = pkgconfig.libs("zcm").split()
    include_dirs = []
    library_dirs = []
except:
    # 方法2：手动指定ZCM安装路径（若pkg-config不可用）
    # zcm_prefix = os.environ.get("ZCM_PREFIX", "/usr/local")  # ZCM安装根目录
    zcm_prefix = os.environ.get("ZCM_PREFIX", "/mnt/d/ClionProjects/zcm_cmake/")  # ZCM安装根目录
    include_dirs = [
        os.path.abspath(zcm_prefix),
        os.path.join(zcm_prefix, "zcm"),  # ZCM头文件目录（含zcm.h）
        os.path.dirname(os.path.abspath(__file__))  # 当前目录（含zcm-python.h）
    ]
    library_dirs = [
        os.path.join(zcm_prefix, "cmake-build-debug/lib")
    ]
    zcm_cflags = []
    zcm_libs = ["zcm"]  # ZCM库名

# 定义扩展模块
ext_modules = [
    Extension(
        name="zerocm",  # Python中导入的模块名（需与文件名对应）
        sources=["zerocm.pyx"],  # Cython源文件
        include_dirs=include_dirs,
        library_dirs=library_dirs,
        libraries=zcm_libs,
        extra_compile_args=zcm_cflags + ["-std=c99"],  # ZCM基于C99
        language="c"  # 绑定的是ZCM的C接口
    )
]

# 编译配置
setup(
    name="zerocm",
    version="1.0.0",
    description="ZCM消息框架的Cython绑定",
    ext_modules=cythonize(ext_modules, language_level=2)  # 对应pyx中的language_level=2
)

```

```shell
# 编译并安装到当前Python环境（系统级安装需sudo，用户级加--user）
python setup.py install --user

# 若仅需编译不安装（生成的模块在当前脚本所在目录下的build/lib.xxx/目录）
python setup.py build
```

编译成功后，会生成类似zerocm.cpython-38-x86_64-linux-gnu.so（Linux）的文件。通过以下命令验证：
```shell
python -c "import zerocm; print('ZCM Python绑定导入成功')"
```

编写自定义消息文件
```c++
struct ExampleMessage 
{
    int64_t timestamp;  // 时间戳
    double  value;      // 数值
    string  name;       // 名称
}
```

```shell
zcm_gen --python ExampleMessage.zcm  # 生成ExampleMessage.py（包含ExampleMessage类）
```

Python 中发布和订阅消息
```python

import zerocm
from example import ExampleMessage  # 导入生成的消息类

# 1. 初始化ZCM（传输方式：ipc/udp/tcp）
zcm = zerocm.ZCM("ipc")  # 本地进程间通信，或"udp://239.255.76.67:7667"
if not zcm.good():
    print("ZCM初始化失败！")
    exit(1)

# 2. 定义订阅回调函数
#   回调参数：channel（频道名）、msg（消息对象）、recv_utime（接收时间戳）
def on_message(channel, msg, recv_utime):
    print(f"收到频道 {channel} 的消息：")
    print(f"接收时间戳：{recv_utime}")
    print(f"消息时间戳：{msg.timestamp}")
    print(f"消息数值：{msg.value}")
    print(f"消息名称：{msg.name}\n")

# 3. 订阅频道（绑定消息类型和回调）
#   参数：频道名、消息类、回调函数
sub = zcm.subscribe("EXAMPLE_CHANNEL", ExampleMessage, on_message)

# 4. 创建并发布消息
msg = ExampleMessage()
msg.timestamp = 1620000000  # 消息时间戳
msg.value = 3.14159         # 数值
msg.name = "Hello from Python"  # 名称
zcm.publish("EXAMPLE_CHANNEL", msg)  # 发布到频道

# 5. 处理消息（阻塞等待，或设置超时）
print("等待消息...（按Ctrl+C退出）")
try:
    while True:
        zcm.handle()  # 处理所有待处理消息
except KeyboardInterrupt:
    print("退出程序")

# 6. 清理资源（可选，__del__会自动处理）
zcm.unsubscribe(sub)
zcm.stop()
```

