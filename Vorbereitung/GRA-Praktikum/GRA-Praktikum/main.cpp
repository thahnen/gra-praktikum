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
            if (debug) {
                // ALLE gefundenen CCs und ihre Eigenschaften ausgeben!
                cout << "\nComponent " << i << std::endl;
                cout << "(X|Y)          = (" << stats.at<int>(i, CC_STAT_LEFT) << "|" << stats.at<int>(i, CC_STAT_TOP) << ")" << endl;
                cout << "Breite x Hoehe = " << stats.at<int>(i, CC_STAT_WIDTH) << "x" << stats.at<int>(i, CC_STAT_HEIGHT) << " = " << stats.at<int>(i, CC_STAT_AREA) << endl;
            }
            
            // Hier muss irgendwie aussortiert werden, welche Components passen und welche nicht :>
            // 1. Flaeche darf nicht unter oder ueber Schwellwert liegen (hier: 1000 < Flaeche < 2000)
            // 2. Breite im gleichen Rahmen wie die anderen (+/-5) UND Hoehe im gleichen Rahmen wie die anderen (+/-10)
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
            cerr << "Es wurden weniger oder mehr als 8 Tasten erkannt!" << endl;
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
            int color = (256/(tasten_labels.size() + 2))*(i+1);
            
            rectangle(tastatur, Point(x, y), Point(x + cc[i][2], y + cc[i][3]), Scalar(color, color, color), FILLED);
            
            // Der Rest hier hinter passiert eigentlich auch nur wegen Praktikum 2 Aufgabe: Nacheinander setzen (und nur wenns hochkannt ist!)
            Mat tastatur_letters = tastatur.clone();
            string letters[] = {
                "A", "B", "C", "D", "E", "F", "G", "H"
            };
            putText(tastatur_letters, letters[i], Point(20, (8-i)*20), FONT_HERSHEY_COMPLEX_SMALL, 1.0, Scalar(128, 128, 128));
            
            imshow("Tasten einzeichnen...", tastatur_letters);
            waitKey(150);
        }
    }
    
    void auswertung_tiefenbild() {
        // Histogramm des Tiefenbilds erstellen& anzeigen (Tiefenbild muss Schwarz-Weiss sein!)
        tiefenbild = frame.clone();
        Mat tiefen_hist = histogramm(tiefenbild);
        imshow("Histogramm (Tiefenbild)", tiefen_hist);
        
        // Gauss-Filter auf Histogramm anwenden (Groesse egal? hier mal 3x3 genommen)
        GaussianBlur(hist_values, hist_values, Size(3, 3), 0);
        
        // Schwellwert suchen
        double min, max;
        minMaxLoc(hist_values, &min, &max);
        
        int value = (int)max;
        for (int i=(int)max; i>-1; i--) {
            if (i > value) {
                break;
            }
        }
        
        // Binaerbild mit Schwellwert erzeugen!
        Mat binaer_tiefen = tiefenbild.clone();
        threshold(binaer_tiefen, binaer_tiefen, value, 255, THRESH_BINARY | THRESH_OTSU);
        
        // Binaerbild mit dem vorherigen vergleichen
        // CCs herausfinden, alle vergleichen, ob sie vorher schon dabei waren
        Mat label_image, stats, centroid;
        int labels = connectedComponentsWithStats(grau_otsu, label_image, stats, centroid, 8, CV_32S);
        vector<vector<int>> cc_tiefen_neue;
        
        for (int i=0; i<labels; i++) {
            cc_tiefen_neue.push_back({
                stats.at<int>(i, CC_STAT_LEFT),
                stats.at<int>(i, CC_STAT_TOP),
                stats.at<int>(i, CC_STAT_WIDTH),
                stats.at<int>(i, CC_STAT_HEIGHT)
            });
        }
        
        // cc_tiefen ist beim ersten Mal noch nicht gesetzt, das muss vorher abgefangen werden!
        cc_tiefen_blau = cc_tiefen;                     // alte kopieren, damit dann dort alle gleichen rausgeloescht werden koennen (uebrig bleiben die weggefallenen!)
        cc_tiefen_rot = cc_tiefen_neue;                 // neue kopieren, damit dann dort alle gleichen rausgeloescht werden koennen (uebrig bleiben die hinzugekommenen!)
        for (vector<int> x : cc_tiefen_rot) {
            // Ueberpruefen, ob die schon im alten waren
            for (vector<int> y : cc_tiefen_blau) {
                // Vergleich einfach, ob neues CC gleich mit irgendeinem alten ist
                if ((x[0] == y[0]) and (x[1] == y[1])) {
                    // Gefunden, also muessen beide Objekte geoescht werden und aeussere Schleife weitergemacht werden (aka naechstes Element)
                    cc_tiefen_blau.erase(find(cc_tiefen_blau.begin(), cc_tiefen_blau.end(), y));
                    cc_tiefen_rot.erase(find(cc_tiefen_rot.begin(), cc_tiefen_rot.end(), x));
                    break;
                }
            }
        } // -> hier sollten in "cc_tiefen_blau" nur alte und in "cc_tiefen_rot" nur neue CCs
        
        // Auf veränderten ein Opening durchfuehren (wie auch immer das nur auf den Bereichen gehen soll)
        
        // Bereiche markieren ([BLAU -> neu,] ROT -> alt)
        Mat tiefenbild_markiert = tiefenbild.clone();
        
        for (vector<int> rot : cc_tiefen_rot) {
            int x = rot[0];
            int y = rot[1];
            
            rectangle(tiefenbild_markiert, Point(x, y), Point(x + rot[2], y + rot[3]), Scalar(255, 255, 255));
        }
        imshow("Rote Bereiche in Tiefenbild", tiefenbild_markiert); // nur zu DEBUG-Zwecken?
        
        // Haeufigkeit berechnen -> alle Pixel in CCs nach alllen Tasten suchen
        vector<int> haeufigkeiten;
        for (vector<int> rot : cc_tiefen_rot) {
            for (vector<int> taste : cc) {
                // Gucken, ob jeder Pixel in Rot (Breite x Hoehe) im Bereich der Tasten liegt!
                int anzahl = 0;
                for (int i=rot[0]; i<rot[0]+rot[2]; i++) {                          // CC.X <= i < CC.X + CC.Breite &
                    for (int j=rot[1]; j<rot[1]+rot[3]; j++) {                      // CC.Y <= j < CC.Y + CC.Hoehe ->
                        if (((i >= taste[0]) and (i < taste[0]+taste[2]))           //      Taste.X <= i < Taste.X + Taste.Breite &
                            and ((j >= taste[1]) and ( j < taste[1]+taste[3]))) {   //      Taste.Y <= j < Taste.Y + Taste.Hoehe ->
                            anzahl++;                                               //          Drin: Anzahl + 1
                        }
                    }
                }
                haeufigkeiten.push_back(anzahl);    // das hier funktioniert nur, wenn es nur eine einzige Rote Flaeche gibt!
                goto annahme_nur_eine_rote_flaeche_alle_anderen_ignoriert;
            }
        }
        
    annahme_nur_eine_rote_flaeche_alle_anderen_ignoriert:
        index_max_value = distance(haeufigkeiten.begin(), max_element(haeufigkeiten.begin(), haeufigkeiten.end())); // -> Die Taste cc[i] ist die gedrueckte!
        
        
        // Tasten in Bild mit Roten Segmenten zeichnen (Taste hervorheben)
        
        // Tasten in Tiefenbild einzeichnen (Taste hervorheben), Buchstaben ausgeben
    }
    
    /********************************************************************************************************************************************************\
     * HILFSFUNKTIONEN
    \********************************************************************************************************************************************************/
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
        for (int i=0; i<hist_w; i++) {
            line(hist_bild, Point(i, hist_h-cvRound(hist.at<float>(i))), Point(i, hist_h), Scalar(255, 255, 255));
        }
        return hist_bild;
    }
    
    void opening(Mat src, Mat element) {
        // open(src, element) = dilate(erode(src, element))
        erode(src, src, element);
        dilate(src, src, element);
    }
    
    void closing(Mat src, Mat element) {
        // close(src, element) = erode(dilate(src, element))
        dilate(src, src, element);
        erode(src, src, element);
    }
    
    string type2str(int type) {
        string r;
        switch (type & CV_MAT_DEPTH_MASK) {
            case CV_8U:  r = "8U"; break;
            case CV_8S:  r = "8S"; break;
            case CV_16U: r = "16U"; break;
            case CV_16S: r = "16S"; break;
            case CV_32S: r = "32S"; break;
            case CV_32F: r = "32F"; break;
            case CV_64F: r = "64F"; break;
            default:     r = "User"; break;
        }
        r += "C";
        r += ((1+(type >> CV_CN_SHIFT))+'0');
        return r;
    }
    
