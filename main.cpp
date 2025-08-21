#include <opencv2/opencv.hpp>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

using namespace cv;
using namespace std;
namespace fs = std::filesystem;

/**
 * @class AttendanceSystem
 * @brief Implements a face recognition-based attendance system using OpenCV.
 * 
 * Features:
 * - Load known faces from a directory.
 * - Capture webcam feed and detect faces in real-time.
 * - Verify faces for 3 seconds before marking attendance.
 * - Prevent duplicate attendance marking for the same person per day.
 * - Display real-time status on the webcam interface.
 */
class AttendanceSystem {
private:
    CascadeClassifier face_cascade;                       ///< Haar cascade classifier for face detection
    string photos_path;                                   ///< Path to stored known face images
    string cascade_path;                                  ///< Path to Haar cascade XML file
    string attendance_file;                               ///< CSV file to store attendance
    unordered_set<string> attendance_set;                ///< Names already marked today
    unordered_map<string, Mat> known_faces;              ///< Map: name -> processed face image
    unordered_map<string, chrono::steady_clock::time_point> last_mark_time; ///< Cooldown tracker
    chrono::seconds mark_cooldown = chrono::seconds(10); ///< Minimum time between consecutive marks
    string current_date;                                  ///< Today's date

public:
    /**
     * @brief Constructor: Loads cascade, known faces, and today's attendance.
     */
    AttendanceSystem(
        const string& photos = "photos",
        const string& cascade = "haarcascade_frontalface_default.xml",
        const string& file = "attendance.csv")
        : photos_path(photos), cascade_path(cascade), attendance_file(file)
    {
        current_date = getCurrentDate();

        if (!face_cascade.load(cascade_path)) {
            cerr << "Error: Could not load face cascade from " << cascade_path << endl;
            exit(EXIT_FAILURE);
        }

        loadAttendance();
        loadKnownFaces();
    }

    /**
     * @brief Get current date as YYYY-MM-DD
     */
    static string getCurrentDate() {
        auto now = chrono::system_clock::now();
        time_t t = chrono::system_clock::to_time_t(now);
        tm local_tm{};
#ifdef _WIN32
        localtime_s(&local_tm, &t);
#else
        localtime_r(&t, &local_tm);
#endif
        stringstream ss;
        ss << (local_tm.tm_year + 1900) << "-"
           << setw(2) << setfill('0') << (local_tm.tm_mon + 1) << "-"
           << setw(2) << setfill('0') << local_tm.tm_mday;
        return ss.str();
    }

    /**
     * @brief Get the current day of the week (Sun, Mon, ..., Sat)
     */
    static string getCurrentDay() {
        auto now = chrono::system_clock::now();
        time_t t = chrono::system_clock::to_time_t(now);
        tm local_tm{};
#ifdef _WIN32
        localtime_s(&local_tm, &t);
#else
        localtime_r(&t, &local_tm);
#endif
        const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        return days[local_tm.tm_wday];
    }

    /**
     * @brief Load already marked attendance for today from CSV.
     */
    void loadAttendance() {
        ifstream file(attendance_file);
        if (!file.is_open()) return;

        string line;
        while (getline(file, line)) {
            auto c1 = line.find(',');
            auto c2 = line.find(',', c1 + 1);
            if (c1 == string::npos || c2 == string::npos) continue;
            string name = line.substr(0, c1);
            string date = line.substr(c1 + 1, c2 - c1 - 1);
            if (date == current_date) attendance_set.insert(name);
        }
        file.close();
    }

    /**
     * @brief Mark attendance for a given name (if not already marked).
     * @param name Name of the person.
     */
    void markAttendance(const string& name) {
        auto now = chrono::steady_clock::now();
        if (last_mark_time.count(name) && (now - last_mark_time[name]) < mark_cooldown)
            return;

        last_mark_time[name] = now;

        if (attendance_set.count(name)) {
            cout << "[Attendance] Already marked today: " << name
                 << " (" << current_date << ", " << getCurrentDay() << ")" << endl;
            return;
        }

        attendance_set.insert(name);

        ofstream file(attendance_file, ios::app);
        if (file.is_open()) {
            string day = getCurrentDay();
            file << name << "," << current_date << "," << day << "\n";
            file.flush();
            cout << "[Attendance] Successfully marked: " << name
                 << " | " << current_date << " (" << day << ")" << endl;
            file.close();
        }
    }

    /**
     * @brief Load known faces from the photos directory into memory.
     */
    void loadKnownFaces() {
        if (!fs::exists(photos_path) || !fs::is_directory(photos_path)) {
            cerr << "Photos path not found: " << photos_path << endl;
            exit(EXIT_FAILURE);
        }

        size_t loaded = 0;
        for (auto& entry : fs::directory_iterator(photos_path)) {
            if (!entry.is_regular_file()) continue;
            string ext = entry.path().extension().string();
            if (!isImage(ext)) continue;

            string name = inferName(entry.path().stem().string());
            Mat img = imread(entry.path().string());
            Mat face = extractFace(img);
            if (!face.empty()) {
                known_faces[name] = face;
                loaded++;
            }
        }

        if (loaded == 0) {
            cerr << "No usable faces found in " << photos_path << endl;
            exit(EXIT_FAILURE);
        }
        cout << "[Info] Loaded " << loaded << " known faces.\n";
    }

