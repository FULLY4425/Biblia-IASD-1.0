"""Verifica la integridad de la Biblia distribuida; no sustituye las pruebas C++ ni OBS."""
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
bible = json.loads((root / "data/bibles/rv1909.json").read_text(encoding="utf-8"))
assert bible["id"] == "rv1909"
books = bible["books"]
assert len(books) == 66
chapters = sum(len(book) for book in books.values())
verses = sum(len(chapter) for book in books.values() for chapter in book.values())
assert chapters == 1189, chapters
assert verses == 31084, verses
for book_name, book in books.items():
    assert sorted(map(int, book)) == list(range(1, len(book) + 1)), book_name
    for chapter_number, chapter in book.items():
        assert sorted(map(int, chapter)) == list(range(1, len(chapter) + 1)), (book_name, chapter_number)
        for verse_number, text in chapter.items():
            assert isinstance(text, str) and text.strip(), (book_name, chapter_number, verse_number)
            assert "<" not in text and "&nbsp;" not in text, (book_name, chapter_number, verse_number)
assert "de tal manera amó Dios" in books["Juan"]["3"]["16"]
assert "mi pastor" in books["Salmos"]["23"]["1"]
assert "crió Dios" in books["Génesis"]["1"]["1"]
assert "Jesucristo" in books["Apocalipsis"]["22"]["21"]
print(f"PASS: {len(books)} libros; {chapters} capítulos; {verses} versículos. Numeración continua, texto y extremos verificados.")
