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