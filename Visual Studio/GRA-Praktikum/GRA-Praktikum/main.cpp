/********************************************************************************************************************************************************\
* Vorlage fuer das Praktikum "Graphische Datenverarbeitung" WS 2018/19
* FB 03 der Hochschule Niedderrhein
* Regina Pohle-Froehlich
*
* Der Code basiert auf den c++-Beispielen der Bibliothek royale
\********************************************************************************************************************************************************/


#include <royale.hpp>
#include <iostream>
#include <mutex>
#include <opencv2/opencv.hpp>

#define debug false

using namespace std;
using namespace cv;


class MyListener : public royale::IDepthDataListener {
public:
	void onNewData(const royale::DepthData *data) {
		// this callback function will be called for every new depth frame
		lock_guard<mutex> lock(flagMutex);
		zImage.create(Size(data->width, data->height), CV_32FC1);
		grayImage.create(Size(data->width, data->height), CV_32FC1);
		zImage = 0;
		grayImage = 0;
		int k = 0;
		for (int y = 0; y < zImage.rows; y++) {
			for (int x = 0; x < zImage.cols; x++) {
				auto curPoint = data->points.at(k);
				if (curPoint.depthConfidence > 0) {
					zImage.at<float>(y, x) = curPoint.z;
					grayImage.at<float>(y, x) = curPoint.grayValue;
				}
				k++;
			}
		}

		Mat temp = zImage.clone();
		undistort(temp, zImage, cameraMatrix, distortionCoefficients);
		temp = grayImage.clone();
		undistort(temp, grayImage, cameraMatrix, distortionCoefficients);


		/********************************************************************************************************************************************************\
		* ANFANG
		* Praktikum 1/2/3 Inhaltliche Veraenderungen
		\********************************************************************************************************************************************************/

		edit_gray_picture();
		edit_depth_picture();

		imshow("Gray (Edit)", grayImage_edit);
		imshow("Depth (Edit)", depthImage_edit);

		// Nur Video aufnehmen, wenn es auch der Parameter sagt
		if (mode == 2) {
			write_video_files();
        } else {
            auswertung_geglaettete_grauwerte();

            /* DAS HIER WAR PRAKTIKUM 2 !!!
            glaettung_grauwerte();
             
            if (glaettung_frame > 21) {
                auswertung_geglaettete_grauwerte();
             
                if (debug) {
                    imshow("OTSU", grau_otsu);
                }
             
                imshow("Mit20", mit20_bild);
                imshow("Med", med_bild);
                imshow("Mit", mit_bild);
             
                imshow("Tastatur", tastatur);
             }
             */
            
            auswertung_tiefenbild();
        }

		/********************************************************************************************************************************************************\
		* Praktikum 1/2/3 Inhaltliche Veraenderungen
		* ENDE
		\********************************************************************************************************************************************************/
	}

	void setLensParameters(const royale::LensParameters &lensParameters) {
		cameraMatrix = (Mat1d(3, 3) << lensParameters.focalLength.first, 0, lensParameters.principalPoint.first,
			0, lensParameters.focalLength.second, lensParameters.principalPoint.second, 0, 0, 1);
		distortionCoefficients = (Mat1d(1, 5) << lensParameters.distortionRadial[0], lensParameters.distortionRadial[1],
			lensParameters.distortionTangential.first, lensParameters.distortionTangential.second, lensParameters.distortionRadial[2]);
	}

    
	/********************************************************************************************************************************************************\
	* ANFANG
	* Praktikum 1/2/3 Inhaltliche Veraenderungen
	\********************************************************************************************************************************************************/

	// Praktikum 1
	void edit_gray_picture() {
		// Kontrastspreizung
		Mat mask;
		compare(grayImage, Scalar(0), mask, CMP_GT);
		Mat gray_CV_8UC1(grayImage.rows, grayImage.cols, CV_8UC1);
		double min, max;
		minMaxLoc(grayImage, &min, &max, 0, 0, (InputArray)mask);

		// Skalierung berechnen & Bild skalieren
		double scale = 255 / (max - min);
		convertScaleAbs(grayImage, gray_CV_8UC1, scale);
		grayImage_edit = gray_CV_8UC1;
	}

