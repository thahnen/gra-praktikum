# Praktikum: Graphische Daten- & Bilderverarbeitung

**Vorlesung: Graphische Daten- & Bildverarbeitung (3. Semester)**

Das Praktikumsprojekt bestehend aus einer großen Aufgabe für 3 Praktikumstermine unterteilt jeweils in Unteraufgaben die an dem entsprechenden Termin erledigt werden müssen.

## ~~Team~~-Member:
1. Tobias Hahnen (1218710)

## Technik
1. C++
* OpenCV-Bibliothek
* Royale-Bibliothek

## Informationen
### Prakikum 1:
1. Aufgabe:
* Beim Grauwert-Bild Kontrastspreizung und anzeigen
* Beim Tiefen-Bild Kontrastspreizung ohne 0, Einfärbung und anzeigen
2. Aufgabe:
* Möglichkeit Bildaufnahme durch "Enter"-Drücken abbrechen
3. Aufgabe:
* Auf Parameterübergabe abfragen
* 1: später Auswertung, hier noch nicht
* 2: Videos aufzeichnen mit übergebenem Dateinamen für Grauwert- und Tiefen-Bild
* 3: Aufgenommenes Video abspielen, Dateinamen übergeben

### Praktikum 2:
1. Aufgabe:
* Glättung der Grauwerte anhand von Medianfilter, Mittelwertfilter und Mittelwert über die ersten 20 Frames
2. Aufgabe:
* Schwellwertsegmentierung (OTSU + Adaptiv)
* Labeln segmentierter Regionen (CC zusammenfaassen + Informationen bekommen)
* Gelabelte CCs sortieren und Richtige auswählen (anhand Höhe + Breite)
* Ausgewählte CCs sortieren (anhand X/Y-Koordinate) und Farben zuweisen
* In Schleife nacheinander einfärben

### Praktikum 3:
... kommt noch ...

### GitHub-Aufbau:
1. ZIP-Datei "royale.zip":
* SDK Bibliothek für die verwendete ["PMD Technologies CamBoard pico flexx" - Kamera](https://www.automation24.de/entwicklungs-kit-pmd-vision-r-camboard-pico-flexx-700-000-094)
2. Ordner "Visual Studio":
* **GRA-Praktikum.props** beinhaltet Einstellungen für Linker etc.
* **GRA-Projekt** beinhaltet das Visual Studio Projekt
