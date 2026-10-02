#pragma once
#include <QPainter>

inline int fitPassage(QPainter &painter, QFont font, const QString &text,
                      const QSize &area, int maximum)
{
    for (int pixel = maximum; pixel >= 18; --pixel) {
        font.setPixelSize(pixel);
        painter.setFont(font);
        const auto bounds = painter.boundingRect(QRect(0, 0, area.width(), 1000000),
            Qt::AlignTop | Qt::AlignHCenter | Qt::TextWordWrap | Qt::TextDontClip, text);
        if (bounds.height() <= area.height() && bounds.width() <= area.width())
            return pixel;
    }
    return 0;
}
