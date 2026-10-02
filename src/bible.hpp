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
    QStringList bookNames() const;
    bool load(const QByteArray &bytes, QString &error);
    bool lookup(const QString &reference, Passage &result, QString &error) const;
    bool chapterVerses(const QString &reference, QVector<Passage> &result, QString &error) const;
    bool adjacent(const QString &reference, int direction, bool wholeChapter, Passage &result, QString &error) const;
private:
    QJsonObject books;
};
