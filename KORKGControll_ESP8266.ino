#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "midiDeviceMapper.h"
#include <SoftwareSerial.h>

char midiSerialData[3];
char* tempString = new char[5];
int dataAvail=0;
unsigned short int iCnt=0, chCnt=0;
byte readyToWrite=0;
byte dataIn;
midiDeviceMapper midiMap;
SoftwareSerial swSer, swSerOut;
byte rxPin = D1, txPin = D2;
byte sr_two_rxPin = D3, sr_two_txPin = D3;


//Network Stuff
//Please enter corect details for wifi details
const char * ssid = "WIFIAP";
const char * password = "WIFIKEY";
WiFiUDP Udp;
const unsigned int udpTXPort = 1000;
const byte packetSize=25;
byte dataToSend[packetSize];
IPAddress serverAddress(192,168,1,150);

//midi data to reject
const byte deadValueCount = 2;
const byte deadMIDIValues[deadValueCount] = {248, 254};
const byte maxMidiChans = 1;
const byte noteON = 144;
const unsigned short int cols=146;
unsigned short int scaledTo = 24;
void setup()
{
  Serial.begin(115200);
  Serial.println("\r\n\r\n");

  //Eable WIFI
  /*
  WiFi.mode(WIFI_OFF);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
        delay(100);
        Serial.print(".");
  }
  Serial.printf("\r\nMIDI Module ONLINE\r\nCurrent Ip Address:\t");
  Serial.print(WiFi.localIP());
  Serial.print("\r\n");
  */
  
  
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  pinMode(sr_two_rxPin, INPUT);
  pinMode(sr_two_txPin, OUTPUT);
 
  swSer.begin(31250, SWSERIAL_8N1, rxPin, txPin, false);
  //swSerOut.begin(31250, SWSERIAL_8N1, sr_two_rxPin, sr_two_txPin, false);
  
  //Add terminator to cstring
  tempString[4]=0;
  //Add all notes
  //for(iCnt=0; iCnt<128; iCnt++)
  for(iCnt=28; iCnt<100; iCnt++)
  {
    //floor(iCnt/12) -> OCTAVE INDEX
    //iCnt%12        -> Note Index
    tempString[0] = midiMap.defaultOctaveList[ (byte)floor((iCnt-28)/12) ][0];
    tempString[1] = midiMap.defaultOctaveList[ (byte)floor((iCnt-28)/12) ][1];
    tempString[2] = midiMap.defaultNoteList[(iCnt-28)%12][0];
    tempString[3] = midiMap.defaultNoteList[(iCnt-28)%12][1];
    //Serial.printf("\r\n\t[%c%c%c%c]", tempString[0], tempString[1], tempString[2], tempString[3]);
    //Serial.printf("\r\n\t[%s]\tsize\t%d", tempString, strlen(tempString));
    for(chCnt=1; chCnt<maxMidiChans+1; chCnt++)
    {
      midiMap.addNode(tempString,chCnt,iCnt,0,1);
      //Serial.printf("\r\nAdding Note\t%d\tchan\t%d\t[", iCnt, chCnt,0);
      //Serial.printf(tempString);
      //Serial.printf("]\tStored\t[");
      //Serial.printf(midiMap.findNode(chCnt,iCnt)->_deviceName);
      //Serial.printf("]");
    }
  }
  //add all controlls
  for(chCnt=1; chCnt<maxMidiChans+1; chCnt++)
  {
    midiMap.addNode("PITCH_VCO2",chCnt,35,0,0);
    midiMap.addNode("SHAPE_VCO1",chCnt,36,0,0);
    midiMap.addNode("SHAPE_VCO2",chCnt,37,0,0);
    midiMap.addNode("LEVEL_VCO1",chCnt,39,0,0);
    midiMap.addNode("LEVEL_VCO2",chCnt,40,0,0);
    midiMap.addNode("CUTOFF",chCnt,43,0,0);
    midiMap.addNode("RESONANCE",chCnt,44,0,0);
    midiMap.addNode("EG_ATTACK",chCnt,16,0,0);
    midiMap.addNode("EG_DECAY",chCnt,17,0,0);
    midiMap.addNode("EG_INT",chCnt,25,0,0);
    midiMap.addNode("LFO_RATE",chCnt,24,0,0);
    midiMap.addNode("LFO_DEPTH",chCnt,26,0,0);
    midiMap.addNode("OCTAVE_VCO2",chCnt,49,0,0);
    midiMap.addNode("WAVE_1",chCnt,50,0,0);
    midiMap.addNode("WAVE_2",chCnt,51,0,0);
    midiMap.addNode("SYNC_RING",chCnt,60,0,0);
    midiMap.addNode("DRIVE",chCnt,28,0,0);
    midiMap.addNode("EG_TYPE",chCnt,61,0,0);
    midiMap.addNode("EG_TARGET",chCnt,62,0,0);
    midiMap.addNode("LFO_TARGET",chCnt,56,0,0);
    midiMap.addNode("LFO_WAVE",chCnt,58,0,0);
    midiMap.addNode("LFO_MODE",chCnt,59,0,0);
    midiMap.addNode("AUDIO_OFF",chCnt,120,0,0);
    midiMap.addNode("RESET_CONTROL",chCnt,121,0,0);
  }

  //dump config
  /*
  Serial.printf("\r\nTotal Configd Nodes\t%d", midiMap.totalNodes);
  for(iCnt=1; iCnt<midiMap.totalNodes+1; iCnt++)
  {
    Serial.printf("\r\nNode\t%d\tChan\t%d\tCID\t%d\tName\t", iCnt, midiMap.findNode(iCnt)->_midiChanel,midiMap.findNode(iCnt)->_controllID);
    Serial.print(midiMap.findNode(iCnt)->_deviceName);
  }
  */
}

