#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp_task_wdt.h>


const char* ssid = "Asura";
const char* password = "30121995@sura";

//Số hiệu của chân điều khiển của ESP32, 1 là số thứ tự led, R, G, B là red green blue
const int ledPin1R = 23;
const int ledPin1G = 22;
const int ledPin1B = 21;

const int ledPin2R = 19;
const int ledPin2G = 18;
const int ledPin2B = 5;

//giá trị RGB đươc chọn trên bảng màu trên giao diện sẽ được lưu vào đây
int redV1 = 255;
int blueV1 = 255;
int greenV1 = 255;

//giá trị thực tế sẽ sáng ra led, được tính toán = công thức ở setBrightness
int realRedV1 = 0;
int realBlueV1 = 0;
int realGreenV1 = 0;


int redV2 = 255;
int blueV2 = 255;
int greenV2 = 255;

int realRedV2 = 0;
int realBlueV2 = 0;
int realGreenV2 = 0;

//biến lưu lại độ sáng hiện thời của 2 led
int brightnessPerc1 = 50;
int brightnessPerc2 = 50;


//biến lưu lại giá trị rgb dạng hex của 2 led
String rgbHex1 = "FFFFFF";
String rgbHex2 = "FFFFFF";

//pwm setting, frequencyHz = số lần nhấp nháy, resolution = bit màu (2^8 = 256)
const int frequencyHz = 1;
const int resolution = 8;

//các channel PWM của ESP32 (từ 0 => 15)
const int redC1 = 0;
const int greenC1 = 1;
const int blueC1 = 2;

const int redC2 = 3;
const int blueC2 = 4;
const int greenC2 = 5;

//lưu trạng thái tắt mở, nhấp nháy của led
bool isOn1 = false;
bool isOn2 = false;
bool isBlinking1 = false;
bool isBlinking2 = false;

//lưu tên của các parameter trong request từ client
const char* INPUT_PARAMETER = "value";
const char* HEX_PARAMETER = "hex";

//khai báo webServer ở port 80
AsyncWebServer webServer(80);

