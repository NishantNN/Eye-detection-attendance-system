# Face Recognition Attendance System 🎥👨‍💻

A **real-time attendance management system** that uses **OpenCV Haar Cascade** for face detection and **Mean Squared Error (MSE)** for recognition.  
The system captures faces via webcam, verifies them for a few seconds, and marks attendance into a **CSV file** — while preventing duplicates for the same day.

---

## Authors 

This model is built by **Electronics, Communication and Information Engineering** students year I part II in partial fulfillment of Bachelors in Engineering Degree under **Institute of Engineering, Thapathali Campus Department of Electronics and Computer Engineering**.
- Krishna Kandel			THA081BEI014
- Nishanta Poudel			THA081BEI025
- Pranish Pokhrel			THA081BEI029
- Prateek Chaulagain		THA081BEI030

## ✨ Features

- 📂 **Loads known faces** from a local directory.
- 📸 **Real-time face detection** using Haar Cascade.
- ✅ **Verification step (3 seconds)** before confirming identity.
- ⏱️ **Cooldown system** to prevent accidental multiple markings.
- 🔁 **Duplicate prevention** – only one attendance per person per day.
- 🖥️ **On-screen status display** (verification, successful, or already marked).
- 📑 **Attendance stored in CSV** with name, date, and day of week.
- 📊 **View today’s attendance** in console.

---

## 🛠️ Requirements

- **C++17** or later
- [OpenCV 4.x](https://opencv.org/releases/) (with `opencv_world` or core modules installed)
- CMake (for building project)
- A working **webcam**
- Modern compiler (MSVC, g++, or clang++)

---

## 📂 Project Structure

```
/photos                # Directory containing known faces (labeled by filename)
/attendance.csv        # CSV file where attendance is saved
/main.cpp              # Main source code (AttendanceSystem class + main function)
```

---

## ⚙️ Installation & Setup

1. **Clone the repository**
   ```bash
   git clone https://github.com/yourusername/Eye-detection-attendance-system.git
   cd Eye-detection-attendance-system
   ```

2. **Add known faces**  
   - Place images in the `/photos` directory.  
   - File names should contain the person’s name (e.g., `Alice.jpg`, `Bob_1.png`).  

3. **Build the project**
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build .
   ```

4. **Run the program**
   ```bash
   ./OOPproject
   ```

---

## ▶️ Usage

After running, choose from the **menu options**:

```
==== Face Attendance ====
1. Start Attendance (webcam)
2. View Today's Attendance
3. Exit
Choice:
```

- **1** → Starts webcam, detects & recognizes faces, and marks attendance.  
- **2** → Displays a list of all people marked present today.  
- **3** → Exits the program.  

---

## 🧮 Methodology

The program follows these **steps**:

1. **Initialization**  
   - Load Haar cascade classifier.  
   - Load known faces from `/photos`.  
   - Load today’s attendance from `attendance.csv`.  

2. **Face Detection & Capture**  
   - Webcam frames converted to grayscale.  
   - Haar Cascade detects face regions.  

3. **Face Recognition**  
   - Extracted face resized to `200x200`.  
   - Compared against stored faces using **Mean Squared Error (MSE)**:  
     \[
     MSE = \frac{1}{N} \sum_{i=1}^{N} (I_1(i) - I_2(i))^2
     \]
     where \( N = 200 \times 200 \).  
   - If MSE < **1500**, a match is confirmed.  

4. **Attendance Marking**  
   - Attendance stored in CSV as:  
     ```
     Name, Date(YYYY-MM-DD), Day
     ```
   - Prevents multiple markings for same person on same date.  

5. **Viewing Attendance**  
   - Console shows list of names already marked present today.  

6. **Termination**  
   - User exits via menu or pressing **q** during webcam session.  

---

## 📑 CSV File Format

The attendance is stored in **attendance.csv**:

```
Name,Date,Day
Alice,2025-08-20,Wed
Bob,2025-08-20,Wed
```

---

## 🚀 Future Improvements

- 🔒 Add **face embedding models** (e.g., FaceNet, dlib) for more accurate recognition.  
- 🖥️ Add a **GUI interface** (Qt/ImGui/Web) instead of console menu.  
- 🌐 Integrate with a **database** (MySQL, SQLite) instead of plain CSV.  
- 📱 Provide **mobile app integration** (Android/iOS).  
- 📊 Add an **analytics dashboard** to track attendance trends.  

---

## 📜 License

This project is open-source under the **MIT License**.  
Feel free to use and modify for personal or academic projects.  

---

## 🙌 Acknowledgements

- [OpenCV](https://opencv.org/) for computer vision.  
- Inspiration from real-world biometric attendance systems.


