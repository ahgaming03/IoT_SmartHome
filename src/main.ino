// #include <Arduino.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Keypad_I2C.h>

// I2C Address
#define I2CKeypad_ADDR 0x20 // Keypad I2C address
#define I2CLCD_ADDR 0x27    // LCD I2C address

#define servoPin 7    // servo pin
#define buttonPin 4   // the number of the pushbutton pin
#define ledPin 2      // the number of the LED pin
#define blinkLedPin 13 // the number of the LED pin for blink
#define ROW_NUM 4      // four rows
#define COLUMN_NUM 4   // four columns

#define waterSensorPin A2 // The pin that the water sensor is connected to
#define spinServoPin A3   // The pin that the servo is connected to

// Define some constants and variables
int lastButtonState;    // variable for reading the pushbutton status
int currentButtonState; // variable for reading the pushbutton status
int ledState = LOW;     // ledState used to set the LED
int blinkState = LOW;   // ledState used to set the LED for blink
int pos = 0;            // variable to store the servo position
int servoIncrement = 1; // increment for each step

int doorState = 0; // 0: close, 1: open
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
char change_pass_key = 'G'; // press "D" key for more than 3 second
char key = 0;               // variable to store the key
int keyState = 0;           // the current state of the output pin
int numKey = 4;
int numDelay = 3;

// Define characters matrix
char hexaKeys[ROW_NUM][COLUMN_NUM] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'},
};

// Define pins for every row of keypad
byte rowPins[ROW_NUM] = {0,1,2,3};
// Define pins for every column of keypad
byte columnPins[COLUMN_NUM] = {4,5,6,7};
// Create an instance for our keypad
Keypad_I2C I2C_Keypad(makeKeymap(hexaKeys), rowPins, columnPins, ROW_NUM, COLUMN_NUM, I2CKeypad_ADDR, PCF8574);

unsigned long currentMillis = 0;

unsigned long blinkPreviousMillis = 0;
const unsigned long blinkPeriod = 500;

unsigned long buttonPreviousMillis = 0;
const unsigned long buttonPeriod = 100;

unsigned long servoPreviousMillis = 0;
const unsigned long servoPeriod = 10;

unsigned long lastKeyPressTime = 0;
const unsigned long debounceDelay = 100;
unsigned long lastKeyHoldTime = 0;
const unsigned long holdDelay = 700;

unsigned long wrongPasswordTime = 0;
const unsigned long wrongPasswordDelay = 3000;
bool isShowingWrongPassword = false;

// Import LCD
LiquidCrystal_I2C lcd(I2CLCD_ADDR, 16, 2); // I2C address 0x27, 16 column and 2 rows
unsigned long LCDStartTime = 0;
const unsigned long LCDStartDelay = 3000;
bool isLCDStarted = false;

// Create an instance for servo motor
Servo servo;
Servo servoWater;

void ledBlink()
{
  if (currentMillis - blinkPreviousMillis >= blinkPeriod)
  {
    blinkPreviousMillis = currentMillis;
    if (blinkState == LOW)
      blinkState = HIGH;
    else
      blinkState = LOW;
    digitalWrite(blinkLedPin, blinkState);
  }
}

void startLCD()
{
  if (isLCDStarted && currentMillis - LCDStartTime >= LCDStartDelay)
  {
    lcd.clear();
    lcd.print(" <3 WELCOME <3 ");
    isLCDStarted = false;
    input_passwd = "";
    old_passwd = "";
    new_passwd = "";
    confirm_passwd = "";
    key = 0;
  }
}

void restartLCDStatus()
{
  if (!isLCDStarted)
  {
    LCDStartTime = currentMillis;
    isLCDStarted = true;
  }
}

char function_key(int n)
{
  char tempKey = I2C_Keypad.getKey();

  if (tempKey != NO_KEY && currentMillis - lastKeyPressTime >= debounceDelay)
  {
    lastKeyPressTime = currentMillis;
    if ((int)I2C_Keypad.getState() == PRESSED)
    {
      key = tempKey;
    }
  }

  if (tempKey == NO_KEY && (int)I2C_Keypad.getState() == HOLD)
  {
    if (currentMillis - lastKeyHoldTime > holdDelay)
    {
      lastKeyHoldTime = currentMillis;
      keyState++;
      keyState = constrain(keyState, 1, n);
    }
  }
  else if ((int)I2C_Keypad.getState() == RELEASED)
  {
    key += keyState;
    keyState = 0;
  }

  return key;
}

void input_password(int n, String &input)
{
  if (input.length() == 0)
  {
    lcd.setCursor(0, 1);
    lcd.print("     ");
  }
  if (input.length() < n)
  {
    char temp = I2C_Keypad.getKey();
    if (temp != NO_KEY)
    {
      input += temp;
    }
    if (!isLCDStarted)
    {
      lcd.setCursor(0, 1);
      for (int i = 0; i < input.length(); i++)
      {
        lcd.print("*");
      }
    }
    Serial.println("Input: " + input);
  }
}

