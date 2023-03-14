#ifndef midiDeviceMapper_h
#define midiDeviceMapper_h

typedef struct midiNode{ 
  unsigned short int _nodeID;
  char* _deviceName;
  char _midiChanel;
  char _controllID;
  char _value;
  float _scaledValue;
  char _isNote;
  midiNode* nextNode; 
}MIDINODE;

class midiDeviceMapper
{
  public:
    midiDeviceMapper();
    
    short int addNode(char* deviceName, char midiChanel, char controllID, char value, char isNote);
    
    MIDINODE* findNode(unsigned short int nodeID);
    MIDINODE* findNode(char* deviceName);
    MIDINODE* findNode(char controllID);
    MIDINODE* findNode(char midiChanel, char controllID);
    MIDINODE* findNoteNode(char midiChanel, char controllID);
    MIDINODE* findNode(char* deviceName, char midiChanel, char controllID);
    
    short int getControllID(char* deviceName);
    short int getmidiChanel(char* deviceName);
    short int getValue(char* deviceName);
    short int getScaledValue(char controllID, char originalValue);
    const char* getNote(char value);
	unsigned short int getOctave(char value);
    unsigned short int getScaledNote(char value, char startAt, char range, char maxTarget);
    
    void setNode(char* deviceName, short int midiChanel=-1, short int controllID=-1, short int value=-1);
    void setNode(unsigned short int nodeID, short int midiChanel=-1, short int controllID=-1, short int value=-1);
    void setNode(char midiChanel, char controllID, char value);
    
    unsigned short int totalNodes;
    MIDINODE* startPointer;
    
	  const char noteCount=12;
	  const char octaveCount=7;
	  const char* defaultNoteList[12] = {"E","F","F#","G","G#","A","A#","B","C","C#","D","D#"};;
	  const char* defaultOctaveList[7] = {"0 ","1 ","2 ","3 ","4 ","5 ","6 "};
	
  private:
    float _maxValue;
    
};
#endif
