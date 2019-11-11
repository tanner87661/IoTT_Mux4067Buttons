#ifndef IoTT_ButtonTypeDef_h
#define IoTT_ButtonTypeDef_h

enum buttonType : uint8_t {btnoff=0, autodetect=1, digitalAct=2, touch=3, analog=9};
enum buttonEvent : byte {onbtndown=0, onbtnup=1, onbtnclick=2, onbtndblclick=4, onbtnhold=3, noevent=255};

enum actionType : byte {blockdet=0, dccswitch=1, dccsignaldyn=2, dccsignalstat=3, dccsignalnmra=4, svbutton=5, analoginp=6, powerstat=7, constantled=8, unknown=255};
enum ctrlTypeType : byte {thrown=0, closed=1, toggle=2, nochange=3, input=4};
enum ctrlValueType : byte {offVal=0, onVal=1, idleVal=2};

#endif
