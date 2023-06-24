#include <FastLED.h>
#include <EEPROM.h>

#define debug 0

#define MATRIX_PIN  8   // the pin at which the data input of the LED matrix is connected
#define MatrixWidth 16
#define MatrixHeight 16

#define NUM_LEDS (MatrixWidth * MatrixHeight) // 256 leds

#define BRIGHTNESS 64   //set brightness (maximum 255)

CRGB leds[NUM_LEDS];  // LED matrix

#define Idle_state 49
#define Delete_state 50
#define FireWork_state 51
#define Paint_state 52
#define Save_state 53
#define Load_state 54
#define animation_state 55

bool newMessage = false;


int state = Idle_state;  //state for the state machine

const int numChars = 1024;  //maximum number of characters
unsigned char receivedChars[1025];   // an array to store the received data
unsigned char image[1024];
int ndx = 0;  //current character 

void setup() {
  //Serial cummunication setup
  Serial1.begin(9600); // Default communication rate of the Bluetooth module
  #if(debug == 1)
    Serial.begin(19200);
  #endif

  //Setup of the LED matrix with the FastLED library
  FastLED.addLeds<NEOPIXEL, MATRIX_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
  
}
//Main loop function
void loop() 
  {
    int static counter = 0;
    int static imageNumber = 0;

    BLEhandler(); //Check if we are being sent a message via bluetooth

    //If the message starts with the string "MODE" it means the next character represents the next state for the state machine
    if(receivedChars[0] == 'M' && receivedChars[1] == 'O' && receivedChars[2] == 'D' && receivedChars[3] == 'E' && state != receivedChars[4] && newMessage == true)
      {
        state = receivedChars[4];
        #if(debug == 1)
          Serial.print("state = ");
          Serial.println((char)state);
        #endif
      }

      if (state == Delete_state) 
      {
        #if(debug == 1)
         Serial.println("deleted");
        #endif
        FastLED.clear();
        FastLED.show();
        state = Idle_state;
        receivedChars[4] = Idle_state;
      }
      else if (state == FireWork_state) 
      {
        #if(debug == 1)
          Serial.println("firework");
        #endif
        firework();
        
      } 
      else if(state == Paint_state)
      {
        if(receivedChars[965] == 63 && receivedChars[1021] == 'F' && receivedChars[1022] == 'I' && receivedChars[1023] == 'N')
        {
          for(int i = 0 ; i<1024 ; i++)
          {
            image[i] = receivedChars[i];
          }
          inputToMatrix(image);
          receivedChars[965] = '0';
          receivedChars[1023] = '0';
          ndx = 0;
          state = Idle_state;
          #if(debug == 1)
            Serial.println("saved");
          #endif
          
          
        }
      }
      else if(state == Save_state)
      {
        int slot = receivedChars[5];
        saveIntoEEPROM(slot*1024 , image , 1024);
        state = Idle_state;
        receivedChars[4] = Idle_state;
      }

      else if(state == Load_state)
      {
        int slot = receivedChars[5];
        unsigned char savedImage[1024];
        readFromEEPROM(slot*1024 , savedImage , 1024);
        inputToMatrix(savedImage);
        state = Idle_state;
        receivedChars[4] = Idle_state;

      }
      else if(state == animation_state)
      {
        if(counter % 40 == 0)
        {

          unsigned char savedImage[1024];
          readFromEEPROM(imageNumber*1024 , savedImage , 1024);
          inputToMatrix(savedImage);

          imageNumber++;
          if(imageNumber == 3)
          {
            imageNumber = 0;
          }
        }
      }

      counter++;
      delay(5);
     
  }

//This function generates a firework at a random location on the matrix and diplays it
void firework()
{
  int static counter = 0;
  int static start_x = random(2 , 14);;
  int static start_y = random(2 , 14);;
  int static start_r = random(2 , 8);;
  int static x = 0;
  int static y = 0;
  int static r = 0;

  //15 cycles * 5 ms = 75 ms for each time the firework is going up
  int goingUpPeriod = start_x * 15;
  //15 cycles * 5 ms = 75 ms for each explosion of the firework
  int explosionPeriod = start_r * 15;

  //going up animation
  if(counter < goingUpPeriod)
  {
    if(counter % 15 == 0)
    {
    int c = Coordonates_To_OrderNumber((15 - x) , start_y);
    FastLED.clear(); 
    leds[c] = CRGB::Orange;
    FastLED.show();
    x++;
    }
  }
  else if(counter < (goingUpPeriod + explosionPeriod))
  {
    //explosion animation
    if(counter % 15 == 0)
    {
      FastLED.clear();
      midPointCircleDraw((15 - start_x) , start_y , r);
      if((r > 2) && (random(1,3) == 1)) 
      {
        midPointCircleDraw((15 - start_x) , start_y , r-2);
      }
      FastLED.show();

      r++;
    }
  }
  else
  {
    //prepare for the next firework
    FastLED.clear();
    counter = 0;
    x = 0;
    y = 0;
    r = 0;
    //Get random coordonates for the firework
    start_x = random(2 , 14);
    start_y = random(2 , 14);
    //Random size of the firework
    start_r = random(2 , 8);
  }
  
  counter++;
}

