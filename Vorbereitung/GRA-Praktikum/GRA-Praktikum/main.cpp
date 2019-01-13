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
    /****************************************************************************\
     * KONSTRUKTOR & DESTRUKTOR
    \****************************************************************************/
    MyListener(bool type) {
        this->type = type;
    }
    ~MyListener() {}
    
    
    /****************************************************************************\
     * FUNKTION, DIE JEDEN FRAME AUSGEFÜHRT WIRD
    \****************************************************************************/
    void onNewData(Mat frame) {
        this->frame = frame;
        
        // Testen, ob es Grauwert- oder Tiefenbild war!
        
        cvtColor(this->frame, grayscale, COLOR_RGB2GRAY);
        
        if (debug) {
            imshow("Video", this->frame);
            imshow("Histogramm", histogramm(this->frame));
        }
        
        // Alles was hier laut Praktikum 1 usw. noch passiert -.-
        
        auswertung_geglaettete_grauwerte();
    }
    
    
    void auswertung_geglaettete_grauwerte() {
        // SIEHE: http://answers.opencv.org/question/120698/drawning-labeling-components-in-a-image-opencv-c/
        // Schwellwertsegmentierung (OTSU)
        // ggf. vor OTSU noch ein Closing durchfuehren? -> Frau P.-F. meinte nicht noetig
        grau_otsu = frame.clone();
        threshold(grayscale, grau_otsu, 0, 255, THRESH_OTSU); // THRESH_OTSU nicht mehr CV_THRESH_OTSU!
        //imshow("OTSU", grau_otsu);
        
        // Segmentierte Regionen labeln -> Pixel zu Connected Components zusammenfassen
        Mat label_image, stats, centroid;
        int labels = connectedComponentsWithStats(grau_otsu, label_image, stats, centroid, 8, CV_32S);
        vector<int> tasten_labels;
        
        for (int i=1; i<labels; i++) {
            if (debug) {
                cout << "\nComponent " << i << std::endl;
                cout << "CC_STAT_LEFT   = " << stats.at<int>(i, CC_STAT_LEFT) << endl;      // X-Koordinate
                cout << "CC_STAT_TOP    = " << stats.at<int>(i, CC_STAT_TOP) << endl;       // Y-Koordinate
                cout << "CC_STAT_WIDTH  = " << stats.at<int>(i, CC_STAT_WIDTH) << endl;     // Breite
                cout << "CC_STAT_HEIGHT = " << stats.at<int>(i, CC_STAT_HEIGHT) << endl;    // Höhe
                cout << "CC_STAT_AREA   = " << stats.at<int>(i, CC_STAT_AREA) << endl;      // Fläche
            }
            
            // Hier muss irgendwie aussortiert werden, welche Components passen und welche nicht :>
            // Das hier funktioniert (nur) bei "kb_portrait_gray.avi"
            if ((stats.at<int>(i, CC_STAT_WIDTH) > 70 && stats.at<int>(i, CC_STAT_WIDTH) < 84)
                && (stats.at<int>(i, CC_STAT_HEIGHT) > 10 && stats.at<int>(i, CC_STAT_HEIGHT) < 24)) {
                tasten_labels.push_back(i);
            }
        }
        
        if (tasten_labels.size() != 8) {
            // Es wurden mehr oder weniger als 8 Tasten erkannt :(
        }
        
        // Gelabelte Connected Components sortieren
        // Das hier funktioniert (nur) bei "kb_portrait_gray.avi"
        sort(tasten_labels.begin(), tasten_labels.end(), [stats](int a, int b) -> bool {
            return (stats.at<int>(a, CC_STAT_TOP) > stats.at<int>(b, CC_STAT_TOP)) ? true : false;
        });
        
        if (debug) {
            cout << endl;
            for (int i=0; i<tasten_labels.size(); i++) {
                cout << "Sortiertes Top (" << i+1 << "):" << stats.at<int>(tasten_labels[i], CC_STAT_TOP) << endl;
            }
            cout << endl;
        }
        
        // Tasten je nach Wert in Schleife Farbe zuweisen und einfaerben
        Mat colored_frame = frame.clone();
        
        for (int i=0; i<tasten_labels.size(); i++) {
            int x = stats.at<int>(tasten_labels[i], CC_STAT_LEFT);
            int y = stats.at<int>(tasten_labels[i], CC_STAT_TOP);
            int x2 = x + stats.at<int>(tasten_labels[i], CC_STAT_WIDTH);
            int y2 = y + stats.at<int>(tasten_labels[i], CC_STAT_HEIGHT);
            int color = (256/(tasten_labels.size() + 2))*(i+1);
            
            rectangle(colored_frame, Point(x, y), Point(x2, y2), Scalar(color, color, color), FILLED);
            
            string letters[] = {
                "A", "B", "C", "D", "E", "F", "G", "H"
            };
            putText(colored_frame, letters[i], Point(20, (8-i)*20), FONT_HERSHEY_COMPLEX_SMALL, 1.0, Scalar(128, 128, 128));
            
            imshow("Eingezeichnete CCs", colored_frame);
            waitKey(100);
        }
    }
    
    
    /****************************************************************************\
    * HILFSFUNKTION
    \****************************************************************************/
    Mat histogramm(Mat &bild) {
        int hist_w = 256;
        int hist_h = 450;
        Mat hist_bild(450, 256, CV_8UC3, Scalar(0, 0, 0));
        Mat hist;
        float range[] = {0, 256};
        const float* hist_range = { range };
        calcHist(&bild, 1, 0, Mat(), hist, 1, &hist_w, &hist_range, true, false);
        normalize(hist, hist, 0, hist_bild.rows, NORM_MINMAX, -1, Mat());
        for (int i=0; i<hist_w; i++) {
            line(hist_bild, Point(i, hist_h-cvRound(hist.at<float>(i))), Point(i, hist_h), Scalar(255, 255, 255));
        }
        return hist_bild;
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
    /****************************************************************************\
     * ALLE KLASSENVARIABLEN
    \****************************************************************************/
    bool type;
    Mat frame, grayscale;
    Mat grau_otsu;
};



/****************************************************************************\
 * MAIN-Funktion
\****************************************************************************/
int main(int argc, const char * argv[]) {
    string files[] = {
        "/Users/thahnen/GitHub/gra-praktikum/Vorbereitung/GRA-Praktikum/GRA-Praktikum/kb_robust_gray.avi",
        "/Users/thahnen/GitHub/gra-praktikum/Vorbereitung/GRA-Praktikum/GRA-Praktikum/kb_portrait_gray.avi",
        "/Users/thahnen/GitHub/gra-praktikum/Vorbereitung/GRA-Praktikum/GRA-Praktikum/kb_portrait_depth.avi"
    };
    
    // Parameter true -> Grauwert!
    MyListener listener(false);
    
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
