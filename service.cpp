#include "service.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QTimer>
//#include <QUrlQuery>

using namespace ONVIF;


Q_GLOBAL_STATIC(Service, onvifHelper)

Service* Service::instance()
{
	return onvifHelper();
}

Service::Service(QObject* parent)
	: QObject(parent)
{

	mClient = new Client(this);
	mUdpSocket = new QUdpSocket(this);
	mUdpSocket->bind(QHostAddress::Any, 0, QUdpSocket::ShareAddress);
	connect(mUdpSocket, &QUdpSocket::readyRead, this, &Service::readPendingDatagrams);

	namesHash.insert("xsi", "http://www.w3.org/2001/XMLSchema-instance");
	namesHash.insert("wsa", "http://schemas.xmlsoap.org/ws/2004/08/addressing");
	namesHash.insert("d", "http://schemas.xmlsoap.org/ws/2005/04/discovery");
	namesHash.insert("tds", "http://www.onvif.org/ver10/device/wsdl");
	namesHash.insert("tt", "http://www.onvif.org/ver10/schema");
	namesHash.insert("trt", "http://www.onvif.org/ver10/media/wsdl");
}

Service::~Service()
{
	if (mClient != nullptr)
	{
		delete mClient;
		mClient = nullptr;
	}
}


MessageParser* Service::sendMessage(Message* message, const QString& url)
{
	if (message == nullptr)
	{
		return nullptr;
	}
	QString result = mClient->sendData(url, message->toXmlStr());
	// qDebug() << "receive data ==> " << result;
	if (result.isEmpty())
	{
		return nullptr;
	}
	return new MessageParser(result, namesHash);
}

void Service::searchDevices()
{

	QFile file(":/Resources/xml/search.xml");
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;
	QByteArray data;
	while (!file.atEnd())
	{
		data = file.readAll();
	}
	data = QString(data).arg(Message::uuid()).toUtf8();
	mUdpSocket->writeDatagram(data, QHostAddress("239.255.255.250"), 3702);
}

void Service::getDeviceInformation(const QString& ip)
{
	if (!devHash.contains(ip))
	{
		return;
	}
	auto dev = devHash.value(ip);
	QHash<QString, QString> names;
	Message* msg = Message::getMessageWithUserInfo(names, dev.szUserName, dev.szPassword);
	auto get = newElement("GetDeviceInformation");
	get.setAttribute("xmlns", "http://www.onvif.org/ver10/device/wsdl");
	msg->appendToBody(get);
	MessageParser* result = sendMessage(msg, dev.szOVFSerAddr);
	if (result != nullptr)
	{
		dev.szFileSysVer = result->getValue("//tds:FirmwareVersion");
		dev.szDeviceType = result->getValue("//tds:Manufacturer");
		dev.model = result->getValue("//tds:Model");

		dev.iDevStatus = 1;
		devHash.insert(ip, dev);
		getCapabilitiesMedia(ip);
	}
	else
	{
		dev.iDevStatus = 2; // user or password error
		devHash.insert(ip, dev);
	}

	emit updateDevInfo(ip);
}

void Service::getCapabilitiesMedia(const QString& ip)
{

	if (!devHash.contains(ip))
	{
		return;
	}
	auto dev = devHash.value(ip);
	QHash<QString, QString> names;
	Message* msg = Message::getMessageWithUserInfo(names, dev.szUserName, dev.szPassword);
	QDomElement cap = newElement("GetCapabilities");
	cap.setAttribute("xmlns", "http://www.onvif.org/ver10/device/wsdl");
	cap.appendChild(newElement("Category", "All"));
	msg->appendToBody(cap);
	MessageParser* result = sendMessage(msg, dev.szOVFSerAddr);
	if (result != NULL)
	{
		auto addr = result->getValue("//tt:Media/tt:XAddr");
		qDebug() << "addrinfo" << addr;
		dev.szOVFMediaSerAddr = addr;
		devHash.insert(ip, dev);
		getProfiles(ip);
	}
}

void Service::getProfiles(const QString& ip)
{
	if (!devHash.contains(ip))
	{
		return;
	}
	auto dev = devHash.value(ip);
	QHash<QString, QString> names;
	Message* msg = Message::getMessageWithUserInfo(names, dev.szUserName, dev.szPassword);

	auto get = newElement("GetProfiles");
	get.setAttribute("xmlns", "http://www.onvif.org/ver10/media/wsdl");
	msg->appendToBody(get);
	MessageParser* result = sendMessage(msg, dev.szOVFMediaSerAddr);

	if (result != NULL)
	{
		QXmlQuery* query = result->query();
		query->setQuery(result->nameSpace() + "doc($inputDocument)//trt:Profiles");
		QXmlResultItems items;
		query->evaluateTo(&items);
		QXmlItem item = items.next();
		QDomDocument doc;
		QString value, bounds, panTilt, zoom, profileNode;
		while (!item.isNull())
		{
			query->setFocus(item);
			query->setQuery(result->nameSpace() + ".");
			query->evaluateTo(&profileNode);
			doc.setContent(profileNode);
			QDomNodeList itemNodeList = doc.elementsByTagName("trt:Profiles");
			QDomNode node;
			for (int i = 0; i < itemNodeList.size(); i++)
			{
				auto tokenvalue = itemNodeList.at(i).toElement().attribute("token");
				qDebug() << "gettoken" << ip << tokenvalue;
				if (!tokenvalue.isEmpty())
				{
					dev.szOVFProTokenList.insert(tokenvalue);
				}
			}
			item = items.next();
		}
	}
	devHash.insert(ip, dev);
	getStreamUri(ip);
}