//This function generates a circle of radius r with the center (x,y)
void midPointCircleDraw(int x_centre, int y_centre, int r)
{
    int x = r, y = 0;
     
    // Printing the initial point on the axes
    // after translation
    if(((x + x_centre) >= 0) && ((x + x_centre) <= 15) && ((y + y_centre) >= 0) && ((y + y_centre) <= 15))
    {
    leds[Coordonates_To_OrderNumber(x + x_centre , y + y_centre)] = CHSV(random8(),255,255);
    }    
     
    // When radius is zero only a single
    // point will be printed
    if (r > 0)
    {
      if(((-x + x_centre) >= 0) && ((-x + x_centre) <= 15) && ((y + y_centre) >= 0) && ((y + y_centre) <= 15))
      {
      leds[Coordonates_To_OrderNumber(-x + x_centre , y + y_centre)] = CHSV(random8(),255,255);
      }

      if(((y + x_centre) >= 0) && ((y + x_centre) <= 15) && ((x + y_centre) >= 0) && ((x + y_centre) <= 15))
      {
      leds[Coordonates_To_OrderNumber(y + x_centre , x + y_centre)] = CHSV(random8(),255,255);
      }

      if(((y + x_centre) >= 0) && ((y + x_centre) <= 15) && ((-x + y_centre) >= 0) && ((-x + y_centre) <= 15))
      {
      leds[Coordonates_To_OrderNumber(y + x_centre , -x + y_centre)] = CHSV(random8(),255,255);
      }

    }
     
    // Initialising the value of P
    int P = 1 - r;
    while (x > y)
    {
        y++;
         
        // Mid-point is inside or on the perimeter
        if (P <= 0)
            P = P + 2*y + 1;
        // Mid-point is outside the perimeter
        else
        {
            x--;
            P = P + 2*y - 2*x + 1;
        }
         
        // All the perimeter points have already been printed
        if (x < y)
            break;
         
        // Printing the generated point and its reflection
        // in the other octants after translation
        if(((x + x_centre) >= 0) && ((x + x_centre) <= 15) && ((y + y_centre) >= 0) && ((y + y_centre) <= 15))
        {
          leds[Coordonates_To_OrderNumber(x + x_centre , y + y_centre)] = CRGB(random8(),random8(),random8());
        }

        if(((-x + x_centre) >= 0) && ((-x + x_centre) <= 15) && ((y + y_centre) >= 0) && ((y + y_centre) <= 15))
        {
          leds[Coordonates_To_OrderNumber(-x + x_centre , y + y_centre)] = CRGB(random8(),random8(),random8());
        }

        if(((x + x_centre) >= 0) && ((x + x_centre) <= 15) && ((-y + y_centre) >= 0) && ((-y + y_centre) <= 15))
        {
          leds[Coordonates_To_OrderNumber(x + x_centre , -y + y_centre)] = CRGB(random8(),random8(),random8());
        }
        
        if(((-x + x_centre) >= 0) && ((-x + x_centre) <= 15) && ((-y + y_centre) >= 0) && ((-y + y_centre) <= 15))
        {
          leds[Coordonates_To_OrderNumber(-x + x_centre , -y + y_centre)] = CRGB(random8(),random8(),random8());
        }
        
        // If the generated point is on the line x = y then
        // the perimeter points have already been printed
        if (x != y)
        {
            if(((y + x_centre) >= 0) && ((y + x_centre) <= 15) && ((x + y_centre) >= 0) && ((x + y_centre) <= 15))
            {
              leds[Coordonates_To_OrderNumber(y + x_centre , x + y_centre)] = CRGB(random8(),random8(),random8());
            }

            if(((-y + x_centre) >= 0) && ((-y + x_centre) <= 15) && ((x + y_centre) >= 0) && ((x + y_centre) <= 15))
            {
              leds[Coordonates_To_OrderNumber(-y + x_centre , x + y_centre)] = CRGB(random8(),random8(),random8());
            }

            if(((y + x_centre) >= 0) && ((y + x_centre) <= 15) && ((-x + y_centre) >= 0) && ((-x + y_centre) <= 15))
            {
              leds[Coordonates_To_OrderNumber(y + x_centre , -x + y_centre)] = CRGB(random8(),random8(),random8());
            }

            if(((-y + x_centre) >= 0) && ((-y + x_centre) <= 15) && ((-x + y_centre) >= 0) && ((-x + y_centre) <= 15))
            {
              leds[Coordonates_To_OrderNumber(-y + x_centre , -x + y_centre)] = CRGB(random8(),random8(),random8());
            }

        }
    }
}

