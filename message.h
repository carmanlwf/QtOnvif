#ifndef ONVIF_MESSAGE_H
#define ONVIF_MESSAGE_H

#include <QDomElement>
#include <QHash>
#include <QObject>


namespace ONVIF
{

QDomElement hashToXml(const QString& name, const QHash<QString, QString>& hash);
QDomElement listToXml(const QString& name, const QString& itemName, const QStringList& list);
QDomElement newElement(const QString& name, const QString& value = "");


class Message : public QObject
{
    Q_OBJECT
public:
    static Message* getMessageWithUserInfo(QHash<QString, QString>& namespaces, const QString& name, const QString& passwd);
    explicit Message(const QHash<QString, QString>& namespaces, QObject* parent = NULL);

    void appendToBody(const QDomElement& body);
    void appendToHeader(const QDomElement& header);

    QString toXmlStr();
    static QString uuid();

private:
    QDomDocument mDoc;
    QHash<QString, QString> mNamespaces;
    QDomElement mBody, mHeader, mEnv;
};


} // namespace ONVIF

#endif // ONVIF_MESSAGE_H
