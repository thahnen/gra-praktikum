//
//  main.cpp
//  GRA-OpenCV
//
//  Created by Tobias Hahnen on 12.01.2019.
//  Copyright © 2018 Tobias Hahnen. All rights reserved.
//


#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#define debug false

using namespace std;
using namespace cv;


class MyListener {
public:
    /********************************************************************************************************************************************************\
     * KONSTRUKTOR & DESTRUKTOR
    \********************************************************************************************************************************************************/
    MyListener(bool type) {
        this->type = type;
    }
    ~MyListener() {}
    
    
    /********************************************************************************************************************************************************\
     * FUNKTION, DIE JEDEN FRAME AUSGEFÜHRT WIRD
    \********************************************************************************************************************************************************/
    void onNewData(Mat frame) {
        this->frame = frame;
        imshow("Frame ohne alles", frame);
        
        // Testen, ob es Grauwert- oder Tiefenbild war!
        if (type) {
            // Es war ein Grauwert-Bild, muss aber wegen Video zurückkonvertiert werden!
            cvtColor(this->frame, grayscale, COLOR_RGB2GRAY);
            
            if (debug) {
                imshow("Video (Grauwert)", grayscale);
                imshow("Histogramm (Grauwert)", histogramm(grayscale));
            }
            
            // Alles was hier laut Praktikum 1 usw. noch passiert -.-
            
            auswertung_geglaettete_grauwerte();
            if (debug) {
                imshow("OTSU", grau_otsu);
            }
            imshow("Tastatur", tastatur);
            
            return;
        }
        
        cvtColor(this->frame, depth_gray, COLOR_RGB2GRAY);
        auswertung_tiefenbild();
    }
    
    
    void auswertung_geglaettete_grauwerte() {
        // SIEHE: http://answers.opencv.org/question/120698/drawning-labeling-components-in-a-image-opencv-c/
        // Schwellwertsegmentierung (OTSU)
        // ggf. vor OTSU noch ein Closing durchfuehren? -> Frau P.-F. meinte nicht noetig
        grau_otsu = grayscale.clone();
        //closing(grau_otsu, Mat());
        threshold(grayscale, grau_otsu, 0, 255, THRESH_BINARY | THRESH_OTSU); // THRESH_BINARY + THRESH_OTSU nicht mehr CV_THRESH_BINARY + CV_THRESH_OTSU!
        
        // Segmentierte Regionen labeln -> Pixel zu Connected Components zusammenfassen
        Mat label_image, stats, centroid;
        int labels = connectedComponentsWithStats(grau_otsu, label_image, stats, centroid, 8, CV_32S);
        vector<int> tasten_labels;
        
        for (int i=1; i<labels; i++) {
            if (/*debug*/true) {
                // ALLE gefundenen CCs und ihre Eigenschaften ausgeben!
                cout << "\nComponent " << i << std::endl;
                cout << "(X|Y)          = (" << stats.at<int>(i, CC_STAT_LEFT) << "|" << stats.at<int>(i, CC_STAT_TOP) << ")" << endl;
                cout << "Breite x Hoehe = " << stats.at<int>(i, CC_STAT_WIDTH) << "x" << stats.at<int>(i, CC_STAT_HEIGHT) << " = " << stats.at<int>(i, CC_STAT_AREA) << endl;
            }
            
            // Hier muss irgendwie aussortiert werden, welche Components passen und welche nicht :>
            // 1. Flaeche darf nicht unter oder ueber Schwellwert liegen (hier: 1000 < Flaeche < 2000)
            // 2. Breite im gleichen Rahmen wie die anderen (+/-10) UND Hoehe im gleichen Rahmen wie die anderen (+/-10)
            // WERTE IM PRAKTIKUM AENDERN; DIE HIER SIND FUER VIDEO!
            int untere = 1000;
            int obere = 2000;
            if ((stats.at<int>(i, CC_STAT_AREA) > untere) and (stats.at<int>(i, CC_STAT_AREA) < obere)) {
                for (int j=1; j<labels; j++) {
                    if (i==j) { continue; }
                    
                    int w_untere = stats.at<int>(j, CC_STAT_WIDTH) - 10;
                    int w_obere = stats.at<int>(j, CC_STAT_WIDTH) + 10;
                    int h_untere = stats.at<int>(j, CC_STAT_HEIGHT) - 10;
                    int h_obere = stats.at<int>(j, CC_STAT_HEIGHT) + 10;
                    if (((stats.at<int>(i, CC_STAT_WIDTH) > w_untere) and (stats.at<int>(i, CC_STAT_WIDTH) < w_obere))
                        and ((stats.at<int>(i, CC_STAT_HEIGHT) > h_untere) and (stats.at<int>(i, CC_STAT_HEIGHT) < h_obere))) {
                        tasten_labels.push_back(i);
                        break;
                    }
                }
            }
        }
        
        if (tasten_labels.size() != 8) {
            // bsp. wenn man das Tastaturen-Blatt dreht oder aus dem Kamera-Feld bewegt
            // Allerdings werden auch zwischendurch irgendwie mal wieder 8 Felder erkannt, auch wenn das nicht ganz stimmt!
            cerr << "Es wurden weniger oder mehr als 8 Tasten erkannt!" << endl;
            return;
        }
        
        // Hier muss irgendwie überprüft werden, ob die Tastatur hochkant ist oder nicht!
        hochkant = true;
        int y_untere = stats.at<int>(tasten_labels[0], CC_STAT_TOP) - 10;
        int y_obere = stats.at<int>(tasten_labels[0], CC_STAT_TOP) + 10;
        if ((stats.at<int>(tasten_labels[1], CC_STAT_TOP) > y_untere) and (stats.at<int>(tasten_labels[1], CC_STAT_TOP) < y_obere)) {
            // nicht hochkant, da alle Y-Koordinaten ungefähr gleich!
            cout << "Nicht hochkant!" << endl;
            hochkant = false;
            // Gelabelte Connected Components sortieren (Nach X-Koordinate)
            sort(tasten_labels.begin(), tasten_labels.end(), [stats](int a, int b) -> bool {
                return (stats.at<int>(a, CC_STAT_LEFT) > stats.at<int>(b, CC_STAT_LEFT)) ? true : false;
            });
        } else {
            cout << "Hochkant!" << endl;
            // Gelabelte Connected Components sortieren (Nach Y-Koordinate)
            sort(tasten_labels.begin(), tasten_labels.end(), [stats](int a, int b) -> bool {
                return (stats.at<int>(a, CC_STAT_TOP) > stats.at<int>(b, CC_STAT_TOP)) ? true : false;
            });
        }
        
        if (debug) {
            for (int i=0; i<tasten_labels.size(); i++) {
                cout << "Sortiertes Top (" << i+1 << "):" << stats.at<int>(tasten_labels[i], CC_STAT_TOP) << endl;
            }
        }
        
        // Tasten je nach Wert in Schleife Farbe zuweisen und einfaerben
        tastatur = frame.clone();
        
        for (int i=0; i<tasten_labels.size(); i++) {
            cc.push_back({
                stats.at<int>(tasten_labels[i], CC_STAT_LEFT),
                stats.at<int>(tasten_labels[i], CC_STAT_TOP),
                stats.at<int>(tasten_labels[i], CC_STAT_WIDTH),
                stats.at<int>(tasten_labels[i], CC_STAT_HEIGHT)
            });
            
            int x = cc[i][0];
            int y = cc[i][1];
            
            farben = {
                Scalar(26, 26, 26), Scalar(52, 52, 52), Scalar(78, 78, 78), Scalar(104, 104, 104),
                Scalar(130, 130, 130), Scalar(156, 156, 156), Scalar(182, 182, 182), Scalar(208, 208, 208)
            };
            rectangle(tastatur, Point(x, y), Point(x + cc[i][2], y + cc[i][3]), farben[i], FILLED);
            
            // Der Rest hier hinter passiert eigentlich auch nur wegen Praktikum 2 Aufgabe: Nacheinander setzen (und nur wenns hochkannt ist!)
            Mat tastatur_letters = tastatur.clone();
            letters = {
                "A", "B", "C", "D", "E", "F", "G", "H"
            };
            putText(tastatur_letters, letters[i], Point(20, (8-i)*20), FONT_HERSHEY_COMPLEX_SMALL, 1.0, Scalar(128, 128, 128));
            
            imshow("Tasten einzeichnen...", tastatur_letters);
            waitKey(150);
        }
    }
    
