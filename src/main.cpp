#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <Artnet.h>


#define DEBUG false


// Wifi settings
const char* ssid = "aether2G";
const char* password = "0,1%Fett";

// Artnet Settings
Artnet artnet;
const int startUniverse = 0;
const int startChannel = 63;
byte broadcast[] = {192, 168, 178, 255};

// Pixel Settings
const int nPix = 240;
const int pPin = 2;
//NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(nPix); //GPIO_3 (RX)
NeoPixelBus<NeoGrbwFeature, NeoEsp8266Uart800KbpsMethod> strip(nPix, pPin); //GPIO_2 (D4)
RgbwColor SqColor1;
RgbwColor SqColor2;
uint8_t SqAnimSpeed = 200;
uint8_t SqFrequency = 1;
uint8_t SqMode;
uint8_t SqWhiteOver;
NeoPixelAnimator animations(1, NEO_CENTISECONDS);
NeoGamma<NeoGammaTableMethod> colorGamma;

// connect to wifi
boolean ConnectWifi(void) {
  boolean state = true;
  int i = 0;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("");
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(">");
    if (i > 20){
      state = false;
      break;
    }
    i++;
  }
  if (state) {
    Serial.println("");
    Serial.println(ssid);
    uint8_t macAddr[6];
    WiFi.macAddress(macAddr);
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.subnetMask());
    WiFi.hostname("SQFloat");
  } else {
    Serial.println("");
    Serial.println("Fail!");
  }
  return state;
}

void PxlTest(){
  for (int i = 0; i < nPix; ++i){
    strip.SetPixelColor(i, RgbwColor(25,0,0,0));
    strip.Show();
    delay(1);
    strip.SetPixelColor(i, RgbwColor(0,25,0,0));
    strip.Show();
    delay(1);
    strip.SetPixelColor(i, RgbwColor(0,25,0,0));
    strip.Show();
    delay(1);
    strip.SetPixelColor(i, RgbwColor(0,0,25,0));
    strip.Show();
    delay(1);
    strip.SetPixelColor(i, RgbwColor(0,0,0,25));
    strip.Show();
    delay(1);
  }
}

void PxlHblend(RgbwColor ic1, RgbwColor ic2, float phase){
  float ci = 1.0f/nPix;
  HsbColor cc;
  HsbColor c1 = HsbColor(RgbColor(ic1.R, ic1.G, ic1.B));
  HsbColor c2 = HsbColor(RgbColor(ic2.R, ic2.G, ic2.B));
  for(int i = 0; i < nPix; ++i){
    float pos = ci*i*SqFrequency;
    pos -= (int)(pos);
    pos += phase;
    if(pos > 1.0f) pos -= 1.0f; 
    if(c1.H == c2.H) cc = HsbColor::LinearBlend<NeoHueBlendLongestDistance>(c1, c2, pos);
    else cc = HsbColor::LinearBlend<NeoHueBlendClockwiseDirection>(c1, c2, pos);
    RgbColor cco = colorGamma.Correct(RgbColor(cc));
    int w = ic1.W + ((ic2.W-ic1.W)*pos);
    RgbwColor cw = RgbwColor(cco.R,cco.G,cco.B,w);
    strip.SetPixelColor(i, cw);
	}
}

void PxlHblendSym(RgbwColor ic1, RgbwColor ic2, float phase){
  float ci = 1.0f/(nPix);
  HsbColor cc;
  HsbColor c1 = HsbColor(RgbColor(ic1.R, ic1.G, ic1.B));
  HsbColor c2 = HsbColor(RgbColor(ic2.R, ic2.G, ic2.B)); 
  for(int i = 0; i < nPix; ++i){
    float pos = ci*i*SqFrequency;
    pos -= (int)(pos);
    pos += phase;
    if(pos > 1.0f) pos -= 1.0f;
    pos *= 2.0f;
    if(pos > 1.0f) pos = 2.0f-pos; 
    if(c1.H == c2.H) cc = HsbColor::LinearBlend<NeoHueBlendLongestDistance>(c1, c2, pos);
    else cc = HsbColor::LinearBlend<NeoHueBlendClockwiseDirection>(c1, c2, pos);
    RgbColor cco = colorGamma.Correct(RgbColor(cc));
    int w = ic1.W + ((ic2.W-ic1.W)*pos);
    RgbwColor cw = RgbwColor(cco.R,cco.G,cco.B,w);
    strip.SetPixelColor(i, cw);
	}
}

