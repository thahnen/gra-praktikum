# Praktikum: Graphische Daten- & Bilderverarbeitung

**Vorlesung: Graphische Daten- & Bildverarbeitung (3. Semester)**

Das Praktikumsprojekt bestehend aus einer großen Aufgabe für 3 Praktikumstermine unterteilt jeweils in Unteraufgaben die an dem entsprechenden Termin erledigt werden müssen.

---

## ~~Team~~-Member:
1. Tobias Hahnen (1218710)

---

## Technik
1. C++
* OpenCV-Bibliothek (Version 3&4 ?)
* Royale-Bibliothek

---

## Informationen
### Prakikum 1:
1. Aufgabe:
* Beim Grauwert-Bild Kontrastspreizung und anzeigen
* Beim Tiefen-Bild Kontrastspreizung ohne 0, Einfaerbung und anzeigen
2. Aufgabe:
* Moeglichkeit Bildaufnahme durch "Enter"-Druecken abbrechen
3. Aufgabe:
* Auf Parameterübergabe abfragen
* 1: spaeter Auswertung (u.a. aus Praktikum 2), hier noch nicht
* 2: Videos aufzeichnen mit übergebenem Dateinamen für Grauwert- und Tiefen-Bild
* 3: Aufgenommenes Video abspielen, Dateinamen übergeben

### Praktikum 2:
1. Aufgabe:
* Glättung der Grauwerte
* Eingesetzte "Mittelwert über die ersten 20 Frames", Medianfilter, Mittelwertfilter vergleichen
* Linienprofile erstellen und anzeigen sowie geeignetesten auswaehlen (Medianfilter)
2. Aufgabe:
* Schwellwertsegmentierung (OTSU oder Adaptiv)
* Labeln segmentierter Regionen (CC zusammenfaassen + Informationen bekommen)
* Gelabelte CCs sortieren und Richtige auswaehlen (anhand Hoehe + Breite)
* Ausgewählte CCs sortieren (anhand X/Y-Koordinate) und Farben zuweisen
* In Schleife nacheinander einfärben

### Praktikum 3:
1. Aufgabe:
* Zuordnung Buchstaben zu Farben
2. Aufgabe:
* Histogramm des Tiefenbilds ermitteln
* Histogramm mit Gauss-Filter glaetten & erstes lokales Minimum links von Maximalwert suchen
3. Aufgabe:
* Diesen Schwellwert für Binärbild nutzen, ggf. erniedrigen
* Binärbild mit vorherigem Vergleichen, Bereiche vergleichen, die nicht in beiden identisch sind
* Auf ungleichen Bereichen Opening durchführen!
4. Aufgabe:
* Neue Bereiche (Hand heben) BLAU markieren, weggefallene ROT (Hand absenken) => Tastatur nur Key-Down!
* Häufigkeit der überdeckten Tasten ermitteln, Element mit Häufigsten hervorheben!
* Buchstabe aus Buchstabenvektor ausgeben!

---

## GitHub-Aufbau:
1. ZIP-Datei "royale.zip":
* SDK Bibliothek für die verwendete ["PMD Technologies CamBoard pico flexx" - Kamera](https://www.automation24.de/entwicklungs-kit-pmd-vision-r-camboard-pico-flexx-700-000-094)
2. Ordner "Visual Studio":
* **GRA-Praktikum** beinhaltet das Visual Studio Projekt -> muss immer auf dem Desktop benutzt werden!
* **GRA-Praktikum.props** beinhaltet Einstellungen für Linker etc.
