//==========================================================================
// Copyright (C) 2024-2025 Chili.CHIPS*ba
//--------------------------------------------------------------------------
//                      PROPRIETARY INFORMATION
//
// The information contained in this file is the property of CHILI CHIPS LLC.
// Except as specifically authorized in writing by CHILI CHIPS LLC, the holder
// of this file: (1) shall keep all information contained herein confidential;
// and (2) shall protect the same in whole or in part from disclosure and
// dissemination to all third parties; and (3) shall use the same for operation
// and maintenance purposes only.
//--------------------------------------------------------------------------
// Description:
//   Project-wide Data Type Definitions
//==========================================================================

package signal_types_pkg;

    //--------------------------------------------------------------------------
    // DAC Memory Sample Structure (32-bit word)
    // Used for streaming dual-channel 14-bit data from BRAM to DAC hardware.
    //--------------------------------------------------------------------------
    typedef struct packed {
        logic [1:0]  dac_unused1; // [31:30] Reserved/Padding
        logic [13:0] dac_ch1;     // [29:16] Channel 1 Data (14-bit)
        logic [1:0]  dac_unused0; // [15:14] Reserved/Padding
        logic [13:0] dac_ch0;     // [13:0]  Channel 0 Data (14-bit)
    } dac_sample_t;

    //--------------------------------------------------------------------------
    // ADC Memory Sample Structure (32-bit word)
    // Used for capturing dual-channel 12-bit data from ADC to BRAM.
    //--------------------------------------------------------------------------
    typedef struct packed {
        logic [3:0]  adc_unused1; // [31:28] Reserved/Padding
        logic [11:0] adc_ch1;     // [27:16] Channel 1 Data (12-bit)
        logic [3:0]  adc_unused0; // [15:12] Reserved/Padding
        logic [11:0] adc_ch0;     // [11:0]  Channel 0 Data (12-bit)
    } adc_sample_t;

endpackage : signal_types_pkg
