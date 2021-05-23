
#undef abs
#include "grid.h"
#include "dfsMaze.h"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino_LSM6DS3.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// 0 dsplay ortogonal to arduino
// [ARDUINO] _
//           D

// 1 display paralell to 
// 2
// 3
//const int orientation = 4;
const int orientation = 3;

const int width = 128;
const int height = 32;
const int cellWidth = 10;
const int columns = floor( width / cellWidth);
const int rows = floor(height / cellWidth);

int leftWall = 0;
int rightWall = 1;
int topWall = 2;
int bottomWall = 3;
std::pair <float, float> newAcceleration;

int ballwidth = 4;
int wallWidth = 2;
float ballx = 2.0;
float bally = 2.0;
float oldballx = ballx;
float oldbally = bally;
float velx = 0.0;
float vely = 0.0;
long oldtime = millis();
float velReduction = 0.4;

long lastTime = millis();

float xaccel = 0.0 ;
float yaccel = 0.0;

typedef struct  {
  int column;
  int row;
  bool walls[4];
  bool visited;
} t_cell;

using namespace huc;
using namespace maze;

t_cell cells[columns][rows];
void createCells() {
  for (int row = 0; row < rows; row++) {
    for (int column = 0; column < columns; column++) {
      cells[column][row] = {column, row, {true, true, true, true}, false};
    }
  }
}

void showCell(t_cell cell ) {
  int x = cell.column * cellWidth;
  int y = cell.row * cellWidth;
  if (cell.walls[leftWall] == true) {
    display.drawFastVLine(x, y, cellWidth , SSD1306_WHITE);
  }
  if (cell.walls[rightWall] == true) {
    display.drawFastVLine(x + cellWidth - 1 , y , cellWidth , SSD1306_WHITE);
  }
  if (cell.walls[topWall] == true) {
    display.drawFastHLine(x, y, cellWidth , SSD1306_WHITE);
  }
  if (cell.walls[bottomWall] == true) {
    display.drawFastHLine(x, y + cellWidth - 1  , cellWidth , SSD1306_WHITE);
  }
  display.fillRect(x + cellWidth / 2 - 1 , y + cellWidth / 2 - 1  , 2 , 2, SSD1306_WHITE);
}

void drawCells () {
  display.clearDisplay();
  for (int j = 0; j < rows; j++) {
    for (int i = 0; i < columns; i++) {
      showCell(cells[i][j]);
    }
  }
  display.display();
}

void removeWallsBetween(t_cell* current, t_cell* choosenCell) {
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
          removeWallsBetween( &cells[x][y],  &cells[point->x][point->y]);
        }
      }
    }
  }
  drawCells();
}

void setup() {
  Serial.begin(9600);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    for (;;); // Don't proceed, loop forever
  }
  if (!IMU.begin()) {
    while (1); // Don't proceed, loop forever
  }
  // Clear the buffer
  display.clearDisplay();
  createMaze();
}

bool rectCollision(int x1, int y1, int w1, int h1, int x2, int y2, int w2) {
  return (x1 < (x2 + w2)
          && (x1 + w1) > x2
          && y1 < (y2 + w2)
          && (y1 + h1) > y2);
}

bool cellCollisionXLeftWall(int x, int y, int cellx, int celly, int ballx, int bally) {
  return ((cells[cellx][celly].walls[leftWall] == true) && (rectCollision(x - 1, y, wallWidth, cellWidth, ballx, bally, ballwidth) ));
}

bool cellCollisionXTopWall(int x, int y, int cellx, int celly, int ballx, int bally) {
  return (((cells[cellx][celly].walls[topWall] == true) && (rectCollision(x, y - 1, cellWidth, wallWidth, ballx, bally, ballwidth) )));
}

bool cellCollisionXBottomWall(int x, int y, int cellx, int celly, int ballx, int bally) {
  return (((cells[cellx][celly].walls[bottomWall] == true) && rectCollision(x, y + cellWidth - 1, cellWidth, wallWidth, ballx, bally, ballwidth))) ;
}

bool cellCollisionXRightWall(int x, int y, int cellx, int celly, int ballx, int bally) {
  return  (((cells[cellx][celly].walls[rightWall] == true) && (rectCollision(x + cellWidth - 1, y, wallWidth, cellWidth, ballx, bally, ballwidth) )));
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
  return ((cells[cellx][celly].walls[topWall] == true) && (rectCollision(x, y - 1, cellWidth, wallWidth, ballx, bally, ballwidth) ));
}

bool cellCollisionYBottomWall(int x, int y, int cellx, int celly, int ballx, int bally) {
  return (((cells[cellx][celly].walls[bottomWall] == true) && rectCollision(x, y + cellWidth - 1, cellWidth, wallWidth, ballx, bally, ballwidth)));
}

bool cellCollisionYLeftWall(int x, int y, int cellx, int celly, int ballx, int bally) {
  return (((cells[cellx][celly].walls[leftWall] == true) && (rectCollision(x - 1, y, wallWidth, cellWidth, ballx, bally, ballwidth) )));
}

bool cellCollisionYRightWall(int x, int y, int cellx, int celly, int ballx, int bally) {
  return (((cells[cellx][celly].walls[rightWall] == true) && (rectCollision(x + cellWidth - 1, y, wallWidth, cellWidth, ballx, bally, ballwidth) )));
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
    } else if (orientation == 4) {
      acceleration.first = acy / 2;
      acceleration.second = acx / 2;
    } 
    else if (orientation == 3) {
      acceleration.first = acx / 2;
      acceleration.second = -acy / 2;
    }else {
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

void paint() {
  // clear old pixels
  if (((int)oldballx != (int)ballx) || ((int)oldbally != (int)bally)) {
    display.fillRoundRect(oldballx, oldbally, ballwidth, ballwidth, 2,  SSD1306_BLACK);
  }
  // erase cookie if any
  display.fillRect((((int)ballx) / cellWidth)*cellWidth + cellWidth / 2 - 2 , (((int)bally) / cellWidth)*cellWidth + cellWidth / 2  -2 , 4 , 4, SSD1306_BLACK);
 
  // paint ball
  display.fillRoundRect((int)(ballx), (int)(bally), ballwidth, ballwidth, 2, SSD1306_WHITE);

//  int eyexoffset = ((velx > 0) - (velx < 0));
//  int eyeyoffset = ((vely > 0) - (vely < 0));
//
//  display.fillRect((int)(ballx) + ballwidth /2 - 1+ eyexoffset , (int)(bally) + ballwidth /2 -1 + eyeyoffset, 2, 2,SSD1306_BLACK);

  oldballx = (int)ballx;
  oldbally = (int)bally;

  // paint score
  display.drawChar(120,0,'8',SSD1306_WHITE,SSD1306_BLACK,1);
  display.display();
}

void loop() {
  getInput(newAcceleration);
  update(newAcceleration);
  paint();
}
