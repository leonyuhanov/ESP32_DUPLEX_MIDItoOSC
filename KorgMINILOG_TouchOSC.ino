#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "FS.h"
#include "SPIFFS.h"
#include "AsyncUDP.h"
#include "midiDeviceMapper.h"
#include "osc.h"

char midiSerialData[3];
char* tempString = new char[5];
int dataAvail=0;
unsigned short int iCnt=0, chCnt=0;
byte readyToWrite=0;
byte dataIn;
midiDeviceMapper midiMap;
MIDINODE* midiNode;
byte rxPin = 25, txPin = 26;


//Network Stuff
//Please enter corect details for wifi details
const char * ssid = "SSID";
const char * password = "KEY";
AsyncUDP udp;
IPAddress touchOSCAddress(192,168,1,113);
IPAddress thisDevice(192,168,1,111);   
IPAddress gateway(192,168,1,254);   
IPAddress subnet(255,255,255,0);

//midi data to reject
const byte deadValueCount = 2;
const byte deadMIDIValues[deadValueCount] = {248, 254};
const byte maxMidiChans = 1;
const byte noteON = 144;
unsigned short int scaledTo = 24;

//Local files
const char* oscConfigFilePath="/oscconfigFile";
const char* midiConfigFilePath="/midiconfigFile";
const char* indexPageFilePath="/index.html";
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
unsigned short int webUIClientID=0;
byte WSConnectState=0;

//OSC COnfig
osc oscObject;
unsigned int oscRXPort = 5555, oscTXPort = 5556;
LLNODE* oscNode;
unsigned short int packetSize=0;

void setup()
{

  
  Serial.begin(115200);
  Serial.println("\r\n\r\n");

  //Eable WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
        delay(100);
        Serial.print(".");
  }
  WiFi.config(thisDevice, gateway, subnet);
  Serial.printf("\r\nMIDI Module ONLINE\r\nCurrent Ip Address:\t");
  Serial.print(WiFi.localIP());
  Serial.print("\r\n");

  //Initiate the SPIFFS
  initFS();
  //load config
  loadConfiguration();
  
  //init Web server
  //Handle Root
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    handleRoot(request);
  });
  server.onNotFound(notFound);
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();

  //Uncoment to load the default system config for the Korg Monologue
  //loadDefaults();
 
  //Set up UDP Endpoint
  if(udp.listen(oscRXPort))
  {
    udp.onPacket([](AsyncUDPPacket packet)
    {
      packetSize = packet.length();
      memcpy(oscObject.packetBuffer, packet.data(), packet.length());
      /*
      for(iCnt=0; iCnt<packet.length(); iCnt++)
      {
        Serial.printf("\r\n%d\t[%d]\t[%c]", iCnt, oscObject.packetBuffer[iCnt], oscObject.packetBuffer[iCnt]);
      }
      */
      oscObject.currentPacketSize = packet.length();
      oscObject.toggleState();
      oscObject.parseOSCPacket();
      sendMidi(oscObject.currentControllID);
      if(WSConnectState==1)
      {
        txLastOSCMessageToUI(oscObject.packetBuffer, packet.length());
      }
    });
  }
  
  //Set up SOftware Serial for MIDI IN/OUT
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);

  Serial2.begin(31250, SERIAL_8N1, rxPin, txPin);
}

void loadDefaults()
{
  LLNODE* oscObjectNode;
  MIDINODE* midiObjectNode;
  unsigned short int id=0;

  //clear all osc objects
  for(id=0; id<oscObject.totalNodes; id++)
  {
    oscObject.deleteNode(id);
  }
  //clear all midi objects
  for(id=0; id<midiMap.totalNodes; id++)
  {
    midiMap.deleteNode(id);
  }
  //Set up default MIDI entries
  midiMap.addNode("PITCH_VCO2",1,35,0,0,0);
  midiMap.addNode("SHAPE_VCO1",1,36,0,0,1);
  midiMap.addNode("SHAPE_VCO2",1,37,0,0,2);
  midiMap.addNode("LEVEL_VCO1",1,39,0,0,3);
  midiMap.addNode("LEVEL_VCO2",1,40,0,0,4);
  midiMap.addNode("CUTOFF",1,43,0,0,5);
  midiMap.addNode("RESONANCE",1,44,0,0,6);
  midiMap.addNode("EG_ATTACK",1,16,0,0,7);
  midiMap.addNode("EG_DECAY",1,17,0,0,8);
  midiMap.addNode("EG_INT",1,25,0,0,9);
  midiMap.addNode("LFO_RATE",1,24,0,0,10);
  midiMap.addNode("LFO_DEPTH",1,26,0,0,11);
  midiMap.addNode("OCTAVE_VCO2",1,49,0,0,12);
  midiMap.addNode("WAVE_1",1,50,0,0,13);
  midiMap.addNode("WAVE_2",1,51,0,0,14);
  midiMap.addNode("SYNC_RING",1,60,0,0,15);
  midiMap.addNode("DRIVE",1,28,0,0,16);
  midiMap.addNode("EG_TYPE",1,61,0,0,17);
  midiMap.addNode("EG_TARGET",1,62,0,0,18);
  midiMap.addNode("LFO_TARGET",1,56,0,0,19);
  midiMap.addNode("LFO_WAVE",1,58,0,0,20);
  midiMap.addNode("LFO_MODE",1,59,0,0,21);
  midiMap.addNode("AUDIO_OFF",1,120,0,0,22);
  midiMap.addNode("RESET_CONTROL",1,121,0,0,23);

  //Set up default osc entries
  oscObject.addControll("/PITCH_VCO2",1,'f');
  oscObject.addControll("/SHAPE_VCO1",1,'f');
  oscObject.addControll("/SHAPE_VCO2",1,'f');
  oscObject.addControll("/LEVEL_VCO1",1,'f');
  oscObject.addControll("/LEVEL_VCO2",1,'f');
  oscObject.addControll("/CUTOFF",1,'f');
  oscObject.addControll("/RESONANCE",1,'f');
  oscObject.addControll("/EG_ATTACK",1,'f');
  oscObject.addControll("/EG_DECAY",1,'f');
  oscObject.addControll("/EG_INT",1,'f');
  oscObject.addControll("/LFO_RATE",1,'f');
  oscObject.addControll("/LFO_DEPTH",1,'f');
  oscObject.addControll("/OCTAVE_VCO2",1,'f');
  oscObject.addControll("/WAVE_1",1,'f');
  oscObject.addControll("/WAVE_2",1,'f');
  oscObject.addControll("/SYNC_RING",1,'f');
  oscObject.addControll("/DRIVE",1,'f');
  oscObject.addControll("/EG_TYPE",1,'f');
  oscObject.addControll("/EG_TARGET",1,'f');
  oscObject.addControll("/LFO_TARGET",1,'f');
  oscObject.addControll("/LFO_WAVE",1,'f');
  oscObject.addControll("/LFO_MODE",1,'f');
  oscObject.addControll("/AUDIO_OFF",1,'f');
  oscObject.addControll("/RESET_CONTROL",1,'f');
}

