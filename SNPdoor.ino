#include <Buzzer.h>
#include <WiFi.h>
#include <Adafruit_MLX90614.h>
#include <Wire.h>
#include <ESP32Servo.h>  // Use the ESP32Servo library
#include <WebServer.h>

// Define the LED pins
#define LED1_PIN 2
#define LED2_PIN 15
#define LED3_PIN 19
#define BUZZER_PIN 17
#define TRIG_PIN 4
#define ECHO_PIN 5
#define SERVO_PIN 16
#define HUMIDITY_SENSOR_PIN 34  // Use GPIO34 for the soil humidity sensor

int wrongAttempts = 0;
unsigned long blockTime = 0; // Time when the block was activated

// Define the password and open/close actions
String doorPassword = "cooking";  // Set your password for opening the door
bool doorOpen = false;

// WiFi credentials
const char* ssid = "OPPO A3s";       // Your WiFi SSID
const char* password = "12345678";  // Your WiFi password

// Create a web server object on port 80
WebServer server(80);

// Create a Servo object
Servo doorServo;
Adafruit_MLX90614 mlx;  // MLX90614 sensor object

void setup() {
  // Start Serial Monitor
  Serial.begin(115200);
  server.on("/door", HTTP_GET, handleDoorPage);
  server.on("/checkcode", HTTP_POST, handleCodeCheck);
  
  // Set LED pins as output
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(SERVO_PIN, OUTPUT);
  pinMode(HUMIDITY_SENSOR_PIN, INPUT);  // Soil humidity sensor pin

  // Set initial states
  digitalWrite(LED1_PIN, LOW); 
  digitalWrite(LED2_PIN, LOW); 
  digitalWrite(LED3_PIN, LOW); 
  digitalWrite(BUZZER_PIN, LOW); 

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Attach servo to the defined pin
  doorServo.attach(SERVO_PIN);
  doorServo.write(0);  // Servo at initial position (closed door)

  // Initialize the MLX90614 sensor
  if (!mlx.begin()) {
    Serial.println("Error connecting to MLX90614 sensor!");
    while (1);
  }

  // Define routes
  server.on("/", HTTP_GET, handleHomePage);
  server.on("/led1", HTTP_GET, handleLed1);
  server.on("/led2", HTTP_GET, handleLed2);
  server.on("/led3", HTTP_GET, handleLed3);
  server.on("/led1off", HTTP_GET, handleLed1Off);
  server.on("/led2off", HTTP_GET, handleLed2Off);
  server.on("/led3off", HTTP_GET, handleLed3Off);
  server.on("/temperature", HTTP_GET, handleTemperature);
  server.on("/humidity", HTTP_GET, handleHumidity);
  server.on("/security", HTTP_GET, handleSecurityMode);

  // Start the server
  server.begin();
  Serial.println("Web server started!");
}

void loop() {
  // Handle client requests
  if (wrongAttempts >= 3 && millis() - blockTime < 30000) {
    digitalWrite(BUZZER_PIN, HIGH);  // Turn on buzzer for 5 seconds
    delay(5000);
    digitalWrite(BUZZER_PIN, LOW);
  }
  server.handleClient();
}

