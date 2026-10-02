#pragma once
#include <QString>
#include <QRegularExpression>
#include <QPainter>
inline void paintPassageBand(QPainter &painter,const QRect &rect,QColor color,int opacity,bool enabled)
{
    if(!enabled)return;
    color.setAlpha(qBound(0,opacity,100)*255/100);
    painter.fillRect(rect,color);
    auto header=color.lighter(140);header.setAlpha(color.alpha());
    painter.fillRect(QRect(rect.x(),rect.y(),rect.width(),60),header);
}
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