void loop()
{
  dataAvail = swSer.available();
  if( dataAvail )
  {
    //Read in 1st Byte
  	dataIn = swSer.read();
  	//debug
    if( rejectedNotes(dataIn) )
    {
      Serial.printf("\r\nRead->\t[%d]",dataIn);
    	//What to do depending on 1st byte
      switch(dataIn)
    	{
    		case	128 ... 143:		//CH1-16 Note OFF ( dataIn-128+1 )
    									//Serial.printf("\tProcess NOTE OFF");
    									processNote(dataIn-128+1, 0);
    									break;
    		case	144 ... 159:		//CH1-16 Note ON ( dataIn-144+1 )
                      //Serial.printf("\tProcess NOTE ON");
    									processNote(dataIn-144+1, 1);
    									break;
    		case	176 ... 191:		//CH1-16 Controll change ( dataIn-176+1 )
                      //Serial.printf("\tProcess CONTROLL CHANGE");
    									processControllChange(dataIn-176+1);
    									break;
    		case	192 ... 207:		//CH1-16 Program change ( dataIn-192+1 )
                      //Serial.printf("\tProcess Program Change");
    									processProgramChange(dataIn-192+1);
    									break;			
  	  }
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
    dataAvail = swSer.available();
    if( dataAvail )
    {
      midiSerialData[readyToWrite] = swSer.read();
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
    //txData(midiMap.findNoteNode(midiSerialData[0], midiSerialData[1])->_nodeID);
    Serial.printf("\r\n\t\t%d\t%d\t%d\t%d",dataIn,midiSerialData[0], midiSerialData[1],midiSerialData[2]);
    delay(1000);
    swSer.write(dataIn);
    swSer.write(midiSerialData[1]);
    swSer.write(midiSerialData[2]);
    Serial.printf("\r\n\tEchoed");    
  }
  else
  {
    delay(1000);
    swSer.write(dataIn);
    swSer.write(midiSerialData[1]);
    swSer.write(midiSerialData[2]);
    Serial.printf("\r\n\tEchoed");    
  }
  //locate MIDI object in system
  
  /*
  if(midiMap.findNoteNode(midiSerialData[0], midiSerialData[1])!=NULL)
  {
    midiMap.setNode(midiSerialData[0], midiSerialData[1], midiSerialData[2]);
    Serial.printf("\r\nNote[ %d ][", noteActionType);
    Serial.print(midiMap.findNoteNode(midiSerialData[0], midiSerialData[1])->_deviceName);
    Serial.printf("] Value [ %d ]", midiSerialData[2]);
    midiMap.findNoteNode(midiSerialData[0], midiSerialData[1])->_scaledValue=midiMap.getScaledNote(midiSerialData[1], 52, 24, 255);
   txData(midiMap.findNoteNode(midiSerialData[0], midiSerialData[1])->_nodeID);    
  }
  */
}

//read in 2 more bytes
void processControllChange(char midiChanel)
{
  readyToWrite=0;
  //set midi chanel
  midiSerialData[readyToWrite] = midiChanel;
  //read in controll ID and its value
  readyToWrite++;
  while(readyToWrite!=3)
  {
    dataAvail = swSer.available();
    if( dataAvail )
    {
      midiSerialData[readyToWrite] = swSer.read();
      readyToWrite++;
    }
  }
  //locate MIDI object in system
  //Serial.printf("\t%d\t%d\t%d", midiSerialData[0], midiSerialData[1], midiSerialData[2]);
  if(midiMap.findNode(midiSerialData[0], midiSerialData[1])!=NULL)
  {
    midiMap.setNode(midiSerialData[0], midiSerialData[1], midiSerialData[2]);
    Serial.printf("\r\nControll [ %d ]\t[");
    Serial.print(midiMap.findNode(midiSerialData[0], midiSerialData[1])->_deviceName);
    Serial.printf("]\tValue [ %d ]\tScaled[ %f ]", midiSerialData[2], round(midiMap.findNode(midiSerialData[0], midiSerialData[1])->_scaledValue*100));
    //txData(midiMap.findNode(midiSerialData[0], midiSerialData[1])->_nodeID);
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
    dataAvail = swSer.available();
    if( dataAvail )
    {
      midiSerialData[readyToWrite] = swSer.read();
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
    //txData(midiMap.findNode(midiSerialData[0], midiSerialData[1])->_nodeID);
  }
}

/*
void txData(unsigned short int midiObjectID)
{
  

  //if(midiMap.findNode((char)midiObjectID)!=NULL)
  //{
  //  Serial.printf("\t\tFakeTX\t[%s]\t[%d]", midiMap.findNode(midiObjectID)->_deviceName,midiMap.findNode(midiObjectID)->_scaledValue);
  //}

  //clear TX packet
  for(iCnt=0; iCnt<packetSize; iCnt++)
  {
    dataToSend[iCnt]=0;
  }
  //Store MIDI Device Name
  memcpy(dataToSend, midiMap.findNode(midiObjectID)->_deviceName, strlen(midiMap.findNode(midiObjectID)->_deviceName)+1);
  //Store MIDI Chan in location 20
  dataToSend[20] = midiMap.findNode(midiObjectID)->_midiChanel;
  //Store controll ID in location 21
  dataToSend[21] = midiMap.findNode(midiObjectID)->_controllID;
  //store value in location 22
  dataToSend[22] = midiMap.findNode(midiObjectID)->_value;
  //store scaled value in location 23
  if(midiMap.findNode(midiObjectID)->_isNote==0)
  {
    dataToSend[23] = round(midiMap.findNode(midiObjectID)->_scaledValue*100);
  }
  else
  {
    dataToSend[23] = midiMap.findNode(midiObjectID)->_scaledValue;
  }
  //store ISNOTE value in location 24
  dataToSend[24] = midiMap.findNode(midiObjectID)->_isNote;
  //TX
  Udp.beginPacket(serverAddress, udpTXPort);
  Udp.write(dataToSend, packetSize);
  Udp.endPacket();
  
}
*/

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
