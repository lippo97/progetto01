#include <Arduino.h>

#define LED_FLASH 9
#define LED_L1 10
#define LED_L2 11
#define LED_L3 12
#define PIN_T1 2
#define PIN_T2 3
#define PIN_T3 4
#define PIN_POT A0

#define MIN_SPEED 1
#define MAX_SPEED 3

#define WELCOME_MSG "Welcome to Led Pong. Press Key T3 to Start"
#define DEBOUNCING_TIME 300
#define PLAYER1 1
#define PLAYER2 2

int currIntensity;
int fadeAmount;
int leds[3] = { LED_L1, LED_L2, LED_L3 };

bool newGame;
int current;
bool gameStatus;
int winner;

unsigned long time;
int period;
int RT;
bool playerPressed;

void initGame();
void game(int verse, int speed);

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0)); /* pin 0 unconnected used as seed */
  currIntensity = 0;
  fadeAmount = 5;
  newGame = true;
  pinMode(LED_FLASH, OUTPUT);
  pinMode(LED_L1, OUTPUT);
  pinMode(LED_L2, OUTPUT);
  pinMode(LED_L3, OUTPUT);
  pinMode(PIN_T1, INPUT);
  pinMode(PIN_T2, INPUT);
  pinMode(PIN_T3, INPUT);
  pinMode(PIN_POT, INPUT);
}

void loop() {
  /*
    Qui il LED pulsa da 0 a 255.
  */
  if (newGame) {
    Serial.println(WELCOME_MSG);
    newGame = false;
  }
  analogWrite(LED_FLASH, currIntensity);   
  currIntensity = currIntensity + fadeAmount;
  if (currIntensity == 0 || currIntensity == 255) {
    fadeAmount = -fadeAmount; 
  }     
  delay(15);
  if (digitalRead(PIN_T3)) //TODO
    initGame();
}

void initGame() {
  int speed = map(analogRead(PIN_POT), 0, 1023, MIN_SPEED, MAX_SPEED);
  /*int speed = 1;*/
  period = 1000 / speed;
  RT = period;
  gameStatus = true;
  current = 1;
  analogWrite(LED_FLASH, 0);
  Serial.println("GO");
  digitalWrite(LED_L2, HIGH);
  delay(1000);
  digitalWrite(LED_L2, LOW);
  attachInterrupt(digitalPinToInterrupt(PIN_T1), onPlayer1Press, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_T2), onPlayer2Press, RISING);
  game(random(2) ? 1 : -1, speed);
}

void game(int verse, int speed) {
  int shots = -1;
  while(gameStatus) {
    noInterrupts();
    playerPressed = false;
    interrupts();
    digitalWrite(leds[current], LOW);
    current = current + verse;
    time = millis();
    digitalWrite(leds[current], HIGH);
    /*Serial.println("Inizio delay");*/
    delay(period);
    /*Serial.println("Fine delay");*/
    if (current == 0 || current == 2) {
      shots++;
      verse = -verse;
      noInterrupts();
      RT = RT * 7 / 8;
      if (!playerPressed) {
        winner = current == 0 ? PLAYER2 : PLAYER1;
        gameStatus = false;
      }
      interrupts();
    }
  }
  detachInterrupt(digitalPinToInterrupt(PIN_T1));
  detachInterrupt(digitalPinToInterrupt(PIN_T2));
  String gameOverMsg = "Game Over - The Winner is the Player ";
  gameOverMsg += winner;
  gameOverMsg += " after ";
  gameOverMsg += shots;
  gameOverMsg += " shots.";
  Serial.println(gameOverMsg);
  digitalWrite(leds[current], LOW);
}

void onPlayer1Press() {
  static unsigned long prev = 0;
  unsigned long now = millis();
  if (now - prev > DEBOUNCING_TIME) {
    Serial.println("onPlayer1Press");
    if (current != 0  || timeExpired(time, RT)) {
      gameStatus = false;
      /*Serial.println("onPlayer1Press - gameStatus -> false");*/
      winner = PLAYER2;
    } else {
      playerPressed = true;
      /*Serial.println("Player1 ha premuto");*/
    }
    prev = now;
  }
}

void onPlayer2Press() {
  static unsigned long prev = 0;
  unsigned long now = millis();
  if (now - prev > DEBOUNCING_TIME) {
    Serial.println("onPlayer2Press");
    if (current != 2  || timeExpired(time, RT)) {
      /*Serial.println("onPlayer2Press - gameStatus -> false");*/
      gameStatus = false;
      winner = PLAYER1;
    } else {
      playerPressed = true;
      /*Serial.println("Player2 ha premuto");*/
    }
    prev = now;
  }
}

bool timeExpired(unsigned long time, unsigned long timeLimit) {
  return (millis() - time) > timeLimit;
}
