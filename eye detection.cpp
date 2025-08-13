#include <opencv2/opencv.hpp>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <unordered_set>
#include <chrono>
#include <ctime>
#include <sstream>

using namespace cv;
using namespace std;
namespace fs = std::filesystem;

/**
 * @class AttendanceSystem
 * @brief Handles face recognition-based attendance system.
 */
class AttendanceSystem {
private:
    CascadeClassifier face_cascade;
    string photos_path;
    string cascade_path;
    string attendance_file;
    unordered_set<string> attendance_set;
    string current_date;

public:
    AttendanceSystem(
        const string& photos = "D:/Projects/OOPproject/photos",
        const string& cascade = "D:/opencv/build/etc/haarcascades/haarcascade_frontalface_default.xml",
        const string& file = "D:/Projects/OOPproject/attendance.csv")
        : photos_path(photos), cascade_path(cascade), attendance_file(file) 
    {
        current_date = getCurrentDate();

        if (!face_cascade.load(cascade_path)) {
            cerr << "Error: Could not load face cascade from " << cascade_path << endl;
            exit(-1);
        }

        loadAttendance();
    }

    string getCurrentDate() {
        auto now = chrono::system_clock::now();
        time_t t = chrono::system_clock::to_time_t(now);
        tm local_tm;
        localtime_s(&local_tm, &t);

        stringstream ss;
        ss << (local_tm.tm_year + 1900) << "-" 
           << (local_tm.tm_mon + 1) << "-" 
           << local_tm.tm_mday;
        return ss.str();
    }

    string getCurrentDay() {
        auto now = chrono::system_clock::now();
        time_t t = chrono::system_clock::to_time_t(now);
        tm local_tm;
        localtime_s(&local_tm, &t);

        const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        return days[local_tm.tm_wday];
    }

    void loadAttendance() {
        ifstream file(attendance_file);
        if (!file.is_open()) return;

        string line;
        while (getline(file, line)) {
            string name = line.substr(0, line.find(','));
            string date = line.substr(line.find(',') + 1);
            date = date.substr(0, date.find(','));
            if (date == current_date)
                attendance_set.insert(name);
        }
        file.close();
    }

    void markAttendance(const string& name) {
        if (attendance_set.count(name)) return;

        attendance_set.insert(name);

        ofstream file(attendance_file, ios::app);
        if (file.is_open()) {
            string day = getCurrentDay();
            file << name << "," << current_date << "," << day << "\n";
            cout << "[Attendance] Marked: " << name << " | " 
                 << current_date << " (" << day << ")" << endl;
            file.close();
        }
    }

    void loadKnownFaces(vector<Mat>& face_images, vector<string>& face_names) {
        for (auto& entry : fs::directory_iterator(photos_path)) {
            if (entry.is_regular_file()) {
                string file_path = entry.path().string();
                string filename = entry.path().stem().string();
                Mat img = imread(file_path);
                if (!img.empty()) {
                    face_images.push_back(img);
                    face_names.push_back(filename);
                }
            }
        }
    }

    string recognizeFace(const Mat& face, const vector<Mat>& face_images, const vector<string>& face_names) {
        Mat face_gray;
        cvtColor(face, face_gray, COLOR_BGR2GRAY);
        resize(face_gray, face_gray, Size(100,100));

        double min_diff = 1e9;
        string name = "Unknown";

        for (size_t i = 0; i < face_images.size(); i++) {
            Mat known_gray;
            cvtColor(face_images[i], known_gray, COLOR_BGR2GRAY);
            resize(known_gray, known_gray, Size(100,100));

            Mat diff;
            absdiff(face_gray, known_gray, diff);
            double mse = sum(diff)[0] / (100*100);

            if (mse < min_diff && mse < 1000.0) {
                min_diff = mse;
                name = face_names[i];
            }
        }
        return name;
    }

    void runAttendance() {
        vector<Mat> known_faces;
        vector<string> known_names;
        loadKnownFaces(known_faces, known_names);

        if (known_faces.empty()) {
            cerr << "No known faces found in " << photos_path << endl;
            return;
        }

        VideoCapture cap(0);
        if (!cap.isOpened()) {
            cerr << "Cannot open webcam!" << endl;
            return;
        }

        cout << "Press 'q' to quit attendance mode.\n";

        while (true) {
            Mat frame;
            cap >> frame;
            if (frame.empty()) continue;

            Mat gray;
            cvtColor(frame, gray, COLOR_BGR2GRAY);

            vector<Rect> faces;
            face_cascade.detectMultiScale(gray, faces, 1.3, 5, 0, Size(50,50));

            for (auto& rect : faces) {
                rectangle(frame, rect, Scalar(255,0,0), 2);
                Mat faceROI = frame(rect);
                string name = recognizeFace(faceROI, known_faces, known_names);
                markAttendance(name);
                putText(frame, name, Point(rect.x, rect.y - 10), FONT_HERSHEY_SIMPLEX, 1, Scalar(0,255,0), 2);
            }

            imshow("Attendance System", frame);
            char c = (char)waitKey(10);
            if (c == 'q' || c == 'Q') break;
        }

        cap.release();
        destroyAllWindows();
    }

    void viewAttendanceToday() {
        cout << "\nAttendance for " << current_date << ":\n";
        if (attendance_set.empty()) {
            cout << "No attendance marked today yet.\n";
            return;
        }

        for (const string& name : attendance_set) {
            cout << "- " << name << endl;
        }
    }
};

/**
 * @brief Main function with menu
 */
int main() {
    AttendanceSystem system;
    int choice = 0;

    do {
        cout << "\n==== Face Recognition Attendance System ====\n";
        cout << "1. Start Attendance\n";
        cout << "2. View Today's Attendance\n";
        cout << "3. Exit\n";
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice) {
            case 1:
                system.runAttendance();
                break;
            case 2:
                system.viewAttendanceToday();
                break;
            case 3:
                cout << "Exiting system. Goodbye!\n";
                break;
            default:
                cout << "Invalid choice. Please try again.\n";
        }
    } while (choice != 3);

    return 0;
}
