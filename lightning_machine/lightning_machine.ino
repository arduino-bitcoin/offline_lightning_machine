// Font file is stored in SPIFFS
#define FS_NO_GLOBALS
#include <FS.h>

// Graphics and font library
#include <TFT_eSPI.h>
#include <SPI.h>

#include <Adafruit_STMPE610.h>
#include <qrcode.h>

#include <Bitcoin.h>
#include <Lightning.h>
#include <Hash.h>

#define STMPE_CS 32
#define TFT_CS   15
#define TFT_DC   33
#define SD_CS    14
#define TFT_RST -1

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 3800
#define TS_MAXX 100
#define TS_MINY 100
#define TS_MAXY 3750

#define BAT_PIN A0

bool batState = false;

TFT_eSPI tft = TFT_eSPI();  // Invoke library
// Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

// rgb to 16-bit color
word RGBColor( byte R, byte G, byte B){
  return ( ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3) );
}

word Q_BLACK = RGBColor(0,0,0);
word Q_GRAY = RGBColor(0x4A, 0x4A, 0x4A);
word Q_WHITE = RGBColor(0xFF, 0xFF, 0xFF);
word Q_BLUE = RGBColor(74,144,226);
word Q_ORANGE = RGBColor(0xFF, 0x8E, 0x00);
word PRIMARY = RGBColor(0x00, 0x77, 0xff);
word PRIMARY_GLOW = RGBColor(0xBE, 0xDB, 0xFF);

void fillPinCircles(int num){
	for(int i=0; i<4; i++){
		int dx = 160+40*(i-2)+20;
		tft.fillCircle(dx, 180, 8, PRIMARY_GLOW);
		tft.fillCircle(dx, 180, 7, PRIMARY);
		if(i>=num){
			tft.fillCircle(dx, 180, 5, PRIMARY_GLOW);
			tft.fillCircle(dx, 180, 4, Q_WHITE);
		}
	}
}

int pinScreen(int code){

	tft.fillScreen(Q_WHITE);
	tft.loadFont("Lato-Bold20");

	tft.setTextColor(Q_BLACK, Q_WHITE);

	tft.setCursor(72, 20);
	tft.print("Enter your code");

	// pin circles
	fillPinCircles(0);

	char numpad[] = "123456789C0<";
	for(int i=0; i<strlen(numpad); i++){
		tft.setCursor(145+95*(i%3-1), 260+60*(i/3));
		tft.print(numpad[i]);
	}
	int pin = 0;
	int num = 0;

	tft.unloadFont();

	while(1){
		// checking touchscreen
		if(!ts.bufferEmpty()){
			TS_Point p = ts.getPoint();
			delay(300);
			// empty buffer
			while(!ts.bufferEmpty()){
				ts.getPoint();
			}
		    p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
		    p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
		    if(p.y > 220){
		    	int v = 1 + p.x * 3 / tft.width() + 3* ((p.y-220)/60);
			    if(v < 10){ // number
			    	num ++;
			    	pin = pin*10 + v;
			    }else if(v == 11){ // 0
			    	num ++;
			    	pin = pin*10;
			    }else if(v == 10){ // cancel
			    	return -1;
			    }else if(v == 12 && num > 0){
			    	num --;
			    	pin = pin / 10;
			    }
			    // Serial.println(pin);
				fillPinCircles(num);
		    }
		    if(num == 4){
		    	if(pin == code){
		    		return 1;
		    	}else{
		    		num = 0;
		    		pin = 0;
					fillPinCircles(num);
		    	}
		    }
		}
		delay(100);
	}
	return -1;
}

void showQRcode(char * text){
  // Start time
  uint32_t dt = millis();

  tft.fillScreen(Q_WHITE);
  tft.loadFont("Lato-Bold20");

  tft.setTextColor(Q_BLACK, Q_WHITE);

  tft.setCursor(72, 20);
  tft.print("Pay 100 satoshi to");
  tft.setCursor(62, 50);
  tft.print("TOGGLE the Bat-Signal");

  tft.setCursor(40, 420);
  tft.print("New invoice");
  tft.setCursor(190, 420);
  tft.print("Enter code");

  tft.unloadFont();

  int qrSize = 10;
  int sizes[17] = { 14, 26, 42, 62, 84, 106, 122, 152, 180, 213, 251, 287, 331, 362, 412, 480, 504 };
  int len = strlen(text);
  // Serial.println(len);
  for(int i=0; i<17; i++){
    if(sizes[i] > len){
      qrSize = i+1;
      break;
    }
  }
  // Serial.println(qrSize);
  // Create the QR code
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(qrSize)];
  qrcode_initText(&qrcode, qrcodeData, qrSize, 1, text);

  // Delta time
  // dt = millis() - dt;
  // Serial.print("QR Code Generation Time: ");
  // Serial.print(dt);
  // Serial.print("\n");

  int width = 17 + 4*qrSize;
  int scale = 300/width;
  int padding = (320 - width*scale)/2;
  for (uint8_t y = 0; y < qrcode.size; y++) {
      for (uint8_t x = 0; x < qrcode.size; x++) {
          if(qrcode_getModule(&qrcode, x, y)){
            tft.fillRect(padding+scale*x, 100+scale*y, scale, scale, Q_BLACK);
          }
      }
  }
}

