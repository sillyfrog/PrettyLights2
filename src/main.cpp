
// To upload to the ESP FS using curl:
// curl 10.7.10.23/edit --form 'data=@index.htm;filename=/index.htm' -v
// or
// espupload.py <ip> <filename> [<filename>]

// To run an update over the air
//   cd .pio/build/d1_mini
//   python3 -m http.server
//   curl http://10.1.10.41/ota?url=http%3A%2F%2F10.1.10.2%3A8000%2Ffirmware.bin
// First IP is the IP of the ESP, the second IP in the %2F is the IP of the host running the web server

#include <Arduino.h>
#include "Adafruit_TLC5947.h"
#include <Adafruit_NeoPixel.h>
#include "FS.h"
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <LittleFS.h>
#include "wificredentials.h"
#include <ArduinoJson.h>
#include <ESP8266httpUpdate.h>

#include "gamma12bit.h"

#define VERSION "0.1.5"
// Frames kept in each file, each file size will increase based on the number of patterns
// This MUST match the same constant in index.htm
#define FRAMES_PER_FILE 25

#define MAX_RGB_STRIPS 3
#define MAX_RGB_LED_PER_STRIP 50
#define MAX_PWM_STRIPS 2
#define MAX_PWM_BOARD_PER_STRIP 4
#define MAX_RGB_PATTERNS 50
#define MAX_PWM_PATTERNS 50

#ifdef ESP32
#define DN 18
#define CLK 19
//uint8_t pwm_lat_pin_allocations[] = {D7, D3};
uint8_t pin_allocations[] = {15, 2, 0};
#else
#define DN D5
#define CLK D6
uint8_t rgb_pin_allocations[] = {D1, D2, D8};
uint8_t pwm_lat_pin_allocations[] = {D7, D3};
#endif

#define LED_PER_BOARD 24

#define MAX_FRAMES 1500
#define MAX_FRAMES_PER_SEC 30

#define RGB_BRIGHTNESS 255

// Delay between each call to identify, testmodes etc
#define TEST_FRAME_DELAY 200

#define MAX_TOTAL_RGB_LED MAX_RGB_STRIPS *MAX_RGB_LED_PER_STRIP
#define MAX_TOTAL_PWM_LED MAX_PWM_STRIPS *MAX_PWM_BOARD_PER_STRIP *LED_PER_BOARD

#define WEB_PORT 80

#define RGBS "rgb"
#define PWMS "pwm"
#define RGBC 'r'
#define PWMC 'c'

// Filesystem to use
#define FFS LittleFS
// #define FFS SPIFFS
AsyncWebServer server(WEB_PORT);

Adafruit_NeoPixel rgb_strips[MAX_RGB_STRIPS];

Adafruit_TLC5947 pwm_strips[MAX_PWM_STRIPS] = {Adafruit_TLC5947(0, 0, 0, 0), Adafruit_TLC5947(0, 0, 0, 0)};
bool pwm_pixel_change[MAX_PWM_STRIPS];

// Patterns start at 0, so the unallocated pattern is the end of the range
#define NO_PATTERN 0xff
struct Config
{
  byte rgb_patterns;
  byte pwm_patterns;
  uint16 frame_count;
  byte framespersec;
  uint16 msperframe;
  byte leds_per_rgb_strip[MAX_RGB_STRIPS];
  uint16 total_rgb_pixels;
  byte boards_per_pwm_strip[MAX_PWM_STRIPS];
  uint16 total_pwm_pixels;
  byte rgb_pattern_assign[MAX_TOTAL_RGB_LED];
  byte pwm_pattern_assign[MAX_TOTAL_PWM_LED];
};
Config config;

// Time when the next frame should be applied
unsigned long nextupdate;
uint16 currentframe = 0;

uint8 applyGamma(uint8 c)
{
  return Adafruit_NeoPixel::gamma8(c);
}

