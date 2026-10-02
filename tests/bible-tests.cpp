#include "bible.hpp"
#include <QCoreApplication>
#include <iostream>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const QByteArray data = R"({"id":"test","name":"Test","license":"CC0","books":{"Génesis":{"1":{"1":"Uno","2":"Dos","3":"Tres"},"2":{"1":"Nuevo","2":"Último"}},"1 Juan":{"2":{"1":"Cuatro"}}}})";
    Bible bible; QString error; Passage passage; int failures = 0;
    auto check = [&](bool ok, const char *label) { if (!ok) { std::cerr << label << '\n'; ++failures; } };
    check(bible.load(data, error), "load valid data");
    check(bible.lookup("gen 1:2-",passage,error) && passage.reference==QStringLiteral("Génesis 1:2-3") && passage.text=="2. Dos\n3. Tres","unique prefix and open range");
    check(bible.lookup("1 ju 2:1",passage,error) && passage.text=="Cuatro","numbered prefix");
    check(bible.bookNames().size()==2,"offline completion books");
    Bible ambiguous;check(ambiguous.load(R"({"id":"a","name":"A","license":"CC0","books":{"Juan":{"1":{"1":"Uno"}},"Judas":{"1":{"1":"Dos"}}}})",error),"ambiguous fixture loads");
    check(!ambiguous.lookup("ju 1:1",passage,error) && error.contains("ambigua"),"ambiguous prefixes rejected");
    check(ambiguous.lookup("Juan 1:1",passage,error) && passage.text=="Uno","exact book resolves before prefixes");
    check(bible.lookup(QStringLiteral("genesis 1:1"), passage, error) && passage.text == "Uno", "accent insensitive lookup");
    check(bible.lookup(QStringLiteral("Génesis 1:1-3"), passage, error) && passage.text == "1. Uno\n2. Dos\n3. Tres", "inclusive range");
    check(bible.lookup("1 Juan 2:1", passage, error) && passage.text == "Cuatro", "numbered book");
    check(bible.lookup(QStringLiteral("Génesis 1"), passage, error) && passage.reference == QStringLiteral("Génesis 1"), "whole chapter");
    check(!bible.lookup("Génesis 1:3-1", passage, error), "reject reversed range");
    check(!bible.lookup("Génesis 1:0", passage, error), "reject verse zero");
    check(!bible.lookup("Génesis 1:1-4", passage, error), "reject missing verse");
    check(!bible.lookup("Génesis 999:1", passage, error), "reject missing chapter");
    check(!bible.lookup("Libro 1:1", passage, error), "reject missing book");
    check(!bible.lookup("Génesis", passage, error), "reject incomplete reference");
    check(!bible.lookup("Génesis 1:1-10000", passage, error), "reject oversized range");
    QVector<Passage> chapter;
    check(bible.chapterVerses("genesis 1:2",chapter,error) && chapter.size()==3 && chapter[1].text=="Dos", "chapter list around selected verse");
    check(bible.adjacent("Génesis 1:3",1,false,passage,error) && passage.reference==QStringLiteral("Génesis 2:1"), "next verse crosses chapter");
    check(bible.adjacent("Génesis 2:1",-1,false,passage,error) && passage.reference==QStringLiteral("Génesis 1:3"), "previous verse crosses chapter");
    check(bible.adjacent("Génesis 1:1-2",1,false,passage,error) && passage.text=="Tres", "next after range");
    check(!bible.adjacent("Génesis 1:1",-1,false,passage,error), "stop at first chapter");
    check(bible.adjacent("Génesis 1:2",1,true,passage,error) && passage.reference==QStringLiteral("Génesis 2"), "next chapter");
    check(!bible.load("{}", error), "reject missing fields");
    check(!bible.load(R"({"id":"t","name":"t","license":"CC0","books":{"Juan":{"1":{"1":10}}}})", error), "reject non-text verses");
    check(bible.lookup("1 Juan 2:1", passage, error), "failed import retains previous data");
    return failures ? 1 : 0;
}
