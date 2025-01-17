#include <SPI.h>
#include <RadioLib.h>
#include <utils/Cryptography.h>

// Initialize the SX1262 radio module
SX1262 radio = new Module(3, 20, 15, 2, SPI1, RADIOLIB_DEFAULT_SPI_SETTINGS);

const size_t MAX_QUEUE_SIZE = 10;
const size_t MAX_MSG_LENGTH = 256;

typedef struct {
  size_t length;
  uint8_t data[MAX_MSG_LENGTH];
} Message;

Message messageQueue[MAX_QUEUE_SIZE];
size_t queueHead = 0;
size_t queueTail = 0;

bool transmitting = false;
volatile bool actionDone = false;
volatile bool receivedFlag = false;

RadioLibAES128* aes = nullptr;
uint8_t key[16];
bool keySet = false;

void setFlag() {
  if (transmitting) {
    actionDone = true;
  } else {
    receivedFlag = true;
  }
}

void enqueueMessage(const uint8_t* data, size_t length) {
  if ((queueTail + 1) % MAX_QUEUE_SIZE == queueHead) {
    Serial.println("Queue full, cannot enqueue message.");
    return;
  }
  messageQueue[queueTail].length = length;
  memcpy(messageQueue[queueTail].data, data, length);
  queueTail = (queueTail + 1) % MAX_QUEUE_SIZE;
}

Message dequeueMessage() {
  if (queueHead == queueTail) {
    Serial.println("Queue empty, no message to dequeue.");
    Message empty;
    empty.length = 0;
    return empty;
  }
  Message msg = messageQueue[queueHead];
  queueHead = (queueHead + 1) % MAX_QUEUE_SIZE;
  return msg;
}

String toHexString(const uint8_t* data, size_t length) {
  String result = "";
  for (size_t i = 0; i < length; i++) {
    if (data[i] < 16) {
      result += "0";
    }
    result += String(data[i], HEX);
  }
  return result;
}

void transmit(const uint8_t* data, size_t length) {
  if (keySet) {
    Serial.println("[SX1262] Starting transmission of encrypted data.");
  } else {
    Serial.println("[SX1262] Starting transmission of plaintext data.");
  }
  int state = radio.startTransmit(data, length);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.println("[SX1262] startTransmit() failed, code " + String(state));
    radio.startReceive();
    transmitting = false;
  } else {
    transmitting = true;
    Serial.println("[SX1262] Message transmitted.");
  }
}

void receive() {
  if (receivedFlag) {
    receivedFlag = false;
    int length = radio.getPacketLength();
    if (length <= 1) {
      radio.startReceive();
      return;
    }
    uint8_t buffer[256];
    if (length > 256) length = 256;
    int state = radio.readData(buffer, length);
    if (state == RADIOLIB_ERR_NONE) {
      if (keySet) {
        uint8_t decrypted[length];
        size_t decryptedLen = aes->decryptECB(buffer, length, decrypted);
        // Remove padding
        if (decryptedLen >= 1) {
          int pad = decrypted[decryptedLen - 1];
          if (pad < 16) {
            decryptedLen -= pad;
          }
        }
        Serial.println("[SX1262] RX Data (decrypted): " + String((char*)decrypted, decryptedLen));
      } else {
        Serial.println("[SX1262] RX Data (plaintext): " + String((char*)buffer));
      }
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      Serial.println("[SX1262] CRC error!");
    } else {
      Serial.println("[SX1262] Receive failed, code " + String(state));
    }
    radio.startReceive();
  }
}