    /**
     * @brief Run real-time attendance using webcam.
     *        Displays verification messages and prevents duplicate attendance marking.
     */
    void runAttendance() {
        VideoCapture cap(0);
        if (!cap.isOpened()) {
            cerr << "Cannot open webcam!" << endl;
            return;
        }
        cout << "Press 'q' to quit.\n";

        string candidate_name = "Unknown";
        chrono::steady_clock::time_point candidate_start;
        bool verified_today = false;

        while (true) {
            Mat frame;
            cap >> frame;
            if (frame.empty()) continue;

            Mat gray;
            cvtColor(frame, gray, COLOR_BGR2GRAY);
            equalizeHist(gray, gray);

            vector<Rect> faces;
            face_cascade.detectMultiScale(gray, faces, 1.1, 5, 0, Size(80,80));

            string detected_name = "Unknown";
            for (auto& r : faces) {
                Mat roi = gray(r);
                resize(roi, roi, Size(200,200));
                detected_name = recognizeFace(roi);

                rectangle(frame, r, Scalar(255,0,0), 2);
                putText(frame, detected_name, Point(r.x, max(0, r.y-10)),
                        FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0,255,0),2);
            }

            // Verification logic
            auto now = chrono::steady_clock::now();
            if (detected_name != "Unknown") {
                if (candidate_name != detected_name) {
                    candidate_name = detected_name;
                    candidate_start = now;
                    verified_today = false;
                } else {
                    auto duration = chrono::duration_cast<chrono::seconds>(now - candidate_start).count();
                    if (duration >= 3) {
                        if (attendance_set.count(candidate_name)) {
                            // Already marked → show red continuously
                            putText(frame, "Attendance Marked For Today: " + candidate_name, Point(10,30),
                                    FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0,165,255),2);
                            verified_today = true;
                        } else if (!verified_today) {
                            markAttendance(candidate_name);
                            verified_today = true;
                            putText(frame, "Attendance Successful: " + candidate_name, Point(10,30),
                                    FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0,255,0),2);
                        } else {
                            putText(frame, "Attendance Successful: " + candidate_name, Point(10,30),
                                    FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0,255,0),2);
                        }
                    } else {
                        putText(frame, "Verifying " + candidate_name + "...", Point(10,30),
                                FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0,255,255),2);
                    }
                }
            } else {
                candidate_name.clear();
                verified_today = false;
            }

            imshow("Attendance", frame);
            char c = (char)waitKey(10);
            if(c == 'q' || c=='Q') break;
        }

        cap.release();
        destroyAllWindows();
    }

    /**
     * @brief Display today's attendance in console.
     */
    void viewAttendanceToday() {
        cout << "\nAttendance for " << current_date << ":\n";
        if(attendance_set.empty()) cout << "No attendance yet.\n";
        for(auto &name : attendance_set) cout << "- " << name << "\n";
    }

private:
    /**
     * @brief Check if a file extension corresponds to an image.
     */
    static bool isImage(const string &ext) {
        string e = ext;
        transform(e.begin(), e.end(), e.begin(), ::tolower);
        return e==".jpg" || e==".jpeg" || e==".png" || e==".bmp" || e==".tiff";
    }

    /**
     * @brief Infer a person's name from filename stem.
     */
    static string inferName(const string& stem) {
        size_t pos = stem.find_first_of("_- ");
        return (pos==string::npos)? stem : stem.substr(0,pos);
    }

    /**
     * @brief Detect and extract the largest face from an image.
     */
    Mat extractFace(const Mat& img) {
        if(img.empty()) return Mat();
        Mat gray;
        cvtColor(img, gray, COLOR_BGR2GRAY);
        vector<Rect> faces;
        face_cascade.detectMultiScale(gray, faces, 1.1, 4, 0, Size(80,80));
        if(faces.empty()) return Mat();
        Rect best = *max_element(faces.begin(), faces.end(),
                                 [](const Rect& a, const Rect& b){ return a.area() < b.area(); });
        Mat roi = gray(best);
        resize(roi, roi, Size(200,200));
        equalizeHist(roi, roi);
        return roi;
    }

    /**
     * @brief Recognize a face by comparing with known faces using mean squared error.
     */
    string recognizeFace(const Mat& face) {
        string best_name = "Unknown";
        double min_mse = DBL_MAX;

        for(auto &kv : known_faces) {
            Mat diff;
            absdiff(face, kv.second, diff);
            double mse = sum(diff.mul(diff))[0] / (200.0*200.0);
            if(mse < min_mse && mse < 1500.0) {
                min_mse = mse;
                best_name = kv.first;
            }
        }
        return best_name;
    }
};

// ----------------- Main Function -----------------
int main() {
    AttendanceSystem system;
    int choice=0;
    do {
        cout << "\n==== Face Attendance ====\n";
        cout << "1. Start Attendance (webcam)\n";
        cout << "2. View Today's Attendance\n";
        cout << "3. Exit\n";
        cout << "Choice: ";
        if(!(cin>>choice)) { cin.clear(); cin.ignore(10000,'\n'); continue; }

        if(choice==1) system.runAttendance();
        else if(choice==2) system.viewAttendanceToday();
        else if(choice==3) cout << "Goodbye!\n";
        else cout << "Invalid choice.\n";
    } while(choice !=3);

    return 0;
}