//mã html css js của webserver
const char htmlCode[] PROGMEM =
R"rawliteral(
<!DOCTYPE HTML><html><head> <meta name="viewport" content="width=device-width, initial-scale=1"> <link rel="preconnect" href="https://fonts.googleapis.com"> <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin> <link href="https://fonts.googleapis.com/css2?family=Baloo+2&display=swap" rel="stylesheet"> <link href="https://res.cloudinary.com/dlrdyoxfv/raw/upload/v1646481980/HTN.min_kymfso.css" rel="stylesheet"><script src="https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.0.4/jscolor.min.js"></script> <title>Demo gi&#7919;a k&#7923;</title></head><body> <h1>Demo b&aacute;o c&aacute;o gi&#7919;a k&#7923;</h1> <img width="300px"src="https://i.ibb.co/4jV8PFm/iot.png" alt="a"> <div class="flex-control"> <div> <strong>Led 1</strong> <div class="led-control"> <div class="inline"> <label class="slider0">Ngu&#7891;n</label> <label class="switch" style="margin-right:55px"> <input id= "led1P" type="checkbox" onchange="power(true)" %phPowerChecked1%> <span class="slider2"></span> </label> <label class="slider0">Nh&#7845;p nh&aacute;y</label> <label class="switch"> <input id= "led1B" type="checkbox" onchange="blink(true)"> <span class="slider2"></span> </label> </div> <div class="inline"> <span><img src = "https://i.ibb.co/G9b8Sd1/2985062.png" width = "30px"></span> <input type="range" onchange="updateSliderPWM(this,true)" id="pwmSlider1" min="0" max="100" step="1" class="slider" value=%phBrightness1%> <span class = "textSliderValue" id="textSliderValue1">%phBrightness1%</span> &#37 </div> <div class="inline"> <button class="btn btn-info" onclick="sendRGB(true)" id="change_color" role="button" >&#272;&#7893;i m&agrave;u</button > <input style = "background-color: '#%phRGB1%';" value=%phRGB1% class="form-control jscolor" id="rgb1" readonly/> </div> </div> </div> <div> <strong>Led 2</strong> <div class="led-control"> <div class="inline"> <label class="slider0">Ngu&#7891;n</label> <label class="switch" style="margin-right:55px"> <input id = "led2P"type="checkbox" onchange="power(false)" %phPowerChecked2%> <span class="slider2"></span> </label> <label class="slider0">Nh&#7845;p nh&aacute;y</label> <label class="switch"> <input id = "led2B" type="checkbox" onchange="blink(false)"> <span class="slider2"></span> </label> </div> <div class="inline"> <span><img src = "https://i.ibb.co/G9b8Sd1/2985062.png" width = "30px"></span> <input type="range" onchange="updateSliderPWM(this,false)" id="pwmSlider2" min="0" max="100" step="1" class="slider" value=%phBrightness2%> <span class = "textSliderValue" id="textSliderValue2">%phBrightness2%</span> &#37 </div> <div class="inline"> <button class="btn btn-info" onclick="sendRGB(false)" id="change_color" role="button" >&#272;&#7893;i m&agrave;u</button > <input style = "background-color: '#%phRGB2%';" value=%phRGB2% class="form-control jscolor" id="rgb2" readonly /> </div> </div> </div> </div> <script> var pow1On = "/power1?value=true"; var pow1Off = "/power1?value=false"; var blink1On = "/blink1?value=true"; var blink1Off = "/blink1?value=false"; var pow2On = "/power2?value=true"; var pow2Off = "/power2?value=false"; var blink2On = "/blink2?value=true"; var blink2Off = "/blink2?value=false"; function sendRequest(request) { var httpRequest = new XMLHttpRequest(); httpRequest.open("GET", request, true); httpRequest.send(); console.log(request); } function updateSliderPWM(element, led1) { var pwmSliderValue; var pmwSliderId; var textSliderValue; var sliderRequest; if (led1) { pmwSliderId = "pwmSlider1"; textSliderValue = "textSliderValue1"; sliderRequest = "/brightness1?value="; } else { pmwSliderId = "pwmSlider2"; textSliderValue = "textSliderValue2"; sliderRequest = "/brightness2?value="; } pwmSliderValue = document.getElementById(pmwSliderId).value; document.getElementById(textSliderValue).innerHTML = pwmSliderValue; sendRequest(sliderRequest + pwmSliderValue); } function sendRGB(led1) { var rgbRequest; var rgbObj; var hexV; if (led1) { rgbRequest = "/rgb1?value="; hexV = document.getElementById("rgb1").value; rgbObj = hexToRGB(hexV); } else { rgbRequest = "/rgb2?value="; hexV = document.getElementById("rgb2").value; rgbObj = hexToRGB(hexV); } var rgbString = "r" + rgbObj.red + "g" + rgbObj.green + "b" + rgbObj.blue; var hexString = "&hex=" + hexV; sendRequest(rgbRequest + rgbString + hexString); } function power(led1) { var powerOn; if(led1){ powerOn = document.getElementById("led1P").checked; if(powerOn){ sendRequest(pow1On); } else{ sendRequest(pow1Off); } } else{ powerOn = document.getElementById("led2P").checked; if(powerOn){ sendRequest(pow2On); } else{ sendRequest(pow2Off); } } if (powerOn && led1 && document.getElementById("led1B").checked) { sendRequest(blink1On); }else if(powerOn && !led1 && document.getElementById("led2B").checked){ sendRequest(blink2On); } } function blink(led1) { if(led1){ if(document.getElementById("led1B").checked){ sendRequest(blink1On); } else{ sendRequest(blink1Off); } } else{ if(document.getElementById("led2B").checked){ sendRequest(blink2On); } else{ sendRequest(blink2Off); } } } function hexToRGB(hexColor){ hexColor = parseInt(hexColor,16); return { red: (hexColor >> 16) & 0xFF, green: (hexColor >> 8) & 0xFF, blue: hexColor & 0xFF, }} </script></body></html>
)rawliteral";

