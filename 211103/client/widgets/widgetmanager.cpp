#include "widgetmanager.h"

WidgetManager::WidgetManager()
{
    lw = new LoginWidget;
    hw = new HallWidget;
    row = new RoomOwnerWidget;
}

WidgetManager::~WidgetManager()
{
    lw->deleteLater();
    hw->deleteLater();
    row->deleteLater();
}

void WidgetManager::clear()
{
    lw->hide();
    hw->hide();
    row->hide();

    for(auto it = rgws.begin(); it != rgws.end(); ++it) {
        (*it)->deleteLater();
    }
}

RoomGuestWidget* WidgetManager::createRoomGuestWidget(int roomid, const QString& username)
{
    // 1. 去重
    for(auto& widget : rgws) {
        if(widget->getRoomId() == roomid) return  nullptr;
    }

    RoomGuestWidget* widget = new RoomGuestWidget;
    widget->setRoomId(roomid);
    widget->setUsername(username);

    rgws.append(widget);

    return widget;
}

RoomGuestWidget* WidgetManager::selectRoomGuestWidget(int roomid)
{
    for(auto it = rgws.begin(); it != rgws.end(); ++it) {
        if((*it)->getRoomId() == roomid) return *it;
    }

    return nullptr;
}

void WidgetManager::removeRoomGuestWidget(int roomid)
{
    for(auto it = rgws.begin(); it != rgws.end(); ++it) {
        if((*it)->getRoomId() == roomid) {
            (*it)->deleteLater();
            rgws.erase(it);

            break;
        }
    }
}