void handleHomePage() {
  String html = "<html><head><style>"
                "body { font-family: Arial; text-align: center; background-color: #f4f4f9; margin: 0; padding: 0; }"
                "h1 { color: #333; }"
                "button { padding: 10px 20px; margin: 10px; font-size: 16px; cursor: pointer; border-radius: 12px; border: none; transition: all 0.3s ease; }"
                "button:hover { background-color: #0056b3; color: white; }"
                "button:focus { outline: none; }"
                "</style></head><body>";
  html += "<h1>ESP32 Control Panel</h1>";
  html += "<button onclick=\"window.location.href='/led1'\">LED 1 ON</button><br><br>";
  html += "<button onclick=\"window.location.href='/led1off'\">LED 1 OFF</button><br><br>";
  html += "<button onclick=\"window.location.href='/led2'\">LED 2 ON</button><br><br>";
  html += "<button onclick=\"window.location.href='/led2off'\">LED 2 OFF</button><br><br>";
  html += "<button onclick=\"window.location.href='/led3'\">LED 3 ON</button><br><br>";
  html += "<button onclick=\"window.location.href='/led3off'\">LED 3 OFF</button><br><br>";
  html += "<button onclick=\"window.location.href='/temperature'\">Temperature</button><br><br>";
  html += "<button onclick=\"window.location.href='/humidity'\">Soil Humidity</button><br><br>";
  html += "<button onclick=\"window.location.href='/security'\">Security Mode</button><br><br>";
  html += "<button onclick=\"window.location.href='/door'\">Open/Close Door</button><br><br>"; // Added door control button
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleDoorPage() {
  String html = "<html><body><h1>Enter Password to Unlock Door:</h1>";
  html += "<form method='POST' action='/checkcode'>";
  html += "<input type='text' name='password' placeholder='Password'><br><br>";
  html += "<button type='submit'>Submit</button>";
  html += "</form><br>";
  html += "<form method='GET' action='/closedoor'>";
  html += "<button type='submit'>Close Door</button>";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleCodeCheck() {
  String inputPassword = server.arg("password");
  
  if (inputPassword == doorPassword) {
    if (wrongAttempts >= 3 && millis() - blockTime < 30000) {
      server.send(200, "text/html", "<html><body><h2>Too many failed attempts. Please try again later.</h2></body></html>");
      return;
    }

    // Open door logic (servo movement)
    doorServo.write(90);  // Open door
    delay(3000);  // Door remains open for 3 seconds
    doorServo.write(0);  // Close door

    server.send(200, "text/html", "<html><body><h2>Door Opened Successfully!</h2></body></html>");
    wrongAttempts = 0; // Reset wrong attempts after success
  } else {
    wrongAttempts++;
    if (wrongAttempts >= 3) {
      blockTime = millis(); // Block user for 30 seconds
    }
    server.send(200, "text/html", "<html><body><h2>Incorrect Password!</h2></body></html>");
  }
}

void handleCloseDoor() {
  doorServo.write(0);  // Close the door
  server.send(200, "text/html", "<html><body><h2>Door Closed!</h2></body></html>");
}

// LED Control Handlers
void handleLed1() {
  digitalWrite(LED1_PIN, HIGH);
  server.send(200, "text/html", "<html><body><h2>LED 1 is ON</h2><button onclick=\"window.location.href='/'\">Back</button></body></html>");
}

void handleLed2() {
  digitalWrite(LED2_PIN, HIGH);
  server.send(200, "text/html", "<html><body><h2>LED 2 is ON</h2><button onclick=\"window.location.href='/'\">Back</button></body></html>");
}

void handleLed3() {
  digitalWrite(LED3_PIN, HIGH);
  server.send(200, "text/html", "<html><body><h2>LED 3 is ON</h2><button onclick=\"window.location.href='/'\">Back</button></body></html>");
}

// LED Off Handlers
void handleLed1Off() {
  digitalWrite(LED1_PIN, LOW);
  server.send(200, "text/html", "<html><body><h2>LED 1 is OFF</h2><button onclick=\"window.location.href='/'\">Back</button></body></html>");
}

void handleLed2Off() {
  digitalWrite(LED2_PIN, LOW);
  server.send(200, "text/html", "<html><body><h2>LED 2 is OFF</h2><button onclick=\"window.location.href='/'\">Back</button></body></html>");
}

void handleLed3Off() {
  digitalWrite(LED3_PIN, LOW);
  server.send(200, "text/html", "<html><body><h2>LED 3 is OFF</h2><button onclick=\"window.location.href='/'\">Back</button></body></html>");
}

// Temperature Handler
void handleTemperature() {
  float temp = mlx.readObjectTempC();  // Read the temperature in Celsius
  String html = "<html><body><h2>Temperature: " + String(temp) + "°C</h2>";
  html += "<button onclick=\"window.location.href='/'\">Back</button></body></html>";
  server.send(200, "text/html", html);
}

// Soil Humidity Handler
void handleHumidity() {
  int humidityValue = analogRead(HUMIDITY_SENSOR_PIN);  // Read the analog value from the soil humidity sensor
  float humidity = map(humidityValue, 0, 4095, 0, 100);  // Map the value to a percentage (0-100%)
  String html = "<html><body><h2>Soil Humidity: " + String(humidity) + "%</h2>";
  html += "<button onclick=\"window.location.href='/'\">Back</button></body></html>";
  server.send(200, "text/html", html);
}

void handleSecurityMode() {
  // Add security logic here (e.g., enable buzzer, detect unauthorized access, etc.)
  server.send(200, "text/html", "<html><body><h2>Security Mode Activated!</h2><button onclick=\"window.location.href='/'\">Back</button></body></html>");
}
