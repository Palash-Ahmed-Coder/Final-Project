import cv2
import serial
import os
import time

# Initialize serial communication with ESP32 (Update the COM port)
ser = serial.Serial('COM8', 9600, timeout=1)

# Load Haar cascades for face and eye detection
face_cascade_path = r"C:\Users\Palash\PycharmProjects\PythonProject\Drowsiness\haarcascade_frontalface_default.xml"
eye_cascade_path = r"C:\Users\Palash\PycharmProjects\PythonProject\Drowsiness\haarcascade_eye.xml"

# Ensure cascade files exist
if not os.path.exists(face_cascade_path) or not os.path.exists(eye_cascade_path):
    print("Error: Haar cascade files not found!")
    exit()

face_cascade = cv2.CascadeClassifier(face_cascade_path)
eye_cascade = cv2.CascadeClassifier(eye_cascade_path)

# Initialize video capture
cap = cv2.VideoCapture(0)

closed_frames = 0  # Counter for closed eyes
CLOSED_THRESHOLD = 15  # Frames needed to trigger an alert

buzzer_on = False  # Track buzzer state to prevent unnecessary signals

while True:
    ret, frame = cap.read()
    if not ret:
        break

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    # Detect faces
    faces = face_cascade.detectMultiScale(gray, scaleFactor=1.3, minNeighbors=5, minSize=(50, 50))

    for (x, y, w, h) in faces:
        roi_gray = gray[y:y + h, x:x + w]
        roi_color = frame[y:y + h, x:x + w]

        # Detect eyes within the detected face
        eyes = eye_cascade.detectMultiScale(roi_gray, scaleFactor=1.1, minNeighbors=5, minSize=(15, 15))

        if len(eyes) == 0:  # No eyes detected = possibly closed eyes
            closed_frames += 1
        else:
            closed_frames = 0  # Reset if eyes are detected

        # Check if drowsiness threshold is reached
        if closed_frames >= CLOSED_THRESHOLD and not buzzer_on:
            ser.write(b'1')  # Turn ON buzzer
            print("Drowsiness detected! Buzzer ON.")
            buzzer_on = True  # Update buzzer state
            time.sleep(0.5)  # Prevent continuous '1' signals

        elif closed_frames == 0 and buzzer_on:
            ser.write(b'0')  # Turn OFF buzzer
            print("Eyes opened. Buzzer OFF.")
            buzzer_on = False  # Reset buzzer state
            time.sleep(0.1)  # Prevent continuous '0' signals

        # Draw rectangles around face and eyes
        cv2.rectangle(frame, (x, y), (x + w, y + h), (255, 0, 0), 2)
        for (ex, ey, ew, eh) in eyes:
            cv2.rectangle(roi_color, (ex, ey), (ex + ew, ey + eh), (0, 255, 0), 2)

    cv2.imshow("Driver Drowsiness Detection", frame)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
ser.close()
