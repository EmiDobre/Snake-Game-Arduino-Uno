#include <LedControl.h>

// Snake Struct
typedef struct Snake Snake;
struct Snake {
  int head[2];     // coordonate cap sarpe
  int body[40][2]; // vector de  coordonate (linie,coloana)
  int len;         // lungime
  int dir[2];      // directie sarpe
};

// Apple Struct
typedef struct Apple Apple;
struct Apple {
  int rPos; // linia
  int cPos; // coloana
};

// MAX72XX LED Matrix Pins
const int DIN = 12;
const int CS = 11;
const int CLK = 10;
LedControl lc = LedControl(DIN, CLK, CS, 1);

const int varXPin = A3;
const int varYPin = A4; 

byte pic[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // cele 8 linii din matrice led

// init oboecutl snake 
Snake snake = {{1, 5}, {{0, 5}, {1, 5}}, 2, {1, 0}};

// init cu pozitie random marul
Apple apple = {(int)random(0, 8), (int)random(0, 8)};

// var pentru game time
unsigned long oldTime = 0;
unsigned long timer = 0;
const float updateRate = 3;

// LED pins
const int redPin = 7;
const int greenPin = 6;
const int bluePin = 5;

// var pentru RGB LED
unsigned long colorChangeTimer = 0;
const unsigned long colorChangeInterval = 1000; //schimbare culoare la o secunda

bool gameOverFlag = false; // flag pentru a indica sfarsitul jocului

void setup() {
  Serial.begin(9600); // pt debugging

  // MAX72XX wakeup call
  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);

  // Joystick Pins - INPUT
  pinMode(varXPin, INPUT);
  pinMode(varYPin, INPUT);

  // LED pins - OUTPUT
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  // Seed random number generator
  randomSeed(analogRead(0));
}

void loop() {
  unsigned long currentTime = millis();
  float deltaTime = currentTime - oldTime;
  oldTime = currentTime;
  timer += deltaTime;

  //citire input
  int xVal = analogRead(varXPin);
  int yVal = analogRead(varYPin);

  // directie sarpe (linie/coloaana) in functie de joystick
  // linie 0 col 1 sau -1 sus/jos, coloana 0, linie 1 -1 dreapta/stanga
  if (xVal < 100 && snake.dir[1] == 0) {
    snake.dir[0] = 0;
    snake.dir[1] = -1;
  } else if (xVal > 920 && snake.dir[1] == 0) {
    snake.dir[0] = 0;
    snake.dir[1] = 1;
  } else if (yVal < 100 && snake.dir[0] == 0) {
    snake.dir[0] = -1;
    snake.dir[1] = 0;
  } else if (yVal > 920 && snake.dir[0] == 0) {
    snake.dir[0] = 1;
    snake.dir[1] = 0;
  }

  // sincronizare miscare in joc pentru iluzie de miscare continua
  if (timer > 2000 / updateRate) {
    timer = 0;
    Update();
  }

  //schimbare rgb la fiecare secunda
  if (!gameOverFlag && currentTime - colorChangeTimer > colorChangeInterval) {
    colorChangeTimer = currentTime;
    changeColor();
  }

  //afisare sarpe si mar
  Render();
}

void reset() {
  for (int j = 0; j < 8; j++) {
    pic[j] = 0;
  }
}

void Update() {
  reset(); // curatare led matrix
  
  int newHead[2] = {snake.head[0] + snake.dir[0], snake.head[1] + snake.dir[1]};

  // aparitie sarpe pe partea cealalta
  if (newHead[0] == 8) {
    newHead[0] = 0;
  } else if (newHead[0] == -1) {
    newHead[0] = 7;
  } else if (newHead[1] == 8) {
    newHead[1] = 0;
  } else if (newHead[1] == -1) {
    newHead[1] = 7;
  }

  //verificare pierdere joc: capul sarpelui atinge orice parte din corp
  for (int j = 0; j < snake.len; j++) {
    if (snake.body[j][0] == newHead[0] && snake.body[j][1] == newHead[1]) {
      Serial.println("Game over");
      gameOver();
      return;
    }
  }

  //verificare marire sarpe:
  if (newHead[0] == apple.rPos && newHead[1] == apple.cPos) {
    snake.len++;
    apple.rPos = (int)random(0, 8);
    apple.cPos = (int)random(0, 8);
    Serial.println("Sarpele mananca");
    eatAppleEffect();
  } else {
    snakeMove();
  }

  //adaugare la body noul cap si actualizare 'cap'
  snake.body[snake.len - 1][0] = newHead[0];
  snake.body[snake.len - 1][1] = newHead[1];
  snake.head[0] = newHead[0];
  snake.head[1] = newHead[1];

  //matricea de leduri updata cu pozitiile sarpelui
  for (int j = 0; j < 8; j++) {
    pic[j] = 0;
  }
  for (int j = 0; j < snake.len; j++) {
    pic[snake.body[j][0]] |= (128 >> snake.body[j][1]);
  }
  pic[apple.rPos] |= (128 >> apple.cPos);

  //debug info
  Serial.print("Snake lungime: ");
  Serial.println(snake.len);
  Serial.print("Snake Head: ");
  Serial.print(snake.head[0]);
  Serial.print(", ");
  Serial.println(snake.head[1]);
  Serial.print("Apple Pos: ");
  Serial.print(apple.rPos);
  Serial.print(", ");
  Serial.println(apple.cPos);

  //corp sarpe verificare:
  Serial.println("Snake Body:");
  for (int i = 0; i < snake.len; i++) {
    Serial.print("Segment ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(snake.body[i][0]);
    Serial.print(", ");
    Serial.println(snake.body[i][1]);
  }
}

void Render() {
  for (int i = 0; i < 8; i++) {
    lc.setRow(0, i, pic[i]);
  }
  Serial.println("Display");
}

void snakeMove() {
  for (int j = 1; j < snake.len; j++) {
    snake.body[j - 1][0] = snake.body[j][0];
    snake.body[j - 1][1] = snake.body[j][1];
  }
}

void changeColor() {
  //Culori (excluzand rosu si verde):
  static int colors[][3] = {
    {0, 0, 255},
    {255, 255, 0},
    {0, 255, 255},
    {255, 0, 255},  
    {255, 165, 0},  
    {128, 0, 128}  
  };

  static int colorIndex = 0;

  analogWrite(redPin, colors[colorIndex][0]);
  analogWrite(greenPin, colors[colorIndex][1]);
  analogWrite(bluePin, colors[colorIndex][2]);

  //culoare next
  colorIndex = (colorIndex + 1) % (sizeof(colors) / sizeof(colors[0]));
}

void eatAppleEffect() {
  for (int i = 0; i < 5; i++) { // Blink green 5 ori
    setColor(0, 255, 0); // Verde
    delay(100);
    setColor(0, 0, 0); // Off
    delay(100);
  }
}

void gameOver() {
  gameOverFlag = true;
  setColor(255, 0, 0); // Rosu
  Serial.println("Game Over!");
}

void setColor(int red, int green, int blue) {
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
}