    void auswertung_tiefenbild() {
        // Histogramm des Tiefenbilds erstellen& anzeigen (Tiefenbild muss Schwarz-Weiss sein!)
        tiefenbild = depth_gray;
        Mat tiefen_hist = histogramm(tiefenbild);
        imshow("Histogramm (Tiefenbild)", tiefen_hist);
        
        // Gauss-Filter auf Histogramm anwenden (Groesse egal? Hier mal 3x3 genommen)
        GaussianBlur(hist_values, hist_values, Size(3, 3), 0);
        
        // Schwellwert suchen (hier muss durch hist_values iteriert werden)
        double min, max;
        minMaxLoc(hist_values, &min, &max);
        
        int highest_position = 256;
        int value = (int)max;
        for (int i=0; i<256; i++) {
            if (hist_values.at<float>(i) == max) {
                highest_position = i;
                break;
            }
        }
        
        for (int i=highest_position; i>=0; i--) {
            uchar wert_an_punkt_i = hist_values.at<float>(i);   // gibt ja nur eine Zeile an Werten!
            if (wert_an_punkt_i > value) {
                break;
            }
            value = wert_an_punkt_i;
        }
        
        // Binaerbild mit Schwellwert erzeugen! -> ggf. erniedrigen
        Mat binaer_tiefen = tiefenbild.clone();
        threshold(binaer_tiefen, binaer_tiefen, value, 255, THRESH_BINARY | THRESH_OTSU);
        imshow("Histogramm (Tiefen-Binaer)", histogramm(binaer_tiefen));
        
        // Bild mit altem Bild vergleichen!
        Mat rot(binaer_tiefen.rows, binaer_tiefen.cols, CV_8UC3, Scalar(0, 0, 0));
        Mat blau(binaer_tiefen.rows, binaer_tiefen.cols, CV_8UC3, Scalar(0, 0, 0));
        
        if (vorhanden) {
            //imshow("Alt bin Tiefen", altes_binaer_tiefen);
            //imshow("Neu bin Tiefen", binaer_tiefen);
            
            // Hier das alte binäre Tiefenbild mit dem neuen Vergleichen!
            // wenn Hand da, dann Farbwert 0 ansonsten 255!
            
            for (uchar i=0; i < binaer_tiefen.rows; i++) {
                for (uchar j = 0; j < binaer_tiefen.cols; j++) {
                    if (binaer_tiefen.at<uchar>(i, j) == 0 && altes_binaer_tiefen.at<uchar>(i, j) == 255) {
                        // Uebergang von schwarz zu weiss -> Finger geht runter
                        rot.at<Vec3b>(i, j) = Vec3b(0, 0, 255);
                    } else if (binaer_tiefen.at<uchar>(i, j) == 255 && altes_binaer_tiefen.at<uchar>(i, j) == 0) {
                        // Uebergang von weiss zu schwarz -> Finger geht hoch (oder erscheint neu!)
                        blau.at<Vec3b>(i, j) = Vec3b(255, 0, 0);
                    }
                }
            }
            
            // dann das neue als das alte setzen!
            altes_binaer_tiefen = binaer_tiefen.clone();
        } else {
            altes_binaer_tiefen = binaer_tiefen.clone();
            vorhanden = true;
        }
        
        // Tasten in Bild mit Roten Segmenten zeichnen (Taste hervorheben)
        for (int i=0; i < cc.size(); i++) {
            int x = cc[i][0];
            int y = cc[i][1];
            
            rectangle(rot, Point(x, y), Point(x + cc[i][2], y + cc[i][3]), Scalar(255, 255, 255));
        }
        imshow("Rot mit Tasten", rot);
        //imshow("Blau", blau);
        
        int anzahl_pro_taste[] = {
            0, 0, 0, 0, 0, 0, 0, 0
        };
        
        for (uchar i = 0; i < rot.rows; i++) {
            for (uchar j = 0; j < rot.cols; j++) {
                if (rot.at<Vec3b>(i, j) == Vec3b(0, 0, 255)) {
                    for (int k = 0; k < cc.size(); k++) {
                        int x = cc[k][0];
                        int y = cc[k][1];
                        int x2 = x + cc[k][2];
                        int y2 = y + cc[k][3];
                        
                        if (i>x && i<x2 && j>y && j<y2) {
                            anzahl_pro_taste[k]++;
                        }
                    }
                }
            }
        }
        
        // Hier den Index vom maximalen Wert in "anzahl_pro_taste" finden
        int max_index = 0;
        cout << "A: " << anzahl_pro_taste[0] << endl;
        cout << "B: " << anzahl_pro_taste[1] << endl;
        cout << "C: " << anzahl_pro_taste[2] << endl;
        cout << "D: " << anzahl_pro_taste[3] << endl;
        cout << "E: " << anzahl_pro_taste[4] << endl;
        cout << "F: " << anzahl_pro_taste[5] << endl;
        cout << "G: " << anzahl_pro_taste[6] << endl;
        cout << "H: " << anzahl_pro_taste[7] << endl;
        
        // Tasten in Tiefenbild einzeichnen (Taste hervorheben), Buchstaben ausgeben
    }
    