//thay thế các place holder (nằm trong cặp ký tự %) thành các giá trị, để hiển thị đúng thông tin khi load lại trang
String showCurrentValues(const String &
   var) {
   if (var == "phBrightness1") {
      return String(brightnessPerc1);
   }
   if (var == "phBrightness2") {
      return String(brightnessPerc2);
   }
   if (var == "phRGB1") {
      return rgbHex1;
   }
   if (var == "phRGB2") {
      return rgbHex2;
   }

   if (var == "phPowerChecked1") {
      if (isOn1) {
         return "checked";
      } else {
         return String();
      }
   }

   if (var == "phPowerChecked2") {
      if (isOn2) {
         return "checked";
      } else {
         return String();
      }
   }
   return String();
}

void setBrightness(int brightness, bool led1) {
   double percent = (double) brightness / (double) 100;
   if (led1) {
      realRedV1 = round((double) redV1 * percent);
      realGreenV1 = round((double) greenV1 * percent);
      realBlueV1 = round((double) blueV1 * percent);

      ledcWrite(redC1, realRedV1);
      ledcWrite(greenC1, realGreenV1);
      ledcWrite(blueC1, realBlueV1);
   } else {
      realRedV2 = round((double) redV2 * percent);
      realGreenV2 = round((double) greenV2 * percent);
      realBlueV2 = round((double) blueV2 * percent);

      ledcWrite(redC2, realRedV2);
      ledcWrite(greenC2, realGreenV2);
      ledcWrite(blueC2, realBlueV2);
   }
}

void ledPowerHandler(AsyncWebServerRequest * request, bool & isOn, int & brightnessPerc, bool led1) {
   String inputMessage;
   Serial.println("entering power method");
   if (request -> hasParam(INPUT_PARAMETER)) {
      inputMessage = request -> getParam(INPUT_PARAMETER) -> value();
      if (inputMessage == "false") {
         isOn = false;
         setBrightness(0, led1);
      } else {
         isOn = true;
         setBrightness(brightnessPerc, led1);
      }
   } else {
      inputMessage = "No message sent";
   }
   request -> send(200, "text/plain", "OK");
}

void ledBlinkingHandler(AsyncWebServerRequest * request, bool & isBlinking, bool & isOn, bool led1) {
   String inputMessage;
   Serial.println("entering blink method");
   if (request -> hasParam(INPUT_PARAMETER)) {
      inputMessage = request -> getParam(INPUT_PARAMETER) -> value();
      if (inputMessage == "false") {
         isBlinking = false;
      } else {
         isBlinking = true;
         Serial.println(isBlinking);
         Serial.println(isOn);
         esp_task_wdt_init(30, false);
         while (isBlinking && isOn) {
            setBrightness(100, led1);
            Serial.println("led blinking on");
            delay(1000);
            setBrightness(0, led1);
            delay(1000);
            Serial.println("led blinking off");
         }
      }
   } else {
      inputMessage = "No message sent";
   }
   request -> send(200, "text/plain", "OK");
}

void ledBrightnessHandler(AsyncWebServerRequest * request, int & brightnessPerc, bool & isOn, bool led1) {
   String inputMessage;
   Serial.println("entering brightness method");
   if (request -> hasParam(INPUT_PARAMETER)) {
      inputMessage = request -> getParam(INPUT_PARAMETER) -> value();
      //update lại giá trị, dù có mở hay ko, để khi mở thì sẽ lấy ngay giá trị mới
      brightnessPerc = inputMessage.toInt();
      if (isOn) {
         setBrightness(brightnessPerc, led1);
      }
   } else {
      inputMessage = "No message sent";
   }
   //reply cho client biết đã nhận và hoàn thành request
   request -> send(200, "text/plain", "OK");
}

