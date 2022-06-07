#ifndef ONVIF_CLIENT_H
#define ONVIF_CLIENT_H

#include <QNetworkAccessManager>
#include <QObject>

namespace ONVIF
{
class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject* parent = nullptr);
    QString sendData(const QString& url, const QString& data);
signals:
    void receiveData(const QString& data);

private:
    void waitForFinish(QNetworkReply* reply);

    QNetworkAccessManager manager;
};
} // namespace ONVIF

#endif // ONVIF_CLIENT_H
