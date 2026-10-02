#include "bible.hpp"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>

static QString normalize(QString text)
{
    text = text.normalized(QString::NormalizationForm_D).toLower();
    text.remove(QRegularExpression(QStringLiteral("\\p{M}")));
    return text.simplified();
}

bool Bible::load(const QByteArray &bytes, QString &error)
{
    QJsonParseError parse;
    const auto document = QJsonDocument::fromJson(bytes, &parse);
    if (parse.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("Archivo JSON inválido: ") + parse.errorString();
        return false;
    }
    const auto root = document.object();
    const auto candidate = root.value("books").toObject();
    if (root.value("id").toString().isEmpty() || root.value("name").toString().isEmpty() ||
        root.value("license").toString().isEmpty() || candidate.isEmpty()) {
        error = QStringLiteral("Se requieren id, name, license y books.");
        return false;
    }
    QStringList normalized;
    for (auto book = candidate.begin(); book != candidate.end(); ++book) {
        const auto key = normalize(book.key());
        if (key.isEmpty() || normalized.contains(key) || !book.value().isObject() || book.value().toObject().isEmpty()) {
            error = QStringLiteral("Libro vacío, duplicado o inválido."); return false;
        }
        normalized.append(key);
        const auto chapters = book.value().toObject();
        for (auto chapter = chapters.begin(); chapter != chapters.end(); ++chapter) {
            bool ok = false;
            const auto number = chapter.key().toInt(&ok);
            if (!ok || number < 1 || QString::number(number) != chapter.key() || !chapter.value().isObject() || chapter.value().toObject().isEmpty()) {
                error = QStringLiteral("Capítulo inválido."); return false;
            }
            const auto verses = chapter.value().toObject();
            for (auto verse = verses.begin(); verse != verses.end(); ++verse) {
                const auto n = verse.key().toInt(&ok);
                if (!ok || n < 1 || QString::number(n) != verse.key() || !verse.value().isString() || verse.value().toString().trimmed().isEmpty()) {
                    error = QStringLiteral("Versículo inválido."); return false;
                }
            }
        }
    }
    id = root.value("id").toString(); name = root.value("name").toString();
    license = root.value("license").toString(); books = candidate;
    error.clear(); return true;
}

bool Bible::lookup(const QString &reference, Passage &result, QString &error) const
{
    static const QRegularExpression pattern(QStringLiteral("^(.+?)\\s+(\\d+)(?::(\\d+)(?:\\s*[-–]\\s*(\\d+))?)?$"));
    const auto match = pattern.match(reference.trimmed());
    if (!match.hasMatch()) {
        error = QStringLiteral("Usa Juan 3:16, Juan 3:16-18 o Juan 3."); return false;
    }
    QString canonical;
    for (auto book = books.begin(); book != books.end(); ++book)
        if (normalize(book.key()) == normalize(match.captured(1))) { canonical = book.key(); break; }
    const int chapterNumber = match.captured(2).toInt();
    const auto chapter = books.value(canonical).toObject().value(QString::number(chapterNumber)).toObject();
    if (canonical.isEmpty() || chapter.isEmpty()) {
        error = QStringLiteral("Ese libro o capítulo no está en esta versión."); return false;
    }
    int first = match.captured(3).isEmpty() ? 1 : match.captured(3).toInt();
    int last = first;
    if (match.captured(3).isEmpty()) {
        for (auto verse = chapter.begin(); verse != chapter.end(); ++verse) last = qMax(last, verse.key().toInt());
    } else if (!match.captured(4).isEmpty()) last = match.captured(4).toInt();
    if (first < 1 || last < first || last - first > 200) {
        error = QStringLiteral("El rango de versículos es inválido o demasiado largo."); return false;
    }
    QStringList lines;
    for (int n = first; n <= last; ++n) {
        const auto verse = chapter.value(QString::number(n)).toString();
        if (verse.isEmpty()) { error = QStringLiteral("No se encontró el versículo %1.").arg(n); return false; }
        lines.append(first == last ? verse : QString::number(n) + QStringLiteral(". ") + verse);
    }
    result.reference = canonical + " " + QString::number(chapterNumber);
    if (!match.captured(3).isEmpty()) result.reference += ":" + QString::number(first) + (last > first ? "-" + QString::number(last) : QString());
    result.text = lines.join("\n"); error.clear(); return true;
}
