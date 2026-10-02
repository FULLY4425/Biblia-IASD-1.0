#pragma once
#include <QString>
#include <QRegularExpression>
inline QString cleanPassage(QString text,bool joinLines,bool removeNotes)
{
    if(removeNotes)text.remove(QRegularExpression(QStringLiteral("\\[\\d+\\]")));
    if(joinLines){
        // Numbered lines delimit verses; keep those boundaries intact.
        const auto lines=text.split(QRegularExpression(QStringLiteral("[\\r\\n]+")));
        QString joined;
        for(const auto &line:lines){
            if(!joined.isEmpty())joined+=QRegularExpression(QStringLiteral("^\\d+\\. ")).match(line).hasMatch()?'\n':' ';
            joined+=line;
        }
        text=joined;
    }
    return text;
}
inline QString slideLetter(int index)
{
    QString result;
    do {result.prepend(QChar('a'+index%26));index=index/26-1;}while(index>=0);
    return result;
}
