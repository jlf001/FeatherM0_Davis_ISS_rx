// Driver definition for HopeRF RFM69W/RFM69HW, Semtech SX1231/1231H used for
// compatibility with the frequency hopped, spread spectrum signals from a Davis Instrument
// wireless Integrated Sensor Suite (ISS)
//
// This is part of the DavisRFM69 library from https://github.com/dekay/DavisRFM69
// (C) DeKay 2014 dekaymail@gmail.com
//
// As I consider this to be a derived work from the RFM69W library from LowPowerLab,
// it is released under the same Creative Commons Attrib Share-Alike License
// You are free to use/extend this library but please abide with the CC-BY-SA license:
// http://creativecommons.org/licenses/by-sa/3.0/
//
// In accordance with the CC-BY-SA, many modifications by GitHub user "kobuki".
//
// Modified by Jason Foss in 2019 to simplify for use only with Adafruit Feather M0 Radio in Rx Mode.

//#define DAVISRFM69_DEBUG

#ifndef DAVISRFM69_h
#define DAVISRFM69_h

// Davis VP2 standalone station types
#define STYPE_ISS         0x0 // ISS
#define STYPE_TEMP_ONLY   0x1 // Temperature Only Station
#define STYPE_HUM_ONLY    0x2 // Humidity Only Station
#define STYPE_TEMP_HUM    0x3 // Temperature/Humidity Station
#define STYPE_WLESS_ANEMO 0x4 // Wireless Anemometer Station
#define STYPE_RAIN        0x5 // Rain Station
#define STYPE_LEAF        0x6 // Leaf Station
#define STYPE_SOIL        0x7 // Soil Station
#define STYPE_SOIL_LEAF   0x8 // Soil/Leaf Station
#define STYPE_SENSORLINK  0x9 // SensorLink Station (not supported for the VP2)
#define STYPE_OFF         0xA // No station OFF
#define STYPE_VUE         0x10 // pseudo station type for the Vue ISS
							   // since the Vue also has a type of 0x0

#define ISS_TYPE	STYPE_ISS // Change to STYPE_VUE to correctly display wind for VUE

#define LED 13
#define SERIAL_BAUD 115200
#define SPI_CS          8 // SS is the SPI slave select pin, for instance D10 on atmega328
#define RF69_IRQ_PIN    3
#define RF69_IRQ_NUM    3

#define NUMSTATIONS			  1
#define DAVIS_PACKET_LEN     10 // ISS has fixed packet lengths of eight bytes including CRC and two bytes trailing repeater info

#define RF69_MODE_SLEEP       0 // XTAL OFF
#define RF69_MODE_STANDBY     1 // XTAL ON
#define RF69_MODE_RX          2 // RX MODE
#define RF69_MODE_TX          3 // TX MODE
#define RF69_MODE_INIT		0xff // USED ONLY FOR INIT THIS IS NOT VALID OTHERWISE

#define RF69_DAVIS_BW_NARROW  1
#define RF69_DAVIS_BW_WIDE    2

#define RESYNC_THRESHOLD	 49 // max. number of lost packets from a station before rediscovery
#define LATE_PACKET_THRESH 5000 // packet is considered missing after this many micros

#define TUNEIN_USEC		 10000L // 10 milliseconds, amount of time before an expect TX to tune the radio in.	    							
								// this includes possible radio turnaround tx->rx or sleep->rx transitions
								// 10 ms is reliable, should be able to get this faster but
								// the loop is polled, so slow loop calls will cause missed packets

#define DISCOVERY_STEP   150000000L	// 150 seconds
#define FIFO_SIZE			8

#define FREQ_TABLE_LENGTH_US 51
#define FREQ_TABLE_LENGTH_AU 51
#define FREQ_TABLE_LENGTH_EU 5
#define FREQ_TABLE_LENGTH_NZ 51

#define FREQ_BAND_US 0
#define FREQ_BAND_AU 1
#define FREQ_BAND_EU 2
#define FREQ_BAND_NZ 3

// added these here because upstream removed them
#define REG_TESTAFC        0x71

#define RF_FDEVMSB_9900    0x00
#define RF_FDEVLSB_9900    0xa1

#define RF_AFCLOWBETA_ON   0x20
#define RF_AFCLOWBETA_OFF  0x00 // Default

// Below are known, publicly documented packet types for the VP2 and the Vue.

// VP2 packet types
#define VP2P_UV           0x4 // UV index
#define VP2P_RAINSECS     0x5 // seconds between rain bucket tips
#define VP2P_SOLAR        0x6 // solar irradiation
#define VP2P_TEMP         0x8 // outside temperature
#define VP2P_WINDGUST     0x9 // 10-minute wind gust
#define VP2P_HUMIDITY     0xA // outside humidity
#define VP2P_RAIN         0xE // rain bucket tips counter
#define VP2P_SOIL_LEAF    0xF // soil/leaf station

