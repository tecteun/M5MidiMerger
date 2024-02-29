
#include <SoftwareSerial.h>
#include <M5Stack.h>
#include <usbhub.h>
#include <MIDI.h>
#include <UHS2-MIDI.h>
//https://github.com/TheKikGen/USBMidiKliK
//send sysex F0 77 77 77 09 F7 to reset interface to serial mode, to flash

#define MIDI_BAUDRATE 31250
#define RXD2 16
#define TXD2 22 //pin 17 doesnt work together with https://shop.m5stack.com/products/usb-module
#define RXD1 3
#define TXD1 1

USB Usb;

// support one hub, four midi devices
USBHub Hub(&Usb);
UHS2MIDI_CREATE_INSTANCE(&Usb, 0, midiUsb);
UHS2MIDI_CREATE_INSTANCE(&Usb, 0, midiUsb1);
UHS2MIDI_CREATE_INSTANCE(&Usb, 0, midiUsb2);
UHS2MIDI_CREATE_INSTANCE(&Usb, 0, midiUsb3);

SoftwareSerial swSerial(RXD2, TXD2);

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1,    midiA);
MIDI_CREATE_INSTANCE(SoftwareSerial, swSerial,    midiB);
bool toggle = false;


// The scrolling area must be a integral multiple of TEXT_HEIGHT
#define TEXT_HEIGHT 16  // Height of text to be printed and scrolled
#define TOP_FIXED_AREA \
    14  // Number of lines in top fixed area (lines counted from top of screen)
#define BOT_FIXED_AREA \
    0  // Number of lines in bottom fixed area (lines counted from bottom of
       // screen).
#define YMAX 240  // Bottom of screen area

// The initial y coordinate of the top of the scrolling area
uint16_t yStart = 0;
// yArea must be a integral multiple of TEXT_HEIGHT
uint16_t yArea = YMAX - TOP_FIXED_AREA - BOT_FIXED_AREA;
// The initial y coordinate of the top of the bottom text line
// uint16_t yDraw = YMAX - BOT_FIXED_AREA - TEXT_HEIGHT;
uint16_t yDraw = 0;

// Keep track of the drawing x coordinate
uint16_t xPos = 0;

// For the byte we read from the serial port
byte data = 0;

// A few test variables used during debugging
boolean change_colour = 1;
boolean selected      = 1;

// We have to blank the top line each time the display is scrolled, but this
// takes up to 13 milliseconds for a full width line, meanwhile the serial
// buffer may be filling... and overflowing We can speed up scrolling of short
// text lines by just blanking the character we drew
int blank[19];  // We keep all the strings pixel lengths to optimise the speed
                // of the top line blanking


void setup()
{
  pinMode(3,INPUT_PULLUP); 
  pinMode(16,INPUT_PULLUP);
  // Setup the TFT display
  M5.begin();
  M5.Power.begin();
  // M5.Lcd.setRotation(5); // Must be setRotation(0) for this sketch to work
  // correctly
  M5.Lcd.fillScreen(TFT_BLACK);

  // Setup baud rate and draw top banner
  // Serial.begin(115200);

  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
  M5.Lcd.fillRect(0, 0, 320, TEXT_HEIGHT, TFT_BLUE);
  M5.Lcd.drawCentreString(" Midi Terminal", 320 / 2, 0, 2);

  // Change colour for scrolling zone text
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  // Setup scroll area
  // setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);
  setupScrollArea(0, 0);

  // Zero the array
  for (byte i = 0; i < 18; i++) blank[i] = 0;

  
  // Make Arduino transparent for serial communications from and to USB(client-hid)
  //pinMode(0,INPUT); // Arduino RX - ATMEGA8U2 TX
  //pinMode(1,INPUT); // Arduino TX - ATMEGA8U2 RX
  Serial1.begin(MIDI_BAUDRATE, SERIAL_8N1, RXD1, TXD1);
  swSerial.begin(MIDI_BAUDRATE, SWSERIAL_8N1);
  
  //pinMode(LED_BUILTIN, OUTPUT);
  // Initiate MIDI communications, listen to all channels

  midiA.begin(MIDI_CHANNEL_OMNI);
  midiB.begin(MIDI_CHANNEL_OMNI);  
  midiUsb.begin(MIDI_CHANNEL_OMNI);
  midiUsb1.begin(MIDI_CHANNEL_OMNI);
  midiUsb2.begin(MIDI_CHANNEL_OMNI);
  midiUsb3.begin(MIDI_CHANNEL_OMNI);
  
  midiA.turnThruOff();
  midiB.turnThruOff();
  midiUsb.turnThruOff();
  midiUsb1.turnThruOff();
  midiUsb2.turnThruOff();
  midiUsb3.turnThruOff();

  if (Usb.Init() == -1) {
    while (1); //halt
  }//if (Usb.Init() == -1...
  delay( 200 );
}