    /********************************************************************************************************************************************************\
     * HILFSFUNKTIONEN
    \********************************************************************************************************************************************************/
    // ggf noch ueberarbeiten, damit nicht float sondern uchar verwendet wird? Ueberall sonst ist ja uchar!
    Mat histogramm(Mat& bild) {
        int hist_w = 256;
        int hist_h = 450;
        Mat hist_bild(450, 256, CV_8UC3, Scalar(0, 0, 0));
        Mat hist;
        float range[] = {0, 256};
        const float* hist_range = { range };
        calcHist(&bild, 1, 0, Mat(), hist, 1, &hist_w, &hist_range, true, false);
        normalize(hist, hist, 0, hist_bild.rows, NORM_MINMAX, -1, Mat());
        hist_values = hist.clone();
        for (int i=0; i<hist_w; i++) { line(hist_bild, Point(i, hist_h-cvRound(hist.at<float>(i))), Point(i, hist_h), Scalar(255, 255, 255)); }
        return hist_bild;
    }
    
    void opening(Mat src, Mat element) {    // open(src, element) = dilate(erode(src, element))
        erode(src, src, element);
        dilate(src, src, element);
    }
    
    void closing(Mat src, Mat element) {    // close(src, element) = erode(dilate(src, element))
        dilate(src, src, element);
        erode(src, src, element);
    }
    
private:
    /********************************************************************************************************************************************************\
     * ALLE KLASSENVARIABLEN
    \********************************************************************************************************************************************************/
    
