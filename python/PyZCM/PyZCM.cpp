//
// Created by liuqi on 2025/8/2.
//

#include "PyZCM.h"

namespace py = pybind11;

// 模块初始化函数（参考PyKDL的入口设计）
PYBIND11_MODULE(PyZCM, m)
{
    m.doc() = "ZCM消息框架的Python绑定模块";
    m.attr("__version__") = "1.0.0";

    // 绑定ZCM核心类
    bind_zcm(m);
}