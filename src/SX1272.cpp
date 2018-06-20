#include "SX1272.h"

SX1272::SX1272(int nss, float freq, Bandwidth bw, SpreadingFactor sf, CodingRate cr, int dio0, int dio1) : SX127x(CH_SX1272, dio0, dio1) {
  _nss = nss;
  _dio0 = dio0;
  _dio1 = dio1;
  
  _bw = bw;
  _sf = sf;
  _cr = cr;
  _freq = freq;
}

uint8_t SX1272::begin() {
  // initialize low-level drivers
  initModule(_nss, _dio0, _dio1);
  
  // execute common part
  uint8_t status = SX127x::begin();
  if(status != ERR_NONE) {
    return(status);
  }
  
  // start configuration
  return(config(_bw, _sf, _cr, _freq));
}

uint8_t SX1272::rxSingle(char* data, uint8_t* length) {
  // get header mode
  bool headerExplMode = false;
  if(getRegValue(SX127X_REG_MODEM_CONFIG_1, 0, 0) == SX1272_HEADER_EXPL_MODE) {
    headerExplMode = true;
  }
  
  // execute common part
  return SX127x::rxSingle(data, length, headerExplMode);
}

uint8_t SX1272::setBandwidth(Bandwidth bw) {
  uint8_t state = config(bw, _sf, _cr, _freq);
  if(state == ERR_NONE) {
    _bw = bw;
  }
  return(state);
}

uint8_t SX1272::setSpreadingFactor(SpreadingFactor sf) {
  uint8_t state = config(_bw, sf, _cr, _freq);
  if(state == ERR_NONE) {
    _sf = sf;
  }
  return(state);
}

uint8_t SX1272::setCodingRate(CodingRate cr) {
  uint8_t state = config(_bw, _sf, cr, _freq);
  if(state == ERR_NONE) {
    _cr = cr;
  }
  return(state);
}

uint8_t SX1272::setFrequency(float freq) {
  uint8_t state = config(_bw, _sf, _cr, freq);
  if(state == ERR_NONE) {
    _freq = freq;
  }
  return(state);
}

uint8_t SX1272::config(Bandwidth bw, SpreadingFactor sf, CodingRate cr, float freq) {
  uint8_t status = ERR_NONE;
  uint8_t newBandwidth, newSpreadingFactor, newCodingRate;
  
  // check the supplied BW, CR and SF values
  switch(bw) {
    case BW_125_00_KHZ:
      newBandwidth = SX1272_BW_125_00_KHZ;
      break;
    case BW_250_00_KHZ:
      newBandwidth = SX1272_BW_250_00_KHZ;
      break;
    case BW_500_00_KHZ:
      newBandwidth = SX1272_BW_500_00_KHZ;
      break;
    default:
      return(ERR_INVALID_BANDWIDTH);
  }
  
  switch(sf) {
    case SF_6:
      newSpreadingFactor = SX127X_SF_6;
      break;
    case SF_7:
      newSpreadingFactor = SX127X_SF_7;
      break;
    case SF_8:
      newSpreadingFactor = SX127X_SF_8;
      break;
    case SF_9:
      newSpreadingFactor = SX127X_SF_9;
      break;
    case SF_10:
      newSpreadingFactor = SX127X_SF_10;
      break;
    case SF_11:
      newSpreadingFactor = SX127X_SF_11;
      break;
    case SF_12:
      newSpreadingFactor = SX127X_SF_12;
      break;
    default:
      return(ERR_INVALID_SPREADING_FACTOR);
  }
  
  switch(cr) {
    case CR_4_5:
      newCodingRate = SX1272_CR_4_5;
      break;
    case CR_4_6:
      newCodingRate = SX1272_CR_4_6;
      break;
    case CR_4_7:
      newCodingRate = SX1272_CR_4_7;
      break;
    case CR_4_8:
      newCodingRate = SX1272_CR_4_8;
      break;
    default:
      return(ERR_INVALID_CODING_RATE);
  }
  
  if((freq < 860.0) || (freq > 1020.0)) {
    return(ERR_INVALID_FREQUENCY);
  }
  
  // execute common part
  status = configCommon(newBandwidth, newSpreadingFactor, newCodingRate, freq);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // configuration successful, save the new settings
  _bw = bw;
  _sf = sf;
  _cr = cr;
  _freq = freq;
  
  return(ERR_NONE);
}

uint8_t SX1272::configCommon(uint8_t bw, uint8_t sf, uint8_t cr, float freq) {
  // configure common registers
  uint8_t status = SX127x::config(bw, sf, cr, freq);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // output power configuration
  status = setRegValue(SX1272_REG_PA_DAC, SX127X_PA_BOOST_ON, 2, 0);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // enable LNA gain setting by register
  status = setRegValue(SX127X_REG_MODEM_CONFIG_2, SX1272_AGC_AUTO_OFF, 2, 2);
  if(status != ERR_NONE) {
    return(status);
  }
  
  // set SF6 optimizations
  if(sf == SX127X_SF_6) {
    status = setRegValue(SX127X_REG_MODEM_CONFIG_1, bw | cr | SX1272_HEADER_IMPL_MODE | SX1272_RX_CRC_MODE_OFF, 7, 1);
  } else {
    status = setRegValue(SX127X_REG_MODEM_CONFIG_1, bw | cr | SX1272_HEADER_EXPL_MODE | SX1272_RX_CRC_MODE_ON,  7, 1);
  }
  if(status != ERR_NONE) {
    return(status);
  }
  
  // set low datarate optimizations for SF11/SF12 with 125 kHz bandwidth
  if((bw == SX1272_BW_125_00_KHZ) && ((sf == SX127X_SF_11) || (sf == SX127X_SF_12))) {
    status = setRegValue(SX127X_REG_MODEM_CONFIG_1, SX1272_LOW_DATA_RATE_OPT_ON,  0, 0);
  } else {
    status = setRegValue(SX127X_REG_MODEM_CONFIG_1, SX1272_LOW_DATA_RATE_OPT_OFF,  0, 0);
  }
  
  return(status);
}
