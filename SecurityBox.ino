#include <EEPROM.h>
#include <Keypad.h>
#include <Key.h>
#include <Servo.h>

#include "pitches.h"

#define OPEN_GATE 80
#define CLOSE_GATE 0
#define RED_PIN A0
#define BLUE_PIN A1
#define GREEN_PIN A2

enum PasswordState {
	none,
	started,
	finished,
	valid,
	invalid
};
enum LockState {
	closed,
	opened
};
enum ledColor {
	blank,
	green,
	red,
	blue
};


void resetObjects();
void turnLightOn(ledColor color);

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
	{ '1', '2', '3', 'A' },
	{ '4', '5', '6', 'B' },
	{ '7', '8', '9', 'C' },
	{ '*', '0', '#', 'D' }
};
byte colPins[COLS] = { 9, 8, 7, 6 }; //2, 3, 4, 5 }; //connect to the column pinouts of the kpd
byte rowPins[ROWS] = { 5, 4, 3, 2};//6, 7, 8, 9 }; //connect to the row pinouts of the kpd

Keypad kpd = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String msg;
Servo servo;
PasswordState startEnterPassword;
PasswordState startNewPassword;
const char masterCode[] = { '8', '8', '2', '2', '4', '6', '4', '6', 'B', 'A' };
KeypadEvent pressedKey;
LockState lock = closed;
PasswordState passwordState = invalid;
int passwordIndex = 0;
char newPassword[16];
char savedPassword[16];
int address = 0;
bool isPasswordValid = true;
/* Pitches variable */
int melody_failed[] = { NOTE_C4, 0, NOTE_C4, 0, NOTE_C4, 0, NOTE_C4 };
int failedNodeDuration = 16;
int melody_success[] = { NOTE_A6, 0, NOTE_A7, 0, NOTE_F3, 0, NOTE_A7 };
int successNoteDuration = 32;
//					0			1		 2			3		4		5			6		7		  8			9		A		  B			C		D		  #			*
int key_tone[] = { NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6, NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7 };

void setup()
{

	Serial.begin(9600);
	msg = "";
	servo.attach(10);
	servo.write(0);
	for (int i = 0; i < 16; ++i)
		savedPassword[i] = '\0';
	kpd.addEventListener(keypadEvent);
	if (EEPROM.read(address) == -1) {
		EEPROM.put(address, "1234");
	}
	EEPROM.get(address, savedPassword);
	Serial.print("current password:"); Serial.println(savedPassword);

	// Initialize RGB pins
	pinMode(RED_PIN, OUTPUT);
	pinMode(BLUE_PIN, OUTPUT);
	pinMode(GREEN_PIN, OUTPUT);
	turnLightOn(blank);
}

void loop()
{
	char k = kpd.getKey();
	if (kpd.getState() == RELEASED) {

		if (startEnterPassword == valid) {
			// valid password 
			// * open the gates.... 
			if (lock == closed) {
				servo.write(OPEN_GATE);
				lock = opened;

			}
			else {
				servo.write(CLOSE_GATE);
				lock = closed;
			}
			// * Green LED....
			turnLightOn(green);
			delay(50);
			turnLightOn(blank);
			// * make a sound....
			playTone(melody_success, successNoteDuration);
			// * Reset enter password object....
			resetObjects();
		}
		else if (startEnterPassword == invalid) {
			servo.write(CLOSE_GATE);
			// invalid password
			// * close the gates.....
			// * make a bad sound.....
			// * Red LED....
			turnLightOn(red);
			delay(50);
			turnLightOn(blank);
			playTone(melody_failed, failedNodeDuration);
			// * Reset enter password object....

			resetObjects();
		}
		else if (startNewPassword == finished) {
			Serial.print("Entered new password:");
			Serial.println(newPassword);
			EEPROM.put(address, newPassword);
			EEPROM.get(address, savedPassword);
			// finished new password
			// * make a sound....
			// * Yello LED....
			turnLightOn(blue);
			turnLightOn(blank);
			// * Save new password in EEPROM....
			// * Reset new password object....
			resetObjects();
		}

	}
}