void PxlRGBblendSym(RgbwColor ic1, RgbwColor ic2, float phase){
  float ci = 1.0f/(nPix);
  RgbColor cc;
  RgbColor c1 = RgbColor(ic1.R, ic1.G, ic1.B);
  RgbColor c2 = RgbColor(ic2.R, ic2.G, ic2.B);
  for(int i = 0; i < nPix; ++i){
    float pos = ci*i*SqFrequency;
    pos -= (int)(pos);
    pos += phase;
    if(pos > 1.0f) pos -= 1.0f;
    pos *= 2.0f;
    if(pos > 1.0f) pos = 2.0f-pos; 
    cc = RgbColor::LinearBlend(c1,c2,pos);
    RgbColor cco = colorGamma.Correct(cc);
    int w = ic1.W + ((ic2.W-ic1.W)*pos);
    RgbwColor cw = RgbwColor(cco.R,cco.G,cco.B,w);
    strip.SetPixelColor(i, cw);
	}
}


void PxlRGBblend(RgbwColor ic1, RgbwColor ic2, float phase){
  float ci = 1.0f/(nPix);
  RgbColor cc;
  RgbColor c1 = RgbColor(ic1.R, ic1.G, ic1.B);
  RgbColor c2 = RgbColor(ic2.R, ic2.G, ic2.B);
  for(int i = 0; i < nPix; ++i){
    float pos = ci*i*SqFrequency;
    pos -= (int)(pos);
    pos += phase;
    if(pos > 1.0f) pos -= 1.0f; 
    cc = RgbColor::LinearBlend(c1,c2,pos);
    RgbColor cco = colorGamma.Correct(cc);
    int w = ic1.W + ((ic2.W-ic1.W)*pos);
    RgbwColor cw = RgbwColor(cco.R,cco.G,cco.B,w);
    strip.SetPixelColor(i, cw);
	}
}

void PxlWhite(int w){
  for(int i = 0; i < nPix; ++i){
    strip.SetPixelColor(i, RgbwColor(w));
	}
}

void animUpdate(AnimationParam param)
{
	float progress = param.progress;
  if (SqMode == 0) PxlHblend(SqColor1,SqColor2,progress);
  if (SqMode == 1) PxlHblendSym(SqColor1,SqColor2,progress);
  if (SqMode == 2) PxlRGBblend(SqColor1,SqColor2,progress);
  if (SqMode == 3) PxlRGBblendSym(SqColor1,SqColor2,progress);
}

void SetupAnimations(){
  int period = 0;
  if(SqAnimSpeed > 0){
    period = 5100/SqAnimSpeed;
  }
	animations.StartAnimation(0, period, animUpdate);
}

// Artnet-DMX Call Back
void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data, IPAddress remoteIP)
{
  SqColor1 = RgbwColor(data[startChannel],
                       data[startChannel+1],
                       data[startChannel+2],
                       data[startChannel+3]);
  SqColor2 = RgbwColor(data[startChannel+4],
                       data[startChannel+5],
                       data[startChannel+6],
                       data[startChannel+7]);

  SqMode = data[startChannel+8];

  if (data[startChannel+9] != SqAnimSpeed){
    SqAnimSpeed = data[startChannel+9];
    SetupAnimations();
    Serial.println(SqAnimSpeed);
  }

  SqFrequency = (data[startChannel+10])+1;

  SqWhiteOver = data[startChannel+11];
 //Serial.println(data[startChannel]);

}


void setup() {
  strip.Begin();
  strip.Show();
  // set-up serial for debug output
  Serial.begin(9600);
  // Wifi
  ConnectWifi();
  // Artnet
  artnet.begin();
  artnet.setBroadcast(broadcast);
  artnet.setArtDmxCallback(onDmxFrame);
  //Pixels
  //PxlTest();
  //delay(1000);
  strip.Show();
  delay(1000);
}

void loop() {
  
  if(SqWhiteOver > 0){
    PxlWhite(SqWhiteOver);
  }
  else{
    if (animations.IsAnimating()){
		  	animations.UpdateAnimations();
		}
		else {
			SetupAnimations();
	  }
  }

  uint16_t r = artnet.read();
  if(r == ART_POLL)
  {
    Serial.println("POLL");
  }
  strip.Show();
}