bool askPayment(char * text, int code){
	showQRcode(text);

	while(1){
		// checking touchscreen
		if(!ts.bufferEmpty()){
			TS_Point p = ts.getPoint();
			delay(300);
			// empty buffer
			while(!ts.bufferEmpty()){
				ts.getPoint();
			}
		    p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
		    p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
		    Serial.println(p.x);
		    if(p.x > 160){
		    	break;
		    }else{
		    	return false;
		    }
		}
		delay(100);
	}

	int result = pinScreen(code);
	if(result <= 0){
		return askPayment(text, code);
	}
  	return (result > 0);
}

byte nodeSecret[] = {
	0x20, 0x43, 0xaa, 0xae, 0xc1, 0x5b, 0x49, 0xd1, 
	0x3b, 0x04, 0xc0, 0x07, 0xd7, 0xf4, 0xba, 0x1f, 
	0xa3, 0x5a, 0x21, 0xeb, 0xa9, 0x26, 0xcb, 0x4d, 
	0xd0, 0x4f, 0x06, 0xdb, 0xbf, 0x51, 0x49, 0x33
};
bool testnet = true;
PrivateKey nodeKey(nodeSecret, true, testnet);

uint16_t count = 23;

int getPreimage(uint16_t i, byte preimage[32]){
	byte msg[2];
	msg[0] = (i & 0xFF);
	msg[1] = (i>>8);
	SHA256 h;
	h.beginHMAC(nodeSecret, 32);
	h.write(msg, 2);
	h.endHMAC(preimage);
	preimage[0] = ((preimage[0] % 10)<<4);
	preimage[0] += (preimage[1] % 10);
	preimage[1] = ((preimage[2] % 10)<<4);
	preimage[1] += (preimage[3] % 10);
	preimage[2] = 0xcc;
	preimage[3] = 0xcc;
	int code = (preimage[0] >> 4)*1000 + (preimage[0] & 0x0F)*100 + (preimage[1] >> 4)*10 + (preimage[1] & 0x0F);
	return code;
}

LightningInvoice generateInvoice(uint16_t i = 0){
	byte preimage[32] = { 0 };
	int code = getPreimage(i, preimage);
	char description[] = "Toggle Bat-Signal!";
	LightningInvoice invoice(
        description, preimage, 1537957842,
        1, 'u', testnet);
	invoice.setExpiry(100000);
	byte cltvExpiry[] = {24, 0, 1, 10};
	invoice.addRawData(cltvExpiry, sizeof(cltvExpiry));
	invoice.sign(nodeKey);
	return invoice;
}

void setup(){
	pinMode(BAT_PIN, OUTPUT);
	digitalWrite(BAT_PIN, batState);

	delay(1000);
	Serial.begin(9600);

	if (!ts.begin()) {
		Serial.println("Couldn't start touchscreen controller");
		while (1);
	}
	Serial.println("Touchscreen started");
	// tft.begin(HX8357D);
	tft.init();
	tft.setRotation(0);

	if (!SPIFFS.begin()) {
		Serial.println("SPIFFS initialisation failed!");
		while (1) yield(); // Stay here twiddling thumbs waiting
	}
	tft.setTextWrap(true, true);
	while(!ts.bufferEmpty()){
		ts.getPoint();
	}
}

void loop(){
	byte preimage[32] = { 0 };
	int code = getPreimage(count, preimage);
	Serial.println(code);
	Serial.println(toHex(preimage, 32));

	LightningInvoice invoice = generateInvoice(count);
	Serial.print("count: ");
	Serial.println(count);
	count ++;
	char arr[300] = "invoice";
	invoice.toCharArray(arr, sizeof(arr));
	bool userPaid = askPayment(arr, code);
	if(userPaid){
		Serial.println("Toggle!");
		batState = !batState;
		digitalWrite(BAT_PIN, batState);
	}else{
		Serial.println("New invoice");
	}
}