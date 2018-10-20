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
#define DEBOUNCING_TIME 200
#define PLAYER1 1
#define PLAYER2 2

int currIntensity;
int fadeAmount;
int leds[3] = { LED_L1, LED_L2, LED_L3 };

bool newGame;
bool gameStatus;
int current;
int winner;

unsigned long time;
unsigned int period;
int RT;
bool playerPressed;

void initGame();
void game(int verse);
void onPlayer1Press();
void onPlayer2Press();
void postGame(int shots);

void setup() {
  /* 
   * Inizializzazione delle variabili, del seme del random, della porta seriale e dei pin.
   */
  Serial.begin(9600);
  randomSeed(analogRead(0));
  currIntensity = 0;
  fadeAmount = 5;
  newGame = true;
  playerPressed = false;
  gameStatus = false;
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
  if (digitalRead(PIN_T3))
    initGame();
}

void initGame() {
  int speed = map(analogRead(PIN_POT), 0, 1023, MIN_SPEED, MAX_SPEED);
  /*
   * period e il tempo che la palla passera in T2, calcolato come inverso di speed
   * ed espresso in millisecondi. RT all'inizio e uguale a period.
   */
  period = 1000 / speed;
  RT = period;
  current = 1;
  analogWrite(LED_FLASH, 0);
  Serial.println("GO");
  digitalWrite(LED_L2, HIGH);
  delay(1000);
  digitalWrite(LED_L2, LOW);
  /*
   * Attach degli interrupt ai pin T1 e T2 e inizio della partita con verso iniziale casuale.
   */
  attachInterrupt(digitalPinToInterrupt(PIN_T1), onPlayer1Press, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_T2), onPlayer2Press, RISING);
  game(random(2) ? 1 : -1);
}

void game(int verse) {
  int shots = -1;
  gameStatus = true;
  while(gameStatus) {
    noInterrupts();
    playerPressed = false;
    digitalWrite(leds[current], LOW);
    current = current + verse;
    digitalWrite(leds[current], HIGH);
    interrupts();
    
    /*
     * time è il tempo in millisecondi in cui la pallina passa alla posizione successiva, 
     */
    time = millis();

    /*
     * Se la pallina è agli estremi il tempo di delay massimo deve essere RT,
     * viene effettuato il cambio del verso della pallina, 
     * viene diminuito il RT di un suo settimo.
     * Inoltre viene controllato se il giocatore di turno ha premuto il pulsante,
     * in caso negativo il giocatore di turno perde la partita.
     * Se la pallina di trova nella posizione in mezzo, il tempo di delay è il periodo.
     * In tutti i delay è stata inserita una clausola di controllo che controlla se 
     * un giocatore ha premuto il proprio pulsante, questo rende il sistema più reattivo
     * all'input utente in quanto consente di interrompere a metà un delay nel caso di pressioni.
     */
    if (current == 0 || current == 2) {
      while((millis() - time < RT) && !playerPressed);
      shots++;
      verse = -verse;
      noInterrupts();
      RT = RT * 7 / 8;
      /*
       * Se il giocatore di turno non ha premuto il pulsante entro il RT allora ha perso.
       */
      if (!playerPressed) {
        winner = current == 0 ? PLAYER2 : PLAYER1;
        gameStatus = false;
      }
      interrupts();
    } else
      while((millis() - time < period) && !playerPressed);
  }
  postGame(shots);
}

void onPlayer1Press() {
  static unsigned long prev = 0;
  unsigned long now = millis();

  /*
   * Controllo che evita che la funzione venga richiamata a partita non in corso
   * ed effettua il debouncing del pulsante.
   */
  if (gameStatus && (now - prev > DEBOUNCING_TIME)) {
    /* Se il giocatore preme il pulsante quando non è il proprio turno oppure
     * se non preme il pulsante entro il RT allora ha perso.
     */
    if (current != 0  || (now - time > RT)) {
      gameStatus = false;
      winner = PLAYER2;
    }
    playerPressed = true;
    prev = now;
  }
}

void onPlayer2Press() {
  static unsigned long prev = 0;
  unsigned long now = millis();
  
  if (gameStatus && (now - prev > DEBOUNCING_TIME)) {
    if (current != 2  || (now - time > RT)) {
      gameStatus = false;
      winner = PLAYER1;
    }
    playerPressed = true;
    prev = now;
  }
}

void postGame(int shots) {
  digitalWrite(leds[current], LOW);
  detachInterrupt(digitalPinToInterrupt(PIN_T1));
  detachInterrupt(digitalPinToInterrupt(PIN_T2));
  String gameOverMsg = "Game Over - The Winner is the Player ";
  gameOverMsg += winner;
  gameOverMsg += " after ";
  gameOverMsg += shots;
  gameOverMsg += " shots.";
  Serial.println(gameOverMsg);
  /*
   * Blink del led del vincitore per 2 secondi.
   */
  for (int i = 0; i < 8; i++) {
    digitalWrite(winner == PLAYER1 ? leds[0] : leds[2],
      i % 2 == 0 ? HIGH : LOW);
    delay(250);
  }
  /*
   * newGame a true in modo tale che loop() stampi il messaggio iniziale di nuovo.
   */
  newGame = true;
}
