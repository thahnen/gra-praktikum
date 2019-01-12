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
    // Konstruktor & Destruktor
    MyListener(bool type) {
        this->type = type;
    }
    ~MyListener() {}
    
    // Funktion, die jeden Frame ausgeführt wird
    void onNewData(Mat frame) {
        // frame nach CV_8UC1 konvertieren?
        //auswertung_geglaettete_grauwerte(frame);
    }
    
    void auswertung_geglaettete_grauwerte(Mat gray_frame) {
        // SIEHE: http://answers.opencv.org/question/120698/drawning-labeling-components-in-a-image-opencv-c/
        // Schwellwertsegmentierung (OTSU)
        // ggf. vor OTSU noch ein Closing durchfuehren? -> Frau P.-F. meinte nicht noetig
        Mat grau_otsu = gray_frame.clone();
        threshold(gray_frame, grau_otsu, 0, 255, THRESH_OTSU); // THRESH_OTSU nicht mehr CV_THRESH_OTSU!
        imshow("OTSU", grau_otsu);
        
        // Segmentierte Regionen labeln:
        // -> Pixel zu Connected Components zusammenfassen
        Mat label_image, stats, centroid;
        int labels = connectedComponentsWithStats(grau_otsu, label_image, stats, centroid, 8, CV_32S);
        vector<int> tasten_labels;
        
        for (int i = 1; i < labels; i++) {
            cout << "Component " << i << std::endl;
            // X-Koordinate?
            cout << "CC_STAT_LEFT   = " << stats.at<int>(i, CC_STAT_LEFT) << endl;
            // Y-Koordinate?
            cout << "CC_STAT_TOP    = " << stats.at<int>(i, CC_STAT_TOP) << endl;
            // Breite
            cout << "CC_STAT_WIDTH  = " << stats.at<int>(i, CC_STAT_WIDTH) << endl;
            // Höhe
            cout << "CC_STAT_HEIGHT = " << stats.at<int>(i, CC_STAT_HEIGHT) << endl;
            // Fläche
            cout << "CC_STAT_AREA   = " << stats.at<int>(i, CC_STAT_AREA) << endl;
            // auswählen, ob es passt (bei mir 500 < AREA < 1500
            // Frau P.-F. sagt das geht so und muss nicht umstaendlicher gemacht werden
            if (stats.at<int>(i, CC_STAT_AREA) > 500 && stats.at<int>(i, CC_STAT_AREA) < 1500) {
                tasten_labels.push_back(i);
            }
        }
        
        if (tasten_labels.size() != 8) {
            cout << "Nicht 8 Tasten erkannt!" << endl;
        } else {
            cout << "8 Tasten erkannt!" << endl;
        }
        
        // Gelabelte Connected Components sortieren
        
        // -> je nach Ausrichtung nach X/Y-Koordinate sortieren
        
        // Tasten je nach Wert in Schleife Farbe zuweisen und einfaerben
    }
    
    // Hilfsfunktionen
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
private:
    bool type;
    Mat frame;
};


int main(int argc, const char * argv[]) {
    string files[] = {
        "/Users/thahnen/GitHub/gra-praktikum/Vorbereitung/GRA-Praktikum/GRA-Praktikum/kb_robust_gray.avi",
        "/Users/thahnen/GitHub/gra-praktikum/Vorbereitung/GRA-Praktikum/GRA-Praktikum/kb_portrait_gray.avi",
        "/Users/thahnen/GitHub/gra-praktikum/Vorbereitung/GRA-Praktikum/GRA-Praktikum/kb_robust_depth.avi"
    };
    
    // je nach Typ entweder ein Grauwert-Bild oder ein Tiefen-Bild
    MyListener listener(false);
    
    VideoCapture cap(files[1]);
    if (!cap.isOpened()) {
        cerr << "Video-Capture konnte nicht gestartet werden!" << endl;
        return 1;
    }
    
    Mat frame;
    for (;;) {
        // Was jeden Frame ausgeführt wird!
        cap >> frame;
        
        if (frame.empty()) {
            cout << "Letztes Frame des Video-Stream" << endl;
            break;
        }
        
        //string ty = type2str(frame.type());
        //printf("Matrix: %s %dx%d \n", ty.c_str(), frame.cols, frame.rows );
        
        imshow("Video:", frame);
        if (debug) {
            imshow("Histogramm (Red)", listener.histogramm(frame));
        }
        
        listener.onNewData(frame);
        
        waitKey(500);
    }
    
    return 0;
}
