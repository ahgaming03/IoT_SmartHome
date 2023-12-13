#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad_I2C.h>

// I2C Address
#define I2C_Keypad_ADDR 0x20  // Keypad I2C address
#define I2C_Display_ADDR 0x27 // LCD I2C address

// Define blink led pin
#define blinkLedPin 13 // the number of the LED pin for blink

// Define pins for feature "Digital Door"
#define servoPin 10   // servo pin
#define btnDoor 11    // the number of the pushbutton pin
#define doorLedPin 12 // the number of the LED pin
#define ROW_NUM 4     // four rows
#define COLUMN_NUM 3  // four columns

// Define pins for feature "Fire Alarm"
#define flamePin 2   // The pin that the flame sensor is connected to
#define btnStop 3    // the number of the pushbutton pin
#define buzzerPin 4  // the number of the buzzer pin
#define fireLedPin 5 // the number of the LED pin

// Define pins for feature "Smart clothesline"
#define RAIN_SENSOR_PIN A0 // The pin that the water sensor is connected to
#define RAIN_SERVO_PIN 8   // The pin that the servo is connected to

// Define some constants and variables
int blinkState = LOW; // ledState used to set the LED for blink

int lastButtonState;    // variable for reading the pushbutton status
int currentButtonState; // variable for reading the pushbutton status
int ledState = LOW;     // ledState used to set the LED
int positionDoor = 0;   // variable to store the servo position
int servoIncrement = 1; // increment for each step
int doorState = 0;      // 0: close, 1: open

// Default password is 0000
String default_passwd = "0000";
// variable to store the user input for password
String input_passwd = "";
String old_passwd = "";
String new_passwd = "";
String confirm_passwd = "";

// Define keys for lock and unlock or change password function
char lock_key = '*';
char unlock_key = '#';
int numKey = 4;
int numDelay = 3;
// Define characters matrix
char hexaKeys[ROW_NUM][COLUMN_NUM] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'},
};
// Define pins for every row of keypad
byte rowPins[ROW_NUM] = {0, 1, 2, 3};
// Define pins for every column of keypad
byte columnPins[COLUMN_NUM] = {4, 5, 6};
// Create an instance for our keypad
Keypad_I2C I2C_Keypad(makeKeymap(hexaKeys), rowPins, columnPins, ROW_NUM, COLUMN_NUM, I2C_Keypad_ADDR, PCF8574);

int flameState = LOW; // flameState used to set the LED

unsigned long currentMillis = 0;

unsigned long blinkPreviousMillis = 0;
const unsigned long blinkPeriod = 500;

// Define variables for feature "Digital Door"
unsigned long buttonPreviousMillis = 0;
const unsigned long buttonPeriod = 100;
unsigned long servoPreviousMillis = 0;
const unsigned long servoPeriod = 10;
unsigned long lastKeyPressTime = 0;
const unsigned long debounceDelay = 100;
unsigned long lastKeyHoldTime = 0;
const unsigned long holdDelay = 700;
bool isUnlock = false;
bool isLock = false;
bool isChangePasswd = false;

// Import LCD
LiquidCrystal_I2C lcd(I2C_Display_ADDR, 16, 2); // I2C address 0x27, 16 column and 2 rows
unsigned long LCDStartTime = 0;
const unsigned long LCDStartDelay = 3000;
bool isLCDStarted = false;

// Create an instance for servo motor
Servo servoDoor;
Servo servoRain;

/* project life */
void ledBlink()
{
  if (currentMillis - blinkPreviousMillis >= blinkPeriod)
  {
    blinkPreviousMillis = currentMillis;
    blinkState = !blinkState;
    digitalWrite(blinkLedPin, blinkState);
  }
}

/* Smart door */
void startLCD()
{
  if (isLCDStarted && currentMillis - LCDStartTime >= LCDStartDelay)
  {
    lcd.clear();
    lcd.print(" <3 WELCOME <3 ");
    isLCDStarted = false;
  }
}

void restartLCDStatus()
{
  input_passwd = "";
  old_passwd = "";
  new_passwd = "";
  confirm_passwd = "";

  LCDStartTime = currentMillis;
  isLCDStarted = true;
}

