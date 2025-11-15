#include "Display.h"

OLEDDisplay::OLEDDisplay(): 
    sda(19), 
    scl(21), 
    rst(-1),
    display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1) {}

void OLEDDisplay::begin() {
    xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
    Wire.begin(sda, scl);
    Wire.setClock(100000);  // Reducido a 100kHz para mayor robustez contra EMI del TRIAC

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("❌ Error al iniciar pantalla OLED");
        while (true);
    }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(10, 25);
    display.print("Iniciando...");
    display.display();
    xSemaphoreGive(getI2CMutex());
}

// Mostrar
void OLEDDisplay::showStatusOffline(const char* left) {
    display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_BLACK);

    //Texto wifi
    display.setTextSize(1);         
    display.setCursor(0, 2);
    display.print(left);
    
    xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
    display.display();
    xSemaphoreGive(getI2CMutex());
}

void OLEDDisplay::showStatusOnline(int wifiStrength, const char* date, const char* time) {
    display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_BLACK);

    //Texto wifi
    display.setTextSize(1);         
    display.setCursor(0, 2);
    display.print("WiFi");
    
    //Barras wifi
    int bars = constrain(map(wifiStrength, -90, -30, 0, 6), 0, 6); 
    int barWidth = 1;             
    int barSpacing = 1;          
    int baseX = 30;               
    int baseY = 10;               
    for (int i = 0; i < bars; i++) {
        int barHeight = (i + 1) * 2; 
        display.fillRect(baseX + i * (barWidth + barSpacing), baseY - barHeight, barWidth, barHeight, SSD1306_WHITE);
    }

    // Hora centrada
    display.setTextSize(1);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(time, 0, 0, &x1, &y1, &w, &h);
    int hourX = (SCREEN_WIDTH - w) / 2;
    display.setCursor(hourX, 2);
    display.print(time);

    // Fecha a la derecha
    char dateBuffer[6];
    strncpy(dateBuffer, date, sizeof(dateBuffer));
    dateBuffer[sizeof(dateBuffer)-1] = '\0';
    display.getTextBounds(dateBuffer, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(SCREEN_WIDTH - w - 2, 2); 
    display.print(dateBuffer);


    xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
    display.display();
    xSemaphoreGive(getI2CMutex());
}

void OLEDDisplay::showCurrentTemperature(float current) {
    static unsigned long lastUpdate = 0;
    unsigned long now = millis();
    if (now - lastUpdate < 1000) return;
    lastUpdate = now;

    display.fillRect(0, 15, SCREEN_WIDTH, 40, SSD1306_BLACK);
    display.setTextSize(2);

    // Convertimos el valor a texto con °C
    char buffer[10];
    sprintf(buffer, "%.1f%cC", current, 247);

    int textLength = strlen(buffer);
    int textWidth = textLength * 6 * 2;  
    int textHeight = 8 * 2;           

    int centerX = SCREEN_WIDTH / 2;
    int centerY = 15 + 40 / 2;

    // Ajustamos posición del texto
    int x = centerX - textWidth / 2;
    int y = centerY - textHeight / 2;

    // Dibujamos
    display.setCursor(x, y);
    display.print(buffer);
    xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
    display.display();
    xSemaphoreGive(getI2CMutex());
}

void OLEDDisplay::showTargetTemperature(int target) {
    display.fillRect(0, 55, SCREEN_WIDTH, 10, SSD1306_BLACK);
    display.setTextSize(1);

    char buffer[10];
    sprintf(buffer, "%d%cC", target, 247);  

    int textWidth = strlen(buffer) * 6;
    int x = (SCREEN_WIDTH - textWidth) / 2;
    int y = 55;

    display.setCursor(x, y);
    display.print(buffer);
    xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
    display.display();
    xSemaphoreGive(getI2CMutex());
}


//Display control
void OLEDDisplay::displayOff() { 
    xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
    display.ssd1306_command(SSD1306_DISPLAYOFF); 
    xSemaphoreGive(getI2CMutex());
}

void OLEDDisplay::displayOn() { 
    xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
    display.ssd1306_command(SSD1306_DISPLAYON); 
    xSemaphoreGive(getI2CMutex());
}

void OLEDDisplay::clearDisplay() { 
    display.clearDisplay(); 
}

// Animación WiFi
void OLEDDisplay::results(const char* label) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    int16_t textWidth = strlen(label) * 6 * 1; 
    int16_t textHeight = 8 * 1;

    int16_t x = (SCREEN_WIDTH - textWidth) / 2;
    int16_t y = (SCREEN_HEIGHT - textHeight) / 2;

    display.setCursor(x, y);
    display.print(label);
    xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
    display.display();
    xSemaphoreGive(getI2CMutex());
}

