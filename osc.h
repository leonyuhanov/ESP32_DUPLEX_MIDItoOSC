#ifndef osc_h
#define osc_h
#include "Arduino.h"
#include <stdlib.h> 
#include <stdio.h>
#include <math.h>
#include <string>
#include <cstring>

typedef struct LLNode{ 
  unsigned short int _nodeID;
  char* _controllName;
  unsigned short int _numOfVars;
  char _varType;
  float _currentValue;
  float* _controllValueArray;
  unsigned long _timer[4];
  LLNode* nextNode; 
  
  ~LLNode()
  {
    //destructor
    delete _controllName;
	  delete _controllValueArray;
  }
}LLNODE;

class osc
{
  public:
    osc();
    void parseOSCPacket();
    short int parseOSCPacketFiltered(char* tempControllName, unsigned short int valueIndex, float valueToKeep, float instrumentID, char* destinationControllName);
    void parseOSCPacketFiltered(char* tempControllName, unsigned short int valueIndex, float valueToKeep);
    void toggleState();
    void clearBuffer();
    void clearTXBuffer();
    void addControll(char* controllName);
    void addControll(char* controllName, unsigned short int numOfValues);
    void addControll(char* controllName, unsigned short int numOfValues, char valueType);
    void addControll(char* controllName, unsigned short int numOfValues, char valueType, unsigned long timeOut);
	  
	  void startTimer(unsigned long durationInMillis, unsigned long* timer);
	  byte hasTimedOut(unsigned long* timer);
	  byte hasControllTimedOut(char* controllName);
    void timeOutControll(char* controllName);
    
    LLNODE* findLast();
    LLNODE* findByID(unsigned short int nodeID);
    float getValue(unsigned short int nodeID);
	  float getValue(char* controllName);
    void setValue(char* controllName, float valueToSet);
    void generateOSCPacket(char* controllName);
    LLNODE* findByName(char* controllName);
    void deleteNode(unsigned short int nodeID);
    LLNODE* findPrev(unsigned short int nodeID);
    char* parseControlName(char* dataPacket);
	
    LLNODE* startPointer;
    unsigned short int totalNodes;
    
    char* packetBuffer;
    char* txPacketBuffer;
    unsigned short int txPacketBufferLength;
    unsigned short int maxPacketBufferSize;
    unsigned short int currentPacketSize;
    byte isChanged;
    float primaryControllValue;
    unsigned char* floatCharPointer;
    unsigned short int currentControllID;
	  unsigned short int counter;
	  unsigned short int packetIndex;
	
};

#endif