void setRGBPixelColor(uint16_t n, uint8 r, uint8 g, uint8 b)
{
  if (n >= config.total_rgb_pixels)
  {
    return;
  }
  // First figure out the strip to apply this to
  int i = 0;
  while (i < MAX_RGB_STRIPS)
  {
    if (n >= config.leds_per_rgb_strip[i])
    {
      n -= config.leds_per_rgb_strip[i];
      i++;
    }
    else
    {
      break;
    }
  }
  // Apply gamma correction to each color component
  // XXX need to handle white should we support it down the track
  rgb_strips[i].setPixelColor(n, applyGamma(r), applyGamma(g), applyGamma(b));
}

// Assigns the correct colors to each pixel for the given frame
File rgbfh;
uint16 curRGBfile = 0xffff;
void setRGBFrame(unsigned int frame)
{
  unsigned long starttime = millis();
  char fname[32];
  char data[config.rgb_patterns * 3];
  uint16 targetfile = frame / FRAMES_PER_FILE;
  if (targetfile != curRGBfile)
  {
    sprintf(fname, "/rgb%02d/frames%04d.dat", targetfile / 10, targetfile);
    rgbfh = FFS.open(fname, "r");
    if (!rgbfh)
    {
      Serial.printf("ERROR Opening file name: %s \n", fname);
    }
    curRGBfile = targetfile;
  }

  size_t bytesread = rgbfh.readBytes(data, config.rgb_patterns * 3);
  if (bytesread == 0)
  {
    // Empty read, don't bother doing anything
    return;
  }
  for (uint16 p = 0; p < config.rgb_patterns; p++)
  {
    uint16 pidx = p * 3;
    for (uint16 led = 0; led < config.total_rgb_pixels; led++)
    {
      if (config.rgb_pattern_assign[led] == p)
      {
        setRGBPixelColor(led, data[pidx], data[pidx + 1], data[pidx + 2]);
      }
    }
  }
  unsigned long endtime = millis();
  if (frame % 77 == 0 || (endtime - starttime) > 30)
  {
    Serial.print("RGB Frame: ");
    Serial.print(frame);
    Serial.print("   Final Time: ");
    Serial.print(endtime - starttime);
    Serial.println("ms");
  }
}

void setPWMPixel(uint16_t n, uint16_t b)
{
  if (n >= config.total_pwm_pixels)
  {
    return;
  }
  uint16_t gval = gamma12(b);
  int i = 0;
  while (i < MAX_PWM_STRIPS)
  {
    if (n >= config.boards_per_pwm_strip[i] * LED_PER_BOARD)
    {
      n -= config.boards_per_pwm_strip[i] * LED_PER_BOARD;
      i++;
    }
    else
    {
      break;
    }
  }
  if (pwm_strips[i].getPWM(n) != gval)
  {
    pwm_strips[i].setPWM(n, gval);
    pwm_pixel_change[i] = true;
  }
}

// Assigns the correct colors to each pixel for the given frame
File pwmfh;
uint16 curPWMfile = 0xffff;
void setPWMFrame(unsigned int frame)
{
  unsigned long starttime = millis();
  char fname[32];
  char data[config.pwm_patterns * 2];
  uint16 targetfile = frame / FRAMES_PER_FILE;

  if (targetfile != curPWMfile)
  {
    sprintf(fname, "/pwm%02d/frames%04d.dat", targetfile / 10, targetfile);
    pwmfh = FFS.open(fname, "r");
    if (!pwmfh)
    {
      Serial.printf("ERROR Opening file name: %s \n", fname);
    }
    curPWMfile = targetfile;
  }

  size_t bytesread = pwmfh.readBytes(data, config.pwm_patterns * 2);
  if (bytesread == 0)
  {
    // Empty read, don't bother doing anything
    return;
  }
  for (uint16 p = 0; p < config.pwm_patterns; p++)
  {
    uint16 pidx = p * 2;
    uint16 pixel = data[pidx + 1];
    pixel = pixel << 8;
    pixel += data[pidx];
    for (uint16 led = 0; led < config.total_pwm_pixels; led++)
    {
      if (config.pwm_pattern_assign[led] == p)
      {
        setPWMPixel(led, pixel);
      }
    }
  }
  unsigned long endtime = millis();
  if (frame % 77 == 0 || (endtime - starttime) > 30)
  {
    Serial.print("PWM Frame: ");
    Serial.print(frame);
    Serial.print("  Final Time: ");
    Serial.print(endtime - starttime);
    Serial.println("ms");
  }
}

