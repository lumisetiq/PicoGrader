#include <pico/multicore.h>

#define c0b ((volatile uint8_t*)0x20040000) // points to base of SRAM bank 4, core 0's scratch (4K)
#define c1b ((volatile uint8_t*)0x20041000) // points to base of SRAM bank 5, core 1's scratch (4K)

char cmd[256] = {0}; // incoming command buffer
uint8_t ci = 0; // command buffer index

uint16_t c0i = 0; // core 0's scratch index
uint16_t c1i = 0; // core 1's scratch index

uint8_t nums[256] = {0}; // grade input buffer
uint8_t ni = 1; // grade input buffer index

volatile bool execute_now = false;
volatile bool core1_done = false;
volatile uint32_t c1_mailbox = 0;

void setup() {
  Serial.begin(115200); // start uart
  uint32_t start = millis();
  while (!Serial && millis() - start < 3000); // wait for uart || 3 second timeout
  multicore_launch_core1(core1_worker);

  Serial.print("    << ");
}

// main IO processing
void loop() {
  // waiting for command
  if (Serial.available() > 0) {
    // collecting command input
    uint8_t incoming = Serial.read();

    if (incoming == '\n' || incoming == '\r') { 
      if (ci > 0) { 
        Serial.println();

        cmd[ci] = '\n';

        if (internal()) {
          parse(); // pairing numbers
          core0_worker(); // actual processing
        }

        ci = 0;

        Serial.print("    << ");
      }
    } else {
      if (ci < 255) {
        cmd[ci++] = incoming;
        Serial.write(incoming); // echo back
      }
    }
  }
}

// internal cmds processing
bool internal() {
  uint8_t i = 1;
  ci = 0;

  while (cmd[ci] != '\n' && ci < 255) {
    if (cmd[ci] == ' ' || cmd[ci] == '\r' || cmd[ci] == '\t') {
      ci++;
      continue; 
    }

    if (cmd[ci] == 'w') { // grade write processing
      ci++; // consume 'w'
      
      while (cmd[ci] == ' ') ci++; // junk/whitespace

      uint8_t grade = 0;
      if (cmd[ci] < '0' || cmd[ci] > '9') {
          Serial.print("    ?? "); Serial.print(i);
          Serial.println(": Missing");
          return false;
      }

      while (cmd[ci] >= '0' && cmd[ci] <= '9') {
        grade = (grade * 10) + (cmd[ci] - '0');
        ci++;
      }

      if (grade > 100) {
        Serial.print("    ?? "); Serial.print(i);
        Serial.print(": Grade "); Serial.print(grade);
        Serial.println(" > 100");
        return false;
      }

      nums[ni++] = grade;
      Serial.print("    >> "); Serial.print(i);
      Serial.print(": Grade: "); Serial.print(grade); Serial.println(", write OK!");
    }
    else if (cmd[ci] == ';') {
      ci++; 
      i++;
    } else if (cmd[ci] == '\n') {
        break;
    }
    else if (cmd[ci] == 'r') { // reading/dumping grades
      ci++; // consume 'r'
      if (ni == 0) {
        Serial.print(" [Empty]");
      } else {
        for (uint8_t j = 1; j < ni; j++) {
          Serial.print(", ");
          Serial.print(j);
          Serial.print(": ");
          Serial.print(nums[j]);
        }
      }
      Serial.println("");
      i++;
    }
    else if (cmd[ci] == 'c') { // clear processing
      ci++; // consume 'c'
      while (cmd[ci] == ' ') ci++; // whitespace/junk
      
      if (cmd[ci] >= '0' && cmd[ci] <= '9') { // targeted clear
          uint8_t target = 0;
          while (cmd[ci] >= '0' && cmd[ci] <= '9') {
            target = (target * 10) + (cmd[ci] - '0');
            ci++;
          }

          if (target < ni) { // shifting
            for (uint8_t s = target; s < ni - 1; s++) {
              nums[s] = nums[s + 1];
            }

            nums[ni - 1] = 0; 
            ni--; 

            Serial.print("    >> "); Serial.print(i);
            Serial.print(": Target: "); Serial.print(target); 
            Serial.println(", clear OK!");
          } else {
            Serial.print("    ?? "); Serial.print(i);
            Serial.println(": Out of bounds");
            return false;
          }
        }
        else {
          ni = 1;
          c0i = 0;
          c1i = 0;

          for (int j = 0; j < 256; j++) nums[j] = 0;

          Serial.print("    >> "); Serial.print(i);
          Serial.println(": Atomic clear OK!");
        }
        i++;
    } else if (cmd[ci] == 'e') { // execution command processing
      Serial.println("    >> Exe...");
      return true;
    }
    else {
      Serial.print("    ?? "); Serial.print(i); Serial.print(": Unknown '");
      Serial.print((char)cmd[ci]); Serial.println("'");
      return false;
    }
  }
  return false;
}

void parse() { // parsing / pairing
  c0i = 0; c1i = 0;
  for (uint8_t j = 1; j < ni; j += 4) {
    // dispatch core 0
    if (j < ni)     c0b[c0i++] = nums[j];
    if (j + 1 < ni) c0b[c0i++] = nums[j + 1];

    // dispatch core 1
    if (j + 2 < ni) c1b[c1i++] = nums[j + 2];
    if (j + 3 < ni) c1b[c1i++] = nums[j + 3];
  }

  c0b[c0i] = 0x65; 
  c1b[c1i] = 0x65;

  Serial.println("    >> Pairs OK!");
}

void core1_worker() {
  while (1) {
    while (!execute_now);

    uint32_t sum = 0;
    uint16_t ptr = 0;

    while (c1b[ptr] != 0x65 && ptr < 1024) {
      sum += c1b[ptr];
      ptr++;
    }

    c1_mailbox = sum;
    core1_done = true;
    
    while (execute_now);
    core1_done = false;
  }
}

bool core0_worker() {
  uint32_t c0_sum = 0;
  uint16_t ptr = 0;

  c1_mailbox = 0;
  core1_done = false;
  execute_now = true;

  while (c0b[ptr] != 0x65 && ptr < 1024) {
    c0_sum += c0b[ptr];
    ptr++;
  }

  while (!core1_done); // waiting on core 1 to finish

  uint32_t total_sum = c0_sum + c1_mailbox;
  uint32_t count = ni - 1;

  if (count > 0) {
    uint32_t average = total_sum / count;

    char letter;
    if (average >= 90) letter = 'A';
    else if (average >= 80) letter = 'B';
    else if (average >= 70) letter = 'C';
    else if (average >= 60) letter = 'D';
    else letter = 'F';

    Serial.println("    >> Process OK!");
    Serial.print("    >> Raw Sum:  "); Serial.println(total_sum);
    Serial.print("    >> Elements: "); Serial.println(count);
    Serial.print("    >> Average:  "); Serial.println(average);
    Serial.print("    >> ("); Serial.print(letter); Serial.println(")");
    Serial.println("");
  } else {
    Serial.println("    ?? ERR: Empty Tape");
    return true;
  }

  execute_now = false; 
  c0i = 0;
  c1i = 0;
  return false;
}