void ledRGBHandler(AsyncWebServerRequest * request, String & rgbHex, int & redV, int & greenV, int &blueV, int &brightnessPerc, bool isOn, bool led1) {
   String inputMessage;
   Serial.println("entering rgb method");
   if (request -> hasParam(INPUT_PARAMETER) && request -> hasParam(HEX_PARAMETER)) {
      inputMessage = request -> getParam(INPUT_PARAMETER) -> value();

      rgbHex = request -> getParam(HEX_PARAMETER) -> value();

      int pos1 = inputMessage.indexOf('r');
      int pos2 = inputMessage.indexOf('g');
      int pos3 = inputMessage.indexOf('b');
      int pos4 = inputMessage.indexOf('&');

      redV = inputMessage.substring(pos1 + 1, pos2).toInt();
      greenV = inputMessage.substring(pos2 + 1, pos3).toInt();
      blueV = inputMessage.substring(pos3 + 1, pos4).toInt();
      if(isOn){
        setBrightness(brightnessPerc, led1);
      }
      
   } else {
      inputMessage = "No message sent";
   }
   request -> send(200, "text/plain", "OK");
}

void setup() {
   // Begine Serial Communications over USB
   Serial.begin(115200);
   //gán các thông số frequencyHz và resolution vào các pwm channel
   ledcSetup(redC1, frequencyHz, resolution);
   ledcSetup(greenC1, frequencyHz, resolution);
   ledcSetup(blueC1, frequencyHz, resolution);
   //gán các pwm channel vào các chân của esp32
   ledcAttachPin(ledPin1R, redC1);
   ledcAttachPin(ledPin1G, greenC1);
   ledcAttachPin(ledPin1B, blueC1);

   ledcSetup(redC2, frequencyHz, resolution);
   ledcSetup(greenC2, frequencyHz, resolution);
   ledcSetup(blueC2, frequencyHz, resolution);

   ledcAttachPin(ledPin2R, redC2);
   ledcAttachPin(ledPin2G, greenC2);
   ledcAttachPin(ledPin2B, blueC2);

   //  setBrightness(100,true);
   //  setBrightness(100,false);

   WiFi.begin(ssid, password);
   Serial.print("Connecting to your WiFi");
   while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
   }
   Serial.println("");
   Serial.println("Connected successfully, your device local IP: ");
   Serial.println(WiFi.localIP());

  //định nghĩa hàm sẽ xử lý request tương ứng với các url có tên request định nghĩa từ trước, request cụ thể của url đó sẽ được lưu trong *request
   webServer.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
      request -> send_P(200, "text/html", htmlCode, showCurrentValues);
   });


   webServer.on("/brightness1", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledBrightnessHandler(request, brightnessPerc1, isOn1, true);
   });


   webServer.on("/brightness2", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledBrightnessHandler(request, brightnessPerc2, isOn2, false);
   });

   webServer.on("/power1", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledPowerHandler(request, isOn1, brightnessPerc1, true);
   });

   webServer.on("/power2", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledPowerHandler(request, isOn2, brightnessPerc2, false);
   });

   webServer.on("/blink1", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledBlinkingHandler(request, isBlinking1, isOn1, true);
   });

   webServer.on("/blink2", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledBlinkingHandler(request, isBlinking2, isOn2, false);
   });

   webServer.on("/rgb1", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledRGBHandler(request, rgbHex1, redV1,greenV1,blueV1, brightnessPerc1, isOn1, true);
   });

   webServer.on("/rgb2", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledRGBHandler(request, rgbHex2, redV2,greenV2,blueV2, brightnessPerc2, isOn2, false);
   });

   webServer.begin();
}

void loop() {

}
