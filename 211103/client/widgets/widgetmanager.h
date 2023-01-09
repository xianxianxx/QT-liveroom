#ifndef WIDGETMANAGER_H
#define WIDGETMANAGER_H

#include "loginwidget.h"
#include "hallwidget.h"
#include "roomownerwidget.h"
#include "roomguestwidget.h"
#include "singleton.h"

class WidgetManager
{
friend class SingleTon<WidgetManager>;
private:
    WidgetManager();
    WidgetManager(const WidgetManager&)=delete;
    WidgetManager(WidgetManager&&)=delete;
    WidgetManager& operator=(const WidgetManager&)=delete;
    WidgetManager& operator=(WidgetManager&&)=delete;

public:
    virtual ~WidgetManager();
    void clear();

    LoginWidget* loginWidget() const{ return lw; }
    HallWidget* hallWidget() const{ return hw; }
    RoomOwnerWidget* roomOwnerWidget() const{ return row; }

    RoomGuestWidget* createRoomGuestWidget(int roomid, const QString& username);
    RoomGuestWidget* selectRoomGuestWidget(int roomid);
    void removeRoomGuestWidget(int roomid);

private:
    LoginWidget* lw;
    HallWidget* hw;

    RoomOwnerWidget* row;
    QList<RoomGuestWidget*> rgws;
};

#endif // WIDGETMANAGER_H