    // Praktikum 2 bzw. Vorbereitung (aka kommt so nicht vor!)
    bool type;                                      // Der jeweilige Bild-Typ (nur HIER zur Unterscheidung von Tiefen- bzw. Grauwertbild)
    Mat frame;                                      // Das jeweilige Frame (nur HIER entweder ein Tiefen- bzw. Grauwertbild)
    Mat grayscale;                                  // Das Grauwertbild (musste im Praktikum nicht konvertiert werden!)
    Mat grau_otsu;                                  // Das mit OTSU segmentierte Grauwert-Bild
    bool hochkant;                                  // Gibt an, ob das Bild hochkant ist oder nicht!
    vector<Scalar> farben;                          // Das Array mit allen Farben zur Zuordnung zu CCs
    vector<string> letters;                         // Das Array nut allen Buchstaben zur Zuordnung zu CCs
    Mat tastatur;                                   // Das Bild der eingefärbten Tastatur.
    vector<vector<int>> cc;                         // Vektoren der einzelnen CCs mit X/Y-Koordinaten, Hoehe/Breite
    
    // Praktikum 3
    Mat depth_gray;                                 // Das Tiefenbild in ein Grauwert-Bild konvertiert
    Mat tiefenbild;                                 // Das Tiefenbild, abgespeichert um es mehrfach zu verwenden
    Mat hist_values;                                // Fuer alle 0-255 Werte des Histogramms, die Anzahl, wie oft jeder Wert vorkommt (gebraucht fuer Min/Max)
    bool vorhanden = false;                         // Ueberprueft, ob es der erste Frame ist!
    Mat altes_binaer_tiefen;                        // Das binäre Tiefenbild aus dem alten Frame!
};



/********************************************************************************************************************************************************\
 * MAIN-FUNKTION
\********************************************************************************************************************************************************/
int main(int argc, const char * argv[]) {
    string files[] = {
        "/Users/thahnen/GitHub/gra-praktikum/Vorbereitung/GRA-Praktikum/GRA-Praktikum/kb_robust_gray.avi",      // das Video ist breit, zeigt aber auch wo noch ungenau!
        "/Users/thahnen/GitHub/gra-praktikum/Vorbereitung/GRA-Praktikum/GRA-Praktikum/kb_portrait_gray.avi",    // das Video ist hochkant
        "/Users/thahnen/GitHub/gra-praktikum/Vorbereitung/GRA-Praktikum/GRA-Praktikum/kb_portrait_depth.avi",   // das Video zeigt nichts sinnvolles! -> kann weg!
        "/Users/thahnen/GitHub/gra-praktikum/Vorbereitung/GRA-Praktikum/GRA-Praktikum/tr_depth_trim.avi",       // Tiefenbild-Video in Farbe von Ilja
        "/Users/thahnen/GitHub/gra-praktikum/Vorbereitung/GRA-Praktikum/GRA-Praktikum/tr_grey_trim.avi"         // Grauwertbild-Video von Ilja
    };
    
    // Parameter true -> Grauwert!
    MyListener listener(true);
    
    // VideoCapture oeffnen
    VideoCapture cap(files[1]);
    if (!cap.isOpened()) {
        cerr << "Video-Capture konnte nicht gestartet werden!" << endl;
        return 1;
    }
    
    // Eingelesenes Frame
    Mat frame;
    for (;;) {
        cap >> frame;
        if (frame.empty()) {
            cout << "Letztes Frame des Video-Streams oder leer!" << endl;
            break;
        }
        
        // Was der Listener jeden Frame macht
        listener.onNewData(frame);
        
        // So lange wird zwischen den Frames gewartet!
        waitKey(1000);
    }
    
    return 0;
}
