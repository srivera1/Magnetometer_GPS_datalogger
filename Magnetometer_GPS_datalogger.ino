/*
   Honeywell Smart Digital Magnetometer (HMR2300)
   and GPS data looger

   Sergio Rivera Lavado
   May 2018

   The digital magnetometer is connected to UART 3
   of a Blue Pill STM32 board. To the same board
   there is conected a GPS at UART2 and a SD card
   at the SPI port too. A max3232 level converter
   must be located between the HMR2300 an the STM32
   board to adecuate the voltage levels.

   Some configuration was preloaded to the
   magnetometer's EEPROM, in particular, a 
   19200 bps bitrate.

   The HMR2300 some configuration commands would be:
   *99WE         -- write enable
   *99D          -- load factory settings
   *99WE
   *99B          -- binary format
   *99R=154      -- sample rate 154 s/s
   *99WE
   *99!BR=F      -- baud rate 19200
   *99WE
   *99SP         -- write to the eeprom


   In this sketch,
   about 20 samples for x, y and z axes and the
   gps position are saved to the sdcard 1 time
   per second.

   I am polling the data in binary format. I tried
   the continuous polling with different settings, but is
   not working 100% of the time. Since I found that
   polling the data value by value is more reliable,
   I use that method here.


*/

#include <SPI.h>
#include <SD.h>

File root;
int Nfiles;
File myFile;
String fileName = "";

void setup() {
  // initialize both serial ports:
  Serial.begin(115200);
  Serial2.begin(9600); //GPS
  Serial3.begin(19200); //Magnetometer
  delay(1000);
  while (false and !Serial3.available()) {
    ;
  }
  delay(2); // magnetometer configuration
  writeCmd("*99WE\r");
  delay(2);
  writeCmd("*99B\r"); // binary
  delay(2);
  writeCmd("*99WE\r");
  delay(2);
  writeCmd("*99R=154\r"); // sample rate
  delay(21);
  // set-reset routine
  /*
    writeCmd("*99WE\r");
    delay(21);
    writeCmd("*99TF\r"); // manual set/reset pulses
    delay(21);
    writeCmd("*99]S\r");  // set
    delay(21);
    writeCmd("*99]R\r");  // reset
    delay(21);
    writeCmd("*99]S\r");  // set
    delay(21);
    writeCmd("*99TN\r"); // auto set/reset pulses
    delay(21);
  */
  writeCmd("*99WE\r");
  delay(2);
  writeCmd("*99ZN\r"); // zero value
  delay(2);

  //writeCmd("*99C\r"); // continuous polling
  writeCmd("*99WE\r");
  delay(2);
  writeCmd("*99VN\r"); // avg, N true, F false
  delay(2);

  Serial.println("config finished");
  Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  root = SD.open("/");

  delay(100);

  Serial.print("Number of files: ");
  findNumberFiles(root, 0);
  Serial.println(Nfiles);
  fileName = "MAG" + (String)(Nfiles + 10000) + ".txt";
  writeFileHeader(fileName);
  Serial.println("setup finished");
  delay(100);
}

int i = 0;
int j = 100;
int k = 10;
int saveFieldFreq = 40; //HZ
int saveGPSFreq = 1;  //HZ
int lastFieldTime = millis();
int lastGPSTime = millis();

void loop() {
  if (true or 1000 / (millis() - lastFieldTime) < saveFieldFreq) {
    //   Serial.print(getFieldCmd("*99P\r"));
    getFieldCmd("*99P\r");
    lastFieldTime = millis();
  }
  if (true or 1000 / (millis() - lastGPSTime) < saveGPSFreq ) {
    //   Serial.print(getGPSCmd());
    getGPSCmd();
    lastGPSTime = millis();
  }
}


void findNumberFiles(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    if (entry.isDirectory()) {
      findNumberFiles(entry, numTabs + 1);
    } else {
      Nfiles++;
    }
    entry.close();
  }
}

void saveData(String fileName, String line) {
  myFile = SD.open(fileName, FILE_WRITE);
  if (myFile and (sizeof(line) > 0)) {
    myFile.print(line);
    myFile.close();
  } else {
    Serial.println("error opening " + fileName);
  }
}

void writeFileHeader(String fileName) {
  myFile = SD.open(fileName, FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to " + fileName + "...");
    myFile.println("Magnetic field and GPS locations\n");
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening " + fileName);
  }
}

String getFieldCmd(String cmd) { // data polling
  int field[] = {0, 0, 0, 0, 0, 0};
  for (int i = 0; i < sizeof(cmd); i++) {
    Serial3.write(cmd[i]);
  }
  int i = 0;
  delay(3);
  while (Serial3.available()) {     // If answer comes in Serial3
    field[i] = Serial3.read();
    i++;
  }
  String line;
  for (int l = 0; l < 3; l++) {
    line = line + ',' + (String)(field[2 * l] * 0x100 + field[2 * l + 1]);
    j--;
  }
  line = line + '\n';
  saveData(fileName, line);
  return line;
}

void writeCmd(String cmd) { // execute any command on the magnetometer
  for (int i = 0; i < sizeof(cmd); i++) {
    Serial3.write(cmd[i]);
  }
  delay(12);
  int k = 1000;
  while (Serial3.available() && k > 0) {     // If answer comes in Serial3
    Serial.write(Serial3.read());   // read it and send it out Serial (USB)
    k--;
  }
  Serial.println("");
  delay(1);
}

String getGPSCmd() {
  int k = 1000;
  String line;
  while (Serial2.available()) {
    line = "";
    while (Serial2.available() && k > 0) {
      byte inByte2 = Serial2.read();
      line += (char)inByte2;
      k--;
    }
    saveData(fileName, line);
  }
  return line;
}