void setup() {
  SPI1.setSCK(10);
  SPI1.setTX(11);
  SPI1.setRX(12);
  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);
  SPI1.begin(false);

  Serial.begin(9600);
  while (!Serial) {}

  Serial.println("Start");

  Serial.println("[SX1262] Initializing ...");
  int initState = radio.begin(868.0);
  if (initState != RADIOLIB_ERR_NONE) {
    Serial.println("Radio init failed, code " + String(initState));
    while (true) {}
  }
  delay(1000);
  Serial.println("[SX1262] Radio init success");

  radio.setDio1Action(setFlag);

  radio.setFrequency(868.0);
  radio.setBandwidth(500.0);
  radio.setSpreadingFactor(12);
  radio.setCodingRate(8);
  radio.setPreambleLength(8);
  radio.setSyncWord(0x12);
  radio.setOutputPower(22);
  radio.setCRC(true);

  radio.startReceive();

  sleep_ms(2000);
  Serial.println("Enter cipher/decipher key (16 bytes) or leave empty and press ENTER:");
  String inputKey = "";
  while (Serial.available() == 0) {
    Serial.read();
  }
  delay(1000);
  inputKey = Serial.readStringUntil('\n');
  inputKey.trim();

  Serial.println("Received key: " + inputKey);

  if (inputKey.length() > 0) {
    if (inputKey.length() < 16) {
      Serial.println("Key is less than 16 bytes. Padding with zeros.");
      inputKey += String(16 - inputKey.length(), '0');
    } else if (inputKey.length() > 16) {
      Serial.println("Key is longer than 16 bytes. Truncating to 16 bytes.");
      inputKey = inputKey.substring(0, 16);
    }
    for (int i = 0; i < 16; i++) {
      key[i] = inputKey[i];
    }
    aes = new RadioLibAES128();
    aes->init(key);
    keySet = true;
    Serial.println("Encryption key set.");
  } else {
    Serial.println("No encryption key set.");
  }
  delay(2000);

  Serial.println("Just chat with the other devices.");
}

void loop() {
  String input = Serial.readStringUntil('\n');
  if (input.length() > 0) {
    // Check if the input is the "/changekey" command
    if (input.startsWith("/changekey")) {
      // Extract the new key from the input string
      String newKeyInput = input.substring(10); // Remove "/changekey " prefix
      newKeyInput.trim();

      if (newKeyInput.length() == 0) {
        // Disable encryption
        if (aes != nullptr) {
          delete aes;
          aes = nullptr;
        }
        keySet = false;
        Serial.println("Encryption disabled.");
      } else {
        // Adjust new key to 16 bytes
        if (newKeyInput.length() < 16) {
          Serial.println("Key is less than 16 bytes. Padding with zeros.");
          newKeyInput += String(16 - newKeyInput.length(), '0');
        } else if (newKeyInput.length() > 16) {
          Serial.println("Key is longer than 16 bytes. Truncating to 16 bytes.");
          newKeyInput = newKeyInput.substring(0, 16);
        }
        for (int i = 0; i < 16; i++) {
          key[i] = newKeyInput[i];
        }
        // Initialize or reinitialize AES with the new key
        if (aes == nullptr) {
          aes = new RadioLibAES128();
        }
        aes->init(key);
        keySet = true;
        Serial.println("Encryption key changed.");
      }
    } else {
      // Proceed with sending the message
      char buf[MAX_MSG_LENGTH];
      memset(buf, 0, sizeof(buf));
      int len = min((int)input.length(), (int)MAX_MSG_LENGTH - 1);
      strncpy(buf, input.c_str(), len);
      if (keySet) {
        // PKCS#7 padding
        int padding = 16 - (len % 16);
        if (padding < 16) {
          memset(buf + len, padding, padding);
          len += padding;
        }
        uint8_t* encrypted = new uint8_t[len];
        size_t encryptedLen = aes->encryptECB((uint8_t*)buf, len, encrypted);
        enqueueMessage(encrypted, encryptedLen);
        delete[] encrypted;
      } else {
        enqueueMessage((uint8_t*)buf, len);
      }
      Serial.println("[SX1262] Message: " + input);
    }
  }

  if (!transmitting && queueHead != queueTail) {
    Message msg = dequeueMessage();
    if (msg.length > 0) {
      transmit(msg.data, msg.length);
    }
  }

  if (actionDone) {
    actionDone = false;
    transmitting = false;
    radio.startReceive();
    if (queueHead != queueTail) {
      Message msg = dequeueMessage();
      if (msg.length > 0) {
        transmit(msg.data, msg.length);
        transmitting = true;
      }
    }
  }

  receive();
}