    // Praktikum 1
	void edit_depth_picture() {
		// Kontrastspreizung
		Mat mask;
		compare(zImage, Scalar(0), mask, CMP_GT);
		Mat depth_CV_8UC1(zImage.rows, zImage.cols, CV_8UC1);
		double min, max;
		minMaxLoc(zImage, &min, &max, 0, 0, (InputArray)mask);

		// Skalierung berechnen & Bild skalieren (musste in Praktikum 3 noch angepasst werden, war nicht richtig skaliert)
		double scale = 255 / (max - min);
		convertScaleAbs(zImage, depth_CV_8UC1, scale, (-min*scale));
        
        // Hier zwischen noch depth_CV_8UC1 abspeichern, damit wird dann später weitergearbeitet
		depthImage_edit_gray = depth_CV_8UC1.clone();

		// Einfaerbung mit COLORMAP_RAINBOW
		Mat color_map;
		applyColorMap(depth_CV_8UC1, color_map, COLORMAP_RAINBOW);
		depthImage_edit = color_map;
	}

    // Praktikum 1
	void open_video_files(string filename, Size bild_groesse, double fps) {
		file_gray = filename + "_gray.avi";
		file_depth = filename + "_depth.avi";

		// VideoWriter fuer Graubild oeffnen
		vw_gray.open(file_gray, VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, bild_groesse, false);
		if (!vw_gray.isOpened()) { cout << "ERROR: Failed open VideoWriter for GrayScale" << endl; exit(1); }

		// VideoWriter fuer Tiefenbild oeffnen
		vw_depth.open(file_depth, VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, bild_groesse, true);
		if (!vw_depth.isOpened()) { cout << "ERROR: Failed open VideoWriter for Depth" << endl; exit(1); }
	}

    // Praktikum 1 -> hier muss beim Tiefen-Bild das nicht-farbige abgespeichert werden!
	void write_video_files() { vw_gray.write(grayImage_edit); vw_depth.write(depthImage_edit_gray); }

    // Praktikum 1
	void close_video_files() { vw_gray.release(); vw_depth.release(); }

    // Praktikum 1
	string get_file_gray() { return file_gray; }
	string get_file_depth() { return file_depth; }
	void setMode(int nmode) { mode = nmode; }

	// Praktikum 2
	void setFrame(int frame) { glaettung_frame = frame; }
	void set_mit_ueber_20_frames(vector<int> v) { mit_ueber_20_frames = v; };

    // Praktikum 2 -> wird nicht mehr benutzt
	void glaettung_grauwerte() {
		if (glaettung_frame < 21) {
			// Fuer die ersten 20 Frames Mittelwert berechnen
			for (int i = 0; i < grayImage_edit.rows; i++) {
				for (int j = 0; j < grayImage_edit.cols; j++) {
					mit_ueber_20_frames[i*grayImage_edit.rows + j] = mit_ueber_20_frames[i*grayImage_edit.rows + j] + grayImage_edit.at<uchar>(i, j);
				}
			}
			glaettung_frame++;
		} else {
			// Linienprofil-Bild (Breite * Hoehe => 256 * [Hoehe des Graubilds])
			Mat linienprofil(256, grayImage_edit.rows, CV_8UC3);

			// Irgendein zufaelliger Wert, der zwischen 0-Breite liegt
			uchar x_wert = grayImage_edit.rows / 2;

			// "Anwendung" Mittelwert über 20 Frames => klappt nicht ganz, irgendwas ist da kaputt!
			if (glaettung_frame == 21) {
				mit20_bild = Mat(grayImage_edit.rows, grayImage_edit.cols, CV_8UC1);
				for (int i = 0; i < mit_ueber_20_frames.size(); i++) { mit_ueber_20_frames[i] = (int)mit_ueber_20_frames[i] / 20; }
				
				for (int i = 0; i < mit20_bild.rows; i++) {
					for (int j = 0; j < mit20_bild.cols; j++) { mit20_bild.at<uchar>(i, j) = mit_ueber_20_frames[i*grayImage_edit.rows + j]; }
				}
				glaettung_frame++;
			}

			// Anwendung Medianfilter (Groesse 3x3)
			med_bild = grayImage_edit.clone();
			medianBlur(grayImage_edit, med_bild, 3);

			// Anwendung Mittelwertfilter (Groesse 3x3)
			mit_bild = grayImage_edit.clone();
			blur(grayImage_edit, mit_bild, Size(3, 3));

			// Linienprofil fuer Mittelwert (20 Frames) zeichnen
			Scalar m20(255, 0, 0);
			for (uchar i = 0; i < linienprofil.cols - 1; i++) {
				line(linienprofil, Point(i, mit20_bild.at<uchar>(x_wert, i)), Point(i + 1, mit20_bild.at<uchar>(x_wert, i + 1)), m20);
			}

			// Linienprofil fuer Medianfilter zeichnen
			Scalar med(0, 255, 0);
			for (uchar i = 0; i < linienprofil.cols-1; i++) {
				line(linienprofil, Point(i, med_bild.at<uchar>(x_wert, i)), Point(i+1, med_bild.at<uchar>(x_wert, i+1)), med);
			}

			// Linienprofil fuer Mittelwertfilter zeichen
			Scalar mit(0, 0, 255);
			for (uchar i = 0; i < linienprofil.cols - 1; i++) {
				line(linienprofil, Point(i, mit_bild.at<uchar>(x_wert, i)), Point(i + 1, mit_bild.at<uchar>(x_wert, i + 1)), mit);
			}
			imshow("Linienprofil", linienprofil);
		}
	}

