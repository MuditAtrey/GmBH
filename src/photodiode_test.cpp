/*
 * Photodiode Hardware Sanity Check
 * 
 * Circuit:
 * - Photodiode CATHODE (shorter leg, marked side) → NodeMCU 3V3
 * - Photodiode ANODE (longer leg) → NodeMCU A0 AND one leg of 4.7kΩ resistor
 * - Other leg of 4.7kΩ resistor → GND
 * 
 * This creates a voltage divider where:
 * - DARK: More reverse current flows, A0 reads LOWER voltage (closer to GND)
 * - BRIGHT: Less reverse current flows, A0 reads HIGHER voltage (closer to 3.3V)
 * 
 * Expected readings:
 * - Dark room: 100-300 (varies with ambient light)
 * - Bright screen/light: 600-900
 * - Difference should be at least 80-100 counts for reliable detection
 */

#include <Arduino.h>

#define PHOTODIODE_PIN A0
#define LED_PIN LED_BUILTIN  // GPIO2, active LOW

// Statistics
int minReading = 1024;
int maxReading = 0;
int currentReading = 0;
unsigned long sampleCount = 0;
unsigned long lastPrint = 0;
const unsigned long PRINT_INTERVAL = 100;  // Print every 100ms

// Running average for noise reduction
const int AVG_SAMPLES = 10;
int readings[AVG_SAMPLES];
int readIndex = 0;
int total = 0;
int average = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // LED off
  
  // Initialize averaging array
  for (int i = 0; i < AVG_SAMPLES; i++) {
    readings[i] = 0;
  }
  
  Serial.println();
  Serial.println("╔═══════════════════════════════════════════════════════════╗");
  Serial.println("║        PHOTODIODE HARDWARE SANITY CHECK                  ║");
  Serial.println("╚═══════════════════════════════════════════════════════════╝");
  Serial.println();
  Serial.println("📋 Circuit Configuration:");
  Serial.println("   Photodiode CATHODE (−) → 3V3");
  Serial.println("   Photodiode ANODE (+)   → A0 + 4.7kΩ resistor");
  Serial.println("   4.7kΩ resistor         → GND");
  Serial.println();
  Serial.println("📊 Monitoring A0 (ADC range: 0-1023)");
  Serial.println("   Expected DARK:   100-300");
  Serial.println("   Expected BRIGHT: 600-900");
  Serial.println("   Minimum Δ needed: 80-100 counts");
  Serial.println();
  Serial.println("🧪 TEST PROCEDURE:");
  Serial.println("   1. Cover photodiode with your hand (or turn off lights)");
  Serial.println("   2. Note the 'Dark' reading in statistics below");
  Serial.println("   3. Point photodiode at bright screen/light");
  Serial.println("   4. Note the 'Bright' reading");
  Serial.println("   5. Check that Δ (difference) is >80");
  Serial.println();
  Serial.println("💡 The built-in LED will indicate light level:");
  Serial.println("   LED ON  (solid) = DARK detected");
  Serial.println("   LED OFF (off)   = BRIGHT detected");
  Serial.println();
  Serial.println("────────────────────────────────────────────────────────────");
  Serial.println();
  
  delay(2000);
}

void loop() {
  // Read photodiode (higher value = more light in reverse-bias configuration)
  currentReading = analogRead(PHOTODIODE_PIN);
  
  // Update running average
  total = total - readings[readIndex];
  readings[readIndex] = currentReading;
  total = total + readings[readIndex];
  readIndex = (readIndex + 1) % AVG_SAMPLES;
  average = total / AVG_SAMPLES;
  
  // Update statistics
  if (currentReading < minReading) minReading = currentReading;
  if (currentReading > maxReading) maxReading = currentReading;
  sampleCount++;
  
  // Simple threshold for LED indicator (adjust based on your readings)
  // Assuming mid-point around 400-500
  if (average < 400) {
    digitalWrite(LED_PIN, LOW);   // LED ON = DARK
  } else {
    digitalWrite(LED_PIN, HIGH);  // LED OFF = BRIGHT
  }
  
  // Print statistics periodically
  unsigned long now = millis();
  if (now - lastPrint >= PRINT_INTERVAL) {
    lastPrint = now;
    
    // Clear line and print compact stats
    Serial.print("\r");  // Carriage return to overwrite line
    Serial.print("📊 Current: ");
    Serial.print(currentReading);
    Serial.print("  │  Avg: ");
    Serial.print(average);
    Serial.print("  │  Min: ");
    Serial.print(minReading);
    Serial.print("  │  Max: ");
    Serial.print(maxReading);
    Serial.print("  │  Δ: ");
    Serial.print(maxReading - minReading);
    Serial.print("  │  Samples: ");
    Serial.print(sampleCount);
    Serial.print("    ");  // Clear any trailing chars
    
    // Every 2 seconds, print a detailed update
    if (sampleCount % 20 == 0) {
      Serial.println();  // New line
      Serial.println();
      
      int delta = maxReading - minReading;
      
      if (delta < 50) {
        Serial.println("⚠️  WARNING: Difference too small! (<50)");
        Serial.println("   → Check photodiode polarity (try flipping it)");
        Serial.println("   → Reduce ambient light");
        Serial.println("   → Move photodiode closer to screen");
      } else if (delta < 80) {
        Serial.println("⚡ MARGINAL: Difference is small (50-80)");
        Serial.println("   → May work but could be unreliable");
        Serial.println("   → Consider reducing ambient light");
      } else if (delta < 200) {
        Serial.println("✅ GOOD: Difference is adequate (80-200)");
        Serial.println("   → Should work for optical communication");
      } else {
        Serial.println("🌟 EXCELLENT: Strong signal difference (>200)");
        Serial.println("   → Perfect for optical communication!");
      }
      
      // Visual bar graph
      Serial.println();
      Serial.print("   DARK ");
      for (int i = 0; i < minReading / 20; i++) Serial.print("█");
      Serial.print(" ");
      Serial.println(minReading);
      
      Serial.print("   BRIGHT ");
      for (int i = 0; i < maxReading / 20; i++) Serial.print("█");
      Serial.print(" ");
      Serial.println(maxReading);
      
      Serial.println();
      Serial.println("────────────────────────────────────────────────────────────");
      Serial.println();
    }
  }
  
  delay(5);  // 200 samples/second
}
