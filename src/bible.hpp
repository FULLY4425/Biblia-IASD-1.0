#pragma once
#include <QJsonObject>
#include <QString>
#include <QVector>

struct Passage {
    QString reference;
    QString text;
};

class Bible {
public:
    QString id, name, license;
    bool load(const QByteArray &bytes, QString &error);
    bool lookup(const QString &reference, Passage &result, QString &error) const;
private:
    QJsonObject books;
};
