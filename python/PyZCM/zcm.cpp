//
// Created by liuqi on 2025/8/2.
//

#include <iostream>
#include <iomanip>
#include <zcm/zcm-cpp.hpp>
#include "PyZCM.h"

namespace py = pybind11;
using namespace zcm;

// 前向声明Subscription类的绑定函数
void bind_subscription(py::module &m);

// ZCM类的绑定实现
void bind_zcm(py::module &m)
{
    // 绑定zcm::ZCM类，与PyKDL中Joint类的绑定风格一致
    py::class_<ZCM> zcm_class(m, "ZCM");

    // 绑定构造函数（多个重载版本）
    zcm_class.def(py::init<>(), "默认构造函数，使用空传输字符串");
#ifndef ZCM_EMBEDDED
    zcm_class.def(py::init<const std::string &>(), py::arg("transport"),
                  "带传输字符串的构造函数，例如\"ipc\"或\"udp://...\"");
#endif
    zcm_class.def(py::init<zcm_trans_t *>(), py::arg("zt"), "使用已有zcm_trans_t指针的构造函数");

    // 绑定状态检查与错误处理方法
    zcm_class.def("good", &ZCM::good, "检查ZCM初始化是否成功，返回布尔值");
    zcm_class.def("err", &ZCM::err, "返回错误代码");
    zcm_class.def("strerror", &ZCM::strerror, "返回当前错误的字符串描述");
    zcm_class.def("strerrno", &ZCM::strerrno, py::arg("err"), "将错误代码转换为字符串描述");

    // 绑定消息处理方法
#ifndef ZCM_EMBEDDED
    zcm_class.def("run", &ZCM::run, "启动事件循环，处理消息直到stop()被调用");
    zcm_class.def("start", &ZCM::start, "启动异步消息处理线程");
    zcm_class.def("stop", &ZCM::stop, "停止异步消息处理线程");
    zcm_class.def("pause", &ZCM::pause, "暂停消息处理");
    zcm_class.def("resume", &ZCM::resume, "恢复消息处理");
    zcm_class.def("handle", &ZCM::handle, "处理所有待处理的消息，阻塞直到完成");
    zcm_class.def("setQueueSize", &ZCM::setQueueSize, py::arg("sz"), "设置消息队列大小");
    zcm_class.def("writeTopology", &ZCM::writeTopology, py::arg("name"), "写入拓扑信息到指定文件");
#endif
    zcm_class.def("handleNonblock", &ZCM::handleNonblock, "非阻塞方式处理消息，立即返回");
    zcm_class.def("flush", &ZCM::flush, "刷新输出缓冲区");

    // 绑定消息发布方法
    zcm_class.def("publish", static_cast<int (ZCM::*)(const std::string &, const uint8_t *, uint32_t)>(&ZCM::publish),
                  py::arg("channel"), py::arg("data"), py::arg("len"), "发布原始字节数据到指定频道");

    // 绑定模板版本的publish方法（需要显式实例化常用类型）
    zcm_class.def(
        "publish_msg",
        [](ZCM &self, const std::string &channel, const void *msg, size_t size)
        {
            return self.publish(channel, static_cast<const uint8_t *>(msg), size);
        },
        py::arg("channel"), py::arg("msg"), py::arg("size"), "发布消息对象到指定频道");

    // 绑定订阅方法（处理函数指针回调）
    zcm_class.def(
        "subscribe",
        static_cast<Subscription *(ZCM::*)(const std::string &,
                                           void (*)(const ReceiveBuffer *, const std::string &, void *), void *)>(
            &ZCM::subscribe),
        py::arg("channel"), py::arg("cb"), py::arg("usr"), "订阅频道，使用C风格函数指针作为回调",
        py::return_value_policy::reference);

    // 绑定C++11及以上的std::function订阅方法
#if __cplusplus > 199711L
    zcm_class.def("subscribe",
                  static_cast<Subscription *(ZCM::*)(const std::string &,
                                                     std::function<void(const ReceiveBuffer *, const std::string &)>)>(
                      &ZCM::subscribe),
                  py::arg("channel"), py::arg("cb"), "订阅频道，使用std::function作为回调（C++11及以上）",
                  py::return_value_policy::reference);
#endif

    // 绑定取消订阅方法
    zcm_class.def("unsubscribe", &ZCM::unsubscribe, py::arg("sub"), "取消指定的订阅", py::arg("sub"));

    // 绑定获取底层ZCM对象的方法
    zcm_class.def("getUnderlyingZCM", &ZCM::getUnderlyingZCM, "获取底层的zcm_t*对象指针",
                  py::return_value_policy::reference);

    // 绑定__repr__方法，方便调试
    zcm_class.def("__repr__",
                  [](const ZCM &z)
                  {
                      return std::string("ZCM(good=") + (z.good() ? "True" : "False") + ")";
                  });

    // 绑定Subscription类
    bind_subscription(m);

#ifndef ZCM_EMBEDDED
    // 绑定LogEvent结构体
    py::class_<LogEvent>(m, "LogEvent")
        .def(py::init<>())
        .def_readwrite("eventnum", &LogEvent::eventnum)
        .def_readwrite("timestamp", &LogEvent::timestamp)
        .def_readwrite("channel", &LogEvent::channel)
        .def_readwrite("datalen", &LogEvent::datalen)
        .def_readwrite("data", &LogEvent::data);

    // 绑定LogFile结构体
    py::class_<LogFile>(m, "LogFile")
        .def(py::init<const std::string &, const std::string &>(), py::arg("path"), py::arg("mode"))
        .def("good", &LogFile::good)
        .def("close", &LogFile::close)
        .def("seekToTimestamp", &LogFile::seekToTimestamp, py::arg("timestamp"))
        .def("getFilePtr", &LogFile::getFilePtr, py::return_value_policy::reference)
        .def("readNextEvent", &LogFile::readNextEvent, py::return_value_policy::reference)
        .def("readPrevEvent", &LogFile::readPrevEvent, py::return_value_policy::reference)
        .def("readEventAtOffset", &LogFile::readEventAtOffset, py::arg("offset"), py::return_value_policy::reference)
        .def("writeEvent", &LogFile::writeEvent, py::arg("event"));
#endif
}

// 绑定Subscription类（前向声明的处理）
void bind_subscription(py::module &m)
{
    py::class_<Subscription>(m, "Subscription")
        .def(py::init<>())
        .def("__repr__",
             [](const Subscription &sub)
             {
                 return "Subscription()";
             });
}