void change_password(int num_char, String current_password)
{
  // Authenticate the old password
  if (old_passwd.length() < num_char)
  {
    Serial.println("Old password: ");
    lcd.setCursor(0, 0);
    lcd.print("Old password    ");
    input_password(num_char, old_passwd);
  }
  if (old_passwd.length() == num_char)
  {
    if (old_passwd != current_password)
    {
      Serial.println("Password does not match! Nothing changes");
      lcd.setCursor(0, 1);
      lcd.print("Pass not match!");
      key = 0;
      restartLCDStatus();
    }
    else
    {
      if (new_passwd.length() < num_char)
      {
        Serial.println("New password: ");
        lcd.setCursor(0, 0);
        lcd.print("New password    ");
        input_password(num_char, new_passwd);
      }
      else if (confirm_passwd.length() < num_char)
      {
        Serial.println("Confirm password: ");
        lcd.setCursor(0, 0);
        lcd.print("Confirm password");
        input_password(num_char, confirm_passwd);
      }
      else
      {
        if (new_passwd == confirm_passwd)
        {
          default_passwd = confirm_passwd;
          Serial.println("Password has changed!!!");
          lcd.setCursor(0, 1);
          lcd.print("Password has    ");
          lcd.setCursor(0, 1);
          lcd.print("changed !!!");
          key = 0;
          restartLCDStatus();
        }
        else
        {
          Serial.println("Password does not match! Nothing changes");
          lcd.setCursor(0, 1);
          lcd.print("Pass not match!");
          key = 0;
          restartLCDStatus();
        }
      }
    }
  }
}

void unlock()
{
  lcd.setCursor(0, 0);
  lcd.print("Input password");
  input_password(numKey, input_passwd);
  if (input_passwd.length() == numKey)
  {
    if (input_passwd == default_passwd)
    {
      restartLCDStatus();
      doorOpen();
    }
    else
    {
      Serial.println("Wrong password");
      lcd.setCursor(0, 1);
      lcd.print("Wrong password");
      restartLCDStatus();
    }
  }
}

void lock()
{
  restartLCDStatus();
  doorClose();
}

void digitalDoor()
{
  key = function_key(numDelay);
  if (key == unlock_key)
  {
    unlock();
  }
  if (key == change_pass_key)
  {
    change_password(numKey, default_passwd);
  }
  if (key == lock_key)
  {
    lock();
  }
}

void doorButton(int button)
{
  if (currentMillis - buttonPreviousMillis >= buttonPeriod)
  {
    buttonPreviousMillis = currentMillis;

    lastButtonState = currentButtonState;     // save the last state
    currentButtonState = digitalRead(button); // read new state
    if (lastButtonState == HIGH && currentButtonState == LOW)
    {
      if (ledState == LOW)
      {
        restartLCDStatus();
        doorOpen();
      }
      else if (ledState == HIGH)
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
      if (pos > 0)
      {
        pos -= servoIncrement;
      }
    }
    else if (doorState == 1)
    {
      if (pos < 90)
      {
        pos += servoIncrement;
      }
    }
    servo.write(pos);
  }
}

void doorOpen()
{
  Serial.println("Open the door");

  lcd.setCursor(0, 1);
  lcd.print("Door is opened");

  ledState = HIGH;
  digitalWrite(ledPin, ledState);
  doorState = 1;
}

void doorClose()
{
  Serial.println("Close the door");

  lcd.setCursor(0, 1);
  lcd.print("Door is closed");

  ledState = LOW;
  digitalWrite(ledPin, ledState);
  doorState = 0;
}

int statusWaterSensor = 0; // 0: no water, 1: water detected

void spinServoClothesline()
{
  for (int i = 0; i <= 180; i += 1)
  {                 // goes from 0 degrees to 180 degrees
    servo.write(i); // Set the servo position
    // delay(15);  // Wait 15 milliseconds for the servo to reach the position
  }
}

void waterSensor()
{
  if (statusWaterSensor == 1)
  {
  }
  if (statusWaterSensor == 0 && digitalRead(waterSensorPin) == HIGH)
  {
    statusWaterSensor = 1;
    Serial.println("Water detected");
    spinServoClothesline();
    Serial.println("Clothesline done");
  }
}

void setup()
{
  Wire.begin();
  I2C_Keypad.begin(makeKeymap(hexaKeys));
  Serial.begin(9600);
  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);
  pinMode(blinkLedPin, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT);

  // servo
  servo.attach(servoPin);
  servo.write(pos);

  digitalWrite(ledPin, ledState);
  currentButtonState = digitalRead(buttonPin);

  lcd.init(); // initialize the lcd
  lcd.backlight();
  lcd.clear();
  lcd.print(" <3 Welcome <3 ");

  // set the water sensor pin as input
  pinMode(waterSensorPin, INPUT);
  // set the servo pin as output
  servoWater.attach(spinServoPin);
}

void loop()
{
  currentMillis = millis();
  ledBlink();
  startLCD();
  doorButton(buttonPin);
  digitalDoor();
  doorOpenClose();
  waterSensor();
}