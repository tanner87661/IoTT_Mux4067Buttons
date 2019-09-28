#ifndef IoTT_ButtonTypeDef_h
#define IoTT_ButtonTypeDef_h

enum buttonType : uint8_t {btnoff=0, autodetect=1, digitalAct=2, touch=3, analog=9};
enum buttonEvent : byte {onbtndown=0, onbtnup=1, onbtnclick=2, onbtndblclick=4, onbtnhold=3, noevent=255};

enum actionType : byte {blockdet=0, dccswitch=1, dccsignal=2, svbutton=3, analoginp=4, powerstat=5, unknown=255};
enum ctrlTypeType : byte {thrown=0, closed=1, toggle=2, nochange=3, input=4};
enum ctrlValueType : byte {offVal=0, onVal=1, idleVal=2};

#endif
