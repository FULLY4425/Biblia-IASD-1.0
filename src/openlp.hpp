#pragma once
#include <QJsonObject>
#include <QString>
bool convertOpenLp(const QString &path, const QString &id, const QString &name,
                   const QString &license, QJsonObject &output, QString &error);
