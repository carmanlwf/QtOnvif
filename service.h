#ifndef ONVIF_SERVICE_H
#define ONVIF_SERVICE_H

#include "client.h"
#include "message.h"
#include "messageparser.h"
#include <QObject>
#include <QSet>
#include <QUdpSocket>
namespace ONVIF
{
class Service : public QObject
{
    Q_OBJECT
public:
    // static Service* onvifHelper;
    static Service* instance();

    explicit Service(QObject* parent = nullptr);

    ~Service() override;

    MessageParser* sendMessage(Message* message, const QString& url);
    using ONVIF_DEV_INFO = struct ONVIF_DEV_INFO
    {

        QString szUserName;
        QString szPassword;
        QString szIPAddr;
        QString szDevSN;
        QString szFileSysVer;
        QString szDeviceType;
        int iDevStatus{};
        QString szOVFSerAddr;
        QString szName;
        QString szHardware;
        QString szLocation;
        QString szOVFMediaSerAddr;
        QString model;
        QSet<QString> szOVFProTokenList;
        QSet<QString> szOVFUriList;
    };

    // Search

    void searchDevices();

    // device
    void getDeviceInformation(const QString& ip);

    void getCapabilitiesMedia(const QString& ip);

    // media
    void getProfiles(const QString& ip);

    void getStreamUri(const QString& ip);

    //
    void modiflyDev(const ONVIF_DEV_INFO& dev);

    QString getUriInfo(const QString& ip, int type);

    [[nodiscard]] QHash<QString, ONVIF_DEV_INFO> getAllDev() const;

signals:
    void updateDevInfo(const QString& ip);

private slots:
    void readPendingDatagrams();

private:
    QUdpSocket* mUdpSocket;
    // QString mUsername, mPassword;
    Client* mClient; // tcp client
    QHash<QString, ONVIF_DEV_INFO> devHash;
    QHash<QString, QString> namesHash;
};
} // namespace ONVIF


#endif // ONVIF_SERVICE_H
