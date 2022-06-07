#include "client.h"
#include <QDebug>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
using namespace ONVIF;

Client::Client(QObject* parent)
    : QObject(parent)
{
}

QString Client::sendData(const QString& url, const QString& data)
{
    // qDebug() << "send to url => " << url << " | data => " << data << "\n\r";
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(("application/soap+xml; charset=utf-8")));
    request.setUrl(QUrl(url));
    auto reply = manager.post(request, data.toUtf8());

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() == QNetworkReply::NoError)
    {
        // qDebug() << "fail " << reply->error() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) << reply->url();
        QString datas = reply->readAll();
        reply->deleteLater();
        return datas;
    }

    qDebug() << "reply error " << url << "\n\r" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) << "\n\r"
             << "data" << data;
    return "";
}

void Client::waitForFinish(QNetworkReply* reply)
{


    qDebug() << "received.";
    if (reply == nullptr)
    {
        qDebug() << "replay is nullptr";
        return;
    }

    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "fail " << reply->error() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) << reply->url();
        return;
    }

    const QByteArray bytes = reply->readAll();

    emit receiveData(bytes);
    // qDebug() << "data" << bytes;
}
