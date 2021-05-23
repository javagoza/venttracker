
#undef abs
#include "grid.h"
#include "dfsMaze.h"
using namespace huc;
using namespace maze;

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Arduino_LSM6DS3.h>
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 160 // OLED display height, in pixels
#define TFT_CS        10
#define TFT_RST        -1 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         8


Adafruit_ST7735 display(TFT_CS, TFT_DC, TFT_RST);

#define ST77XX_ARDUINO_TEAL 0x0431
#define ST77XX_ARDUINO_LIGHT_TEAL 0x7576
#define ST77XX_ARDUINO_ORANGE 0xD3A7
#define ST77XX_ARDUINO_YELLOW 0xDD68
#define ST77XX_ARDUINO_BROWN 0x8BCC





uint16_t getWallColor() {
  return ST77XX_ARDUINO_LIGHT_TEAL;
}

uint16_t getBackgroundColor(){
  return 0x386C;
}

uint16_t getBallColor() {
  return ST77XX_ARDUINO_YELLOW;
}

uint16_t getCookieColor() {
  return ST77XX_RED;
}



// 0 dsplay ortogonal to arduino
// [ARDUINO] _
//           D

// 1 display paralell to 
// 2
// 3
//const int orientation = 3;
const int orientation = 5;

const int width = 128;
const int height = 128;

const int cellWidth = 20;
int cellsPerColumn = (int) (width / cellWidth);
int cellsNumber = cellsPerColumn * cellsPerColumn;

int offsetx = (SCREEN_WIDTH - cellsPerColumn * cellWidth) /2;
int offsety = (SCREEN_HEIGHT - cellsPerColumn * cellWidth)/2 ;
const static int columns = floor( width / cellWidth);
const static int rows = floor(height / cellWidth);

int leftWall = 0;
int rightWall = 1;
int topWall = 2;
int bottomWall = 3;
std::pair <float, float> newAcceleration;

int ballwidth = 8;
int wallWidth = 4;
float ballx = 4.0;
float bally = 4.0;
float oldballx = ballx;
float oldbally = bally;
float velx = 0.0;
float vely = 0.0;
long oldtime = millis();
float velReduction = 0.4;

long lastTime = millis();

float xaccel = 0.0 ;
float yaccel = 0.0;

struct t_cell {
  int column;
  int row;
  bool walls[4];
  bool visited;
} ;

typedef struct t_cell t_cell; 

t_cell cells[12 * 12];

int score = 0;
int totalScore = 0;
int remainigTime = 100; 

// In global declarations:

GFXcanvas1 canvasBallOn(ballwidth, ballwidth); // ball on pixel canvas
GFXcanvas1 canvasScore(SCREEN_WIDTH-4, 14); // score canvas
GFXcanvas1 canvasTime(20, 14); // score canvas


void clearDisplay() {

 display.fillScreen(getBackgroundColor());
 

}

void displayDisplay() {

}



void createCells() {
  for (int row = 0; row < rows; row++) {
    for (int column = 0; column < columns; column++) {
      cells[column + row *columns] = {column, row, {true, true, true, true}, false};
    }
  }
}

void showCell(struct t_cell cell ) {
  int x = cell.column * cellWidth;
  int y = cell.row * cellWidth;
  if (cell.walls[leftWall] == true) {
    display.drawFastVLine(offsetx + x, offsety + y, cellWidth , getWallColor());
  }
  if (cell.walls[rightWall] == true) {
    display.drawFastVLine(offsetx + x + cellWidth - 1 , offsety +  y , cellWidth , getWallColor());
  }
  if (cell.walls[topWall] == true) {
    display.drawFastHLine(offsetx + x, offsety + y, cellWidth , getWallColor());
  }
  if (cell.walls[bottomWall] == true) {
    display.drawFastHLine(offsetx + x,offsety +  y + cellWidth - 1  , cellWidth , getWallColor());
  }
  display.fillRect(offsetx + x + cellWidth / 2 - 2 ,offsety +  y + cellWidth / 2 - 2  , 4 , 4, getCookieColor());
}

void drawCells () {
  clearDisplay();
  for (int j = 0; j < rows; j++) {
    for (int i = 0; i < columns; i++) {
      showCell(cells[i + j * columns]);
    }
  }
  displayDisplay();
}