//---------------------------------------------------UI---------------------------------------------------
void initFS()
{
  Serial.printf("\r\n\tSetting up SPIFS...");
  SPIFFS.begin(1);
  Serial.printf("\tSPIFS READY!");
}

void handleRoot(AsyncWebServerRequest *request)
{
  File tempFile;
	unsigned short int indexFileSize=0;

  tempFile = SPIFFS.open(indexPageFilePath, "r");
  indexFileSize = tempFile.size();
  tempFile.close();
  if(indexFileSize>0)
  {
    request->send(SPIFFS, indexPageFilePath, "text/html");
    Serial.printf("\r\nServed UI\tSent\t%d Bytes", indexFileSize);
  }
  else
  {
    request->send(200, "text/html", "UI not found.");
    Serial.printf("\r\nUI not found....");
  }
}

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Nothing Here :)");
}

void blankFunction()
{
  
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{  
  char* tempCommandString;
 
  //Hadnle socket COnnect event
  if(type == WS_EVT_CONNECT)
  {
    Serial.println("\r\nClient connected!");
    WSConnectState=1;  
  }
  else if(type == WS_EVT_DISCONNECT)
  {
    Serial.println("\r\nClient Disconnected!");
    WSConnectState=0;
  }
  //Handle Data reception
  else if(type == WS_EVT_DATA)
  {
    webUIClientID = client->id();
    //Store received command
    tempCommandString = new char[len+1];
    memcpy(tempCommandString, data, len);
    tempCommandString[len]=0;   //Terminate string
    if(strcmp(tempCommandString, "getosc")==0)
    {
      Serial.printf("\r\n\tSending OSC config...");
      sendOSCConfig(client);
    }
    else if(strcmp(tempCommandString, "getmidi")==0)
    {
      Serial.printf("\r\n\tSending MIDI config...");
      sendMIDIConfig(client);
    }
    else if(strcmp(tempCommandString, "countosc")==0)
    {
      Serial.printf("\r\n\tSending OSC record count...");
      countOSC(client);
    }
    else if(strcmp(tempCommandString, "countmidi")==0)
    {
      Serial.printf("\r\n\tSending MIDI record count...");
      countMIDI(client);
    }
    else if(strcmp(tempCommandString, "clearosc")==0)
    {
      Serial.printf("\r\n\tClearing OSC Config...");
      clearOSC();
    }
    else if(strcmp(tempCommandString, "clearmidi")==0)
    {
      Serial.printf("\r\n\tClearing MIDI Config...");
      clearMIDI();
    }
    else if(tempCommandString[0]=='O')
    {
      Serial.printf("\r\n\tAdding OSC Record.");
      addOSC(tempCommandString);
    }
    else if(tempCommandString[0]=='M')
    {
      Serial.printf("\r\n\tAdding MIDI Record.");
      addMIDI(tempCommandString);
    }
    else if(tempCommandString[0]=='S')
    {
      //Save OSC and MIDI configs to SPIFFS
      Serial.printf("\r\n\tSaving configuration to SPIFFS");
      saveOSCConfig();
      saveMIDIConfig();
    }
    else if(tempCommandString[0]=='C')
    {
      //Clear internal config and reload the defaults
      Serial.printf("\r\n\tLoading Default Korg Monologue settings");
      loadDefaults();
    }
  }
}

void clearOSC()
{
  unsigned short int oscItemCounter=0;
  unsigned short int totalNodes = oscObject.totalNodes;
  for(oscItemCounter=0; oscItemCounter<totalNodes; oscItemCounter++)
  {
    oscObject.deleteNode(oscItemCounter);
  }
}

void clearMIDI()
{
  unsigned short int midiItemCounter=0;
  unsigned short int totalNodes = midiMap.totalNodes;
  for(midiItemCounter=0; midiItemCounter<totalNodes; midiItemCounter++)
  {
    midiMap.deleteNode(midiItemCounter);
  }
}

void addOSC(char* commandString)
{
  unsigned short int commandTrack[2] = {0,0};
  unsigned short int needleTrack=1, tempOSCID, tempNumberOfVars;
  char tempValueType;
  char* tempOSCName;
  char* tempIntegerString;

  //ID
  commandTrack[0] = 2;
  commandTrack[1] = findNeedleCount(commandString, strlen(commandString), ',', needleTrack);
  tempIntegerString = new char[ commandTrack[1]-commandTrack[0] ];
  memcpy(tempIntegerString, commandString+commandTrack[0], commandTrack[1]-commandTrack[0]);
  tempIntegerString[ commandTrack[1]-commandTrack[0] ] = 0;
  needleTrack++;
  tempOSCID = atoi(tempIntegerString);

  //Name
  commandTrack[0] = commandTrack[1]+1;
  commandTrack[1] = findNeedleCount(commandString, strlen(commandString), ',', needleTrack);
  tempOSCName = new char[ commandTrack[1]-commandTrack[0] ];
  memcpy(tempOSCName, commandString+commandTrack[0], commandTrack[1]-commandTrack[0]);
  tempOSCName[commandTrack[1]-commandTrack[0]]=0;
  needleTrack++;

  //Value Count
  commandTrack[0] = commandTrack[1]+1;
  commandTrack[1] = findNeedleCount(commandString, strlen(commandString), ',', needleTrack);
  tempIntegerString = new char[ commandTrack[1]-commandTrack[0] ];
  memcpy(tempIntegerString, commandString+commandTrack[0], commandTrack[1]-commandTrack[0]);
  tempIntegerString[commandTrack[1]-commandTrack[0]]=0;
  needleTrack++;
  tempNumberOfVars = atoi(tempIntegerString);

  //Value Type
  commandTrack[0] = commandTrack[1]+1;
  commandTrack[1] = findNeedleCount(commandString, strlen(commandString), ';', 1);
  tempValueType = commandString[commandTrack[0]];
  needleTrack++;

  oscObject.addControll(tempOSCName,tempNumberOfVars, tempValueType);
}

void addMIDI(char* commandString)
{
  unsigned short int commandTrack[2] = {0,0};
  short int needleTrack=1, tempMIDIID, tempMapToOSC;
  byte tempMIDIChanel, tempCommand, tempOpcode, tempOpcodeTwo;
  char* tempMIDIName;
  char* tempIntegerString;

  //MIDI Node ID
  commandTrack[0] = 2;
  commandTrack[1] = findNeedleCount(commandString, strlen(commandString), ',', needleTrack);
  tempIntegerString = new char[ commandTrack[1]-commandTrack[0] ];
  memcpy(tempIntegerString, commandString+commandTrack[0], commandTrack[1]-commandTrack[0]);
  tempIntegerString[ commandTrack[1]-commandTrack[0] ] = 0;
  needleTrack++;
  tempMIDIID = atoi(tempIntegerString);

  //MIDI Controll Name
  commandTrack[0] = commandTrack[1]+1;
  commandTrack[1] = findNeedleCount(commandString, strlen(commandString), ',', needleTrack);
  tempMIDIName = new char[ commandTrack[1]-commandTrack[0] ];
  memcpy(tempMIDIName, commandString+commandTrack[0], commandTrack[1]-commandTrack[0]);
  tempMIDIName[commandTrack[1]-commandTrack[0]]=0;
  needleTrack++;

  //MIDI Chanel
  commandTrack[0] = commandTrack[1]+1;
  commandTrack[1] = findNeedleCount(commandString, strlen(commandString), ',', needleTrack);
  tempIntegerString = new char[ commandTrack[1]-commandTrack[0] ];
  memcpy(tempIntegerString, commandString+commandTrack[0], commandTrack[1]-commandTrack[0]);
  tempIntegerString[commandTrack[1]-commandTrack[0]]=0;
  needleTrack++;
  tempMIDIChanel = atoi(tempIntegerString);

  //MIDI Command
  commandTrack[0] = commandTrack[1]+1;
  commandTrack[1] = findNeedleCount(commandString, strlen(commandString), ',', needleTrack);
  tempIntegerString = new char[ commandTrack[1]-commandTrack[0] ];
  memcpy(tempIntegerString, commandString+commandTrack[0], commandTrack[1]-commandTrack[0]);
  tempIntegerString[commandTrack[1]-commandTrack[0]]=0;
  needleTrack++;
  tempCommand = atoi(tempIntegerString);

  //MIDI Opcode
  commandTrack[0] = commandTrack[1]+1;
  commandTrack[1] = findNeedleCount(commandString, strlen(commandString), ',', needleTrack);
  tempIntegerString = new char[ commandTrack[1]-commandTrack[0] ];
  memcpy(tempIntegerString, commandString+commandTrack[0], commandTrack[1]-commandTrack[0]);
  tempIntegerString[commandTrack[1]-commandTrack[0]]=0;
  needleTrack++;
  tempOpcode = atoi(tempIntegerString);

  //MIDI Opcode 2
  commandTrack[0] = commandTrack[1]+1;
  commandTrack[1] = findNeedleCount(commandString, strlen(commandString), ',', needleTrack);
  tempIntegerString = new char[ commandTrack[1]-commandTrack[0] ];
  memcpy(tempIntegerString, commandString+commandTrack[0], commandTrack[1]-commandTrack[0]);
  tempIntegerString[commandTrack[1]-commandTrack[0]]=0;
  needleTrack++;
  tempOpcodeTwo = atoi(tempIntegerString);

  //Map to OSC ID
  commandTrack[0] = commandTrack[1]+1;
  commandTrack[1] = findNeedleCount(commandString, strlen(commandString), ';', 1);
  tempIntegerString = new char[ commandTrack[1]-commandTrack[0] ];
  memcpy(tempIntegerString, commandString+commandTrack[0], commandTrack[1]-commandTrack[0]);
  tempIntegerString[commandTrack[1]-commandTrack[0]]=0;
  needleTrack++;
  tempMapToOSC = atoi(tempIntegerString);

  midiMap.addNode(tempMIDIName,tempMIDIChanel,tempCommand,tempOpcode,tempOpcodeTwo,tempMapToOSC);
}

short int findNeedleCount(char* haystack, unsigned short int hayStackLength, char needle, unsigned short needleCount)
{
  unsigned short int hayCount=0, nCount=0;
  for(hayCount; hayCount<hayStackLength; hayCount++)
  {
    if(haystack[hayCount]==needle)
    {
      nCount++;
      if(nCount==needleCount)
      {
        return hayCount;
      }
    }
  }
  return -1;
}

void loadConfiguration()
{
  File configFileObject;
  byte noConfig=0;

  //Load OSC configuration
  configFileObject = SPIFFS.open(oscConfigFilePath, "r");
  if(!configFileObject|| configFileObject.size()==0)
  {
    Serial.printf("\r\n\tNo OSC Config found.");
  }
  else
  {
    Serial.printf("\r\n\tLoading %d bytes of OSC Configuration from file...", configFileObject.size());
    loadOSCConfig(configFileObject);
    Serial.printf("\tLoaded %d OSC items!", oscObject.totalNodes);
    noConfig++;
  }
  configFileObject.close();
  
  //Load MIDI configuration
  configFileObject = SPIFFS.open(midiConfigFilePath, "r");
  if(!configFileObject|| configFileObject.size()==0)
  {
    Serial.printf("\r\n\tNo MIDI Config found.");
  }
  else
  {
    Serial.printf("\r\n\tLoading %d bytes of MIDI COnfiguration from file...", configFileObject.size());
    loadMIDIConfig(configFileObject);
    Serial.printf("\tLoaded %d MIDI items!", midiMap.totalNodes);
    noConfig++;
  }
  configFileObject.close();
  if(noConfig==0)
  {
    //If config is empty laod defaults in memory but NOT into file system
    Serial.printf("\r\n\tNo configuration in SPIFFs.");
    //loadDefaults();
  }
}

void loadOSCConfig(File configFileObject)
{
  unsigned short int configFileSize = configFileObject.size(), fileIndex=0;
  unsigned short int commandTrack[2] = {0,0};
  unsigned short int needleTrack=1, terminatorNeedleTrack=1, tempOSCID, tempNumberOfVars;
  char tempValueType;
  char* tempOSCName;
  unsigned char* integerCharPointer;
  char* fileBuffer = new char[configFileSize];
  
  configFileObject.readBytes(fileBuffer, configFileSize);
 
  while(fileIndex<configFileSize)
  {
    //ID
    //commandTrack[1] = findNeedleCount(fileBuffer, configFileSize, ',', needleTrack);
    commandTrack[1] = commandTrack[0]+1;
    integerCharPointer = (unsigned char*)&tempOSCID;
    integerCharPointer[0] = fileBuffer[commandTrack[0]];
    integerCharPointer[1] = fileBuffer[commandTrack[0]+1];
    //needleTrack++;

    //OSC Name
    commandTrack[0] = commandTrack[1]+1;
    commandTrack[1] = findNeedleCount(fileBuffer, configFileSize, ',', needleTrack);
    tempOSCName = new char[commandTrack[1]-commandTrack[0]];
    memcpy(tempOSCName, fileBuffer+commandTrack[0], commandTrack[1]-commandTrack[0]);
    tempOSCName[commandTrack[1]-commandTrack[0]]=0;
    needleTrack++;

    //Number of Values
    commandTrack[0] = commandTrack[1]+1;
    //commandTrack[1] = findNeedleCount(fileBuffer, configFileSize, ',', needleTrack);
    commandTrack[1] = commandTrack[0]+1;
    integerCharPointer = (unsigned char*)&tempNumberOfVars;
    integerCharPointer[0] = fileBuffer[commandTrack[0]];
    integerCharPointer[1] = fileBuffer[commandTrack[0]+1];
    //needleTrack++;

    //Value Type
    commandTrack[0] = commandTrack[1]+1;
    commandTrack[1] = findNeedleCount(fileBuffer, configFileSize, ';', terminatorNeedleTrack);
    tempValueType = fileBuffer[commandTrack[0]];
    terminatorNeedleTrack++;
    oscObject.addControll(tempOSCName,tempNumberOfVars, tempValueType);
    
    commandTrack[0] = commandTrack[1]+1;
    fileIndex=commandTrack[0];

  }
}

void loadMIDIConfig(File configFileObject)
{
  unsigned short int configFileSize = configFileObject.size(), fileIndex=0, dataStartIndex=0;
  unsigned short int commandTrack[2] = {0,0};
  unsigned short int needleTrack=1, terminatorNeedleTrack=1, tempMIDIID, tempMapToOSC;
  unsigned char* integerCharPointer;
  char tempMIDIChanel, tempCommand, tempOpcode, tempOpcodeTwo;
  char* tempMIDIName;

  char* fileBuffer = new char[configFileSize];  
  configFileObject.readBytes(fileBuffer, configFileSize);

  //Find the index of where the data vars start in the buffer
  dataStartIndex = findNeedleCount(fileBuffer, configFileSize, ';', terminatorNeedleTrack)+1;
  while(fileIndex<=configFileSize-1)
  {
    //MIDI Name
    commandTrack[1] = findNeedleCount(fileBuffer, configFileSize, ',', needleTrack);
    tempMIDIName = new char[commandTrack[1]-commandTrack[0]];
    memcpy(tempMIDIName, fileBuffer+commandTrack[0], commandTrack[1]-commandTrack[0]);
    tempMIDIName[commandTrack[1]-commandTrack[0]]=0;
    needleTrack++;
    commandTrack[0] = commandTrack[1]+1;     
    //ID
    integerCharPointer = (unsigned char*)&tempMIDIID;
    integerCharPointer[0] = fileBuffer[dataStartIndex];
    integerCharPointer[1] = fileBuffer[dataStartIndex+1];
    dataStartIndex+=2;

    //MIDI Chanel
    tempMIDIChanel = fileBuffer[dataStartIndex];
    dataStartIndex++;
    
    //MIDI Command Byte
    tempCommand = fileBuffer[dataStartIndex];
    dataStartIndex++;
    
    //MIDI OP 0
    tempOpcode = fileBuffer[dataStartIndex];
    dataStartIndex++;
    
    //MIDI OP 1
    tempOpcodeTwo = fileBuffer[dataStartIndex];
    dataStartIndex++;

    //Map to OSC ID
    integerCharPointer = (unsigned char*)&tempMapToOSC;
    integerCharPointer[0] = fileBuffer[dataStartIndex];
    integerCharPointer[1] = fileBuffer[dataStartIndex+1];
    dataStartIndex+=2;

    midiMap.addNode(tempMIDIName,tempMIDIChanel,tempCommand,tempOpcode,tempOpcodeTwo,tempMapToOSC);
    fileIndex=dataStartIndex;
  }
}

void saveOSCConfig()
{
  File configFileObject;
  LLNODE* oscNode;
  unsigned short int oscNodeCounter=0, bytesWritten=0;
  
  configFileObject = SPIFFS.open(oscConfigFilePath, "w");
  {
    for(oscNodeCounter=0; oscNodeCounter<oscObject.totalNodes; oscNodeCounter++)
    {
      oscNode = oscObject.findByID(oscNodeCounter);
      if(oscNode!=NULL)
      {
        //Node ID
        bytesWritten += configFileObject.write((byte*)&(oscNode->_nodeID), sizeof((oscNode->_nodeID)));
        //Record Seperator
        //bytesWritten += configFileObject.write(',');
        //Node Name
        bytesWritten += configFileObject.write((uint8_t*)oscNode->_controllName, strlen(oscNode->_controllName));
        //Record Seperator
        bytesWritten += configFileObject.write(',');
        //Number of Vars
        bytesWritten += configFileObject.write((byte*)&(oscNode->_numOfVars), sizeof((oscNode->_numOfVars)));
        //Record Seperator
        //bytesWritten += configFileObject.write(',');
        //Var Type
        bytesWritten += configFileObject.write(oscNode->_varType);
        //Record Seperator
        bytesWritten += configFileObject.write(';');
      }
    }
  } 
  configFileObject.close();
  Serial.printf("\r\nSaved\t%d bytes to the OSC Config file!", bytesWritten);
}

void saveMIDIConfig()
{
  File configFileObject;
  MIDINODE* midiNode;
  unsigned short int midiNodeCounter=0, bytesWritten=0;
  
  configFileObject = SPIFFS.open(midiConfigFilePath, "w");
  {
    //Save the MIDI Control Names in the 1st write operation
    for(midiNodeCounter=0; midiNodeCounter<midiMap.totalNodes; midiNodeCounter++)
    {
      midiNode = midiMap.findNode(midiNodeCounter);
      if(midiNode!=NULL)
      {
        //Node Name
        bytesWritten += configFileObject.write((uint8_t*)midiNode->_deviceName, strlen(midiNode->_deviceName));
        //Record Seperator
        if(midiNodeCounter+1<midiMap.totalNodes)
        {
          bytesWritten += configFileObject.write(',');
        }
        else
        {
          //End the Names list with a ';'
          bytesWritten += configFileObject.write(',');
          bytesWritten += configFileObject.write(';');
        }
      }
    }
    //Save each MIDI control items other vars, no terminator bytes
    for(midiNodeCounter=0; midiNodeCounter<midiMap.totalNodes; midiNodeCounter++)
    {
      midiNode = midiMap.findNode(midiNodeCounter);
      if(midiNode!=NULL)
      {
        //Node ID
        bytesWritten += configFileObject.write((byte*)&(midiNode->_nodeID), sizeof((midiNode->_nodeID)));
        //MIDI Chanel
        bytesWritten += configFileObject.write(midiNode->_midiChanel);
        //Controll ID Byte
        bytesWritten += configFileObject.write(midiNode->_controllID);
        //Controll OP Byte 0
        bytesWritten += configFileObject.write(midiNode->_value);
        //Controll OP Byte 1
        bytesWritten += configFileObject.write(midiNode->_isNote);
        //Maps to OSC ID
        bytesWritten += configFileObject.write((byte*)&(midiNode->_mapsToOSCnodeID), sizeof((midiNode->_mapsToOSCnodeID)));
      }
    }
  } 
  configFileObject.close();
  Serial.printf("\r\nSaved\t%d bytes to the MIDI Config file!", bytesWritten);
}

void txLastOSCMessageToUI(char* packetBuffer, unsigned short int packetLength)
{
  unsigned short int bufferSize = 0, currentIndex=0;
  float currentValue=0;
  unsigned char* oscObjectsBuffer;
  unsigned char* floatCharPointer;

  //init the bufferSize
  bufferSize = strlen(packetBuffer)+1+1+1+4;
  oscObjectsBuffer = new unsigned char[bufferSize];

  //OSC Name
  memcpy(oscObjectsBuffer, packetBuffer, strlen(packetBuffer));
  currentIndex+=(strlen(packetBuffer));
  oscObjectsBuffer[currentIndex]=44;
  currentIndex++;
  
  //OSC Value Type
  if( findNeedleCount(packetBuffer, packetLength, 'f', 1)!=-1 )
  {
    oscObjectsBuffer[currentIndex] = 'f';
  }
  else
  {
    oscObjectsBuffer[currentIndex] = 'i';
  }
  currentIndex++;
  oscObjectsBuffer[currentIndex]=44;
  currentIndex++;

  oscObjectsBuffer[currentIndex] = packetBuffer[packetLength-1];
  oscObjectsBuffer[currentIndex+1] = packetBuffer[packetLength-2];
  oscObjectsBuffer[currentIndex+2] = packetBuffer[packetLength-3];
  oscObjectsBuffer[currentIndex+3] = packetBuffer[packetLength-4];
    
  ws.binary(webUIClientID, oscObjectsBuffer, bufferSize); 
}

void txLastMIDIMessageToUI(char midiCommand, char controllID, char value)
{
  unsigned short int bufferSize = 3;
  unsigned char* midiObjectsBuffer = new unsigned char[bufferSize];

  midiObjectsBuffer[0] = midiCommand;
  midiObjectsBuffer[1] = controllID;
  midiObjectsBuffer[2] = value;

  ws.binary(webUIClientID, midiObjectsBuffer, bufferSize); 
}

void sendOSCConfig(AsyncWebSocketClient * client)
{
  unsigned short int bufferSize=0, oscNodeCounter=0, currentIndex=0;
  unsigned char* oscObjectsBuffer; 
  unsigned char* integerCharPointer;

  //Add each OSC items length to the buffer
  for(oscNodeCounter=0; oscNodeCounter<oscObject.totalNodes; oscNodeCounter++)
  {
    oscNode = oscObject.findByID(oscNodeCounter);
    if(oscNode!=NULL)
    {
      bufferSize+=2;                      //2 bytes for node_id
      bufferSize++;                       //1 byte for record seperator '44'
      bufferSize+=strlen(oscNode->_controllName); //X bytes for name string
      bufferSize++;                       //1 byte for record seperator '44'
      bufferSize+=2;                        //1 bytes for number of vars
      bufferSize++;                       //1 byte for record seperator '44'
      bufferSize++;                        //1 byte for var type
      bufferSize++;                        //1 byte fo the terminator char 59
    }
  }
  if(bufferSize>0)
  {
    //Alocated memory for the buffer
    oscObjectsBuffer = new unsigned char[bufferSize];
    //fill out the buffer with each nodes details
    for(oscNodeCounter=0; oscNodeCounter<oscObject.totalNodes; oscNodeCounter++)
    {
      oscNode = oscObject.findByID(oscNodeCounter);
      if(oscNode!=NULL)
      {
        //Copy the Node id - 2 bytes
        integerCharPointer = (unsigned char*)&(oscNode->_nodeID);
        oscObjectsBuffer[currentIndex] = integerCharPointer[0];
        oscObjectsBuffer[currentIndex+1] = integerCharPointer[1];
        currentIndex+=2;
        //Terminate Field
        oscObjectsBuffer[currentIndex] = 44;
        currentIndex++;
        //Copy the nodes name x bytes via strlen
        memcpy(oscObjectsBuffer+currentIndex, oscNode->_controllName, strlen(oscNode->_controllName));
        currentIndex+=strlen(oscNode->_controllName);
        //Terminate Field
        oscObjectsBuffer[currentIndex] = 44;
        currentIndex++;
        //copy the number of vars 2 bytes
        integerCharPointer = (unsigned char*)&(oscNode->_numOfVars);
        oscObjectsBuffer[currentIndex] = integerCharPointer[0];
        oscObjectsBuffer[currentIndex+1] = integerCharPointer[1];
        currentIndex+=2;
        //Terminate Field
        oscObjectsBuffer[currentIndex] = 44;
        currentIndex++;
        //Copy the var type - 1 byte
        oscObjectsBuffer[currentIndex] = oscNode->_varType;
        currentIndex++;
        //Terminate Record with a 0
        oscObjectsBuffer[currentIndex] = 59;
        currentIndex++;
      }
    }
  }
  else
  {
    bufferSize = 2;
    //Alocated memory for an empty buffer
    oscObjectsBuffer = new unsigned char[bufferSize];
    oscObjectsBuffer[0]=0;
    oscObjectsBuffer[1]=0;
  }
  //Trasnmit buffer via websocket
  ws.binary(client->id(), oscObjectsBuffer, bufferSize);  
  Serial.printf("\r\n\tSent %d Bytes", bufferSize);
}

void sendMIDIConfig(AsyncWebSocketClient * client)
{
  unsigned short int bufferSize=0, midiNodeCounter=0, currentIndex=0;
  unsigned char* midiObjectsBuffer; 
  unsigned char* integerCharPointer;

  //Add each MIDI items length to the buffer
  for(midiNodeCounter=0; midiNodeCounter<midiMap.totalNodes; midiNodeCounter++)
  {
    midiNode = midiMap.findNode(midiNodeCounter);
    if(midiNode!=NULL)
    {
      bufferSize+=2;                                  //2 bytes for node_id
      bufferSize++;                                   //1 byte for record seperator '44'
      bufferSize+=strlen(midiNode->_deviceName);      //X bytes for name string
      bufferSize++;                                   //1 byte for record seperator '44'
      bufferSize++;                                   //1 bytes for MIDI Chanel
      bufferSize++;                                   //1 byte for record seperator '44'
      bufferSize++;                                   //1 byte for controll id
      bufferSize++;                                   //1 byte for record seperator '44'
      bufferSize++;                                   //1 byte fo control value
      bufferSize++;                                   //1 byte for record seperator '44'
      bufferSize++;                                   //1 byte fo control value 2
      bufferSize++;                                   //1 byte for record seperator '44'
      bufferSize+=2;                                  //2 bytes for map to osc id
      bufferSize++;                                   //1 byte fo the terminator char '59'
    }
  }
  if(bufferSize>0)
  {
    //Alocated memory for the buffer
    midiObjectsBuffer = new unsigned char[bufferSize];
    //fill out the buffer with each nodes details
    for(midiNodeCounter=0; midiNodeCounter<midiMap.totalNodes; midiNodeCounter++)
    {
      midiNode = midiMap.findNode(midiNodeCounter);
      if(midiNode!=NULL)
      {
        //Copy the MIDI Node ID - 2 bytes
        integerCharPointer = (unsigned char*)&(midiNode->_nodeID);
        midiObjectsBuffer[currentIndex] = integerCharPointer[0];
        midiObjectsBuffer[currentIndex+1] = integerCharPointer[1];
        currentIndex+=2;
        //Terminate Field
        midiObjectsBuffer[currentIndex] = 44;
        currentIndex++;
        //Copy the nodes name x bytes via strlen
        memcpy(midiObjectsBuffer+currentIndex, midiNode->_deviceName, strlen(midiNode->_deviceName));
        currentIndex+=strlen(midiNode->_deviceName);
        //Terminate Field
        midiObjectsBuffer[currentIndex] = 44;
        currentIndex++;
        //copy midi chanel
        midiObjectsBuffer[currentIndex] = midiNode->_midiChanel;
        currentIndex++;
        //Terminate Field
        midiObjectsBuffer[currentIndex] = 44;
        currentIndex++;
        //copy midi ID byte
        midiObjectsBuffer[currentIndex] = midiNode->_controllID;
        currentIndex++;
        //Terminate Field
        midiObjectsBuffer[currentIndex] = 44;
        currentIndex++;
        //copy midi VALUE
        midiObjectsBuffer[currentIndex] = midiNode->_value;
        currentIndex++;
        //Terminate Field
        midiObjectsBuffer[currentIndex] = 44;
        currentIndex++;
        //copy midi VALUE 2
        midiObjectsBuffer[currentIndex] = 0;
        currentIndex++;
        //Terminate Field
        midiObjectsBuffer[currentIndex] = 44;
        currentIndex++;
        //copy map to osc id
        integerCharPointer = (unsigned char*)&(midiNode->_mapsToOSCnodeID);
        midiObjectsBuffer[currentIndex] = integerCharPointer[0];
        midiObjectsBuffer[currentIndex+1] = integerCharPointer[1];
        currentIndex+=2;
        //Terminate
        midiObjectsBuffer[currentIndex] = 59;
        currentIndex++;
      }
    }
  }
  else
  {
    bufferSize = 2;
    //Alocated memory for an empty buffer
    midiObjectsBuffer = new unsigned char[bufferSize];
    midiObjectsBuffer[0]=0;
    midiObjectsBuffer[1]=0;
  }
  //Trasnmit buffer via websocket
  ws.binary(client->id(), midiObjectsBuffer, bufferSize);
  Serial.printf("\r\n\tSent %d Bytes", bufferSize);
}

void countOSC(AsyncWebSocketClient * client)
{
  unsigned short int bufferSize=2;
  unsigned char* integerCharPointer;
  unsigned char* dataBuffer = new unsigned char[bufferSize];
  
  integerCharPointer = (unsigned char*)&(oscObject.totalNodes);
  dataBuffer[0] = integerCharPointer[0];
  dataBuffer[1] = integerCharPointer[1];
  
  ws.binary(client->id(), dataBuffer, bufferSize);
}

void countMIDI(AsyncWebSocketClient * client)
{
  unsigned short int bufferSize=2;
  unsigned char* integerCharPointer;
  unsigned char* dataBuffer = new unsigned char[bufferSize];
  
  integerCharPointer = (unsigned char*)&(midiMap.totalNodes);
  dataBuffer[0] = integerCharPointer[0];
  dataBuffer[1] = integerCharPointer[1];
  
  ws.binary(client->id(), dataBuffer, bufferSize);
}

//---------------------------------------------------UI---------------------------------------------------


void loop()
{
  dataAvail = Serial2.available();
  if( dataAvail )
  {
    //Read in 1st Byte
    dataIn = Serial2.read();
  	//debug
    if( rejectedNotes(dataIn) )
    {
      //Serial.printf("\r\nRead->\t[%d]",dataIn);
    	//What to do depending on 1st byte
      switch(dataIn)
    	{
    		case	128 ... 143:		//CH1-16 Note OFF ( dataIn-128+1 )
    									//Serial.printf("\r\n\tProcess NOTE OFF");
    									//processNote(dataIn-128+1, 0);
    									break;
    		case	144 ... 159:		//CH1-16 Note ON ( dataIn-144+1 )
                      //Serial.printf("\r\n\tProcess NOTE ON");
    									//processNote(dataIn-144+1, 1);
    									break;
    		case	176 ... 191:		//CH1-16 Controll change ( dataIn-176+1 )
                      //Serial.printf("\r\n\tProcess CONTROLL CHANGE");
    									processControllChange(dataIn-176+1);
    									break;
    		case	192 ... 207:		//CH1-16 Program change ( dataIn-192+1 )
                      //Serial.printf("\r\n\tProcess Program Change");
    									//processProgramChange(dataIn-192+1);
    									break;			
  	  }
    }
    else
    {
      Serial.printf("\r\n\t\t\t\t\tREJECTED[%d]", dataIn);
    }
  }
  yield();
}

//noteActionType=0=OFF 			noteActionType=1=ON
//read in 2 more bytes
void processNote(char midiChanel, char noteActionType)
{
	readyToWrite=0;
	//set midi chanel
	midiSerialData[readyToWrite] = midiChanel;
  //read in controll ID and its value
  readyToWrite++;
  while(readyToWrite!=3)
  {
    dataAvail = Serial2.available();
    if( dataAvail )
    {
      midiSerialData[readyToWrite] = Serial2.read();
      readyToWrite++;
    }
  }
  //debug
  Serial.printf("\tNOTE VALUE[%d.%d.%d]\t[", midiSerialData[0],midiSerialData[1],midiSerialData[2]);
  Serial.print(midiMap.getNote(midiSerialData[1]));
  Serial.printf("-%d]\tScaler[%d]", midiMap.getOctave(midiSerialData[1]), midiMap.getScaledNote(midiSerialData[1], 52, 24, scaledTo));

  
  if(noteActionType==1)//NOTE ON
  {
    midiMap.findNoteNode(midiSerialData[0], midiSerialData[1])->_scaledValue=midiMap.getScaledNote(midiSerialData[1], 52, 24, scaledTo);
    Serial.printf("\r\n\t\t%d\t%d\t%d\t%d",dataIn,midiSerialData[0], midiSerialData[1],midiSerialData[2]);
  }
  else
  {
   
  }

}

//read in 2 more bytes
void processControllChange(char midiChanel)
{
  LLNODE* oscItem;
  
  readyToWrite=0;
  //set midi chanel
  midiSerialData[readyToWrite] = midiChanel;
  //read in controll ID and its value
  readyToWrite++;
  while(readyToWrite!=3)
  {
    dataAvail = Serial2.available();
    if( dataAvail )
    {
      midiSerialData[readyToWrite] = Serial2.read();
      readyToWrite++;
    }
  }
  Serial.printf("\r\n\tInside ProcessChange\tmidiChanel[%d]\tReceived[%d|%d|%d]", midiChanel, dataIn ,midiSerialData[1],midiSerialData[2]);
  //locate MIDI object in system
  if(midiMap.findNode(midiSerialData[0], midiSerialData[1])!=NULL)
  {
    midiMap.setNode(midiSerialData[0], midiSerialData[1], midiSerialData[2]);
    //Serial.printf("\r\nControll [ %d ]\t[");
    //Serial.print(midiMap.findNode(midiSerialData[0], midiSerialData[1])->_deviceName);
    //Serial.printf("]\tValue [ %d ]\tScaled[ %f ]", midiSerialData[2], round(midiMap.findNode(midiSerialData[0], midiSerialData[1])->_scaledValue*100));
    //locate OSC maping
    oscItem = oscObject.findByID(midiMap.findNode(midiSerialData[0], midiSerialData[1])->_mapsToOSCnodeID);
    if(oscItem!=NULL)
    {
      oscItem->_currentValue = ((float)midiSerialData[2])/128;
      txOSC(midiMap.findNode(midiSerialData[0], midiSerialData[1])->_mapsToOSCnodeID);
    }
    else
    {
      Serial.printf("\r\n\t\tNON maped Control!");
    }
  }
  //Transmit last MIDI message to UI
  if(WSConnectState==1)
  {
    txLastMIDIMessageToUI(midiChanel, midiSerialData[1], midiSerialData[2]);
  }
}

//read in 1 more byte
void processProgramChange(char midiChanel)
{
	readyToWrite=0;
  //set midi chanel
  midiSerialData[readyToWrite] = midiChanel;
  //read in controll ID and its value
  readyToWrite++;
  while(readyToWrite!=2)
  {
    dataAvail = Serial2.available();
    if( dataAvail )
    {
      midiSerialData[readyToWrite] = Serial2.read();
      readyToWrite++;
    }
  }
  //locate MIDI object in system
  //Serial.printf("\t%d\t%d", midiSerialData[0], midiSerialData[1]);
  if(midiMap.findNode(midiSerialData[0], midiSerialData[1])!=NULL)
  {
    midiMap.setNode(midiSerialData[0], midiSerialData[1], 0);
    Serial.printf("\r\nProgram [ %d ]\t[");
    Serial.print(midiMap.findNode(midiSerialData[0], midiSerialData[1])->_deviceName);
    Serial.printf("]");
  }
}

void txOSC(unsigned short int OSCNodeID)
{
  unsigned short int txPacketSize = 8, bufferIndex=0, dataSent=0;
  double fractpart, intpart, padding=0;
  char* oscBuffer;
  char oscBufferPadding[4] = {44,102,0,0}, padBuffer[3] = {0,0,0};
  unsigned char* floatCharPointer;
  
  LLNODE* oscItem = oscObject.findByID(OSCNodeID);
  if(oscItem!=NULL)
  {
    txPacketSize += strlen(oscItem->_controllName)+1;
    //Calculate Padding to make the string 32bit able as per osc spec https://opensoundcontrol.stanford.edu/spec-1_0.html#osc-packets
    padding = (strlen(oscItem->_controllName)+1)*8;
    padding = padding/32;
    fractpart = modf(padding , &intpart);
    if(fractpart==0.75)
    {
      padding=1;
    }
    else if(fractpart==0.5)
    {
      padding=2;
    }
    else if(fractpart==0.25)
    {
      padding=3;
    }
    else
    {
      padding=0;
    }
    txPacketSize+=padding;
    oscBuffer = new char[txPacketSize];

    //Store Control Name
    memcpy(oscBuffer, oscItem->_controllName, strlen(oscItem->_controllName));
    bufferIndex+=strlen(oscItem->_controllName);
      oscBuffer[bufferIndex]=0;
      bufferIndex++;
    //Add padding to make it 32bitable
    if(padding!=0)
    {
      memcpy(oscBuffer+bufferIndex, padBuffer, padding);
      bufferIndex+=padding;
    }
    //Add middle of packet
    memcpy(oscBuffer+bufferIndex, oscBufferPadding, 4);
    bufferIndex+=4;
    //Grab 4 bytes of te value float
    floatCharPointer = (unsigned char*)&oscItem->_currentValue;
    
    oscBuffer[bufferIndex] = floatCharPointer[3];
    oscBuffer[bufferIndex+1] = floatCharPointer[2];
    oscBuffer[bufferIndex+2] = floatCharPointer[1];
    oscBuffer[bufferIndex+3] = floatCharPointer[0];
    
    //tx to touch osc
    dataSent = udp.writeTo((uint8_t *)oscBuffer, txPacketSize, touchOSCAddress, oscTXPort);
    /*
    Serial.printf("\t\tSent %d bytes.", dataSent);
    for(bufferIndex=0; bufferIndex<txPacketSize; bufferIndex++)
    {
      Serial.printf("\r\n%d\t[%d]\t[%c]", bufferIndex, oscBuffer[bufferIndex], oscBuffer[bufferIndex]);
    }
    */	  
  }
}

void sendMidi(unsigned short int oscNodeID)
{
	MIDINODE* tempMIDINode;
	LLNODE* tempOscNode;
	unsigned short int nodeCounter = 0;
  char* midiCommand = new char[3];
	
	//locate the midi item to transmit to
	for(nodeCounter=0; nodeCounter<midiMap.totalNodes; nodeCounter++)
	{
		tempMIDINode = midiMap.findNode(nodeCounter);
		if(tempMIDINode!=NULL)
		{
			if(tempMIDINode->_mapsToOSCnodeID==oscNodeID)
			{
				tempOscNode = oscObject.findByID(oscNodeID);
				midiCommand[0] = 176;                                     //Controll Command
        midiCommand[1] = tempMIDINode->_controllID;               //controll id
        midiCommand[2] = (byte)(tempOscNode->_currentValue*128);  //value
       
        Serial2.write(midiCommand, 3);

				break;
			}
		}
	}
		
	//TX MIDI data of the OSC node
}

byte rejectedNotes(byte value)
{
  byte dCnt=0;
  for(dCnt=0; dCnt<deadValueCount; dCnt++)
  {
    if(deadMIDIValues[dCnt] == value)
    {
      return 0;
    }
  }
  return 1;
}