// Vue packet types
#define VUEP_VCAP         0x2 // supercap voltage
#define VUEP_VSOLAR       0x7 // solar panel voltage

/** DavisRFM69 state machine modes */
enum sm_mode {
	SM_IDLE = 0,  				// no stations configured
	SM_SEARCHING = 1, 			// searching for station(s)
	SM_SYNCHRONIZED = 2, 		// in sync with all stations
	SM_RECEIVING = 3,			// receiving from a station
	};

	// Station data structure for managing radio reception
typedef struct __attribute__((packed)) Station {
	byte id;                	// station ID (set with the DIP switch on original equipment)
							  // set it ONE LESS than advertised station id, eg. 0 for station 1 (default) etc.
	byte type;              	// STYPE_XXX station type, eg. ISS, standalone anemometer transmitter, etc. 
	bool active;            	// true when the station is actively listened and will queue packets
	byte repeaterId;        	// repeater id when packet is coming via a repeater, otherwise 0
							  // repeater IDs A..H are stored as 0x8..0xf here

	uint32_t lastRx;   	 	// last time a packet is seen or should have been seen when missed
	uint32_t lastSeen; 	 	// last factual reception time
	uint32_t interval;    	// packet transmit interval for the station: (41 + id) / 16 * 1M microsecs
	uint32_t numResyncs;  	// number of times discovery of this station started because of packet loss
	uint32_t packets; 		// total number of received packets after (re)restart
	uint32_t lostPackets;     // missed packets since a packet was last seen from this station
	uint32_t syncBegan; 		// time sync began for this station.
	uint32_t recvBegan; 		// time we tuned in to receive
	uint32_t earlyAmt;		// microseconds from when we turned on rx to when the last packet was rx'ed (for tuning, we want this small)
	byte progress;			// search(sync) progress in percent.
	byte channel;           	// rx channel the next packet of the station is expected on (moved by amm for packing on 32 bit machines)
	};

typedef struct __attribute__((packed)) WxData {
	byte rain = 0;
	uint16_t rainrate = 0;
	uint16_t rh = 0;
	int16_t soilleaf = -1;
	float solar = -1;
	int16_t temp = 0;
	float uv = -1;
	int16_t vcap = -1;
	int16_t vsolar = -1;
	uint16_t windd = 0;
	uint8_t winddraw = 0;
	uint8_t windgust = 0;
	byte windgustd = 0;
	uint16_t windv = 0;
	};

struct __attribute__((packed)) RadioData {
	byte packet[DAVIS_PACKET_LEN];
	byte channel;
	byte rssi;
	int16_t fei;
	uint32_t delta;
	};

class DavisRFM69 {
public:
	static volatile byte packetOut, qLen;
	RadioData packetFifo[FIFO_SIZE];
	static volatile uint32_t lostPackets;
	static volatile uint32_t packets;
	static volatile byte numStations;
	static enum sm_mode mode;
	static Station *stations;

	DavisRFM69(byte slaveSelectPin, byte interruptPin, byte interruptNum) {
		_slaveSelectPin = slaveSelectPin;
		_interruptPin = interruptPin;
		_interruptNum = interruptNum;
		_mode = RF69_MODE_STANDBY;
		}

	void initialize(byte freqBand);
	void setBandwidth(byte bw);
	void loop();

protected:
	static volatile byte packetIn;
	static volatile byte DATA[DAVIS_PACKET_LEN];
	static volatile byte _mode;
	static volatile byte CHANNEL;
	static volatile int RSSI;
	static volatile int16_t FEI;
	static volatile byte band;
	static volatile uint32_t numResyncs;
	static volatile uint32_t lostStations;
	static volatile byte stationsFound;
	static volatile byte curStation;

	static DavisRFM69* selfPointer;

	byte _slaveSelectPin;
	byte _interruptPin;
	byte _interruptNum;

	void setChannel(byte channel);
	static uint16_t crc16_ccitt(volatile byte *buf, byte len, uint16_t initCrc = 0);
	byte readReg(byte addr);
	void writeReg(byte addr, byte val);
	byte nextChannel(byte channel);
	int findStation(byte id);
	void handleRadioInt();
	uint32_t difftime(uint32_t after, uint32_t before);
	void nextStation();
	void(*userInterrupt)();
	void virtual interruptHandler();
	byte reverseBits(byte b);
	static void isr0();
	void setMode(byte mode);
	void select();
	void unselect();
	};

	// FRF_MSB, FRF_MID, and FRF_LSB for the 51 North American, Australian, New Zealander & 5 European channels
	// used by Davis in frequency hopping