private:
    /********************************************************************************************************************************************************\
     * ALLE KLASSENVARIABLEN
    \********************************************************************************************************************************************************/
    bool type;                                      // Der jeweilige Bild-Typ (nur HIER zur Unterscheidung von Tiefen- bzw. Grauwertbild)
    Mat frame;                                      // Das jeweilige Frame (nur HIER entweder ein Tiefen- bzw. Grauwertbild)
    Mat grayscale;                                  // Das Grauwertbild (musste im Praktikum nicht konvertiert werden!)
    Mat grau_otsu;                                  // Das mit OTSU segmentierte Grauwert-Bild
    bool hochkant;                                  // Gibt an, ob das Bild hochkant ist oder nicht!
    Mat tastatur;                                   // Das Bild der eingefärbten Tastatur.
    vector<vector<int>> cc;                         // Vektoren der einzelnen CCs mit X/Y-Koordinaten, Hoehe/Breite
    Mat tiefenbild;                                 // Das Tiefenbild, abgespeichert um es mehrfach zu verwenden
    Mat hist_values;                                // Fuer alle 0-255 Werte des Histogramms, die Anzahl, wie oft jeder Wert vorkommt (gebraucht fuer Min/Max)
    vector<vector<int>> cc_tiefen;                  // Vektoren der einzelnen Tiefen-CCs mit X/Y-Koordinaten, Hoehe/Breite fuer Vergleich naechster Frame
    vector<vector<int>> cc_tiefen_blau;             // Alle alten CCs, die nicht auch bei den neuen CCs dabei sind!
    vector<vector<int>> cc_tiefen_rot;              // Alle neuen CCs, die nicht auch bei den alten CCs dabei sind!
    int index_max_value;                            // Gibt Index der Taste an, die gedrueckt wurde (muss aus allen roten Flaechen ausgewaehlt werden?)
};



/********************************************************************************************************************************************************\
 * MAIN-FUNKTION
\********************************************************************************************************************************************************/
int main(int argc, const char * argv[]) {
    string files[] = {
        "/Users/thahnen/GitHub/gra-praktikum/Vorbereitung/GRA-Praktikum/GRA-Praktikum/kb_robust_gray.avi",      // hier wird nur rumgewackelt und gedreht -> kann weg
        "/Users/thahnen/GitHub/gra-praktikum/Vorbereitung/GRA-Praktikum/GRA-Praktikum/kb_portrait_gray.avi",    // das Video is hochkant
        "/Users/thahnen/GitHub/gra-praktikum/Vorbereitung/GRA-Praktikum/GRA-Praktikum/kb_portrait_depth.avi"    // das Video zeigt nicht wirklich irgendwas -> kann weg
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
