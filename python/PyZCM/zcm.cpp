//
// Created by liuqi on 2025/8/2.
//

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <zcm/zcm-cpp.hpp>

#ifndef ZCM_EMBEDDED
#include <zcm/eventlog.h>
#endif

namespace py = pybind11;
using namespace zcm;

// Python Subscription 包装器
class PySubscription
{
public:
    Subscription *sub;
    std::function<void(std::string, py::bytes, int64_t)> handler;

    PySubscription(Subscription *s, std::function<void(std::string, py::bytes, int64_t)> h)
        : sub(s), handler(std::move(h))
    {
    }

    static void callback(const ReceiveBuffer *rbuf, const std::string &channel, void *usr)
    {
        PySubscription *self = static_cast<PySubscription *>(usr);
        if (self)
        {
            py::gil_scoped_acquire gil;
            py::bytes data(reinterpret_cast<const char *>(rbuf->data), rbuf->data_size);
            self->handler(channel, data, rbuf->recv_utime);
        }
    }
};

void bind_zcm(pybind11::module &m)
{
    m.doc() = "ZCM C++ bindings using pybind11";
    m.attr("__version__") = "1.0.0";

    // ZCM 类绑定
    py::class_<ZCM, std::shared_ptr<ZCM>>(m, "ZCM")
        .def(py::init<>(), "默认构造函数")
#ifndef ZCM_EMBEDDED
        .def(py::init<const std::string &>(), py::arg("transport"), "带传输字符串的构造函数")
#endif
        .def("good", &ZCM::good)
        .def("err", &ZCM::err)
        .def("strerror", &ZCM::strerror)
        .def("strerrno", &ZCM::strerrno, py::arg("err"))

#ifndef ZCM_EMBEDDED
        .def("run", &ZCM::run)
        .def("start", &ZCM::start)
        .def("stop", &ZCM::stop)
        .def("pause", &ZCM::pause)
        .def("resume", &ZCM::resume)
        .def("handle", &ZCM::handle)
        .def("setQueueSize", &ZCM::setQueueSize, py::arg("size"))
        .def("writeTopology", &ZCM::writeTopology, py::arg("filename"))
#endif
        .def("handleNonblock", &ZCM::handleNonblock)
        .def("flush", &ZCM::flush)

        // 发布原始数据
        .def(
            "publish_raw",
            [](ZCM &self, const std::string &channel, py::bytes data)
            {
                std::string d = data;
                return self.publish(channel, reinterpret_cast<const uint8_t *>(d.data()),
                                    static_cast<uint32_t>(d.size()));
            },
            py::arg("channel"), py::arg("data"), "发布原始字节数据")

        // 发布消息对象（msg应实现encode方法）
        .def(
            "publish",
            [](ZCM &self, const std::string &channel, py::object msg)
            {
                py::bytes b = msg.attr("encode")();
                std::string d = b;
                return self.publish(channel, reinterpret_cast<const uint8_t *>(d.data()),
                                    static_cast<uint32_t>(d.size()));
            },
            py::arg("channel"), py::arg("msg"), "发布msg对象（Python应实现encode()）")

        // 订阅（返回 capsule）
        .def(
            "subscribe",
            [](ZCM &self, const std::string &channel, std::function<void(std::string, py::bytes, int64_t)> callback)
            {
                auto *py_sub = new PySubscription(nullptr, callback);
                Subscription *sub = self.subscribe(channel, PySubscription::callback, py_sub);
                py_sub->sub = sub;
                return py::capsule(py_sub,
                                   [](void *p)
                                   {
                                       delete static_cast<PySubscription *>(p);
                                   });
            },
            py::arg("channel"), py::arg("callback"), "订阅频道，回调(channel:str, data:bytes, timestamp:int)")

        // 取消订阅
        .def(
            "unsubscribe",
            [](ZCM &self, py::capsule capsule)
            {
                auto *py_sub = capsule.get_pointer();
                if (!py_sub)
                    throw std::runtime_error("无效的订阅对象");
                Subscription *sub = static_cast<PySubscription *>(py_sub)->sub;
                self.unsubscribe(sub);
            },
            py::arg("subscription"), "取消订阅");

#ifndef ZCM_EMBEDDED
    // LogEvent 绑定
    py::class_<LogEvent>(m, "LogEvent")
        .def(py::init<>())
        .def_property(
            "eventnum",
            [](const LogEvent &e)
            {
                return e.eventnum;
            },
            [](LogEvent &e, int64_t v)
            {
                e.eventnum = v;
            })
        .def_property(
            "timestamp",
            [](const LogEvent &e)
            {
                return e.timestamp;
            },
            [](LogEvent &e, int64_t v)
            {
                e.timestamp = v;
            })
        .def_property(
            "channel",
            [](const LogEvent &e)
            {
                return e.channel;
            },
            [](LogEvent &e, const std::string &v)
            {
                e.channel = v;
            })
        .def_property(
            "datalen",
            [](const LogEvent &e)
            {
                return e.datalen;
            },
            [](LogEvent &e, int32_t v)
            {
                e.datalen = v;
            })
        .def_property(
            "data",
            [](const LogEvent &e)
            {
                return py::bytes(reinterpret_cast<const char *>(e.data), e.datalen);
            },
            [](LogEvent &e, py::bytes b)
            {
                std::string s = b;
                e.data = reinterpret_cast<uint8_t *>(const_cast<char *>(s.data()));
                e.datalen = static_cast<int32_t>(s.size());
            });

    // LogFile 绑定
    py::class_<LogFile>(m, "LogFile")
        .def(py::init<const std::string &, const std::string &>(), py::arg("path"), py::arg("mode"))
        .def("good", &LogFile::good)
        .def("close", &LogFile::close)
        .def("seekToTimestamp", &LogFile::seekToTimestamp, py::arg("timestamp"))
        .def("readNextEvent", &LogFile::readNextEvent, py::return_value_policy::reference)
        .def("readPrevEvent", &LogFile::readPrevEvent, py::return_value_policy::reference)
        .def("readEventAtOffset", &LogFile::readEventAtOffset, py::arg("offset"), py::return_value_policy::reference)
        .def("writeEvent", &LogFile::writeEvent, py::arg("event"))
        .def("getFilePtr", &LogFile::getFilePtr, py::return_value_policy::reference);
#endif
}