typedef uint8_t FRF_ITEM[3];

static const __attribute__((progmem)) FRF_ITEM FRF_US[FREQ_TABLE_LENGTH_US] =
	{
	  {0xE3, 0xDA, 0x7C},
	  {0xE1, 0x98, 0x71},
	  {0xE3, 0xFA, 0x92},
	  {0xE6, 0xBD, 0x01},
	  {0xE4, 0xBB, 0x4D},
	  {0xE2, 0x99, 0x56},
	  {0xE7, 0x7D, 0xBC},
	  {0xE5, 0x9C, 0x0E},
	  {0xE3, 0x39, 0xE6},
	  {0xE6, 0x1C, 0x81},
	  {0xE4, 0x5A, 0xE8},
	  {0xE1, 0xF8, 0xD6},
	  {0xE5, 0x3B, 0xBF},
	  {0xE7, 0x1D, 0x5F},
	  {0xE3, 0x9A, 0x3C},
	  {0xE2, 0x39, 0x00},
	  {0xE4, 0xFB, 0x77},
	  {0xE6, 0x5C, 0xB2},
	  {0xE2, 0xD9, 0x90},
	  {0xE7, 0xBD, 0xEE},
	  {0xE4, 0x3A, 0xD2},
	  {0xE1, 0xD8, 0xAA},
	  {0xE5, 0x5B, 0xCD},
	  {0xE6, 0xDD, 0x34},
	  {0xE3, 0x5A, 0x0A},
	  {0xE7, 0x9D, 0xD9},
	  {0xE2, 0x79, 0x41},
	  {0xE4, 0x9B, 0x28},
	  {0xE5, 0xDC, 0x40},
	  {0xE7, 0x3D, 0x74},
	  {0xE1, 0xB8, 0x9C},
	  {0xE3, 0xBA, 0x60},
	  {0xE6, 0x7C, 0xC8},
	  {0xE4, 0xDB, 0x62},
	  {0xE2, 0xB9, 0x7A},
	  {0xE5, 0x7B, 0xE2},
	  {0xE7, 0xDE, 0x12},
	  {0xE6, 0x3C, 0x9D},
	  {0xE3, 0x19, 0xC9},
	  {0xE4, 0x1A, 0xB6},
	  {0xE5, 0xBC, 0x2B},
	  {0xE2, 0x18, 0xEB},
	  {0xE6, 0xFD, 0x42},
	  {0xE5, 0x1B, 0xA3},
	  {0xE3, 0x7A, 0x2E},
	  {0xE5, 0xFC, 0x64},
	  {0xE2, 0x59, 0x16},
	  {0xE6, 0x9C, 0xEC},
	  {0xE2, 0xF9, 0xAC},
	  {0xE4, 0x7B, 0x0C},
	  {0xE7, 0x5D, 0x98}
	};

static const __attribute__((progmem)) FRF_ITEM FRF_AU[FREQ_TABLE_LENGTH_AU] =
	{
	   {0xE5, 0x84, 0xDD},
	   {0xE6, 0x43, 0x43},
	   {0xE7, 0x1F, 0xCE},
	   {0xE6, 0x7F, 0x7C},
	   {0xE5, 0xD5, 0x0E},
	   {0xE7, 0x5B, 0xF7},
	   {0xE6, 0xC5, 0x81},
	   {0xE6, 0x07, 0x2B},
	   {0xE6, 0xED, 0xA1},
	   {0xE6, 0x61, 0x58},
	   {0xE5, 0xA3, 0x02},
	   {0xE6, 0xA7, 0x8D},
	   {0xE7, 0x3D, 0xB2},
	   {0xE6, 0x25, 0x3F},
	   {0xE5, 0xB7, 0x0A},
	   {0xE6, 0x93, 0x85},
	   {0xE7, 0x01, 0xDB},
	   {0xE5, 0xE9, 0x26},
	   {0xE7, 0x70, 0x00},
	   {0xE6, 0x57, 0x6C},
	   {0xE5, 0x98, 0xF5},
	   {0xE6, 0xB1, 0x99},
	   {0xE7, 0x29, 0xDB},
	   {0xE6, 0x11, 0x37},
	   {0xE7, 0x65, 0xE3},
	   {0xE5, 0xCB, 0x33},
	   {0xE6, 0x75, 0x60},
	   {0xE6, 0xD9, 0xA9},
	   {0xE7, 0x47, 0xDF},
	   {0xE5, 0x8E, 0xF9},
	   {0xE6, 0x2F, 0x4B},
	   {0xE7, 0x0B, 0xB6},
	   {0xE6, 0x89, 0x68},
	   {0xE5, 0xDF, 0x2B},
	   {0xE6, 0xBB, 0xA5},
	   {0xE7, 0x79, 0xFB},
	   {0xE6, 0xF7, 0xAE},
	   {0xE5, 0xFD, 0x2F},
	   {0xE6, 0x4D, 0x4F},
	   {0xE6, 0xCF, 0x8D},
	   {0xE5, 0xAD, 0x0E},
	   {0xE7, 0x33, 0xD7},
	   {0xE6, 0x9D, 0x91},
	   {0xE6, 0x1B, 0x33},
	   {0xE6, 0xE3, 0xA5},
	   {0xE5, 0xC1, 0x16},
	   {0xE7, 0x15, 0xC2},
	   {0xE5, 0xF3, 0x33},
	   {0xE6, 0x6B, 0x64},
	   {0xE7, 0x51, 0xDB},
	   {0xE6, 0x39, 0x58}
	};

