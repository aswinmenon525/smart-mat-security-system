/*
 * ============================================================
 *   Smart Mat Security System
 * ============================================================
 *   Author      : Aswin (VIT Vellore, ECE - 24BEC0525)
 *   Board       : ESP32
 *   Description : FSR pressure mat triggers face recognition
 *                 via browser. Known persons open the door;
 *                 unknown persons are denied. Gmail alerts sent
 *                 via Google Apps Script for every event.
 *
 *   Hardware:
 *     - ESP32 Dev Board
 *     - 4x FSR (Force Sensitive Resistor) sensors
 *     - PIR motion sensor
 *     - Buzzer
 *
 *   Dependencies:
 *     - WiFi.h         (built-in ESP32)
 *     - WebServer.h    (built-in ESP32)
 *     - HTTPClient.h   (built-in ESP32)
 *     - face-api.js    (loaded via CDN in browser)
 *
 *   Setup:
 *     1. Copy config.h.example to config.h
 *     2. Fill in your WiFi credentials and Google Script URL
 *     3. Upload to ESP32
 *     4. Open Serial Monitor at 115200 baud
 *     5. Navigate to the IP shown → click "Open Face Scan"
 * ============================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

// ============================================================
//   CONFIGURATION  —  edit config.h (do NOT hardcode here)
// ============================================================
// Create a file called config.h with:
//   #define WIFI_SSID      "your_wifi_name"
//   #define WIFI_PASSWORD  "your_wifi_password"
//   #define SCRIPT_URL     "your_google_apps_script_url"
//   #define OWNER_PHOTO    "your_photo_url"

#include "config.h"

const char* ssid       = WIFI_SSID;
const char* password   = WIFI_PASSWORD;
const char* scriptURL  = SCRIPT_URL;
const char* ownerPhoto = OWNER_PHOTO;

// ============================================================
//   PIN DEFINITIONS
// ============================================================
#define FSR1   34
#define FSR2   35
#define FSR3   32
#define FSR4   33
#define PIR    25
#define BUZZER 14

// ============================================================
//   THRESHOLDS
// ============================================================
int THRESHOLD = 150;

// ============================================================
//   GLOBALS
// ============================================================
WebServer server(80);
bool matTriggered = false;
int  savedF1 = 0, savedF2 = 0, savedF3 = 0, savedF4 = 0;

// ============================================================
//   GMAIL ALERT
// ============================================================
void sendGmail(int f1, int f2, int f3, int f4, String type) {
  Serial.println("Sending Gmail...");
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected - Gmail skipped");
    return;
  }
  type.replace(" ", "%20");
  HTTPClient http;
  String url = String(scriptURL) +
               "?fsr1=" + String(f1) +
               "&fsr2=" + String(f2) +
               "&fsr3=" + String(f3) +
               "&fsr4=" + String(f4) +
               "&type=" + type;
  Serial.println("URL: " + url);
  http.begin(url);
  http.setTimeout(8000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int code = http.GET();
  Serial.println("Gmail HTTP code: " + String(code));
  if (code > 0) {
    String response = http.getString();
    Serial.println("Response: " + response);
  } else {
    Serial.println("Error: " + http.errorToString(code));
  }
  http.end();
  Serial.println("Gmail done");
}

// ============================================================
//   DOOR ANIMATION PAGE
// ============================================================
String getDoorAnimationHTML() {
  String html = "";
  html += "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Door Opening</title>";
  html += "<style>";
  html += "body{margin:0;background:#0a0a0a;color:#fff;font-family:sans-serif;overflow:hidden;}";
  html += ".container{display:flex;flex-direction:column;align-items:center;justify-content:center;height:100vh;}";
  html += ".door-frame{position:relative;width:300px;height:500px;background:#2a2a2a;border:8px solid #444;border-radius:12px;overflow:hidden;box-shadow:0 10px 50px rgba(0,0,0,0.5);}";
  html += ".door-left,.door-right{position:absolute;top:0;width:50%;height:100%;background:linear-gradient(to right,#8B4513,#A0522D);transition:transform 2s ease-in-out;}";
  html += ".door-left{left:0;border-right:4px solid #654321;transform-origin:left;}";
  html += ".door-right{right:0;border-left:4px solid #654321;transform-origin:right;}";
  html += ".door-left.open{transform:perspective(600px) rotateY(-120deg);}";
  html += ".door-right.open{transform:perspective(600px) rotateY(120deg);}";
  html += ".handle{position:absolute;top:50%;width:40px;height:12px;background:#FFD700;border-radius:6px;}";
  html += ".door-left .handle{right:20px;transform:translateY(-50%);}";
  html += ".door-right .handle{left:20px;transform:translateY(-50%);}";
  html += ".status{margin-top:30px;font-size:1.4em;font-weight:bold;text-align:center;}";
  html += ".welcome{color:#00ff00;animation:pulse 1.5s infinite;}";
  html += "@keyframes pulse{0%,100%{opacity:1;}50%{opacity:0.6;}}";
  html += ".light{position:absolute;top:50%;left:50%;width:200px;height:200px;background:radial-gradient(circle,rgba(0,255,0,0.3),transparent);transform:translate(-50%,-50%);opacity:0;transition:opacity 1s;}";
  html += ".light.on{opacity:1;}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<div class='door-frame'>";
  html += "<div class='light' id='light'></div>";
  html += "<div class='door-left' id='doorLeft'><div class='handle'></div></div>";
  html += "<div class='door-right' id='doorRight'><div class='handle'></div></div>";
  html += "</div>";
  html += "<div class='status' id='status'>Access Granted!</div>";
  html += "</div>";
  html += "<script>";
  html += "setTimeout(function(){";
  html += "  document.getElementById('doorLeft').classList.add('open');";
  html += "  document.getElementById('doorRight').classList.add('open');";
  html += "  document.getElementById('light').classList.add('on');";
  html += "  document.getElementById('status').textContent='Welcome!';";
  html += "  document.getElementById('status').classList.add('welcome');";
  html += "},500);";
  html += "setTimeout(function(){";
  html += "  document.getElementById('status').textContent='Door will close in 5 seconds...';";
  html += "},3000);";
  html += "setTimeout(function(){";
  html += "  document.getElementById('doorLeft').classList.remove('open');";
  html += "  document.getElementById('doorRight').classList.remove('open');";
  html += "  document.getElementById('light').classList.remove('on');";
  html += "  document.getElementById('status').textContent='Door Closed. Have a great day!';";
  html += "  document.getElementById('status').classList.remove('welcome');";
  html += "},8000);";
  html += "setTimeout(function(){window.location.href='/scan';},11000);";
  html += "</script></body></html>";
  return html;
}

// ============================================================
//   FACE SCAN PAGE
// ============================================================
String getFaceScanHTML() {
  String ip    = WiFi.localIP().toString();
  String photo = String(ownerPhoto);

  String html = "";
  html += "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Face Scan</title>";
  html += "<script src='https://cdn.jsdelivr.net/npm/face-api.js@0.22.2/dist/face-api.min.js'></script>";
  html += "<style>";
  html += "body{margin:0;background:#111;color:#fff;font-family:sans-serif;text-align:center;padding:20px;}";
  html += "video{border-radius:12px;width:90%;max-width:380px;display:none;}";
  html += "#status{font-size:1.1em;margin:14px auto;padding:14px;border-radius:10px;background:#333;max-width:380px;}";
  html += "#waiting{font-size:1.2em;margin:40px auto;padding:20px;color:#aaa;max-width:380px;}";
  html += ".dot{display:inline-block;animation:blink 1.4s infinite;font-size:2em;}";
  html += ".dot:nth-child(2){animation-delay:0.2s;}";
  html += ".dot:nth-child(3){animation-delay:0.4s;}";
  html += "@keyframes blink{0%,80%,100%{opacity:0;}40%{opacity:1;}}";
  html += ".ok{background:#1a5c1a;}.deny{background:#5c1a1a;}";
  html += "</style></head><body>";
  html += "<h2>Smart Mat</h2>";

  html += "<div id='waiting'>";
  html += "<p>Waiting for someone to step on the mat</p>";
  html += "<span class='dot'>.</span><span class='dot'>.</span><span class='dot'>.</span>";
  html += "</div>";

  html += "<div id='scanArea' style='display:none'>";
  html += "<p style='color:#aaa;margin-bottom:8px'>Mat detected! Scanning face...</p>";
  html += "<video id='v' autoplay muted playsinline></video>";
  html += "<div id='status'>Starting camera...</div>";
  html += "</div>";

  html += "<script>";
  html += "var V=document.getElementById('v');";
  html += "var S=document.getElementById('status');";
  html += "var HOST='http://" + ip + "';";
  html += "var PHOTO='" + photo + "';";
  html += "var ref=null, busy=false, scanning=false, modelsLoaded=false;";

  html += "async function loadModels(){";
  html += "  try{";
  html += "    var M='https://raw.githubusercontent.com/justadudewhohacks/face-api.js/master/weights';";
  html += "    await faceapi.nets.tinyFaceDetector.loadFromUri(M);";
  html += "    await faceapi.nets.faceLandmark68TinyNet.loadFromUri(M);";
  html += "    await faceapi.nets.faceRecognitionNet.loadFromUri(M);";
  html += "    var img=await faceapi.fetchImage(PHOTO);";
  html += "    var det=await faceapi.detectSingleFace(img,new faceapi.TinyFaceDetectorOptions()).withFaceLandmarks(true).withFaceDescriptor();";
  html += "    if(!det){console.log('No face in photo');return;}";
  html += "    ref=det.descriptor;";
  html += "    modelsLoaded=true;";
  html += "    console.log('Models ready, waiting for mat...');";
  html += "  }catch(err){console.log('Model error: '+err.message);}";
  html += "}";

  html += "async function pollMat(){";
  html += "  try{";
  html += "    var r=await fetch(HOST+'/status');";
  html += "    var t=await r.text();";
  html += "    if(t==='TRIGGERED' && !scanning){";
  html += "      scanning=true;";
  html += "      startScan();";
  html += "    }";
  html += "  }catch(e){}";
  html += "  setTimeout(pollMat,1000);";
  html += "}";

  html += "async function startScan(){";
  html += "  document.getElementById('waiting').style.display='none';";
  html += "  document.getElementById('scanArea').style.display='block';";
  html += "  document.getElementById('v').style.display='block';";
  html += "  S.textContent='Starting camera...';";
  html += "  if(!modelsLoaded){S.textContent='Loading models... please wait';";
  html += "    await loadModels();}";
  html += "  try{";
  html += "    var stream=await navigator.mediaDevices.getUserMedia({video:{facingMode:'user'}});";
  html += "    V.srcObject=stream;";
  html += "    V.onloadedmetadata=function(){S.textContent='Look at the camera...';scan();};";
  html += "  }catch(err){S.textContent='Camera error: '+err.message;}";
  html += "}";

  html += "async function scan(){";
  html += "  if(busy)return;";
  html += "  busy=true;";
  html += "  try{";
  html += "    var det=await faceapi.detectSingleFace(V,new faceapi.TinyFaceDetectorOptions()).withFaceLandmarks(true).withFaceDescriptor();";
  html += "    if(det){";
  html += "      var dist=faceapi.euclideanDistance(ref,det.descriptor);";
  html += "      console.log('Distance: '+dist);";
  html += "      if(dist<0.5){";
  html += "        S.textContent='Face matched! Opening door...';";
  html += "        S.className='ok';";
  html += "        await fetch(HOST+'/open');";
  html += "        setTimeout(function(){window.location.href=HOST+'/door';},1000);";
  html += "        return;";
  html += "      }else{";
  html += "        S.textContent='Not recognized (score: '+dist.toFixed(2)+'). Try again.';";
  html += "        S.className='deny';";
  html += "        await fetch(HOST+'/deny');";
  html += "        setTimeout(function(){S.textContent='Look at camera...';S.className='';busy=false;scan();},3000);";
  html += "        return;";
  html += "      }";
  html += "    }else{";
  html += "      S.textContent='No face detected. Position your face clearly.';";
  html += "    }";
  html += "  }catch(err){S.textContent='Scan error: '+err.message;}";
  html += "  busy=false;";
  html += "  setTimeout(scan,800);";
  html += "}";

  html += "loadModels();";
  html += "pollMat();";
  html += "</script></body></html>";
  return html;
}

// ============================================================
//   WEB SERVER ROUTES
// ============================================================
void handleRoot() {
  server.send(200, "text/html",
    "<html><body style='font-family:sans-serif;text-align:center;"
    "padding:30px;background:#111;color:#fff'>"
    "<h2>Smart Mat Online</h2>"
    "<a href='/scan'><button style='font-size:1.1em;padding:14px 32px;"
    "background:#1a5c1a;color:#fff;border:none;border-radius:10px;cursor:pointer'>"
    "Open Face Scan</button></a>"
    "</body></html>"
  );
}

void handleScanPage()  { server.send(200, "text/html", getFaceScanHTML()); }
void handleDoorPage()  { server.send(200, "text/html", getDoorAnimationHTML()); }

void handleStatus() {
  server.send(200, "text/plain", matTriggered ? "TRIGGERED" : "WAITING");
}

void handleDoorOpen() {
  server.send(200, "text/plain", "Door opened!");
  tone(BUZZER, 1500, 100); delay(150);
  tone(BUZZER, 2200, 250); delay(300);
  noTone(BUZZER);
  Serial.println("==============================");
  Serial.println("KNOWN PERSON RECOGNIZED — DOOR OPENED");
  sendGmail(savedF1, savedF2, savedF3, savedF4, "Known Person");
  matTriggered = false;
  Serial.println("==============================");
}

void handleDoorDeny() {
  server.send(200, "text/plain", "Access denied.");
  tone(BUZZER, 400, 800); delay(900);
  noTone(BUZZER);
  Serial.println("==============================");
  Serial.println("UNKNOWN PERSON — ACCESS DENIED");
  sendGmail(savedF1, savedF2, savedF3, savedF4, "Unknown Person");
  matTriggered = false;
  Serial.println("==============================");
}

// ============================================================
//   SETUP
// ============================================================
void setup() {
  Serial.begin(115200);

  pinMode(PIR,    INPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println("IP: http://" + WiFi.localIP().toString());

  server.on("/",       handleRoot);
  server.on("/scan",   handleScanPage);
  server.on("/door",   handleDoorPage);
  server.on("/status", handleStatus);
  server.on("/open",   handleDoorOpen);
  server.on("/deny",   handleDoorDeny);
  server.begin();

  Serial.println("Web server started");
  Serial.println("Open /scan on your browser and keep it open");

  tone(BUZZER, 1000, 100); delay(150);
  tone(BUZZER, 1500, 100); delay(200);
  noTone(BUZZER);
}

// ============================================================
//   MAIN LOOP
// ============================================================
void loop() {
  server.handleClient();

  int f1 = analogRead(FSR1);
  int f2 = analogRead(FSR2);
  int f3 = analogRead(FSR3);
  int f4 = analogRead(FSR4);

  if ((f1 > THRESHOLD || f2 > THRESHOLD || f3 > THRESHOLD || f4 > THRESHOLD) && !matTriggered) {
    delay(300); // debounce

    savedF1 = analogRead(FSR1);
    savedF2 = analogRead(FSR2);
    savedF3 = analogRead(FSR3);
    savedF4 = analogRead(FSR4);

    matTriggered = true;

    Serial.println("==============================");
    Serial.println("MAT STEPPED ON");
    Serial.printf("FSR Values: %d, %d, %d, %d\n", savedF1, savedF2, savedF3, savedF4);
    Serial.println("Face scan triggered");
    Serial.println("==============================");

    tone(BUZZER, 1200, 200);
    delay(250);
    noTone(BUZZER);
  }

  delay(200);
}