void removeWallsBetween(struct t_cell* current, struct t_cell* choosenCell) {
  // remove walls bwtween choosen cell
  if (choosenCell->row > current->row) {
    // remove top wall
    current->walls[bottomWall] = false;
    choosenCell->walls[topWall] = false;
  } else if (choosenCell->row < current->row) {
    current->walls[topWall] = false;
    choosenCell->walls[bottomWall] = false;
  } else if (choosenCell->column > current->column) {
    // remove right wall
    current->walls[rightWall] = false;
    choosenCell->walls[leftWall] = false;
  } else if (choosenCell->column < current->column) {
    // remove left wall
    current->walls[leftWall] = false;
    choosenCell->walls[rightWall] = false;
  }
}

void createMaze() {
  auto maze = DFSGenerator()(columns, rows, DFSGenerator::Point(0, 0), millis());
  createCells();
  for (uint16_t x = 0 ; x < maze->Width(); x++) {
    for (uint16_t y = 0 ; y < maze->Height(); y++) {
      for (auto it = (*maze)[x][y]->connectedCells.begin(); it != (*maze)[x][y]->connectedCells.end(); ++it) {
        auto point = (*it).lock();
        if (point)
        {
          removeWallsBetween( &cells[x+y*columns],  &cells[point->x+point->y*columns]);
        }
      }
    }
  }
  drawCells();
}

void gameOver(){
    totalScore = 0;  
    score = 0;
  display.fillRect(10,SCREEN_HEIGHT/2-60,SCREEN_WIDTH-20, 60, ST77XX_BLACK);
  display.setTextSize(3);
  display.setTextColor(getBallColor());
  display.setCursor(16,SCREEN_HEIGHT/2-60);
  display.print(" GAME");
  display.setCursor(16,SCREEN_HEIGHT/2-30);
  display.print("OVER!");
  delay(3500);
  newGame();
}
void youWin(){
    totalScore = 0;  
  display.fillRect(10,SCREEN_HEIGHT/2-60,SCREEN_WIDTH-20, 60, ST77XX_BLACK);
  display.setTextSize(3);
  display.setTextColor(getBallColor());
  display.setCursor(16,SCREEN_HEIGHT/2-60);
  display.print(" YOU");
  display.setCursor(16,SCREEN_HEIGHT/2-30);
  display.print(" WIN!");
  delay(3500);
  newGame();
}

void newGame() {
  totalScore += score;  
  display.fillRect(10,SCREEN_HEIGHT/2-60,SCREEN_WIDTH-20, 60, ST77XX_BLACK);
  display.setTextSize(3);
  display.setTextColor(getBallColor());
  display.setCursor(16,SCREEN_HEIGHT/2-60);
  display.print(" GET");
  display.setCursor(16,SCREEN_HEIGHT/2-30);
  display.print("READY!");
  delay(3500);

  clearDisplay();

  createMaze();
  canvasScore.setTextWrap(false);
  canvasScore.setTextSize(1);
  canvasScore.setTextColor(ST77XX_WHITE);
  display.fillRect(2, 2, SCREEN_WIDTH-4, offsety - 4 , ST77XX_BLACK);
  display.fillRect(2, SCREEN_HEIGHT - offsety +2 , SCREEN_WIDTH-4, offsety - 4, ST77XX_BLACK);
  score = 0;

  ballx = 4.0;
  bally = 4.0;
  oldballx = ballx;
  oldbally = bally;
  velx = 0.0;
  vely = 0.0;
  remainigTime = 100; 

  paintScore();
  paintTime();
  
}

void setup() {
  Serial.begin(9600);
  delay(1000);
 Serial.println(F("Starting IMU"));
  if (!IMU.begin()) {
    while (1); // Don't proceed, loop forever
  }

  Serial.print(F("Hello! IMU LSM6DS3"));

      // Use this initializer if using a 1.8" TFT screen:
  display.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab
  
 Serial.println(F("Hello! ST77xx TFT Test"));
 canvasBallOn.fillCircle(ballwidth/2-1, ballwidth/2-1, ballwidth/2 -1,   getBallColor());
  
  
  newGame();

}



bool rectCollision(int x1, int y1, int w1, int h1, int x2, int y2, int w2) {
  return (x1 < (x2 + w2)
          && (x1 + w1) > x2
          && y1 < (y2 + w2)
          && (y1 + h1) > y2);
}

bool cellCollisionXLeftWall(int x, int y, int cellx, int celly, int ballx, int bally) {
  return ((cells[cellx+celly*columns].walls[leftWall] == true) && (rectCollision(x - 1, y, wallWidth, cellWidth, ballx, bally, ballwidth) ));
}

bool cellCollisionXTopWall(int x, int y, int cellx, int celly, int ballx, int bally) {
  return (((cells[cellx+celly*columns].walls[topWall] == true) && (rectCollision(x, y - 1, cellWidth, wallWidth, ballx, bally, ballwidth) )));
}