// Function to draw MIDI information on the LCD
void drawMidiInfo(String name, midi::MidiType type, midi::DataByte data1, midi::DataByte data2, midi::Channel channel) {
    // Convert MIDI data to hexadecimal strings
    String hexD1 = String(data1, HEX);
    String hexD2 = String(data2, HEX);
    if(type != 248){
      // Create a string with MIDI information
      String midiInfo = name + " " +
                        "Type: " + String(type) +
                        ", 0x" + hexD1 +
                        ", 0x" + hexD2 +
                        ", Channel: " + String(channel);
  
      yDraw = scroll_line();
      // Display MIDI information on the LCD
      M5.Lcd.drawString(midiInfo.c_str(), 0, yDraw, 2);
    }
}

void loop()
{
  Usb.Task();
  
  M5.update();

  
  if (midiUsb.read())
  {
    //digitalWrite(LED_BUILTIN, toggle = !toggle ? LOW : HIGH);
    midi::MidiType t = midiUsb.getType();
    midi::DataByte d1 = midiUsb.getData1();
    midi::DataByte d2 = midiUsb.getData2();
    midi::Channel  c = midiUsb.getChannel();
    drawMidiInfo("midiUsb", t, d1, d2, c);
    midiA.send(t, d1, d2, c);
    midiB.send(t, d1, d2, c);
    midiUsb1.send(t, d1, d2, c);
    midiUsb2.send(t, d1, d2, c);
    midiUsb3.send(t, d1, d2, c);
  }
  Usb.Task();
  
  if (midiUsb1.read())
  {
    //digitalWrite(LED_BUILTIN, toggle = !toggle ? LOW : HIGH);
    midi::MidiType t = midiUsb1.getType();
    midi::DataByte d1 = midiUsb1.getData1();
    midi::DataByte d2 = midiUsb1.getData2();
    midi::Channel  c = midiUsb1.getChannel();
    drawMidiInfo("midiUsb1", t, d1, d2, c);
    midiA.send(t, d1, d2, c);
    midiB.send(t, d1, d2, c);
    midiUsb.send(t, d1, d2, c);
    midiUsb2.send(t, d1, d2, c);
    midiUsb3.send(t, d1, d2, c);
  }
  Usb.Task();
  
  if (midiUsb2.read())
  {
    //digitalWrite(LED_BUILTIN, toggle = !toggle ? LOW : HIGH);
    midi::MidiType t = midiUsb2.getType();
    midi::DataByte d1 = midiUsb2.getData1();
    midi::DataByte d2 = midiUsb2.getData2();
    midi::Channel  c = midiUsb2.getChannel();
    drawMidiInfo("midiUsb2", t, d1, d2, c);
    midiA.send(t, d1, d2, c);
    midiB.send(t, d1, d2, c);
    midiUsb.send(t, d1, d2, c);
    midiUsb1.send(t, d1, d2, c);
    midiUsb3.send(t, d1, d2, c);
  }
  Usb.Task();
  
  if (midiUsb3.read())
  {
    //digitalWrite(LED_BUILTIN, toggle = !toggle ? LOW : HIGH);
    midi::MidiType t = midiUsb3.getType();
    midi::DataByte d1 = midiUsb3.getData1();
    midi::DataByte d2 = midiUsb3.getData2();
    midi::Channel  c = midiUsb3.getChannel();
    drawMidiInfo("midiUsb3", t, d1, d2, c);
    midiA.send(t, d1, d2, c);
    midiB.send(t, d1, d2, c);
    midiUsb.send(t, d1, d2, c);
    midiUsb1.send(t, d1, d2, c);
    midiUsb2.send(t, d1, d2, c);
  }
  Usb.Task();
  
  if (midiA.read())
  {
    //digitalWrite(LED_BUILTIN, toggle = !toggle ? LOW : HIGH);
    midi::MidiType t = midiA.getType();
    midi::DataByte d1 = midiA.getData1();
    midi::DataByte d2 = midiA.getData2();
    midi::Channel  c = midiA.getChannel();
    drawMidiInfo("midiA", t, d1, d2, c);
    midiB.send(t, d1, d2, c);
    midiUsb.send(t, d1, d2, c);
  }
  Usb.Task();
  
  if (midiB.read())
  {
    //digitalWrite(LED_BUILTIN, toggle = !toggle ? LOW : HIGH);
    midi::MidiType t = midiB.getType();
    midi::DataByte d1 = midiB.getData1();
    midi::DataByte d2 = midiB.getData2();
    midi::Channel  c = midiB.getChannel();
    drawMidiInfo("midiB", t, d1, d2, c);
    midiA.send(t, d1, d2, c);
    midiUsb.send(t, d1, d2, c);
  }
  
}
