#include "LPC214x.h"
#include <stdint.h>
#include <stdio.h>
#include "t962.h"
#include "lcd.h"
#include "nvstorage.h"
#include "eeprom.h"
#include "reflow.h"

#include "reflow_profiles.h"

#define RAMPTEST
#define PIDTEST

extern uint8_t graphbmp[];

// 20 sec dwell time for all profiles

// Chip Quik Thermally Stable Solder Paste No-Clean Sn63/Pb37 T4 leaded profile - TS391AX
static const profile am4300profile = {
	"QC TS 63SN/37PB", {
		 50, 50, 50, 60, 73, 86,100,113,126,140,145,150,155,163,173,183, // 0-150s
		191,200,210,220,230,235,235,235,220,205,190,183,168,154,140,125, // Adjust peak from 205 to 220C
		111, 97, 82, 68, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0  // 320-470s
	}
};

// Chip Quik Thermally Stable Solder Paste No-Clean Sn42/Bi57.6/Ag0.4 T4 low-temp lead-free profile - TS391LT
static const profile nc31profile = {
	"QC TS LOW-TEMP LF", {
		 50, 50, 50, 50, 55, 70, 85, 90, 95,100,102,105,107,110,112,115, // 0-150s
		120,125,130,133,135,140,150,160,165,165,165,158,148,138,123,108, // Adjust peak from 158 to 165C
		 98, 90, 82, 74, 66, 58,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0  // 320-470s
	}
};

// Chip Quik Thermally Stable Solder Paste No-Clean Sn96.5/Ag3.0/Cu0.5 T4 lead-free profile - TS391SNL
/*
static const profile syntechlfprofile = {
	"QC TS TS391SNL LF", {
		 50, 50, 50, 60, 73, 86,100,113,126,140,145,150,155,160,165,170, // 0-150s
		177,185,195,205,215,225,235,245,249,249,249,240,230,217,205,190, // Adjust peak from 230 to 249C
		175,160,145,130,115, 97, 77, 57,  0,  0,  0,  0,  0,  0,  0,  0  // 320-470s
	}
};
*/

// Chip Quik Thermally Stable Solder Paste No-Clean Sn96.5/Ag3.0/Cu0.5 T4 lead-free profile - TS391SNL // no dwell time
static const profile syntechlfprofile = {
	"QC TS TS391SNL LF", {
		 50, 50, 50, 60, 73, 86,100,113,126,140,145,150,155,160,165,170, // 0-150s
		175,180,185,195,205,215,225,235,245,249,245,232,217,200,185,170, // Adjust peak from 230 to 249C
		160,145,130,115,100, 85, 70, 55,  0,  0,  0,  0,  0,  0,  0,  0  // 320-470s
	}
};


#ifdef RAMPTEST
// Ramp speed test temp profile
static const profile rampspeed_testprofile = {
	"RAMP SPEED TEST", {
		 50, 50, 50, 50,245,245,245,245,245,245,245,245,245,245,245,245, // 0-150s
		245,245,245,245,245,245,245,245,245, 50, 50, 50, 50, 50, 50, 50, // 160-310s
		 50, 50, 50, 50, 50, 50, 50, 50,  0,  0,  0,  0,  0,  0,  0,  0  // 320-470s
	}
};
#endif

#ifdef PIDTEST
// PID gain adjustment test profile (5% setpoint change)
static const profile pidcontrol_testprofile = {
	"PID CONTROL TEST",	{
		171,171,171,171,171,171,171,171,171,171,171,171,171,171,171,171, // 0-150s
		180,180,180,180,180,180,180,180,171,171,171,171,171,171,171,171, // 160-310s
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0  // 320-470s
	}
};
#endif

// EEPROM profile 1
static ramprofile ee1 = { "CUSTOM #1" };

// EEPROM profile 2
static ramprofile ee2 = { "CUSTOM #2" };

static const profile* profiles[] = {
	&syntechlfprofile,
	&nc31profile,
	&am4300profile,
#ifdef RAMPTEST
	&rampspeed_testprofile,
#endif
#ifdef PIDTEST
	&pidcontrol_testprofile,
#endif
	(profile*) &ee1,
	(profile*) &ee2
};

#define NUMPROFILES (sizeof(profiles) / sizeof(profiles[0]))

// current profile index
static uint8_t profileidx = 0;

static void ByteswapTempProfile(uint16_t* buf) {
	for (int i = 0; i < NUMPROFILETEMPS; i++) {
		uint16_t word = buf[i];
		buf[i] = word >> 8 | word << 8;
	}
}

void Reflow_LoadCustomProfiles(void) {
	EEPROM_Read((uint8_t*)ee1.temperatures, 2, 96);
	ByteswapTempProfile(ee1.temperatures);

	EEPROM_Read((uint8_t*)ee2.temperatures, 128 + 2, 96);
	ByteswapTempProfile(ee2.temperatures);
}

