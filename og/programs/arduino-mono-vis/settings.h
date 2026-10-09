// select instrument
#define VIS
// #define NIR

// ******** ADJUST HERE FOR INSTRUMENT-SPECIFIC CALIBRATION ***************
#ifdef VIS

// Polynomial constants to convert nm position to steps, from calibration
#define C0  0
// change c1 to -16 since increasing steps leads to shorter wavelength position
#define C1  -16  // visible: 16 steps per nm. 125 steps per second. So 7.8125 nm/s scanning
#define C2  0
#define C3  0

// speed selection
// stepInterval in us: 1000000 / (2*nm_per_min*8/60) with c1=(-)8
#define STEPINTERVAL 4000;  // 4000 microseconds half-period for 125 steps per second
//long speed_nm_min=0; // selected speed in nm/min
//long slow_us=0;   // delay based on speed
#define SPEEDMIN 240 // minimum speed, nm/min: slower than 240 incompatible with current delay scheme
#define SPEEDMAX 420 // Speed max, nm/min, max=420 for stepInterval=4000
#define SPEEDINCR 60 // scan speeds multiple of 60 nm/min
//long autoscan_speed=0; //initialize autoscan speed 
#define COARSEBUTTONINCR 10 // for faster scrolling (manual) and setting auto endpoints

// Wavelength configuration and counters
#define STEPSOFFSET 8984   // Offset after homing
//long stepCounter = 0;            // Tracks current step position from home
#define STARTPOSITION 546  // Start position in nm (green mercury line)
//long position;                   // current position in nm
#define POSITIONMAX 1000  // longest wavelength allowed
#define POSITIONMIN 240   // shortest wavelength allowed
#define POSITIONINCR 1 // increment for position selection button presses 
// ******** END INSTRUMENT-SPECIFIC CALIBRATION ***************

#endif

#ifdef NIR

// Polynomial constants to convert nm position to steps, from calibration
#define C0  0
// change c1 to -16 since increasing steps leads to shorter wavelength position
#define C1  -8  // visible: 16 steps per nm. 125 steps per second. So 7.8125 nm/s scanning
#define C2  0
#define C3  0

// speed selection
// stepInterval in us: 1000000 / (2*nm_per_min*8/60) with c1=(-)8
#define STEPINTERVAL 6250;  // 4000 microseconds half-period for 125 steps per second
//long speed_nm_min=0; // selected speed in nm/min
//long slow_us=0;   // delay based on speed
#define SPEEDMIN 300 // minimum speed, nm/min: slower than 240 incompatible with current delay scheme
#define SPEEDMAX 600 // Speed max, nm/min, max=420 for stepInterval=4000
#define SPEEDINCR 60 // scan speeds multiple of 60 nm/min
//long autoscan_speed=0; //initialize autoscan speed 
#define COARSEBUTTONINCR 10 // for faster scrolling (manual) and setting auto endpoints

// Wavelength configuration and counters
#define STEPSOFFSET 1328   // Offset after homing
//long stepCounter = 0;            // Tracks current step position from home
#define STARTPOSITION 1990  // Start position in nm (green mercury line)
//long position;                   // current position in nm
#define POSITIONMAX 2000  // longest wavelength allowed
#define POSITIONMIN 700   // shortest wavelength allowed
#define POSITIONINCR 10 // increment for position selection button presses 
// ******** END INSTRUMENT-SPECIFIC CALIBRATION ***************

#endif