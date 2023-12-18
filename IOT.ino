#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <deprecated.h>
#include <require_cpp11.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>

// Chân của cảm biến rfc522
#define SS_PIN 5
#define RST_PIN 3
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Tạo MFRC522 instance.

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int DHT_PIN = 15;

// Chân điều khiển cửa
const int motor1Pin2 = 26;
const int motor1Pin1 = 27;
const int openDoorBtn = 4;
const int openDoorState = 12;
const int closeDoorBtn = 13;
const int closeDoorState = 14;

// Chân điều khiển dàn phơi
const int motor2Pin1 = 25;
const int motor2Pin2 = 33;
const int rainSensor = 16;
const int closeDryingCurtainState = 32;
const int openDryingCurtainState = 17;
int DryingCurtainState = LOW;

// Chân điều khiển đèn
const int ledPin = 2;
const int ledControlBtn = 35;
int ledState = LOW;          // the current state of LED
int led_button_state = LOW;  // the current state of button
int led_last_button_state;


DHTesp dht;

// Ký tự độ C
byte degree[8] = {
  0B01110,
  0B01010,
  0B01110,
  0B00000,
  0B00000,
  0B00000,
  0B00000,
  0B00000
};

// Địa chỉ kết nối mạng và máy chủ mqtt
const char* ssid = "Psyado";
const char* password = "0356840373";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
float temp = 0;
float hum = 0;

