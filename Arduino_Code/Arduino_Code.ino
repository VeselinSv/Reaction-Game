#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4); // смени на 0x3F ако не работи

const int redPin = 9;
const int greenPin = 10;

bool waitingForStart = false;
bool waitingForReaction = false;
bool gameActive = false;
bool redLedSequenceCompleted = false;

unsigned long nextRedLedTime = 0;
unsigned long nextGreenLedTime = 0;
unsigned long reactionStartTime = 0;
unsigned long gameRestartTime = 0;

int redLedCount = 0;
int greenLedCount = 0;
bool redLedState = false;
bool greenLedState = false;
bool canRestart = true;

void setup() {
  Serial.begin(9600);

  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);

  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connect...");

  // RGB тест – червено и зелено
  for (int i = 0; i < 2; i++) {
    digitalWrite(redPin, HIGH);
    delay(200);
    digitalWrite(redPin, LOW);
    delay(200);
    digitalWrite(greenPin, HIGH);
    delay(200);
    digitalWrite(greenPin, LOW);
    delay(200);
  }

  Serial.println("Arduino готов - RGB LED тестван");
}

void resetGame() {
  waitingForStart = false;
  waitingForReaction = false;
  gameActive = false;
  redLedSequenceCompleted = false;

  nextRedLedTime = 0;
  nextGreenLedTime = 0;
  reactionStartTime = 0;

  redLedCount = 0;
  greenLedCount = 0;
  redLedState = false;
  greenLedState = false;

  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, LOW);
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "NEW_GAME" && canRestart) {
      resetGame();
      gameActive = true;
      waitingForStart = true;
      redLedCount = 0;

      nextRedLedTime = millis() + 500;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Get ready...");
      Serial.println("WAITING");
    }

    if (cmd == "BUTTON_PRESSED") {
      if (gameActive && waitingForStart && !redLedSequenceCompleted) {
        Serial.println("TOO_EARLY");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Too early!");
        lcd.setCursor(0, 1);
        lcd.print("Start again");

        resetGame();
        canRestart = false;
        gameRestartTime = millis() + 3000;

      } else if (gameActive && waitingForReaction && redLedSequenceCompleted) {
        unsigned long reactionTime = millis() - reactionStartTime;
        Serial.print("TIME:");
        Serial.println(reactionTime);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Time:");
        lcd.setCursor(0, 1);
        lcd.print(reactionTime);
        lcd.print(" ms");

        resetGame();
      }
    }
  }

  // Червено мигане
  if (waitingForStart && gameActive && !redLedSequenceCompleted) {
    if (millis() >= nextRedLedTime) {
      if (!redLedState && redLedCount < 5) {
        digitalWrite(redPin, HIGH);
        redLedState = true;
        nextRedLedTime = millis() + 200;
        Serial.print("RED LED ON - count: ");
        Serial.println(redLedCount + 1);
      } else if (redLedState) {
        digitalWrite(redPin, LOW);
        redLedState = false;
        redLedCount++;

        if (redLedCount < 5) {
          nextRedLedTime = millis() + random(500, 1500);
          Serial.print("RED LED OFF - waiting: ");
          Serial.println(nextRedLedTime - millis());
        } else {
          redLedSequenceCompleted = true;
          waitingForStart = false;
          waitingForReaction = true;

          greenLedCount = 0;
          nextGreenLedTime = millis() + random(200, 800);

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Attention!");
          Serial.println("Red sequence completed, starting green");
        }
      }
    }
  }

  // Зелено мигане
  if (waitingForReaction && redLedSequenceCompleted && gameActive) {
    if (millis() >= nextGreenLedTime) {
      if (!greenLedState && greenLedCount < 3) {
        digitalWrite(greenPin, HIGH);
        greenLedState = true;
        nextGreenLedTime = millis() + 150;

        if (greenLedCount == 0) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Now!");
          Serial.println("STARTED");
          Serial.println("GREEN LED ON - FIRST");

          // Реално стартиране на реакционния таймер
          reactionStartTime = millis();
        } else {
          Serial.print("GREEN LED ON - count: ");
          Serial.println(greenLedCount + 1);
        }

      } else if (greenLedState) {
        digitalWrite(greenPin, LOW);
        greenLedState = false;
        greenLedCount++;

        Serial.print("GREEN LED OFF - count: ");
        Serial.println(greenLedCount);

        if (greenLedCount < 3) {
          nextGreenLedTime = millis() + random(100, 400);
        } else {
          Serial.println("Green sequence completed");
        }
      }
    }
  }

  // Автоматичен рестарт
  if (!canRestart && millis() >= gameRestartTime) {
    canRestart = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Automatic");
    lcd.setCursor(0, 1);
    lcd.print("restart...");

    delay(1000);
    resetGame();

    gameActive = true;
    waitingForStart = true;
    redLedCount = 0;
    nextRedLedTime = millis() + 500;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Get ready...");
    Serial.println("WAITING");
  }
}
