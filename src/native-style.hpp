#pragma once
#include <QFont>
#include <QString>
inline QString verseFontStyle(const QFont &font,int pixels)
{
    QString family=font.family();family.replace("\\","\\\\");family.replace("\"","\\\"");
    return QStringLiteral("QLabel#verse {font-family:\"%1\";font-size:%2px;font-weight:%3;font-style:%4;}")
        .arg(family).arg(pixels).arg(font.bold()?QStringLiteral("bold"):QStringLiteral("normal"))
        .arg(font.italic()?QStringLiteral("italic"):QStringLiteral("normal"));
}