// hàm kết nối wifi
void setup_wifi() {
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// hàm xử lý các lệnh từ mqtt server
void callback(char* topic, byte* payload, unsigned int length) {
  String temp;
  // Serial.print("Message arrived [");
  // Serial.print(topic);
  // Serial.print("] ");
  for (int i = 0; i < length; i++) {
    temp += (char)payload[i];
  }
  // Serial.println(temp);
  // Lệnh điều khiển đèn
  if (strcmp_P(topic, "/PTIT_Test/vinh/turnLED") == 0) {
    if (temp == "ON") {
      digitalWrite(ledPin, HIGH);
    } else {
      digitalWrite(ledPin, LOW);
    }
  }
  // Lệnh điều khiển cửa
  if (strcmp_P(topic, "/PTIT_Test/vinh/door") == 0) {
    if (temp == "OPEN") {
      while (digitalRead(openDoorState) == HIGH) {
        digitalWrite(motor1Pin2, LOW);
        digitalWrite(motor1Pin1, HIGH);
        delay(30);
      }
      digitalWrite(motor1Pin1, LOW);
      client.publish("/PTIT_Test/vinh/door", "OPENED");
    } else if (temp == "CLOSE") {
      while (digitalRead(closeDoorState) == HIGH) {
        digitalWrite(motor1Pin1, LOW);
        digitalWrite(motor1Pin2, HIGH);
        delay(30);
      }
      digitalWrite(motor1Pin2, LOW);
      client.publish("/PTIT_Test/vinh/door", "CLOSED");
    } else {
      digitalWrite(motor1Pin1, LOW);
      digitalWrite(motor1Pin2, LOW);
    }
  }
  // Lệnh điều khiển dàn phơi quần áo
  if (strcmp_P(topic, "/PTIT_Test/vinh/drying_curtain") == 0) {
    if (temp == "OPEN") {
      while (digitalRead(openDryingCurtainState) == HIGH) {
        digitalWrite(motor2Pin1, LOW);
        digitalWrite(motor2Pin2, HIGH);
        delay(30);
      }
      digitalWrite(motor2Pin2, LOW);
      DryingCurtainState = HIGH;
      client.publish("/PTIT_Test/vinh/drying_curtain", "OPENED");
    } else if (temp == "CLOSE") {
      while (digitalRead(closeDryingCurtainState) == HIGH) {
        digitalWrite(motor2Pin2, LOW);
        digitalWrite(motor2Pin1, HIGH);
        delay(30);
      }
      digitalWrite(motor2Pin1, LOW);
      DryingCurtainState = LOW;
      client.publish("/PTIT_Test/vinh/drying_curtain", "CLOSED");
    } else {
      digitalWrite(motor2Pin1, LOW);
      digitalWrite(motor2Pin2, LOW);
    }
  }
}

// kết nối đến 
void reconnect() {
  // Lặp lại đến khi kết nối thành công
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Thực hiện kết nối
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected to " + clientId);
      // Sau khi kết nối, xuất thông báo
      client.publish("/PTIT_Test/vinh/mqtt", "PTIT_Test");
      // ... và đăng ký lại
      client.subscribe("/PTIT_Test/vinh/mqtt");
      client.subscribe("/PTIT_Test/vinh/temp");
      client.subscribe("/PTIT_Test/vinh/hum");
      client.subscribe("/PTIT_Test/vinh/turnLED");
      client.subscribe("/PTIT_Test/vinh/door");
      client.subscribe("/PTIT_Test/vinh/rain");
      client.subscribe("/PTIT_Test/vinh/drying_curtain");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Đợi 5s trước khi kết nối lại
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  // khởi tạo lcd
  lcd.init();
  lcd.backlight();
  // chân điều khiển cửa
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(openDoorBtn, INPUT_PULLUP);
  pinMode(openDoorState, INPUT_PULLUP);
  pinMode(closeDoorBtn, INPUT_PULLUP);
  pinMode(closeDoorState, INPUT_PULLUP);
  // chân điều khiển đèn
  pinMode(ledPin, OUTPUT);
  pinMode(ledControlBtn, INPUT_PULLUP);
  led_button_state = digitalRead(ledControlBtn);
  // chân giàn phơi
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);
  pinMode(closeDryingCurtainState, INPUT_PULLUP);
  pinMode(openDryingCurtainState, INPUT_PULLUP);
  // chân cảm biến mưa
  pinMode(rainSensor, INPUT);
  // setup wifi và kết nối mqtt server
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  // setup cảm biến nhiệt độ, độ ẩm
  dht.setup(DHT_PIN, DHTesp::DHT11);
  lcd.createChar(1, degree);
  // cảm biến rfc522
  SPI.begin();         // Initiate  SPI bus
  mfrc522.PCD_Init();  // Initiate MFRC522
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    TempAndHumidity data = dht.getTempAndHumidity();
    // cập nhật nhiệt độ lên mqtt server
    String temp = String(data.temperature, 2);
    client.publish("/PTIT_Test/vinh/temp", temp.c_str());
    // cập nhật độ ẩm lên mqtt server
    String hum = String(data.humidity, 1);
    client.publish("/PTIT_Test/vinh/hum", hum.c_str());
    // cập nhật trạng thái mưa
    int rain_state = digitalRead(rainSensor);  //Đọc tín hiệu cảm biến mưa
    if (rain_state == HIGH) {                  // Cảm biến đang không mưa
      client.publish("/PTIT_Test/vinh/rain", "Not Rain");
    } else {
      client.publish("/PTIT_Test/vinh/rain", "Rain");
      // Nếu mưa thì đóng dàn phơi
      if (DryingCurtainState == HIGH) {
        while (digitalRead(closeDryingCurtainState) == HIGH) {
          digitalWrite(motor2Pin2, LOW);
          digitalWrite(motor2Pin1, HIGH);
          delay(30);
        }
        digitalWrite(motor2Pin1, LOW);
        DryingCurtainState == LOW;
        client.publish("/PTIT_Test/vinh/drying_curtain", "CLOSED");
      }
    }

    // cập nhật nhiệt độ lên lcd
    lcd.setCursor(0, 0);
    lcd.print("Temp: " + temp);
    lcd.write(1);
    lcd.print("C");
    // cập nhật độ ẩm lên lcd
    lcd.setCursor(0, 1);
    lcd.print("Humidity: " + hum + "%");
  }

  if (digitalRead(closeDoorBtn) == LOW) {  // Đóng cửa
    while (digitalRead(closeDoorState) == HIGH) {
      digitalWrite(motor1Pin1, LOW);
      digitalWrite(motor1Pin2, HIGH);
      delay(30);
    }
    digitalWrite(motor1Pin2, LOW);
  }
  if (digitalRead(openDoorBtn) == LOW) {  // Mở cửa
    while (digitalRead(openDoorState) == HIGH) {
      digitalWrite(motor1Pin1, HIGH);
      digitalWrite(motor1Pin2, LOW);
      delay(30);
    }
    digitalWrite(motor1Pin1, LOW);
  }

  // Cảm biến rfc522
  // Tìm kiếm một thẻ mới
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Chọn một thẻ trong các thẻ 
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  // Hiển thị UID của thẻ trong Serial monitor
  Serial.print("UID tag :");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  // Xác thực UID của thẻ nfc
  if (content.substring(1) == "ED 92 02 2D")
  {
    Serial.println("Authorized access");
    while (digitalRead(openDoorState) == HIGH) {
      digitalWrite(motor1Pin2, LOW);
      digitalWrite(motor1Pin1, HIGH);
      delay(30);
    }
    digitalWrite(motor1Pin1, LOW);
  } else {
    Serial.println("Access denied");
  }
}