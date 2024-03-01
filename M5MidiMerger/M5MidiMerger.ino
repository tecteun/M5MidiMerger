
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

// Define the structure for MIDI data
struct MidiData {
    String name;
    midi::MidiType type;
    midi::DataByte data1;
    midi::DataByte data2;
    midi::Channel channel;
};

void setup()
{
  pinMode(3,INPUT_PULLUP); 
  pinMode(16,INPUT_PULLUP);
  ledcDetachPin(SPEAKER_PIN);
  pinMode(SPEAKER_PIN, INPUT);

  // Setup scroll area
  // setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);
  setupScrollArea(0, 0);

  Serial1.begin(MIDI_BAUDRATE, SERIAL_8N1, RXD1, TXD1);
  swSerial.begin(MIDI_BAUDRATE, SWSERIAL_8N1);

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

  // Create a new task on core 1 to handle the drawing:
    xTaskCreatePinnedToCore(
        drawMidiInfoTask,   // Task function
        "DrawMidiInfoTask", // Task name
        4096,              // Stack size (in words)
        NULL,               // Task input parameter
        1,                  // Task priority
        NULL, 1                // Task handle
    );
}

// Create a queue for MIDI data
QueueHandle_t midiQueue = xQueueCreate(10, sizeof(MidiData));

// Task to draw MIDI information on the LCD
void drawMidiInfoTask(void * parameter) {
  M5.begin();
  M5.Power.begin();
  M5.Lcd.setTextPadding(M5.Lcd.width());
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
  M5.Lcd.fillRect(0, 0, 320, TEXT_HEIGHT, TFT_BLUE);
  M5.Lcd.drawCentreString(" Midi Terminal", 320 / 2, 0, 2);

  // Change colour for scrolling zone text
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  M5.Lcd.setBrightness(255);
  MidiData midiData;
  while (true) {
      //this makes m5stack more silent
      dacWrite(25,0);
      while (xQueueReceive(midiQueue, &midiData, portMAX_DELAY)) {
          drawMidiInfo(midiData.name, midiData.type, midiData.data1, midiData.data2, midiData.channel);
      }
      M5.update();
  }
}// Color table in RGB565 format
uint16_t colorTable[] = {0x000F, 0x03E0, 0x7800, 0x780F, 0x7BE0, 0xC618, 0x7BEF, 0x001F, 0xB7E0, 0x07E0, 0xFFE0, 0xFDA0, 0xFC9F, 0x07FF, 0x03EF, 0xF800, 0xF81F,
                         /* Additional 30 bright colors */
                         0xFFFF, 0x07FF, 0xF81F, 0xFFE0, 0xF83E, 0x07E0, 0x001F, 0x8410, 0x0400, 0x0200, 0x83E0, 0x83EF, 0x83FF, 0xC300, 0xFD20, 0xFBE0,
                         0xFBDF, 0x839F, 0xFF9E, 0xF79C, 0x7BEF, 0x7B6D, 0x7B4C, 0x73AE, 0x738E, 0x736D, 0x6B4D, 0x6B2C, 0x6B0C, 0x62EC};

void drawMidiInfo(String name, midi::MidiType type, midi::DataByte data1, midi::DataByte data2, midi::Channel channel) {
    // Convert MIDI data to hexadecimal strings
    // Convert MIDI data to hexadecimal strings
    String hexD1 = String(data1, HEX);
    String hexD2 = String(data2, HEX);


    // Calculate color intensity based on data2 value
    uint8_t intensity = data2 * 2;

    // Calculate actual color based on data1 and type values
    uint16_t color = colorTable[data1 % (sizeof(colorTable) / sizeof(colorTable[0]))];

    // Combine intensity and color to create the final text color in RGB565 format
    // Extract red, green, and blue components from the color
    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;
    
    // Apply intensity to each component
    r = (r * intensity) / 255;
    g = (g * intensity) / 255;
    b = (b * intensity) / 255;
    
    // Combine the components back into a single color
    uint16_t textColor = (r << 11) | (g << 5) | b;

    if(type != 248){
      // Create a string with MIDI information
      char midiInfo[100];
      sprintf(midiInfo, "%s Type: %d, 0x%s, 0x%s, Channel: %d", name, type, hexD1, hexD2, channel);
  
      yDraw = scroll_line();
      // Display MIDI information on the LCD with the calculated text color
      M5.Lcd.setTextColor(TFT_WHITE, textColor);
      
      M5.Lcd.drawString(midiInfo, 0, yDraw, 2);
    }
}

void loop()
{
  Usb.Task();
  while (midiUsb.read())
  {
    MidiData midiData;
    midiData.name = "midiUsb";
    midiData.type = midiUsb.getType();
    midiData.data1 = midiUsb.getData1();
    midiData.data2 = midiUsb.getData2();
    midiData.channel = midiUsb.getChannel();
    midiA.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiB.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    //midiUsb.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb1.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb2.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb3.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    // Send MIDI data to the queue
    xQueueSend(midiQueue, &midiData, 0);
  }
  
  if (midiUsb1.read())
  {
    MidiData midiData;
    midiData.name = "midiUsb1";
    midiData.type = midiUsb1.getType();
    midiData.data1 = midiUsb1.getData1();
    midiData.data2 = midiUsb1.getData2();
    midiData.channel = midiUsb1.getChannel();
    midiA.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiB.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    //midiUsb1.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb2.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb3.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    // Send MIDI data to the queue
    xQueueSend(midiQueue, &midiData, 0);
  }
  if (midiUsb2.read())
  {
    MidiData midiData;
    midiData.name = "midiUsb2";
    midiData.type = midiUsb2.getType();
    midiData.data1 = midiUsb2.getData1();
    midiData.data2 = midiUsb2.getData2();
    midiData.channel = midiUsb2.getChannel();
    midiA.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiB.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb1.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    //midiUsb2.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb3.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    // Send MIDI data to the queue
    xQueueSend(midiQueue, &midiData, 0);
  }
  if (midiUsb3.read())
  {
    MidiData midiData;
    midiData.name = "midiUsb3";
    midiData.type = midiUsb3.getType();
    midiData.data1 = midiUsb3.getData1();
    midiData.data2 = midiUsb3.getData2();
    midiData.channel = midiUsb3.getChannel();
    midiA.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiB.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb1.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb2.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    //midiUsb3.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    // Send MIDI data to the queue
    xQueueSend(midiQueue, &midiData, 0);
  }
  if (midiA.read())
  {
    MidiData midiData;
    midiData.name = "midiA";
    midiData.type = midiA.getType();
    midiData.data1 = midiA.getData1();
    midiData.data2 = midiA.getData2();
    midiData.channel = midiA.getChannel();
    //midiA.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiB.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb1.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb2.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb3.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    // Send MIDI data to the queue
    xQueueSend(midiQueue, &midiData, 0);
  }
  if (midiB.read())
  {
    MidiData midiData;
    midiData.name = "midiB";
    midiData.type = midiB.getType();
    midiData.data1 = midiB.getData1();
    midiData.data2 = midiB.getData2();
    midiData.channel = midiB.getChannel();
    midiA.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    //midiB.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb1.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb2.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    midiUsb3.send(midiData.type, midiData.data1, midiData.data2, midiData.channel);
    // Send MIDI data to the queue
    xQueueSend(midiQueue, &midiData, 0);
  }
}
