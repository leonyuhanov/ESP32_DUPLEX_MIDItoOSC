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
  LLNode* nextNode; 
}LLNODE;

class osc
{
  public:
    osc();
    void parseOSCPacket();
    void toggleState();
    void clearBuffer();
    void addControll(char* controllName);
    void addControll(char* controllName, unsigned short int numOfValues);
    void addControll(char* controllName, unsigned short int numOfValues, char valueType);
    LLNODE* findLast();
    LLNODE* findByID(unsigned short int nodeID);
    float getValue(unsigned short int nodeID);
    LLNODE* findByName(char* controllName);
    void deleteNode(unsigned short int nodeID);
    LLNODE* findPrev(unsigned short int nodeID);
	
    LLNODE* startPointer;
    unsigned short int totalNodes;
    
    char* packetBuffer;
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
