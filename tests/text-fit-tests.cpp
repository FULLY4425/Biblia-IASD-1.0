#include <QGuiApplication>
#include <QImage>
#include <QFile>
#include <iostream>
#include "bible.hpp"
#include "text-fit.hpp"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QFile file(QString::fromUtf8(argv[1]));
    if (!file.open(QIODevice::ReadOnly)) return 1;
    Bible bible; QString error; Passage chapter, verse;
    if (!bible.load(file.readAll(), error) || !bible.lookup("Génesis 2", chapter, error)
        || !bible.lookup("Juan 3:16", verse, error)) return 2;
    QImage image(1920, 1080, QImage::Format_RGBA8888);
    QPainter painter(&image);
    QFont font("Arial"); font.setBold(true);
    const QSize area(1660, 730);
    const int shortSize = fitPassage(painter, font, verse.text, area, 64);
    const int chapterSize = fitPassage(painter, font, chapter.text, area, 64);
    if (shortSize != 64 || chapterSize < 18 || chapterSize >= 64) return 3;
    const auto bounds = painter.boundingRect(QRect(0, 0, 1660, 1000000),
        Qt::AlignTop | Qt::AlignHCenter | Qt::TextWordWrap | Qt::TextDontClip, chapter.text);
    if (bounds.height() > 730 || bounds.width() > 1660) return 4;
    if (fitPassage(painter, font, chapter.text.repeated(10), area, 64) != 0) return 5;
    const int lowerSize=fitPassage(painter,font,verse.text,QSize(1800,168),64);
    if(lowerSize<18 || lowerSize>64) return 6;
    if(fitPassage(painter,font,chapter.text,QSize(1800,168),64)!=0) return 7;
    if(fitPassage(painter,font,verse.text,area,12)!=12 || fitPassage(painter,font,verse.text,area,6)!=6)return 8;
    std::cout << "Verse size: " << shortSize << ", chapter size: " << chapterSize
              << ", measured chapter height: " << bounds.height() << "\n";
    return 0;
}