void Reflow_ValidateNV(void) {
	if (NV_GetConfig(REFLOW_BEEP_DONE_LEN) == 255) {
		// Default 1 second beep length
		NV_SetConfig(REFLOW_BEEP_DONE_LEN, 10);
	}

	if (NV_GetConfig(REFLOW_MIN_FAN_SPEED) == 255) {
		// Default fan speed is now 8
		NV_SetConfig(REFLOW_MIN_FAN_SPEED, 8);
	}

	if (NV_GetConfig(REFLOW_BAKE_SETPOINT_H) == 255 || NV_GetConfig(REFLOW_BAKE_SETPOINT_L) == 255) {
		NV_SetConfig(REFLOW_BAKE_SETPOINT_H, SETPOINT_DEFAULT >> 8);
		NV_SetConfig(REFLOW_BAKE_SETPOINT_L, (uint8_t)SETPOINT_DEFAULT);
		printf("Resetting bake setpoint to default.");
	}

	Reflow_SelectProfileIdx(NV_GetConfig(REFLOW_PROFILE));
}

int Reflow_GetProfileIdx(void) {
	return profileidx;
}

int Reflow_SelectProfileIdx(int idx) {
	if (idx < 0) {
		profileidx = (NUMPROFILES - 1);
	} else if(idx >= NUMPROFILES) {
		profileidx = 0;
	} else {
		profileidx = idx;
	}
	NV_SetConfig(REFLOW_PROFILE, profileidx);
	return profileidx;
}

int Reflow_SelectEEProfileIdx(int idx) {
	if (idx == 1) {
		profileidx = (NUMPROFILES - 2);
	} else if (idx == 2) {
		profileidx = (NUMPROFILES - 1);
	}
	return profileidx;
}

int Reflow_GetEEProfileIdx(void) {
	if (profileidx == (NUMPROFILES - 2)) {
		return 1;
	} else 	if (profileidx == (NUMPROFILES - 1)) {
		return 2;
	} else {
		return 0;
	}
}

int Reflow_SaveEEProfile(void) {
	int retval = 0;
	uint8_t offset;
	uint16_t* tempptr;
	if (profileidx == (NUMPROFILES - 2)) {
		offset = 0;
		tempptr = ee1.temperatures;
	} else if (profileidx == (NUMPROFILES - 1)) {
		offset = 128;
		tempptr = ee2.temperatures;
	} else {
		return -1;
	}
	offset += 2; // Skip "magic"
	ByteswapTempProfile(tempptr);

	// Store profile
	retval = EEPROM_Write(offset, (uint8_t*)tempptr, 96);
	ByteswapTempProfile(tempptr);
	return retval;
}

void Reflow_ListProfiles(void) {
	for (int i = 0; i < NUMPROFILES; i++) {
		printf("%d: %s\n", i, profiles[i]->name);
	}
}

const char* Reflow_GetProfileName(void) {
	return profiles[profileidx]->name;
}

uint16_t Reflow_GetSetpointAtIdx(uint8_t idx) {
	if (idx > (NUMPROFILETEMPS - 1)) {
		return 0;
	}
	return profiles[profileidx]->temperatures[idx];
}

void Reflow_SetSetpointAtIdx(uint8_t idx, uint16_t value) {
	if (idx > (NUMPROFILETEMPS - 1)) { return; }
	if (value > SETPOINT_MAX) { return; }

	uint16_t* temp = (uint16_t*) &profiles[profileidx]->temperatures[idx];
	if (temp >= (uint16_t*)0x40000000) {
		*temp = value; // If RAM-based
	}
}

void Reflow_PlotProfile(int highlight) {
	LCD_BMPDisplay(graphbmp, 0, 0);

	// No need to plot first value as it is obscured by Y-axis
	for(int x = 1; x < NUMPROFILETEMPS; x++) {
		int realx = (x << 1) + XAXIS;
		int y = profiles[profileidx]->temperatures[x] / 5;
		y = YAXIS - y;
		LCD_SetPixel(realx, y);

		if (highlight == x) {
			LCD_SetPixel(realx - 1, y - 1);
			LCD_SetPixel(realx + 1, y + 1);
			LCD_SetPixel(realx - 1, y + 1);
			LCD_SetPixel(realx + 1, y - 1);
		}
	}
}

void Reflow_DumpProfile(int profile) {
	if (profile > NUMPROFILES) {
		printf("\nNo profile with id: %d\n", profile);
		return;
	}

	int current = profileidx;
	profileidx = profile;

	for (int i = 0; i < NUMPROFILETEMPS; i++) {
		printf("%4d,", Reflow_GetSetpointAtIdx(i));
		if (i == 15 || i == 31) {
			printf("\n ");
		}
	}
	printf("\n");
	profileidx = current;
}