void keypadEvent(KeypadEvent key)
{
  switch (I2C_Keypad.getState())
  {
  case PRESSED:
    switch (key)
    {
    case '#':
      isLock = false;
      isUnlock = true;
      break;
    case '*':
      isUnlock = false;
      isLock = true;
      break;
    }
    break;
  case HOLD:
    switch (key)
    {
    case '*':
      isChangePasswd = true;
      isUnlock = false;
      isLock = false;
      break;
    }
    break;
  }
}

void input_password(int n, String &input)
{
  if (input.length() == 0)
  {
    lcd.setCursor(0, 1);
    lcd.print("                ");
  }
  if (input.length() < n)
  {
    char temp = I2C_Keypad.getKey();
    if (temp)
    {
      input += temp;
      if (!isLCDStarted)
      {
        Serial.println(input);

        lcd.setCursor(0, 1);
        for (int i = 0; i < input.length(); i++)
        {
          lcd.print("*");
        }
      }
    }
  }
}

void change_password(int num_char, String current_password)
{
  // Authenticate the old password
  if (old_passwd.length() < num_char)
  {
    lcd.setCursor(0, 0);
    lcd.print("Old password    ");
    input_password(num_char, old_passwd);
  }
  else if (old_passwd != current_password)
  {
    Serial.println("Password does not match! Nothing changes");
    lcd.clear();
    lcd.print("Pass not match!");
    restartLCDStatus();
    resetDigitalDoor();
  }
  else
  {
    if (new_passwd.length() < num_char)
    {
      lcd.setCursor(0, 0);
      lcd.print("New password    ");
      input_password(num_char, new_passwd);
    }
    else if (confirm_passwd.length() < num_char)
    {
      lcd.setCursor(0, 0);
      lcd.print("Confirm password");
      input_password(num_char, confirm_passwd);
    }
    else if (new_passwd != confirm_passwd)
    {
      Serial.println("Password does not match! Nothing changes");
      lcd.clear();
      lcd.print("Pass not match!");
      restartLCDStatus();
      resetDigitalDoor();
    }
    else
    {
      default_passwd = confirm_passwd;
      Serial.println("Password has changed!!!");
      lcd.clear();
      lcd.print("Password has    ");
      lcd.setCursor(0, 1);
      lcd.print("changed !!!");
      restartLCDStatus();
      resetDigitalDoor();
    }
  }
}

void unlock()
{
  if (doorState == 0)
  {
    lcd.setCursor(0, 0);
    lcd.print("Input password");
    input_password(numKey, input_passwd);
    if (input_passwd.length() == numKey)
    {
      if (input_passwd == default_passwd)
      {
        doorOpen();
      }
      else
      {
        Serial.println("Wrong password");
        lcd.clear();
        lcd.print("Wrong password");
        resetDigitalDoor();
        restartLCDStatus();
      }
    }
  }
}

void lock()
{
  if (doorState == 1)
  {
    doorClose();
  }
}

void resetDigitalDoor()
{
  isUnlock = false;
  isLock = false;
  isChangePasswd = false;
}

void digitalDoor()
{
  I2C_Keypad.getKey();
  if (!digitalRead(btnDoor))
  {
    if (isUnlock)
    {
      unlock();
    }
    if (isLock)
    {
      lock();
    }
    if (isChangePasswd)
    {
      change_password(numKey, default_passwd);
    }
  }
}

void doorButton()
{
  if (currentMillis - buttonPreviousMillis >= buttonPeriod)
  {
    buttonPreviousMillis = currentMillis;

    lastButtonState = currentButtonState;      // save the last state
    currentButtonState = digitalRead(btnDoor); // read new state
    if (lastButtonState == HIGH && currentButtonState == LOW)
    {
      if (doorState == 0)
      {
        restartLCDStatus();
        doorOpen();
      }
      else if (doorState == 1)
      {
        restartLCDStatus();
        doorClose();
      }
    }
  }
}

void doorOpenClose()
{
  if (currentMillis - servoPreviousMillis >= servoPeriod)
  {
    servoPreviousMillis = currentMillis;
    if (doorState == 0)
    {
      if (positionDoor > 0)
      {
        positionDoor -= servoIncrement;
      }
    }
    else if (doorState == 1)
    {
      if (positionDoor < 90)
      {
        positionDoor += servoIncrement;
      }
    }
    servoDoor.write(positionDoor);
  }
}