static const __attribute__((progmem)) FRF_ITEM FRF_EU[FREQ_TABLE_LENGTH_EU] =
	{
	  {0xD9, 0x04, 0x45},
	  {0xD9, 0x13, 0x04},
	  {0xD9, 0x21, 0xC2},
	  {0xD9, 0x0B, 0xA4},
	  {0xD9, 0x1A, 0x63}
	};

static const __attribute__((progmem)) FRF_ITEM FRF_NZ[FREQ_TABLE_LENGTH_NZ] =
	{
	  {0xE6, 0x45, 0x0E},
	  {0xE7, 0x43, 0xC7},
	  {0xE6, 0xF4, 0xAC},
	  {0xE6, 0x9C, 0xEE},
	  {0xE7, 0xA4, 0x7B},
	  {0xE6, 0xC0, 0x31},
	  {0xE7, 0xD0, 0x52},
	  {0xE7, 0x20, 0x93},
	  {0xE6, 0x68, 0x31},
	  {0xE7, 0x67, 0x0A},
	  {0xE6, 0xDA, 0x5E},
	  {0xE7, 0xE1, 0xEC},
	  {0xE7, 0x8A, 0x0C},
	  {0xE6, 0x82, 0xA0},
	  {0xE7, 0x0F, 0x1B},
	  {0xE7, 0xBE, 0xE9},
	  {0xE6, 0xB7, 0x3B},
	  {0xE7, 0x4C, 0x6A},
	  {0xE7, 0xFC, 0x5A},
	  {0xE6, 0x4D, 0xF4},
	  {0xE7, 0x92, 0xD1},
	  {0xE6, 0xEB, 0xF8},
	  {0xE6, 0x94, 0x39},
	  {0xE7, 0xEA, 0xC1},
	  {0xE7, 0x29, 0x79},
	  {0xE6, 0x5F, 0x7D},
	  {0xE7, 0x5E, 0x35},
	  {0xE6, 0xC8, 0xC5},
	  {0xE7, 0xB6, 0x25},
	  {0xE6, 0xA5, 0xB2},
	  {0xE6, 0xFD, 0x81},
	  {0xE7, 0x6F, 0xCF},
	  {0xE6, 0x79, 0xCB},
	  {0xE7, 0x9B, 0xB6},
	  {0xE7, 0x32, 0x2D},
	  {0xE7, 0xC7, 0x7D},
	  {0xE6, 0x8B, 0x54},
	  {0xE7, 0x81, 0x37},
	  {0xE6, 0xD1, 0x89},
	  {0xE7, 0x55, 0x60},
	  {0xE7, 0xD9, 0x17},
	  {0xE6, 0x56, 0xA8},
	  {0xE7, 0x06, 0x35},
	  {0xE7, 0xAD, 0x2F},
	  {0xE6, 0xAE, 0x77},
	  {0xE7, 0x3B, 0x12},
	  {0xE7, 0xF3, 0x85},
	  {0xE6, 0x71, 0x06},
	  {0xE7, 0x17, 0xCF},
	  {0xE6, 0xE3, 0x12},
	  {0xE7, 0x78, 0xA4}
	};

static const FRF_ITEM *bandTab[4] = {
  FRF_US,
  FRF_AU,
  FRF_EU,
  FRF_NZ
	};

static const uint8_t bandTabLengths[4] = {
  FREQ_TABLE_LENGTH_US,
  FREQ_TABLE_LENGTH_AU,
  FREQ_TABLE_LENGTH_EU,
  FREQ_TABLE_LENGTH_NZ
	};

#endif  // DAVISRFM_h