void playTone(int melody[7], int duration) {
	for (int index = 0; index < 7; ++index) {
		tone(12, melody[index], 1000 / duration);
		delay(1000 / duration*1.3);
		noTone(8);
	}
}

void resetObjects() {
	turnLightOn(blank);
	startNewPassword = none;
	startEnterPassword = none;
	passwordIndex = 0;
	isPasswordValid = true;
}
void turnLightOn(ledColor color) {
	int delayMS = 500;
	switch (color) {
	case green:
		digitalWrite(RED_PIN, LOW);
		digitalWrite(BLUE_PIN, LOW);
		digitalWrite(GREEN_PIN, HIGH);
		delay(delayMS);
		break;
	case red:
		digitalWrite(BLUE_PIN, LOW);
		digitalWrite(GREEN_PIN, LOW);
		digitalWrite(RED_PIN, HIGH);
		delay(delayMS);
		break;
	case blue:
		digitalWrite(BLUE_PIN, HIGH);
		digitalWrite(RED_PIN, LOW);
		digitalWrite(GREEN_PIN, LOW);
		delay(delayMS);
		break;
	default:
		digitalWrite(BLUE_PIN, LOW);
		digitalWrite(RED_PIN, LOW);
		digitalWrite(GREEN_PIN, LOW);
		break;
	}
}
void keypadEvent(KeypadEvent key) {
	switch (kpd.getState()) {
	case RELEASED:
		if (key == '#') {
			if (startNewPassword == started) {
				newPasswordUpdateState();
			}
			else {
				enterPasswordUpdateState();
			}
		}
		else if (key == '*') {
			newPasswordUpdateState();
		}
		else if (startEnterPassword == started) {
			pressedKey = key;
			isValidPassword(key);
		}
		else if (startNewPassword == started) {
			updateNewPassword(key);
		}
		playKeyTone(key);
		break;
	default:
		pressedKey = '\0';
		break;
	}
}
void playKeyTone(KeypadEvent key) {
	int duration = 1000 / 32;
	int t;
	switch (key) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		t = key_tone[atoi(&key)];
		break;
	case 'A':
	case 'B':
	case 'C':
	case 'D':
		t = key_tone[atoi(&key) - 55];
		break;
	case '#':
		t = key_tone[14];
		break;
	case '*':
		t = key_tone[15];
		break;
	default:
		t = 0;
		break;
	}

	tone(12, t, duration);
	delay(duration*1.3);
	noTone(8);
}

void updateNewPassword(KeypadEvent key) {
	newPassword[passwordIndex] = key;
	++passwordIndex;
}
bool isValidPassword(KeypadEvent key) {
	bool isValid = false;
	if (key == '#') {
		isValid = savedPassword[passwordIndex] == '\0' && isPasswordValid;
		passwordIndex = 0;
	}
	else if (startEnterPassword == started) {

		isValid = savedPassword[passwordIndex] == key;
		++passwordIndex;
		isPasswordValid = isPasswordValid && isValid;
	}

	return isValid;
}

void enterPasswordUpdateState() {
	if (startEnterPassword == none || startEnterPassword == invalid) {
		startEnterPassword = started;
		passwordIndex = 0;
	}
	else if (startEnterPassword == started) {
		++passwordIndex;
		if (isValidPassword('#'))
			startEnterPassword = valid;
		else
			startEnterPassword = invalid;
	}

}

void newPasswordUpdateState() {
	if (startNewPassword == none) {
		startNewPassword = started;
		passwordIndex = 0;
	}
	else if (startNewPassword == started) {
		startNewPassword = finished;
		newPassword[passwordIndex] = '\0';
		passwordIndex = 0;
	}
}
