/**
   Testing Casement Window and Door Orientation with Arduino nano 33 IoT LSM6DS3 IMU and Madgwick filter
  Processing 3D Visualizer
   @author Enrique Albertos
   @link https://www.element14.com/community/community/design-challenges/design-for-a-cause-2021/
*/
import processing.serial.*;
Serial myPort;
 float yaw = 0.0;
 void setup()
{
  size(1200, 1000, P3D);
  myPort = new Serial(this, "COM4", 9600);  
  textSize(16); // set text size
  textMode(SHAPE); // set text mode to shape
}

void draw()
{
  serialEvent();  // read and parse incoming serial message
  background(255); // set background to white
  lights();
   translate(width/2, height/2); // set position to centre
   pushMatrix(); // begin object
   float c1 = cos(radians(0));
  float s1 = sin(radians(0));
  float c2 = cos(radians(0));
  float s2 = sin(radians(0));
  float c3 = cos(radians(yaw));
  float s3 = sin(radians(yaw));
  applyMatrix( c2*c3, s1*s3+c1*c3*s2, c3*s1*s2-c1*s3, 0,
               -s2, c1*c2, c2*s1, 0,
               c2*s3, c1*s2*s3-c3*s1, c1*c3+s1*s2*s3, 0,
               0, 0, 0, 1);

  drawCasementWindow();
   popMatrix(); // end of object
   // Print values to console
  print(yaw);
  println();
}

void serialEvent()
{
  int newLine = 13; // new line character in ASCII
  String message;
  do {
    message = myPort.readStringUntil(newLine); // read from port until new line
    if (message != null) {
      String[] list = split(trim(message), " ");
      if (list.length >= 2 && list[0].equals("Orientation:")) {
        yaw = float(list[1]); // convert to float yaw
      }
    }
  } while (message != null);
}

void drawCasementWindow()
{
  translate(0, 0, 100); 
  stroke(255, 255, 255);
  fill(250, 250, 250); 
  box(10, 300, 200); 
  stroke(90, 90, 0);
  fill(180, 180, 0); 
  translate(0, 0, -100); 
  box(10,300,20);
  stroke(90, 90, 0);
  fill(180, 180, 0);
  translate(0, 0, 200); 
  box(10,300,20); 
  stroke(90, 90, 0);
  fill(180, 180, 0);
  translate(0, 150, -100); 
  box(10,20,200); 
  stroke(90, 90, 0);
  fill(180, 180, 0);
  translate(0, -300, 0); 
  box(10,20,200);  
}
