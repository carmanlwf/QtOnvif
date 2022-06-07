#include "message.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QStringList>
#include <QTime>
#include <QUuid>
using namespace ONVIF;


QDomElement ONVIF::hashToXml(const QString& name, const QHash<QString, QString>& hash)
{
    QDomElement element = newElement(name);
    QHashIterator<QString, QString> i(hash);
    while (i.hasNext())
    {
        i.next();
        element.appendChild(newElement(i.key(), i.value()));
    }
    return element;
}

QDomElement ONVIF::listToXml(const QString& name, const QString& itemName, const QStringList& list)
{
    QDomElement element = newElement(name);
    for (int i = 0; i < list.length(); i++)
    {
        element.appendChild(newElement(itemName, list.at(i)));
    }
    return element;
}

QDomElement ONVIF::newElement(const QString& name, const QString& value)
{
    QDomDocument doc;
    QDomElement element = doc.createElement(name);
    if (value != "")
    {
        QDomText textNode = doc.createTextNode(value);
        element.appendChild(textNode);
    }
    doc.appendChild(element);
    return doc.firstChildElement();
}


Message* Message::getMessageWithUserInfo(QHash<QString, QString>& namespaces, const QString& name, const QString& passwd)
{
    Message* msg = new Message(namespaces);
    QDomElement security = newElement("Security");
    security.setAttribute("s:mustUnderstand", "1");
    security.setAttribute("xmlns", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd");
    QDomElement usernameToken = newElement("UsernameToken");
    QDomElement username = newElement("Username", name);

    QString forrmat = "yyyy-MM-dd'T'HH:mm:ss.zzzZ";
    QString nonce = QDateTime::currentDateTimeUtc().toString(forrmat);
    const QString& date = nonce;
    QCryptographicHash md(QCryptographicHash::Sha1);
    md.addData(nonce.toUtf8());
    md.addData(date.toUtf8());
    md.addData(passwd.toUtf8());
    auto result = md.result().toBase64();

    QDomElement password = newElement("Password", result);
    password.setAttribute("Type", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordDigest");

    QDomElement nonceEle = newElement("Nonce", nonce.toUtf8().toBase64());
    nonceEle.setAttribute("EncodingType", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soap-message-security-1.0#Base64Binary");

    QDomElement created = newElement("Created", date);
    created.setAttribute("xmlns", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd");

    usernameToken.appendChild(username);
    usernameToken.appendChild(password);
    usernameToken.appendChild(nonceEle);
    usernameToken.appendChild(created);

    security.appendChild(usernameToken);
    msg->appendToHeader(security);
    // qDebug() << "msg xml" << msg->toXmlStr();
    return msg;
}


Message::Message(const QHash<QString, QString>& namespaces, QObject* parent)
    : QObject(parent)
{
    this->mNamespaces = namespaces;
    mDoc.appendChild(mDoc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""));
    mEnv = mDoc.createElementNS("http://www.w3.org/2003/05/soap-envelope", "s:Envelope");
    mEnv.setAttribute("xmlns:trt", "http://www.onvif.org/ver10/media/wsdl");

    mHeader = mDoc.createElement("s:Header");

    mBody = mDoc.createElement("s:Body");
    mBody.setAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema");
    mBody.setAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema-instance");
}

QString Message::toXmlStr()
{
    QHashIterator<QString, QString> i(mNamespaces);
    while (i.hasNext())
    {
        i.next();
        mEnv.setAttribute("xmlns:" + i.key(), i.value());
    }
    mEnv.appendChild(mHeader);
    mEnv.appendChild(mBody);
    mDoc.appendChild(mEnv);
    return mDoc.toString();
}

QString Message::uuid()
{
    QUuid id = QUuid::createUuid();
    return id.toString();
}

void Message::appendToBody(const QDomElement& body)
{
    mBody.appendChild(body);
}

void Message::appendToHeader(const QDomElement& header)
{
    mHeader.appendChild(header);
}