bool cellCollisionXBottomWall(int x, int y, int cellx, int celly, int ballx, int bally) {
  return (((cells[cellx+celly*columns].walls[bottomWall] == true) && rectCollision(x, y + cellWidth - 1, cellWidth, wallWidth, ballx, bally, ballwidth))) ;
}

bool cellCollisionXRightWall(int x, int y, int cellx, int celly, int ballx, int bally) {
  return  (((cells[cellx+celly*columns].walls[rightWall] == true) && (rectCollision(x + cellWidth - 1, y, wallWidth, cellWidth, ballx, bally, ballwidth) )));
}

int cellCollisionX(int cellx, int celly, int ballx, int bally) {
  int x = cellx * cellWidth;
  int y = celly * cellWidth;
  return  cellCollisionXLeftWall(x, y, cellx, celly, ballx, bally)
          || cellCollisionXRightWall(x, y, cellx, celly, ballx, bally)
          || cellCollisionXTopWall(x, y, cellx, celly, ballx, bally)
          || cellCollisionXBottomWall(x, y, cellx, celly, ballx, bally) ;
}

bool cellCollisionYTopWall(int x, int y, int cellx, int celly, int ballx, int bally) {
  return ((cells[cellx+celly*columns].walls[topWall] == true) && (rectCollision(x, y - 1, cellWidth, wallWidth, ballx, bally, ballwidth) ));
}

bool cellCollisionYBottomWall(int x, int y, int cellx, int celly, int ballx, int bally) {
  return (((cells[cellx+celly*columns].walls[bottomWall] == true) && rectCollision(x, y + cellWidth - 1, cellWidth, wallWidth, ballx, bally, ballwidth)));
}

bool cellCollisionYLeftWall(int x, int y, int cellx, int celly, int ballx, int bally) {
  return (((cells[cellx+celly*columns].walls[leftWall] == true) && (rectCollision(x - 1, y, wallWidth, cellWidth, ballx, bally, ballwidth) )));
}

bool cellCollisionYRightWall(int x, int y, int cellx, int celly, int ballx, int bally) {
  return (((cells[cellx+celly*columns].walls[rightWall] == true) && (rectCollision(x + cellWidth - 1, y, wallWidth, cellWidth, ballx, bally, ballwidth) )));
}

bool cellCollisionY(int cellx, int celly, int ballx, int bally) {
  int x = cellx * cellWidth;
  int y = celly * cellWidth;
  return cellCollisionYTopWall(x, y, cellx, celly, ballx, bally)
         || cellCollisionYBottomWall(x, y, cellx, celly, ballx, bally)
         || cellCollisionYLeftWall(x, y, cellx, celly, ballx, bally)
         || cellCollisionYRightWall(x, y, cellx, celly, ballx, bally);
}

bool checkFourCornersCollisionsY(int newx, int newy) {
  return cellCollisionY(newx / cellWidth, newy / cellWidth, newx, newy)
         || cellCollisionY((newx + ballwidth) / cellWidth, newy / cellWidth, newx, newy)
         || cellCollisionY(newx  / cellWidth,  (newy + ballwidth) / cellWidth, newx, newy)
         || cellCollisionY((newx + ballwidth ) / cellWidth, (newy + ballwidth) / cellWidth, newx, newy);
}

bool checkFourCornersCollisionsX(int newx, int newy) {
  return cellCollisionX( newx / cellWidth, newy / cellWidth, newx,  newy)
         || cellCollisionX(newx / cellWidth,  (newy + ballwidth) / cellWidth, newx,  newy)
         || cellCollisionX((newx + ballwidth) / cellWidth, newy / cellWidth, newx,  newy)
         || cellCollisionX( (newx + ballwidth ) / cellWidth, (newy + ballwidth) / cellWidth, newx, newy);
}


void getInput(std::pair <float, float> &acceleration) {
  float acz = 0;
  float acy = 0;
  float acx = 0;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(acx, acy, acz);
    if (orientation == 0) {
      acceleration.first = acy / 2;
      acceleration.second = -acx / 2;
    } else if (orientation == 1) {
      acceleration.first = -acx / 2;
      acceleration.second = acy / 2;
    } else if (orientation == 2) {
      acceleration.first = -acx / 2;
      acceleration.second = -acy / 2;
      
    } else if (orientation == 3) {
      acceleration.first = acx / 16;
      acceleration.second = -acy / 16;
    }else if (orientation == 4) {
      acceleration.first = acy / 2;
      acceleration.second = acx / 2;
        } else if (orientation == 5) {
      acceleration.first = acx / 16;
      acceleration.second = acy / 16;
    } 
    else {
      acceleration.first = - acy / 2;
      acceleration.second = -acx / 2;
    }
  }
}