void applyFrame()
{
  // Applies the currently set frame to the hardware
  for (int i = 0; i < MAX_RGB_STRIPS; i++)
  {
    if (config.leds_per_rgb_strip[i] > 0)
    {
      rgb_strips[i].show();
    }
  }
  for (int i = 0; i < MAX_PWM_STRIPS; i++)
  {
    if (config.boards_per_pwm_strip[i] > 0)
    {
      if (pwm_pixel_change[i])
      {
        pwm_strips[i].write();
        pwm_pixel_change[i] = false;
      }
    }
  }
}

void printDirInfo(String path)
{
  Dir dir = FFS.openDir(path);
  while (dir.next())
  {
    Serial.print(dir.fileName());
    Serial.print(": ");
    if (dir.isFile())
    {
      File f = dir.openFile("r");
      Serial.print(f.size());
    }
    else if (dir.isDirectory())
    {
      Serial.println("Entering...");
      printDirInfo(dir.fileName());
    }
    Serial.println();
  }
}

void printfsinfo()
{
  FSInfo fs_info;
  FFS.info(fs_info);
  Serial.println("=== Filesystem Info ===");
  Serial.print("totalBytes: ");
  Serial.println(fs_info.totalBytes);
  Serial.print("usedBytes: ");
  Serial.println(fs_info.usedBytes);
  Serial.print("Free Bytes: ");
  Serial.println(fs_info.totalBytes - fs_info.usedBytes);
  Serial.print("blockSize: ");
  Serial.println(fs_info.blockSize);
  Serial.print("pageSize: ");
  Serial.println(fs_info.pageSize);
  Serial.print("maxOpenFiles: ");
  Serial.println(fs_info.maxOpenFiles);
  Serial.print("maxPathLength: ");
  Serial.println(fs_info.maxPathLength);
  Serial.println("=== File Info ===");
  printDirInfo("/");
  Serial.println("=== Done ===");
}

void handleRename(AsyncWebServerRequest *request)
{
  String src = request->arg("src");
  String dst = request->arg("dst");
  String message;
  Serial.print("Renaming from: ");
  Serial.print(src);
  Serial.print(" to: ");
  Serial.println(dst);
  if (FFS.rename(src, dst))
  {
    message = "OK";
  }
  else
  {
    message = "FAIL";
  }
  request->send(200, "text/plain", message);
}

