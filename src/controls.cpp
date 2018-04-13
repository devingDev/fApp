#include "header/controls.hpp"
#include <psp2/ctrl.h>


SceCtrlData ctrl;

void SetupControls(){
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
}


void ReadControls(){
	sceCtrlPeekBufferPositive(0, &ctrl, 1);
}

void ReadBlockingPressCross(){
	while(true){
		if(ctrl.buttons & (SCE_CTRL_CROSS )){
			break;
		}
	}
}


bool PressedCross(){
	if(ctrl.buttons & SCE_CTRL_CROSS ){
		return true;
	}
	return false;
}

bool PressedSquare(){
	if(ctrl.buttons & SCE_CTRL_SQUARE ){
		return true;
	}
	return false;
}

bool PressedCircle(){
	if(ctrl.buttons & SCE_CTRL_CIRCLE ){
		return true;
	}
	return false;
}

bool PressedStart(){
	if(ctrl.buttons & SCE_CTRL_START ){
		return true;
	}
	return false;
}

bool PressedDown(){
	if(ctrl.buttons & SCE_CTRL_DOWN ){
		return true;
	}
	return false;
}


bool PressedUp(){
	if(ctrl.buttons & SCE_CTRL_UP ){
		return true;
	}
	return false;
}

bool PressedRight(){
	if(ctrl.buttons & SCE_CTRL_RIGHT ){
		return true;
	}
	return false;
}


bool PressedLeft(){
	if(ctrl.buttons & SCE_CTRL_LEFT ){
		return true;
	}
	return false;
}

bool PressedR1(){
	if(ctrl.buttons & SCE_CTRL_RTRIGGER ){
		return true;
	}
	return false;
}


bool PressedL1(){
	if(ctrl.buttons & SCE_CTRL_LTRIGGER ){
		return true;
	}
	return false;
}