void Service::getStreamUri(const QString& ip)
{
	if (!devHash.contains(ip))
	{
		return;
	}
	auto dev = devHash.value(ip);
	QHash<QString, QString> names;
	Message* msg = Message::getMessageWithUserInfo(names, dev.szUserName, dev.szPassword);
	QDomElement stream = newElement("Stream", "RTP-Unicast");
	stream.setAttribute("xmlns", "http://www.onvif.org/ver10/schema");
	QDomElement transport = newElement("Transport");
	transport.setAttribute("xmlns", "http://www.onvif.org/ver10/schema");
	QDomElement protocol = newElement("Protocol", "UDP");
	QDomElement streamSetup = newElement("StreamSetup");
	QDomElement getStreamUri = newElement("GetStreamUri");

	for (auto&& token : dev.szOVFProTokenList)
	{
		QDomElement profileToken = newElement("ProfileToken", token);
		getStreamUri.setAttribute("xmlns", "http://www.onvif.org/ver10/media/wsdl");
		getStreamUri.appendChild(streamSetup);
		getStreamUri.appendChild(profileToken);
		streamSetup.appendChild(stream);
		streamSetup.appendChild(transport);
		transport.appendChild(protocol);
		msg->appendToBody(getStreamUri);
		MessageParser* result = sendMessage(msg, dev.szOVFMediaSerAddr);
		if (result != nullptr)
		{
			auto uri = result->getValue("//tt:Uri").trimmed();
			dev.szOVFUriList.insert(uri);
		}
	}
	devHash.insert(ip, dev);
}

void Service::readPendingDatagrams()
{
	while (mUdpSocket->hasPendingDatagrams())
	{
		QByteArray datagram;
		datagram.resize(mUdpSocket->pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		mUdpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

		// qDebug() << "========> \n" << datagram << "\n++++++++++++++++++++++++\n";
		MessageParser parser(QString(datagram), namesHash);
		ONVIF_DEV_INFO dev;

		auto addrPath = parser.getValue("//d:ProbeMatches/d:ProbeMatch/d:XAddrs");
		auto uuid = parser.getValue("//d:ProbeMatches/d:ProbeMatch/wsa:EndpointReference/wsa:Address");
		auto scopes = parser.getValue("//d:ProbeMatches/d:ProbeMatch/d:Scopes");
		// uuid
		dev.szDevSN = uuid.split(":").last();
		dev.szOVFSerAddr = addrPath.split(" ").first();

		// split url  for  ip
		auto strList = dev.szOVFSerAddr.split('/');
		const int size = 2; //
		QString m_strIPAddr;
		if (strList.size() > size && (!strList.at(size).isEmpty()))
		{
			m_strIPAddr = strList.at(size);
			//  split  ip prot  for  ip
			if (m_strIPAddr.contains(":"))
			{
				m_strIPAddr = m_strIPAddr.split(":").first();
			}

			dev.szIPAddr = m_strIPAddr;
		}
		auto list = scopes.split(" ");
		foreach (QString str, list)
		{
			QStringList l = str.split("/");
			if (l.contains("name"))
			{
				dev.szName = l.last();
			}
			else if (l.contains("location"))
			{
				dev.szLocation = l.last();
			}
			else if (l.contains("hardware"))
			{
				dev.szHardware = l.last();
			}
		}

		QString strName = "admin";
		QString strPassword = "admin123456";

		dev.szUserName = strName;
		dev.szPassword = strPassword;
		devHash.insert(dev.szIPAddr, dev);
		qDebug() << "Device =============>\n" << dev.szIPAddr << dev.szOVFSerAddr << dev.szDevSN;

		QTimer::singleShot(100, this, [m_strIPAddr, this]() { getDeviceInformation(m_strIPAddr); });
	}
}


QHash<QString, Service::ONVIF_DEV_INFO> Service::getAllDev() const
{
	return devHash;
}

QString Service::getUriInfo(const QString& ip, int type)
{
	auto&& dev = devHash.value(ip);
	auto index = qMin(type, dev.szOVFUriList.size() - 1);
	if (index < 0)
	{
		qDebug() << "no  data ";
		return "no url data";
	}

	qDebug() << type << index;
	auto list = dev.szOVFUriList.toList();
	return list.at(index);
}

void Service::modiflyDev(const ONVIF_DEV_INFO& dev)
{
	devHash.insert(dev.szIPAddr, dev);
	getDeviceInformation(dev.szIPAddr);
}