void handleConfig(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  const size_t capacity = JSON_OBJECT_SIZE(12);
  DynamicJsonDocument doc(capacity);
  FSInfo fs_info;
  FFS.info(fs_info);

  doc["heap"] = ESP.getFreeHeap();
  doc["ssid"] = WiFi.SSID();
  doc["version"] = VERSION;
  doc["total"] = fs_info.totalBytes;
  doc["used"] = fs_info.usedBytes;
  doc["free"] = fs_info.totalBytes - fs_info.usedBytes;
  doc["flashrealsize"] = ESP.getFlashChipRealSize();
  doc["flashidesize"] = ESP.getFlashChipMode();
  FlashMode_t ideMode = ESP.getFlashChipMode();
  doc["flashmode"] = (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN");

  serializeJson(doc, *response);
  request->send(response);
}

bool doReboot = false;
void handleReboot(AsyncWebServerRequest *request)
{
  // Reboot next mainloop, the response probably won't make it through
  doReboot = true;
  request->send(200, "text/plain", "OK");
}

void printConfiguration()
{
  Serial.printf("RGB Patterns: %d, PWM Patterns: %d, Total Frame Count: %d, Frames/sec: %d, Total RGB Pixels: %d, Total PWM Pixels: %d\n",
                config.rgb_patterns, config.pwm_patterns, config.frame_count, config.framespersec, config.total_rgb_pixels, config.total_pwm_pixels);
  for (int i = 0; i < MAX_RGB_STRIPS; i++)
  {
    Serial.printf("LEDs on strip %d: %d\n", i, config.leds_per_rgb_strip[i]);
  }
  for (int i = 0; i < MAX_PWM_STRIPS; i++)
  {
    Serial.printf("PWM Boards on strip %d: %d\n", i, config.boards_per_pwm_strip[i]);
  }
  Serial.println("RGB LED Pattern Assignment:");
  for (int i = 0; i < config.total_rgb_pixels; i++)
  {
    Serial.printf("%d: %d, ", i, config.rgb_pattern_assign[i]);
  }
  Serial.println("\nPWM LED Pattern Assignment:");
  for (int i = 0; i < config.total_pwm_pixels; i++)
  {
    Serial.printf("%d: %d, ", i, config.pwm_pattern_assign[i]);
  }
  Serial.println("");
}

void loadConfiguration()
{
  // Open file for reading
  File file = FFS.open("/hwsetup.json", "r");

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use http://arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<2048> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
  {
    Serial.print(F("Failed to read file, using default configuration: "));
    Serial.println(error.c_str());
  }

  config.rgb_patterns = doc["patterns"]["rgb"] | 0;
  config.pwm_patterns = doc["patterns"]["pwm"] | 0;

  config.frame_count = doc["frames"]["count"] | 50;
  config.framespersec = doc["frames"]["framespersec"] | 25;
  config.msperframe = 1000 / config.framespersec;

  // Set the strips to empty initially as working with an empty JSON
  // array casuses a crash
  for (unsigned int i = 0; i < MAX_RGB_STRIPS; i++)
  {
    config.leds_per_rgb_strip[i] = 0;
  }
  for (unsigned int i = 0; i < MAX_PWM_STRIPS; i++)
  {
    config.boards_per_pwm_strip[i] = 0;
  }

  config.total_rgb_pixels = 0;
  JsonArray ledcount = doc["rgb"];
  for (unsigned int i = 0; i < ledcount.size(); i++)
  {
    config.leds_per_rgb_strip[i] = doc["rgb"][i] | 0;
    config.total_rgb_pixels += config.leds_per_rgb_strip[i];
  }

  config.total_pwm_pixels = 0;
  ledcount = doc["pwm"];
  for (unsigned int i = 0; i < ledcount.size(); i++)
  {
    config.boards_per_pwm_strip[i] = doc["pwm"][i] | 0;
    config.total_pwm_pixels += config.boards_per_pwm_strip[i] * LED_PER_BOARD;
  }
}

void loadRGBAssignments()
{
  char data[config.total_rgb_pixels];
  // Set to NO_PATTERN initially
  for (unsigned int led = 0; led < config.total_rgb_pixels; led++)
  {
    config.rgb_pattern_assign[led] = NO_PATTERN;
  }
  // Open file for reading
  File file = FFS.open("/patternrgb.dat", "r");
  if (!file)
  {
    Serial.println("ERROR Opening patternrgb file!");
    return;
  }
  size_t bytesread = file.readBytes(data, config.total_rgb_pixels);
  for (unsigned int led = 0; led < bytesread; led++)
  {
    config.rgb_pattern_assign[led] = data[led];
  }
  file.close();
}

void loadPWMAssignments()
{
  char data[config.total_pwm_pixels];
  // Set to NO_PATTERN initially
  for (unsigned int led = 0; led < config.total_pwm_pixels; led++)
  {
    config.pwm_pattern_assign[led] = NO_PATTERN;
  }
  // Open file for reading
  File file = FFS.open("/patternpwm.dat", "r");
  if (!file)
  {
    Serial.println("ERROR Opening patternpwm file!");
    return;
  }
  size_t bytesread = file.readBytes(data, config.total_pwm_pixels);
  for (unsigned int led = 0; led < bytesread; led++)
  {
    config.pwm_pattern_assign[led] = data[led];
  }
  file.close();
}

void loadRGBDefaultColors()
{
  char data[config.total_rgb_pixels * 3];
  // Open file for reading
  File file = FFS.open("/colorrgb.dat", "r");
  if (!file)
  {
    Serial.println("ERROR Opening colorrgb file!");
    return;
  }
  size_t bytesread = file.readBytes(data, config.total_rgb_pixels * 3);
  for (unsigned int led = 0; led < bytesread / 3; led++)
  {
    unsigned int i = led * 3;
    setRGBPixelColor(led, data[i], data[i + 1], data[i + 2]);
  }
  file.close();
}

void loadPWMDefaultColors()
{
  char data[config.total_pwm_pixels * 2];
  // Open file for reading
  File file = FFS.open("/colorpwm.dat", "r");
  if (!file)
  {
    Serial.println("ERROR Opening colorpwm file!");
    return;
  }
  size_t bytesread = file.readBytes(data, config.total_pwm_pixels * 2);
  for (unsigned int led = 0; led < bytesread / 2; led++)
  {
    unsigned int i = led * 2;
    uint16 pixel = data[i + 1];
    pixel = pixel << 8;
    pixel += data[i];
    setPWMPixel(led, pixel);
  }
  file.close();
}

void applyConfiguration()
{
  for (int i = 0; i < MAX_RGB_STRIPS; i++)
  {
    rgb_strips[i].updateLength(config.leds_per_rgb_strip[i]);
    rgb_strips[i].updateType(NEO_GRB + NEO_KHZ800);
    rgb_strips[i].setPin(rgb_pin_allocations[i]);
    rgb_strips[i].setBrightness(RGB_BRIGHTNESS);
    rgb_strips[i].begin();
  }

  for (int i = 0; i < MAX_PWM_STRIPS; i++)
  {
    pwm_strips[i] = Adafruit_TLC5947(config.boards_per_pwm_strip[i], CLK, DN, pwm_lat_pin_allocations[i]);
    pwm_strips[i].begin();
    pwm_pixel_change[i] = true;
  }
}

void blankPWM()
{
  // Blanks all potential TLC strips ASAP in the boot sequence as they
  // default to full on
  Serial.println("Blanking PWM...");
  for (int i = 0; i < MAX_PWM_STRIPS; i++)
  {
    // Init'ing the Adafruit_TLC5947 sets all values to 0 automatically
    Adafruit_TLC5947 pwm_strip = Adafruit_TLC5947(MAX_PWM_BOARD_PER_STRIP, CLK, DN, pwm_lat_pin_allocations[i]);
    pwm_strip.begin();
    pwm_strip.write();
  }
}

uint16_t identifyled = 0;
byte identifytype = '\x0';
unsigned long identifyexpiry = 0;
uint16_t identify_i = 0;
#define IDENTIFY_DURATION 10 * 1000
void handleIdentify(AsyncWebServerRequest *request)
{
  Serial.println("Identifying...");
  String message = "OK";
  String sled;
  String stype;

  sled = request->arg("led");
  stype = request->arg("ledtype");
  identifyled = sled.toInt();
  identify_i = 0;
  if (stype.equalsIgnoreCase(RGBS))
  {
    identifytype = RGBC;
    identifyexpiry = millis() + IDENTIFY_DURATION;
  }
  else if (stype.equalsIgnoreCase(PWMS))
  {
    identifytype = PWMC;
    identifyexpiry = millis() + IDENTIFY_DURATION;
  }
  else
  {
    identifyexpiry = 0;
    message = "Invalid data";
  }
  Serial.println(message);
  request->send(200, "text/plain", message);
}

bool doapplyassign = false;
void applyAssign()
{
  Serial.println("Loading assignments");
  loadRGBAssignments();
  loadPWMAssignments();
  delay(0);
  Serial.println("Loading default colors");
  loadRGBDefaultColors();
  loadPWMDefaultColors();
  doapplyassign = false;
}

void runIdentify()
{
  uint8_t coloron1[] = {255, 0, 255};
  uint8_t coloron2[] = {0, 255, 0};
  uint8_t coloroff[] = {0, 0, 0};
  uint16_t led = 0;
  for (led = 0; led < config.total_rgb_pixels; led++)
  {
    setRGBPixelColor(led, coloroff[0], coloroff[1], coloroff[2]);
  }
  for (led = 0; led < config.total_pwm_pixels; led++)
  {
    setPWMPixel(led, 0);
  }
  if (identify_i % 2 == 0)
  {
    Serial.print("Identify: on: ");
    Serial.println(identifyled);
    if (identifytype == RGBC)
    {
      if (identify_i % 4 == 0)
      {
        setRGBPixelColor(identifyled, coloron1[0], coloron1[1], coloron1[2]);
      }
      else
      {
        setRGBPixelColor(identifyled, coloron2[0], coloron2[1], coloron2[2]);
      }
    }
    else
    {
      setPWMPixel(identifyled, 4000);
    }
  }
  else
  {
    // Ensure everything is off before exiting
    if (millis() > identifyexpiry)
    {
      Serial.println("Identify: Complete");
      identifyexpiry = 0;
      loadRGBDefaultColors();
      loadPWMDefaultColors();
    }
  }
  delay(0);
  applyFrame();
  identify_i++;
}

byte dotestmode = 0;
void handleTestMode(AsyncWebServerRequest *request)
{
  String smode = request->arg("mode");
  dotestmode = smode.toInt();
  String message;
  if (dotestmode == 0)
  {
    // Reset everything to defaults
    message = "OFF";
    loadRGBDefaultColors();
    loadPWMDefaultColors();
  }
  else
  {
    message.concat(dotestmode);
  }
  request->send(200, "text/plain", message);
}

#define TEST_LED_SPACING 7
unsigned int testmodeloop = 0;
void testMode1()
{
  uint8_t coloron1[] = {255, 0, 255};
  uint8_t coloron2[] = {0, 255, 0};
  uint8_t coloroff[] = {50, 0, 0};
  uint16_t led = 0;
  unsigned int oni = testmodeloop % TEST_LED_SPACING;
  bool even = (testmodeloop % (TEST_LED_SPACING * 2)) >= TEST_LED_SPACING;

  for (led = 0; led < config.total_rgb_pixels; led++)
  {
    if (led % TEST_LED_SPACING == oni)
    {
      if (even)
        setRGBPixelColor(led, coloron1[0], coloron1[1], coloron1[2]);
      else
        setRGBPixelColor(led, coloron2[0], coloron2[1], coloron2[2]);
    }
    else
    {
      setRGBPixelColor(led, coloroff[0], coloroff[1], coloroff[2]);
    }
  }
  Serial.printf("Test updated %u RGB LEDS, ending with %d, loop: %d\n", config.total_rgb_pixels, led, testmodeloop);
  for (led = 0; led < config.total_pwm_pixels; led++)
  {
    if (led % TEST_LED_SPACING == oni)
      setPWMPixel(led, 4000);
    else
      setPWMPixel(led, 0);
  }
  applyFrame();
  testmodeloop++;
}

void handleAssign(AsyncWebServerRequest *request)
{
  doapplyassign = true;
  request->send(200, "text/plain", "OK");
}

String url = "";
bool doUpdate = false;
void handleOta(AsyncWebServerRequest *request)
{
  Serial.print("Starting OTA: ");
  Serial.println(millis());
  String message = "";

  if (request->hasArg("url"))
  {
    url = request->arg("url");
    message.concat("Scheduling update from URL: ");
    message.concat(url);
    message.concat("\n");
    doUpdate = true;
  }
  else
  {
    message.concat("No URL supplied");
  }
  Serial.println(message);
  request->send(200, "text/plain", message);
}

void handleGotoFrame(AsyncWebServerRequest *request)
{
  String framestr;
  framestr = request->arg("frame");
  currentframe = framestr.toInt();
  Serial.printf("Gone to frame: %u\n", currentframe);
  request->send(200, "text/plain", "OK");
}

void runOta()
{
  doUpdate = false;
  WiFiClient client;
  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, url, VERSION);
  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
    break;
  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("[update] Update no Update.");
    break;
  case HTTP_UPDATE_OK:
    Serial.println("[update] Update ok."); // may not be called since we reboot the ESP
    break;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("👋");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, !HIGH);
  Serial.println("👋");
  Serial.print("Version: ");
  Serial.println(VERSION);
  blankPWM();
  delay(50);

  Serial.println("Connecting to WiFi");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Started connection process...");

  bool mountok = FFS.begin();
  if (FFS.exists("/doformat.txt") || !mountok)
  {
    Serial.println("Formatting...");
    FFS.format();
  }

  printfsinfo();

  loadConfiguration();
  printConfiguration();
  applyConfiguration();
  applyAssign();

  // Wait for connection
  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println("Connected.");

  // Serial.println("");
  // Serial.print("Connected to ");
  // Serial.println(ssid);
  // Serial.print("IP address: ");
  // Serial.println(WiFi.localIP());

  // Handles the /edit URL and uploading files
  server.addHandler(new SPIFFSEditor("", "", FFS));
  // Handles serving files from the Filesystem
  server.serveStatic("/", FFS, "/").setDefaultFile("index.htm");
  server.on("/ota", HTTP_GET + HTTP_POST, handleOta);
  server.on("/rename", HTTP_GET + HTTP_POST, handleRename);
  server.on("/config", HTTP_GET, handleConfig);
  server.on("/reboot", HTTP_POST, handleReboot);
  server.on("/identify", HTTP_GET + HTTP_POST, handleIdentify);
  server.on("/testmode", HTTP_GET + HTTP_POST, handleTestMode);
  server.on("/gotoframe", HTTP_GET + HTTP_POST, handleGotoFrame);
  server.on("/assign", HTTP_POST, handleAssign);
  server.begin();
  nextupdate = millis();
}

