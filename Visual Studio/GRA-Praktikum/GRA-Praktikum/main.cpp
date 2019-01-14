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
        if (debug) {
            imshow("Gray (Edit)", grayImage_edit);
            imshow("Depth (Edit)", depthImage_edit);
        }

		// Nur Video aufnehmen, wenn es auch der Parameter sagt
		if (mode == 2) {
			write_video_files();
        } else {
            glaettung_grauwerte();
            
            imshow("Mit20", mit20_bild);
            imshow("Med", med_bild);
            imshow("Mit", mit_bild);
            
            if (glaettung_frame > 21) {
                auswertung_geglaettete_grauwerte();
                
                if (debug) {
                    imshow("OTSU", grau_otsu);
                }
                
                imshow("Tastatur", tastatur);
            }
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

	void edit_depth_picture() {
		// Kontrastspreizung
		Mat mask;
		compare(zImage, Scalar(0), mask, CMP_GT);
		Mat depth_CV_8UC1(zImage.rows, zImage.cols, CV_8UC1);
		double min, max;
		minMaxLoc(zImage, &min, &max, 0, 0, (InputArray)mask);

		// Skalierung berechnen & Bild skalieren
		double scale = 255 / (max - min);
		convertScaleAbs(zImage, depth_CV_8UC1, scale);

		// Einfaerbung mit COLORMAP_RAINBOW
		Mat color_map;
		applyColorMap(depth_CV_8UC1, color_map, COLORMAP_RAINBOW);
		depthImage_edit = color_map;
	}

	void open_video_files(string filename, Size bild_groesse, double fps) {
		file_gray = filename + "_gray.avi";
		file_depth = filename + "_depth.avi";

		// VideoWriter fuer Graubild oeffnen
		vw_gray.open(file_gray, VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, bild_groesse, false);
		if (!vw_gray.isOpened()) {
			cout << "ERROR: Failed open VideoWriter for GrayScale" << endl;
			exit(1);
		}

		// VideoWriter fuer Tiefenbild oeffnen
		vw_depth.open(file_depth, VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, bild_groesse, true);
		if (!vw_depth.isOpened()) {
			cout << "ERROR: Failed open VideoWriter for Depth" << endl;
			exit(1);
		}
	}

	void write_video_files() {
		vw_gray.write(grayImage_edit);
		vw_depth.write(depthImage_edit);
	}

	void close_video_files() {
		vw_gray.release();
		vw_depth.release();
	}

	string get_file_gray() { return file_gray; }
	string get_file_depth() { return file_depth; }
	void setMode(int nmode) { mode = nmode; }

	// Praktikum 2
	void setFrame(int frame) { glaettung_frame = frame; }
	void set_mit_ueber_20_frames(vector<int> v) { mit_ueber_20_frames = v; };

	void glaettung_grauwerte() {
		if (glaettung_frame < 21) {
			// Fuer die ersten 20 Frames Mittelwert berechnen
			for (int i = 0; i < grayImage_edit.rows; i++) {
				for (int j = 0; j < grayImage_edit.cols; j++) {
					mit_ueber_20_frames[i*grayImage_edit.rows + j] = mit_ueber_20_frames[i*grayImage_edit.rows + j] + grayImage_edit.at<uchar>(i, j);
				}
			}
			glaettung_frame++;
		}
		else {
			// Linienprofil-Bild (Breite * Hoehe => 256 * [Hoehe des Graubilds])
			Mat linienprofil(256, grayImage_edit.rows, CV_8UC3);

			// Irgendein zufaelliger Wert, der zwischen 0-Breite liegt
			uchar x_wert = grayImage_edit.rows / 2;

			// "Anwendung" Mittelwert über 20 Frames => klappt nicht ganz, irgendwas ist da kaputt!
			if (glaettung_frame == 21) {
				mit20_bild = Mat(grayImage_edit.rows, grayImage_edit.cols, CV_8UC1);
				for (int i = 0; i < mit_ueber_20_frames.size(); i++) {
					mit_ueber_20_frames[i] = (int)mit_ueber_20_frames[i] / 20;
				}
				
				for (int i = 0; i < mit20_bild.rows; i++) {
					for (int j = 0; j < mit20_bild.cols; j++) {
						mit20_bild.at<uchar>(i, j) = mit_ueber_20_frames[i*grayImage_edit.rows + j];
					}
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
				line(linienprofil, Point(i, median_bild.at<uchar>(x_wert, i)), Point(i+1, median_bild.at<uchar>(x_wert, i+1)), med);
			}

			// Linienprofil fuer Mittelwertfilter zeichen
			Scalar mit(0, 0, 255);
			for (uchar i = 0; i < linienprofil.cols - 1; i++) {
				line(linienprofil, Point(i, mittelwert_bild.at<uchar>(x_wert, i)), Point(i + 1, mittelwert_bild.at<uchar>(x_wert, i + 1)), mit);
			}

			imshow("Linienprofil", linienprofil);
		}
	}

	void auswertung_geglaettete_grauwerte() {
		// SIEHE: http://answers.opencv.org/question/120698/drawning-labeling-components-in-a-image-opencv-c/
		// Schwellwertsegmentierung (OTSU)
		// ggf. vor OTSU noch ein Closing durchfuehren? -> Frau P.-F. meinte nicht noetig
		grau_otsu = med_bild.clone();
        //closing(grau_otsu, Mat());
		threshold(med_bild, grau_otsu, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU); // hier ggf. wie in der Vorbereitung abaendern wenn OpenCV4!

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
            // WERTE IM PRAKTIKUM AENDERN; DIE HIER SIND FUERS VIDEO!
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
        tastatur = grayImage.clone();
        cvtColor(tastatur, tastatur, COLOR_GRAY2BGR);   // muss, da hier im Gegensatz zur Vorbereitung Bild (aus Video) nicht farbig ist!
        
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
            
            rectangle(tastatur, Point(x, y), Point(x+cc[i][2], y+cc[i][3]), Scalar(color, color, color), FILLED);
            
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
    
    // Praktikum 3
    void auswertung_tiefenbild() {
        // Histogramm des Tiefenbilds erstellen& anzeigen (Tiefenbild muss Schwarz-Weiss sein!)
        tiefenbild = depthImage_edit.clone();
        cvtColor(tiefenbild, tiefenbild, COLOR_BGR2GRAY);
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
	Mat depthImage_edit                             // Das Tiefenbild, nach Kontrastspreizung, Skalierung und Einfaerbung
    Mat grayImage_edit;                             // Das Grauwertbild, nach Kontrastspreizung und Skalierung
	VideoWriter vw_gray, vw_depth;                  // Die VideoWriter zum abspeichern der Videos!
    string file_depth;                              // Der Dateiname fuer das Tiefenbild
	string file_gray;                               // Der Dateiname fuer das Grauwertbild
	int mode;                                       // Der Modus, in dem das Programm ablaeuft (Wiedergabe, Aufnahme, Auswertung)

	// Praktikum 2
	Mat mit20_bild                                  // Das Grauwertbild, das ueber 20 Frames gemittelt ist (wird nicht weiter verwendet)
    vector<int> mit_ueber_20_frames;                // Der Vektor fuer die 20 Frames gemittelt (wird nicht weiter verwendet)
    int glaettung_frame;                            // Gibt an, wie viele Frames durchlaufen wurden, um 20 Frames zu mitteln (wird nicht weiter verwendet)
    Mat med_bild                                    // Das Grauwertbild, das ueber den Median ermittelt wurde => WIRD WEITER VERWENDET!
    Mat mit_bild;                                   // Das Grauwertbild, das ueber den Mittelwert ermittelt wurde (wird nicht weiter verwendet)
    Mat grau_otsu;                                  // Das Grauwertbild, das mit OTSU segmentiert wurde
    bool hochkant;                                  // Gibt an, ob das Bild hochkant ist oder nicht!
    Mat tastatur;                                   // Das Bild der eingefärbten Tastatur.
    vector<vector<int>> cc;                         // Vektoren der einzelnen CCs mit X/Y-Koordinaten, Hoehe/Breite
    
    // Praktikum 3
    Mat tiefenbild;                                 // Das Tiefenbild, abgespeichert um es mehrfach zu verwenden
    Mat hist_values;                                // Fuer alle 0-255 Werte des Histogramms, die Anzahl, wie oft jeder Wert vorkommt (gebraucht fuer Min/Max)
    vector<vector<int>> cc_tiefen;                  // Vektoren der einzelnen Tiefen-CCs mit X/Y-Koordinaten, Hoehe/Breite fuer Vergleich naechster Frame
    vector<vector<int>> cc_tiefen_blau;             // Alle alten CCs, die nicht auch bei den neuen CCs dabei sind!
    vector<vector<int>> cc_tiefen_rot;              // Alle neuen CCs, die nicht auch bei den alten CCs dabei sind!
    int index_max_value;                            // Gibt Index der Taste an, die gedrueckt wurde (muss aus allen roten Flaechen ausgewaehlt werden?)

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
			cout << "Bitte Datei-Namen des fertigen Videos eingeben" << endl;
			cin >> filename;

			VideoCapture cap(filename);
			if (!cap.isOpened()) {
				cerr << "Kann Video nicht wiedergeben" << endl;
				return 1;
			}

			Mat frame;
			for (;;) {
				cap >> frame;
				if (frame.empty()) {
					cerr << "Frame ist leer" << endl;
					break;
				}
				imshow("Video (Tiefen)", frame);
				waitKey(20);
			}
			return 0;
		} else if (param == 2) {
			cout << "Bitte Datei-Praefix des aufnehmenden Videos eingeben" << endl;
			cin >> filename;
		}
	}
	/********************************************************************************************************************************************************\
	* Praktikum 1/2/3 Inhaltliche Veraenderungen
	* ENDE
	\********************************************************************************************************************************************************/

	// this represents the main camera device object
	unique_ptr<royale::ICameraDevice> cameraDevice;

	// the camera manager will query for a connected camera
	{
		royale::CameraManager manager;

		// try to open the first connected camera
		royale::Vector<royale::String> camlist(manager.getConnectedCameraList());
		std::cout << "Detected " << camlist.size() << " camera(s)." << endl;

		if (!camlist.empty()) {
			cameraDevice = manager.createCamera(camlist[0]);
		} else {
			cerr << "No suitable camera device detected." << endl
				<< "Please make sure that a supported camera is plugged in, all drivers are "
				<< "installed, and you have proper USB permission" << endl;
			return 1;
		}
		camlist.clear();
	}

	// the camera device is now available and CameraManager can be deallocated here
	if (cameraDevice == nullptr) {
		// no cameraDevice available
		if (argc > 1) {
			cerr << "Could not open " << argv[1] << endl;
			return 1;
		} else {
			cerr << "Cannot create the camera device" << endl;
			return 1;
		}
	}

	// call the initialize method before working with the camera device
	auto status = cameraDevice->initialize();
	if (status != royale::CameraStatus::SUCCESS) {
		cerr << "Cannot initialize the camera device, error string : " << getErrorString(status) << endl;
		return 1;
	}

	// retrieve the lens parameters from Royale
	royale::LensParameters lensParameters;
	status = cameraDevice->getLensParameters(lensParameters);
	if (status != royale::CameraStatus::SUCCESS) {
		cerr << "Can't read out the lens parameters" << endl;
		return 1;
	}

	listener.setLensParameters(lensParameters);

	// register a data listener
	if (cameraDevice->registerDataListener(&listener) != royale::CameraStatus::SUCCESS) {
		cerr << "Error registering data listener" << endl;
		return 1;
	}

	// Belichtung muss auf Automatisch gesetzt werden
	cameraDevice->setExposureMode(royale::ExposureMode::AUTOMATIC);

	// Ganz normales Vorgehen nach Code-Geruest!
	// start capture mode
	if (cameraDevice->startCapture() != royale::CameraStatus::SUCCESS) {
		cerr << "Error starting the capturing" << endl;
		return 1;
	}

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
	for (;;) { if (waitKey(1) == 13) { break; } }

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

	// stop capture mode
	if (cameraDevice->stopCapture() != royale::CameraStatus::SUCCESS) {
		cerr << "Error stopping the capturing" << endl;
		return 1;
	}
	return 0;
}