void updateCollisions() {
  // check collisions in X direction
  if (!checkFourCornersCollisionsX(ballx + velx, bally)) {
    ballx += velx;
  } else { // collided then bounce ball in X direction
    velx = -velx * velReduction;
  }

  // check collisions in Y direction
  if (!checkFourCornersCollisionsY(ballx, bally + vely)) {
    // no obstacles
    bally += vely;
  } else  { // collided then bounce ball in Y direction
    vely = -vely * velReduction;
  }
}

void updateVelocity(std::pair <float, float> &acceleration) {
  velx +=  acceleration.first ;
  vely += acceleration.second ;

  // Keep velocity under control to keep collision detection simple
  if (abs(ballx + velx - oldballx) > ballwidth) {
    velx = ballwidth  * ((velx > 0) - (velx < 0));
  }
  if (abs(bally + vely - oldbally) > ballwidth) {
    vely = ballwidth * ((vely > 0) - (vely < 0));
  }
}

void update(std::pair <float, float> &acceleration) {
  updateVelocity(acceleration);
  updateCollisions();

}

void paintScore() {
    canvasScore.setTextColor(ST77XX_WHITE);
    canvasScore.fillRect(0, 0, SCREEN_WIDTH-2, 14, ST77XX_BLACK);
    canvasScore.setCursor(10,4);
    canvasScore.print("SCORE: ");
    canvasScore.print(totalScore);
    canvasScore.print("     ");
    if (score<10) {
      canvasScore.print("0");
    }
    canvasScore.print(score);
    canvasScore.print("");
    display.drawBitmap(2, 2, canvasScore.getBuffer(), SCREEN_WIDTH-4, 12, getBallColor(), ST77XX_BLACK); 
}

void paintTime() {
  static int counter =0;
  counter = (counter % 100);
  if(counter==0) {
    remainigTime--;    
    canvasTime.fillRect(0, 0,20, 14, ST77XX_BLACK);
    canvasTime.setTextColor(ST77XX_WHITE);
    canvasTime.setCursor(4,4);
    canvasTime.print(remainigTime);
    if(remainigTime<25) {
      display.drawBitmap(SCREEN_HEIGHT/2 -20, SCREEN_HEIGHT - 16, canvasTime.getBuffer(), 20, 14, getBallColor(), ST77XX_RED); 
    } else {
      display.drawBitmap(SCREEN_HEIGHT/2 -20, SCREEN_HEIGHT - 16, canvasTime.getBuffer(), 20, 14, getBallColor(), ST77XX_BLACK); 
    }
  }
  counter ++;
}

void paint() {

  // clear old pixels
  if (((int)oldballx != (int)ballx) || ((int)oldbally != (int)bally)) {
    //display.fillRoundRect(oldballx, oldbally, ballwidth, ballwidth, 4,  getBackgroundColor());
    display.drawBitmap(offsetx + (int)oldballx, offsety + (int)oldbally, canvasBallOn.getBuffer(), ballwidth, ballwidth, getBackgroundColor()); // Copy to screen
    //display.fillCircle(offsetx + (int)oldballx +ballwidth/2-1, offsety + (int)oldbally+ballwidth/2-1, ballwidth/2);
   
  }
  // erase cookie if any
  if ( !cells[(int)ballx / cellWidth + ((int)bally / cellWidth) * columns].visited) {
    display.fillRect(offsetx +(((int)ballx) / cellWidth)*cellWidth + cellWidth / 2 - 2 , offsety +(((int)bally) / cellWidth)*cellWidth + cellWidth / 2  -2 , 4 , 4,  getBackgroundColor());
    cells[(int)ballx / cellWidth + ((int)bally / cellWidth) * columns].visited = true;
    // update score
    //canvasScore.fillScreen(ST77XX_BLACK);
    ++score;
    paintScore();
    
  }
  // paint ball
  display.drawBitmap(offsetx + (int)ballx, offsety + (int)bally, canvasBallOn.getBuffer(), ballwidth, ballwidth, getBallColor()); // Copy to screen
  paintTime();
  

  oldballx = (int)ballx;
  oldbally = (int)bally;

  displayDisplay();
}

void loop() {
  getInput(newAcceleration);
  update(newAcceleration);
  paint();
  if (score >= cellsNumber) {
      youWin();
  } else if (remainigTime <= 0){
      gameOver();
  }
  delay(2);
}