void doorOpen()
{
  resetDigitalDoor();
  restartLCDStatus();
  Serial.println("Open the door");

  lcd.clear();
  lcd.print("Door is opened");

  doorState = 1;
  ledState = HIGH;
  digitalWrite(doorLedPin, ledState);
}

void doorClose()
{
  resetDigitalDoor();
  restartLCDStatus();
  Serial.println("Close the door");

  lcd.clear();
  lcd.print("Door is closed");

  doorState = 0;
  ledState = LOW;
  digitalWrite(doorLedPin, ledState);
}

/* Fire alarm */
// Define variables for feature "Fire Alarm"
bool isFire = false;
unsigned long lastFireTime = 0;
const unsigned long fireDelay = 1000;

void fireAlarm()
{
  if (digitalRead(flamePin) == LOW)
  {
    isFire = true;
  }
  else if (digitalRead(btnStop) == HIGH)
  {
    isFire = false;
  }
  if (isFire)
  {
    if (currentMillis - lastFireTime >= fireDelay)
    {
      lastFireTime = currentMillis;
      flameState = !flameState;
      if (flameState == HIGH)
      {
        digitalWrite(fireLedPin, flameState);
        tone(buzzerPin, 400);
      }
      else if (flameState == LOW)
      {
        digitalWrite(fireLedPin, flameState);
        tone(buzzerPin, 200);
      }
    }
    Serial.println("Fire detected");
  }
  else
  {
    digitalWrite(fireLedPin, LOW);
    noTone(buzzerPin);
  }
}

/* Smart clothesline*/
int rainValue = 0;
bool isRain = false;

unsigned long RainTime = 0;

unsigned long previousRainTime = 0;
const unsigned long rainDelay = 1000;

unsigned long previousServoTime = 0;
const unsigned long servoRainDelay = 10000;

void smartClothesline()
{
  rainValue = analogRead(RAIN_SENSOR_PIN);
  if (currentMillis - RainTime >= 1000)
  {
    RainTime = currentMillis;
    Serial.print("Rain value: ");
    Serial.println(rainValue);
  }
  if (rainValue > 400)
  {
    previousRainTime = currentMillis;
  }

  if (!isRain && currentMillis - previousRainTime >= rainDelay)
  {

    Serial.print("Raining: ");
    Serial.println(rainValue);
    isRain = true;
    previousServoTime = currentMillis;
  }

  if (isRain && currentMillis - previousServoTime < servoRainDelay)
  {
    servoRain.write(85);
  }
  else
  {
    servoRain.write(90);
    isRain = false;
  }
}



void setup()
{
  Serial.begin(9600);

  /* Project life */
  // initialize the LED pin as an output:
  pinMode(blinkLedPin, OUTPUT);

  /* Smart door */
  // initialize the keypad:
  Wire.begin();
  I2C_Keypad.begin();
  I2C_Keypad.addEventListener(keypadEvent);
  // initialize the pushbutton pin as an input:
  pinMode(btnDoor, INPUT);
  // servo door
  servoDoor.attach(servoPin);
  servoDoor.write(positionDoor);
  // led door
  pinMode(doorLedPin, OUTPUT);
  digitalWrite(doorLedPin, ledState);
  currentButtonState = digitalRead(btnDoor);
  // initialize the lcd
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print(" <3 Welcome <3 ");

  /* fire alarm */
  pinMode(flamePin, INPUT);
  pinMode(btnStop, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(fireLedPin, OUTPUT);
  digitalWrite(fireLedPin, LOW);

  /* Smart clothesline */
  // set the water sensor pin as input
  pinMode(RAIN_SENSOR_PIN, INPUT);
  // set the servo rain pin as output
  servoRain.attach(RAIN_SERVO_PIN);
  servoRain.write(90);
}

void loop()
{
  currentMillis = millis();
  ledBlink();
  fireAlarm();
  smartClothesline();
  startLCD();
  doorButton();
  digitalDoor();
  doorOpenClose();
}
