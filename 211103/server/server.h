#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>

/**
 * @class Server
 * @brief 管理建立连接
 * @details
 *  当新的连接建立后，委托给ServerSocket进行连接的管理
 */
class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);

    bool listen(const QString& addr, int port);

private slots:
    void onNewConnection();

signals:

private:
    QTcpServer* server;
};

#endif // SERVER_H