uint16 waitloop = 0;
bool frameset = false;
unsigned long nextwificheck = 1;
void loop()
{
  unsigned long now = millis();

  if (identifyexpiry || dotestmode)
  {
    if (now >= nextupdate)
    {
      if (identifyexpiry)
      {

        runIdentify();
      }
      else if (dotestmode == 1)
      {
        testMode1();
      }
      nextupdate = now + TEST_FRAME_DELAY;
    }
  }
  else
  {
    if (!frameset)
    {
      // Queue up the next frame, ready to apply
      setRGBFrame(currentframe);
      delay(0);
      setPWMFrame(currentframe);
      frameset = true;
    }
    if (now >= nextupdate)
    {
      currentframe++;
      if (currentframe >= config.frame_count)
      {
        currentframe = 0;
      }
      nextupdate += config.msperframe;
      delay(0);
      applyFrame();
      frameset = false;
      if (waitloop == 0 || currentframe % 100 == 0)
      {
        Serial.printf("Looping waiting count: %u   Frame #: %u    At: %lums\n", waitloop, currentframe, now);
        if (now > nextupdate)
        {
          Serial.printf("Currently %lums behind\n", now - nextupdate);
        }
      }
      waitloop = 0;
    }
    else
    {
      waitloop++;
    }
  }

  if (nextwificheck && now >= nextwificheck)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.print("Not connected: ");
      Serial.println(WiFi.status());
      nextwificheck = now + 500;
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
    else
    {
      Serial.println("WiFi Connected!");
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      nextwificheck = 0;
      digitalWrite(LED_BUILTIN, !LOW);
    }
  }

  if (doapplyassign)
  {
    applyAssign();
  }

  if (doUpdate)
  {
    runOta();
  }

  if (doReboot)
  {
    delay(10);
    ESP.restart();
  }
}
