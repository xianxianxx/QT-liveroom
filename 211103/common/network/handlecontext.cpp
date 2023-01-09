#include "handlecontext.h"
#include "packer.h"
#include "packet.h"
#include "handler.h"

HandleContext::HandleContext(Packer* packer, QVector<Handler*> handlers)
    : packer(packer), handlers(handlers)
{
}

HandleContext::~HandleContext()
{
    qDeleteAll(handlers);
    handlers.clear();
}

void HandleContext::handle()
{
    // 1. 根据缓存进行拆包处理
    Packet packet;

    while(1) {
        int ret = packer->unpack(buffer, packet);
        if(ret < 0) {
            // 后面的缓存中的数据错误
            buffer.clear();
            break;
        }else if(ret == 0) break;

        buffer = buffer.mid(ret);

        // 调度
        handle(packet);
    }
}

void HandleContext::handle(const Packet& packet)
{
    int idx = packet.getType();
    if(idx < 0 || idx >= handlers.size()) return;

    Handler* handler = handlers[idx];
    if(handler == nullptr) return;

    handler->handle(packet);
}

Handler* HandleContext::getHandler(int idx)
{
    if(idx < 0 || idx >= handlers.size()) return nullptr;
    return handlers[idx];
}

void HandleContext::reset()
{
    for(auto handler : handlers) {
        if(handler) handler->reset();
    }
}