void OLEDDisplay::onConnecting(const char* message) {
    int centerX = SCREEN_WIDTH / 2;
    int centerY = SCREEN_HEIGHT / 2;

    for (int step = 0; step < 15; step++) {
        display.clearDisplay();
        display.setTextSize(1);

        int textWidth = strlen(message) * 6; 
        int textX = (SCREEN_WIDTH - textWidth) / 2;

        int textY = centerY - 15;
        display.setCursor(textX, 5);
        display.print(message);

        int radius = 8 + abs(6 - (step % 12));
        display.drawCircle(centerX, centerY, radius, SSD1306_WHITE);
        xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
        display.display();
        xSemaphoreGive(getI2CMutex());
        delay(70);
    }
}

void OLEDDisplay::message(const char* title, const char* text) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(2);
    int16_t titleWidth = strlen(title) * 6 * 2; 
    int16_t titleX = (SCREEN_WIDTH - titleWidth) / 2;
    display.setCursor(titleX, 0);
    display.println(title);

    display.setTextSize(1);

    String msg = String(text);
    int numLines = 1;
    for (int i = 0; i < msg.length(); i++) {
        if (msg[i] == '\n') numLines++;
    }

    int textHeight = numLines * 8; 
    int startY = (SCREEN_HEIGHT - textHeight) / 2 + 16;

    int currentY = startY;
    int startPos = 0;
    while (startPos < msg.length()) {
        int endPos = msg.indexOf('\n', startPos);
        if (endPos == -1) endPos = msg.length();

        String line = msg.substring(startPos, endPos);
        int16_t lineWidth = line.length() * 6;
        int16_t x = (SCREEN_WIDTH - lineWidth) / 2;

        display.setCursor(x, currentY);
        display.print(line);

        currentY += 8; 
        startPos = endPos + 1;
    }

    xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
    display.display();
    xSemaphoreGive(getI2CMutex());
}

void OLEDDisplay::showStartupAnimation() {
    display.clearDisplay();

    // Animación de carga circular
    for (int r = 0; r < 30; r += 2) {
        display.clearDisplay();
        display.drawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, r, SSD1306_WHITE);
        xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
        display.display();
        xSemaphoreGive(getI2CMutex());
        delay(40);
    }

    // Pequeña expansión y contracción
    for (int r = 30; r > 10; r -= 2) {
        display.clearDisplay();
        display.drawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, r, SSD1306_WHITE);
        xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
        display.display();
        xSemaphoreGive(getI2CMutex());
        delay(25);
    }

    // Mensaje de inicio con efecto de escritura
    const char* msg = "KiLL";
    for (int i = 0; i <= strlen(msg); i++) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor((SCREEN_WIDTH - (i * 6)) / 2, SCREEN_HEIGHT / 2 - 5);
        for (int j = 0; j < i; j++) display.print(msg[j]);
        xSemaphoreTake(getI2CMutex(), portMAX_DELAY);
        display.display();
        xSemaphoreGive(getI2CMutex());
        delay(100);
    }

    delay(700);
    display.clearDisplay();
    delay(1000);
}

void OLEDDisplay::startAutoStatus() {
    if (wifiClockTaskHandle != nullptr) return; 
    xTaskCreatePinnedToCore([](void* param) {
        OLEDDisplay* self = static_cast<OLEDDisplay*>(param);
        struct tm timeinfo;

        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        setenv("TZ", "PST8PDT,M3.2.0/2,M11.1.0/2", 1); 
        tzset();

        for (;;) {
            if (WiFi.status() != WL_CONNECTED) {
                self->showStatusOffline("WiFi");
                vTaskDelay(pdMS_TO_TICKS(10000)); 
                continue;
            }

            if (getLocalTime(&timeinfo)) {
                // Hora HH:MM
                char timeString[6];
                strftime(timeString, sizeof(timeString), "%H:%M", &timeinfo);

                // Fecha DD/MM
                char dateString[6];
                strftime(dateString, sizeof(dateString), "%d/%m", &timeinfo);

                // Fuerza WiFi
                int wifiStrength = WiFi.RSSI();

                // Mostrar en la barra superior
                self->showStatusOnline(wifiStrength, dateString, timeString);
                vTaskDelay(pdMS_TO_TICKS(10000)); 
            }
        }
    },
        "WiFiClockTask",
        8192,
        this,
        1,
        &wifiClockTaskHandle,
        1
    );
}