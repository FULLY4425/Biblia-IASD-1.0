#pragma once
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSaveFile>
#include <QUrlQuery>
#include <functional>

inline void loadCachedFonts(const QString &directory)
{
    QDir dir(directory);
    for (const auto &file : dir.entryList({"*.ttf","*.otf"},QDir::Files))
        QFontDatabase::addApplicationFont(dir.filePath(file));
}
inline void downloadGoogleFont(QNetworkAccessManager *manager, const QString &family,
                               const QString &directory, std::function<void(QString,QString)> done)
{
    if (family.trimmed().isEmpty() || family.size()>100) { done({},QStringLiteral("Escribe el nombre de la fuente.")); return; }
    QUrl url("https://fonts.googleapis.com/css"); QUrlQuery query; query.addQueryItem("family",family.trimmed()); url.setQuery(query);
    QNetworkRequest request(url); request.setRawHeader("User-Agent","Mozilla/4.0"); request.setTransferTimeout(20000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
    auto *reply=manager->get(request);
    QObject::connect(reply,&QNetworkReply::downloadProgress,reply,[reply](qint64 received,qint64){if(received>1024*1024) reply->abort();});
    QObject::connect(reply,&QNetworkReply::finished,manager,[=] {
        const auto bytes=reply->readAll(); const bool ok=reply->error()==QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200;
        reply->deleteLater();
        const auto match=QRegularExpression("url\\((https://fonts\\.gstatic\\.com/[^)]+)\\)").match(QString::fromUtf8(bytes));
        if(!ok || !match.hasMatch()){done({},QStringLiteral("Google Fonts no respondió con una fuente compatible. Puedes importar un TTF/OTF local."));return;}
        const QUrl fontUrl(match.captured(1));
        if(fontUrl.scheme()!="https" || fontUrl.host()!="fonts.gstatic.com"){done({},QStringLiteral("Dirección de fuente inválida."));return;}
        QNetworkRequest fontRequest(fontUrl); fontRequest.setTransferTimeout(20000);
        fontRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
        auto *fontReply=manager->get(fontRequest);
        QObject::connect(fontReply,&QNetworkReply::downloadProgress,fontReply,[fontReply](qint64 received,qint64){if(received>10*1024*1024)fontReply->abort();});
        QObject::connect(fontReply,&QNetworkReply::finished,manager,[=] {
            auto data=fontReply->readAll(); const bool downloaded=fontReply->error()==QNetworkReply::NoError && fontReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200;
            fontReply->deleteLater();
            if(!downloaded || data.size()>10*1024*1024){done({},QStringLiteral("No se pudo descargar la fuente."));return;}
            const int fontId=QFontDatabase::addApplicationFontFromData(data);
            const auto families=QFontDatabase::applicationFontFamilies(fontId);
            if(fontId<0 || families.isEmpty()){done({},QStringLiteral("Qt no admite el formato recibido. Importa un TTF/OTF local."));return;}
            QDir().mkpath(directory);
            QSaveFile file(QDir(directory).filePath(QString::fromLatin1(QCryptographicHash::hash(data,QCryptographicHash::Sha256).toHex())+".ttf"));
            if(!file.open(QIODevice::WriteOnly)||file.write(data)!=data.size()||!file.commit()){done({},QStringLiteral("Fuente descargada, pero no se pudo guardar para uso sin conexión."));return;}
            done(families.first(),{});
        });
    });
}
