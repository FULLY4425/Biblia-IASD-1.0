#pragma once
#include <QFont>
#include <QStringList>
#include <QTextLayout>

// QTextLayout chooses Unicode-safe wrap positions, including RTL and CJK text.
inline QStringList passageSlides(const QString &text, const QFont &font, int width, int maximumLines)
{
    QStringList slides, lines;
    if (width < 1 || maximumLines < 1) return slides;
    for (const auto &paragraph : text.split('\n')) {
        if (paragraph.isEmpty()) { lines.append(QString()); continue; }
        QTextLayout layout(paragraph, font);
        QTextOption option; option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        layout.setTextOption(option); layout.beginLayout();
        while (true) {
            auto line = layout.createLine(); if (!line.isValid()) break;
            line.setLineWidth(width);
            lines.append(paragraph.mid(line.textStart(), line.textLength()));
        }
        layout.endLayout();
    }
    for (int i = 0; i < lines.size(); i += maximumLines)
        slides.append(lines.mid(i, maximumLines).join('\n'));
    return slides;
}