    // Praktikum 2
	void auswertung_geglaettete_grauwerte() {
        // Anwendung Medianfilter (Groesse 3x3), weil glaettung_grauwerte() nicht mehr aufgerufen wird!!!
        med_bild = grayImage_edit.clone();
        medianBlur(grayImage_edit, med_bild, 3);
        
		// SIEHE: http://answers.opencv.org/question/120698/drawning-labeling-components-in-a-image-opencv-c/
		// Schwellwertsegmentierung (OTSU)
		// ggf. vor OTSU noch ein Closing durchfuehren? -> Frau P.-F. meinte nicht noetig
		grau_otsu = med_bild.clone();
        //closing(grau_otsu, Mat());
		threshold(med_bild, grau_otsu, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

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
            // 2. Breite im gleichen Rahmen wie die anderen (+/-10) UND Hoehe im gleichen Rahmen wie die anderen (+/-10)
            // WERTE IM PRAKTIKUM AENDERN; DIE HIER SIND FUERS VIDEO! -> mussten nicht abgeaendert werden!
            int untere = 1000;
            int obere = 2000;
            if ((stats.at<int>(i, CC_STAT_AREA) > untere) && (stats.at<int>(i, CC_STAT_AREA) < obere)) {
                for (int j=1; j<labels; j++) {
                    if (i==j) { continue; }
                    
                    int w_untere = stats.at<int>(j, CC_STAT_WIDTH) - 10;
                    int w_obere = stats.at<int>(j, CC_STAT_WIDTH) + 10;
                    int h_untere = stats.at<int>(j, CC_STAT_HEIGHT) - 10;
                    int h_obere = stats.at<int>(j, CC_STAT_HEIGHT) + 10;
                    if (((stats.at<int>(i, CC_STAT_WIDTH) > w_untere) && (stats.at<int>(i, CC_STAT_WIDTH) < w_obere))
                        && ((stats.at<int>(i, CC_STAT_HEIGHT) > h_untere) && (stats.at<int>(i, CC_STAT_HEIGHT) < h_obere))) {
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
        if ((stats.at<int>(tasten_labels[1], CC_STAT_TOP) > y_untere) && (stats.at<int>(tasten_labels[1], CC_STAT_TOP) < y_obere)) {
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
        tastatur = grayImage_edit.clone();
        cvtColor(tastatur, tastatur, COLOR_GRAY2BGR);   // muss, da hier im Gegensatz zur Vorbereitung Bild (aus Video) nicht farbig ist!
        
		// Loescht die bestehenden CCs!
		cc.clear();
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

            letters = {
                "A", "B", "C", "D", "E", "F", "G", "H"
            };

            waitKey(1);
        }
		imshow("Tastatur", tastatur);
	}
    
    // Praktikum 3
    void auswertung_tiefenbild() {
        // Histogramm des Tiefenbilds erstellen& anzeigen (Tiefenbild muss Schwarz-Weiss sein!)
        tiefenbild = depthImage_edit_gray.clone();
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
        threshold(binaer_tiefen, binaer_tiefen, value, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
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
	/********************************************************************************************************************************************************\
	* Praktikum 1/2/3 Inhaltliche Veraenderungen
	* ENDE
	\********************************************************************************************************************************************************/

private:
    Mat zImage;                                     // Das Tiefenbild, vor Kontrastspreizung, Skalierung und Einfaerbung
    Mat grayImage;                                  // Das Grauwertbild, vor Kontrastspreizung und Skalierung
	Mat cameraMatrix, distortionCoefficients;       // Nicht wichtig!
	mutex flagMutex;                                // Nicht wichtig!

	/********************************************************************************************************************************************************\
	* ANFANG
	* Praktikum 1/2/3 Inhaltliche Veraenderungen
	\********************************************************************************************************************************************************/

	// Praktikum 1
    Mat depthImage_edit_gray;                       // Das Tiefenbild, nach Kontrastspreizung und Skalierung
    Mat depthImage_edit;                            // Das Tiefenbild, nach Kontrastspreizung, Skalierung und Einfaerbung
    Mat grayImage_edit;                             // Das Grauwertbild, nach Kontrastspreizung und Skalierung
	VideoWriter vw_gray, vw_depth;                  // Die VideoWriter zum abspeichern der Videos!
    string file_depth;                              // Der Dateiname fuer das Tiefenbild
	string file_gray;                               // Der Dateiname fuer das Grauwertbild
	int mode;                                       // Der Modus, in dem das Programm ablaeuft (Wiedergabe, Aufnahme, Auswertung)

	// Praktikum 2
	Mat mit20_bild;                                  // Das Grauwertbild, das ueber 20 Frames gemittelt ist (wird nicht weiter verwendet)
    vector<int> mit_ueber_20_frames;                // Der Vektor fuer die 20 Frames gemittelt (wird nicht weiter verwendet)
    int glaettung_frame;                            // Gibt an, wie viele Frames durchlaufen wurden, um 20 Frames zu mitteln (wird nicht weiter verwendet)
	Mat med_bild;                                    // Das Grauwertbild, das ueber den Median ermittelt wurde => WIRD WEITER VERWENDET!
    Mat mit_bild;                                   // Das Grauwertbild, das ueber den Mittelwert ermittelt wurde (wird nicht weiter verwendet)
    Mat grau_otsu;                                  // Das Grauwertbild, das mit OTSU segmentiert wurde
    bool hochkant;                                  // Gibt an, ob das Bild hochkant ist oder nicht!
    vector<Scalar> farben;                          // Das Array mit allen Farben zur Zuordnung zu CCs
    vector<string> letters;                         // Das Array nut allen Buchstaben zur Zuordnung zu CCs
    Mat tastatur;                                   // Das Bild der eingefärbten Tastatur.
    vector<vector<int>> cc;                         // Vektoren der einzelnen CCs mit X/Y-Koordinaten, Hoehe/Breite
    
    // Praktikum 3
    Mat tiefenbild;                                 // Das Tiefenbild, abgespeichert um es mehrfach zu verwenden
    Mat hist_values;                                // Fuer alle 0-255 Werte des Histogramms, die Anzahl, wie oft jeder Wert vorkommt (gebraucht fuer Min/Max)
	bool vorhanden = false;							// Ueberprueft, ob es der erste Frame ist!
	Mat altes_binaer_tiefen;						// Das binäre Tiefenbild aus dem alten Frame!

	/********************************************************************************************************************************************************\
	* Praktikum 1/2/3 Inhaltliche Veraenderungen
	* ENDE
	\********************************************************************************************************************************************************/
};


int main(int argc, char *argv[]) {
	MyListener listener;

	/********************************************************************************************************************************************************\
	* ANFANG
	* Praktikum 1/2/3 Inhaltliche Veraenderungen
	\********************************************************************************************************************************************************/

	int param = 0;
	string filename = "test"; // Default-Wert ggf. loeschen!

    // Parameter abfragen & enrsprechend handeln
	if (argc > 1) {
		param = atoi(argv[1]);
		if (param == 3) {
			cout << "Bitte Datei-Namen des fertigen Videos eingeben" << endl; cin >> filename;
			VideoCapture cap(filename);
			if (!cap.isOpened()) { cerr << "Kann Video nicht wiedergeben" << endl; return 1; }
			Mat frame;
			for (;;) {
				cap >> frame;
				if (frame.empty()) { cerr << "Frame ist leer" << endl; break; }
				imshow("Video (Tiefen)", frame);
				waitKey(20);
			}
			return 0;
		} else if (param == 2) {
			cout << "Bitte Datei-Praefix des aufnehmenden Videos eingeben" << endl; cin >> filename;
		}
	}
	/********************************************************************************************************************************************************\
	* Praktikum 1/2/3 Inhaltliche Veraenderungen
	* ENDE
	\********************************************************************************************************************************************************/

	unique_ptr<royale::ICameraDevice> cameraDevice;
	{
		royale::CameraManager manager;
		royale::Vector<royale::String> camlist(manager.getConnectedCameraList());
		std::cout << "Detected " << camlist.size() << " camera(s)." << endl;
		if (!camlist.empty()) { cameraDevice = manager.createCamera(camlist[0]); }
        else {
			cerr << "No suitable camera device detected." << endl
				<< "Please make sure that a supported camera is plugged in, all drivers are "
				<< "installed, and you have proper USB permission" << endl;
			return 1;
		}
		camlist.clear();
	}
	if (cameraDevice == nullptr) {
		if (argc > 1) { cerr << "Could not open " << argv[1] << endl; return 1; }
        else { cerr << "Cannot create the camera device" << endl; return 1; }
	}
	auto status = cameraDevice->initialize();
	if (status != royale::CameraStatus::SUCCESS) { cerr << "Cannot initialize the camera device, error string : " << getErrorString(status) << endl; return 1; }
	royale::LensParameters lensParameters;
	status = cameraDevice->getLensParameters(lensParameters);
	if (status != royale::CameraStatus::SUCCESS) { cerr << "Can't read out the lens parameters" << endl; return 1; }
	listener.setLensParameters(lensParameters);
	if (cameraDevice->registerDataListener(&listener) != royale::CameraStatus::SUCCESS) { cerr << "Error registering data listener" << endl; return 1; }
	cameraDevice->setExposureMode(royale::ExposureMode::AUTOMATIC);
	if (cameraDevice->startCapture() != royale::CameraStatus::SUCCESS) { cerr << "Error starting the capturing" << endl; return 1; }

	/********************************************************************************************************************************************************\
	* ANFANG
	* Praktikum 1/2/3 Inhaltliche Veraenderungen
	\********************************************************************************************************************************************************/

	listener.setMode(param);
	listener.setFrame(0);
	uint16_t height, width;
    cameraDevice->getMaxSensorWidth(width);
    cameraDevice->getMaxSensorHeight(height);
	vector<int> v;
	for (int i = 0; i < width*height; i++) { v.push_back(0); }
	listener.set_mit_ueber_20_frames(v);

	if (param == 2) {
		// Zum aufnehmen bereit machen
		uint16_t fps;
		Size bild(width, height);
		cameraDevice->getFrameRate(fps);
		listener.open_video_files(filename, bild, fps);
		cout << "Videoaufnahme gestartet" << endl;
	}

	cout << "Fenster oeffnen, damit Abbrechen mit Enter-Taste funktioniert!" << endl;
	namedWindow("Gray (Edit)");
	namedWindow("Depth (Edit)");

	// "Endlosschleife" fuer Anzeige der Tiefen- / Grauwertbilder usw. sowie Aufnahme der Bilder
	for (;;) {
        listener.write_video_files();
        if (waitKey(1) == 13) {
            break;
        }
    }

	if (param == 2) {
		listener.close_video_files();
		cout << "Videoaufnahme gestoppt" << endl;
	}

    cout << "Alle bestehenden Fenster schliessen" << endl;
	destroyAllWindows();

	/********************************************************************************************************************************************************\
	* Praktikum 1/2/3 Inhaltliche Veraenderungen
	* ENDE
	\********************************************************************************************************************************************************/

	if (cameraDevice->stopCapture() != royale::CameraStatus::SUCCESS) { cerr << "Error stopping the capturing" << endl; return 1; }
	return 0;
}
