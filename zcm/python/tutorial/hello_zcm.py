import sys
import os
import ctypes

# 替换为你的编译产物目录的绝对路径
zerocm_path = "/mnt/d/ClionProjects/zcm_cmake/zcm/python/build/lib.linux-x86_64-3.10"  # Linux 示例

# 将路径添加到 Python 搜索路径
sys.path.insert(0, zerocm_path)

# 手动预加载 libzcm.so
ctypes.CDLL("/mnt/d/ClionProjects/zcm_cmake/cmake-build-debug/lib/libzcm.so")

# 现在可以导入 zerocm 了
import zerocm
print("zerocm 模块路径：", zerocm.__file__)  # 验证是否导入正确路径的模块