//This function returns a number from 0 to 255 after being given 2 coordonates
//That number represents the LED on column x , row y
uint16_t Coordonates_To_OrderNumber( uint8_t x, uint8_t y)
{
  uint8_t OrderNumber;
      //y & 0x01 is true for odd numbers
      if( y & 0x01) {
        // Odd rows run backwards
        uint8_t reverseX = (MatrixWidth - 1) - x;
        OrderNumber = (y * MatrixWidth) + reverseX;
      } else {
        // Even rows run forwards
        OrderNumber = (y * MatrixWidth) + x;
      }
  return OrderNumber;
}


//This function handles all bluetooth communication
//At each call it chacks if there is any character available on the serial connection
//Puts all messages in a vector
void BLEhandler() {
    static bool isData = true;
    static int count = 0;
    //char endMarker = '\n';
    unsigned char rc;
    
    //Stay in the function while there is something in the serial buffer
    while (Serial1.available() > 0) {
      //Read the first character
        rc = Serial1.read();

        //This switch is looking for the string "FIN" , string that represents that a message has finished
        switch(rc)
        {
          case 'F':
            if(count==0)
            {
              count++;
            }
            else if(count > 1)
            {
              count = 0;
            }         
            break;
          case 'I':
            if(count==1)
            {
              count++;
            }
            else
            {
              count = 0;
            }   
            break;
          case 'N':
            if(count==2)
            {
              count++;
            }
            else
            {
              count = 0;
            }    
            break;
          default:
            count = 0;
            isData = true;
            break;
        }
        //message is finished , prepare for the next one
        if(count == 3)
        {
          newMessage = true;
          count = 0;
          isData = false;
        }
        else
        {
          newMessage = false;
        }

        //if the message is not finished
        if (isData == true) 
        {
          //save the current character
          receivedChars[ndx] = rc;
          #if(debug == 1)
            Serial.print(ndx);
            Serial.print(" ");
            Serial.println((int)receivedChars[ndx]);
          #endif
          ndx++;
          //if we go over the maximum number we just write over the last one
          if (ndx >= numChars) 
          {  
            #if(debug == 1)
              Serial.print(ndx);
              Serial.print(" String finished with so many characters : ");
              Serial.println(numChars);
            #endif
            
            ndx = numChars - 1;
          }
        }
        else
        {
          //if we finished the message , save the last character and terminate the string
          // also reset the counter for the next message
          receivedChars[ndx] = rc;
          receivedChars[ndx+1] = '\0'; 
          #if(debug == 1)
            Serial.print(ndx);
            Serial.print(" ");
            Serial.println((int)receivedChars[ndx]);
            Serial.print(ndx+1);
            Serial.print(" ");
            Serial.println((int)receivedChars[ndx+1]);
          #endif
          ndx = 0;
        }
    }
}

//this is a function that transforms the bytes received via bluetooth into colors and sets them up in the matrix
void inputToMatrix(unsigned char image[])
{
  for(int j = 0 ; j<16 ; j++)
  {
    for(int i = 0; i <16 ; i++)
    {
      byte r = image[(j*64)+(i*3)+6];
      byte g = image[(j*64)+(i*3)+7];
      byte b = image[(j*64)+(i*3)+8];
      CRGB a = CRGB(r,g,b);
      leds[Coordonates_To_OrderNumber(j,i)] = a;
      if(j == 0 && i == 0)
      {
        Serial.print("r = ");
        Serial.println(r);
        Serial.print("g = ");
        Serial.println(g);
        Serial.print("b = ");
        Serial.println(b);
      }
    }
  }
  FastLED.show();
}

//this function saves data into the EEPROM(Electrically Erasable Programmable Read-Only Memory) in pieces of 1024 bytes
void saveIntoEEPROM(int address, byte array[], int arraySize)
{
  int adressIndex = address;
  for(int i = 0 ; i < arraySize ; i++)
  {
    EEPROM.write(adressIndex , array[i]);
    #if(debug == 1)
      Serial.print(i);
      Serial.print(" = ");
      Serial.println(array[i]);
    #endif
    adressIndex++;
  }
}

//this function reads data from the EEPROM in pieces of 1024 bytes
void readFromEEPROM(int address, byte array[], int arraySize)
{
  int adressIndex = address;
  for(int i = 0 ; i < arraySize ; i++)
  {
    array[i] = EEPROM.read(adressIndex);
    #if(debug == 1)
      Serial.print(i);
      Serial.print(" = ");
      Serial.println(array[i]);
    #endif
    adressIndex++;
  }
}