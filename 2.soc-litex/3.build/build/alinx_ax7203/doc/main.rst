MAIN
====

Register Listing for MAIN
-------------------------

+--------------------------------------------------------+--------------------------------------------+
| Register                                               | Address                                    |
+========================================================+============================================+
| :ref:`MAIN_BYPASS_EN <MAIN_BYPASS_EN>`                 | :ref:`0xf000d000 <MAIN_BYPASS_EN>`         |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_MUX_SEL <MAIN_MUX_SEL>`                     | :ref:`0xf000d004 <MAIN_MUX_SEL>`           |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_METHOD_SEL <MAIN_METHOD_SEL>`               | :ref:`0xf000d008 <MAIN_METHOD_SEL>`        |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_UPSAMPLE_FACTOR <MAIN_UPSAMPLE_FACTOR>`     | :ref:`0xf000d00c <MAIN_UPSAMPLE_FACTOR>`   |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_TX_LPF_CUTOFF <MAIN_TX_LPF_CUTOFF>`         | :ref:`0xf000d010 <MAIN_TX_LPF_CUTOFF>`     |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_CORDIC_TX_PHASE <MAIN_CORDIC_TX_PHASE>`     | :ref:`0xf000d014 <MAIN_CORDIC_TX_PHASE>`   |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_DOWNSAMPLE_FACTOR <MAIN_DOWNSAMPLE_FACTOR>` | :ref:`0xf000d018 <MAIN_DOWNSAMPLE_FACTOR>` |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_RX_LPF_CUTOFF <MAIN_RX_LPF_CUTOFF>`         | :ref:`0xf000d01c <MAIN_RX_LPF_CUTOFF>`     |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_CORDIC_RX_PHASE <MAIN_CORDIC_RX_PHASE>`     | :ref:`0xf000d020 <MAIN_CORDIC_RX_PHASE>`   |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_GAIN_TX <MAIN_GAIN_TX>`                     | :ref:`0xf000d024 <MAIN_GAIN_TX>`           |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_GAIN_RX <MAIN_GAIN_RX>`                     | :ref:`0xf000d028 <MAIN_GAIN_RX>`           |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_PHYSICS_RUN <MAIN_PHYSICS_RUN>`             | :ref:`0xf000d02c <MAIN_PHYSICS_RUN>`       |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_PHYSICS_BUSY <MAIN_PHYSICS_BUSY>`           | :ref:`0xf000d030 <MAIN_PHYSICS_BUSY>`      |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_HS_DBG_ADDR <MAIN_HS_DBG_ADDR>`             | :ref:`0xf000d034 <MAIN_HS_DBG_ADDR>`       |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_HS_DBG_WDATA <MAIN_HS_DBG_WDATA>`           | :ref:`0xf000d038 <MAIN_HS_DBG_WDATA>`      |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_HS_DBG_RDATA <MAIN_HS_DBG_RDATA>`           | :ref:`0xf000d03c <MAIN_HS_DBG_RDATA>`      |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_LS_DBG_ADDR <MAIN_LS_DBG_ADDR>`             | :ref:`0xf000d040 <MAIN_LS_DBG_ADDR>`       |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_LS_DBG_WDATA <MAIN_LS_DBG_WDATA>`           | :ref:`0xf000d044 <MAIN_LS_DBG_WDATA>`      |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_LS_DBG_RDATA <MAIN_LS_DBG_RDATA>`           | :ref:`0xf000d048 <MAIN_LS_DBG_RDATA>`      |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_SD_CMD <MAIN_SD_CMD>`                       | :ref:`0xf000d04c <MAIN_SD_CMD>`            |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_SD_STATUS <MAIN_SD_STATUS>`                 | :ref:`0xf000d050 <MAIN_SD_STATUS>`         |
+--------------------------------------------------------+--------------------------------------------+
| :ref:`MAIN_PHASE_INC <MAIN_PHASE_INC>`                 | :ref:`0xf000d054 <MAIN_PHASE_INC>`         |
+--------------------------------------------------------+--------------------------------------------+

MAIN_BYPASS_EN
^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0x0 = 0xf000d000`

    0=processed path, 1=raw bypass

    .. wavedrom::
        :caption: MAIN_BYPASS_EN

        {
            "reg": [
                {"name": "bypass_en", "bits": 1},
                {"bits": 31},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


MAIN_MUX_SEL
^^^^^^^^^^^^

`Address: 0xf000d000 + 0x4 = 0xf000d004`

    TX channel select (0–4)

    .. wavedrom::
        :caption: MAIN_MUX_SEL

        {
            "reg": [
                {"name": "mux_sel[2:0]", "bits": 3},
                {"bits": 29},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


MAIN_METHOD_SEL
^^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0x8 = 0xf000d008`

    Operating method (1–5)

    .. wavedrom::
        :caption: MAIN_METHOD_SEL

        {
            "reg": [
                {"name": "method_sel[2:0]", "bits": 3},
                {"bits": 29},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


MAIN_UPSAMPLE_FACTOR
^^^^^^^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0xc = 0xf000d00c`

    Upsample ratio

    .. wavedrom::
        :caption: MAIN_UPSAMPLE_FACTOR

        {
            "reg": [
                {"name": "upsample_factor[3:0]", "bits": 4},
                {"bits": 28},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


MAIN_TX_LPF_CUTOFF
^^^^^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0x10 = 0xf000d010`

    TX LPF cutoff code

    .. wavedrom::
        :caption: MAIN_TX_LPF_CUTOFF

        {
            "reg": [
                {"name": "tx_lpf_cutoff[7:0]", "bits": 8},
                {"bits": 24},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 1 }, "options": {"hspace": 400, "bits": 32, "lanes": 1}
        }


MAIN_CORDIC_TX_PHASE
^^^^^^^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0x14 = 0xf000d014`

    TX-CORDIC phase

    .. wavedrom::
        :caption: MAIN_CORDIC_TX_PHASE

        {
            "reg": [
                {"name": "cordic_tx_phase[18:0]", "bits": 19},
                {"bits": 13},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 1 }, "options": {"hspace": 400, "bits": 32, "lanes": 1}
        }


MAIN_DOWNSAMPLE_FACTOR
^^^^^^^^^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0x18 = 0xf000d018`

    Downsample ratio

    .. wavedrom::
        :caption: MAIN_DOWNSAMPLE_FACTOR

        {
            "reg": [
                {"name": "downsample_factor[3:0]", "bits": 4},
                {"bits": 28},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


MAIN_RX_LPF_CUTOFF
^^^^^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0x1c = 0xf000d01c`

    RX LPF cutoff code

    .. wavedrom::
        :caption: MAIN_RX_LPF_CUTOFF

        {
            "reg": [
                {"name": "rx_lpf_cutoff[7:0]", "bits": 8},
                {"bits": 24},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 1 }, "options": {"hspace": 400, "bits": 32, "lanes": 1}
        }


MAIN_CORDIC_RX_PHASE
^^^^^^^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0x20 = 0xf000d020`

    RX-CORDIC phase

    .. wavedrom::
        :caption: MAIN_CORDIC_RX_PHASE

        {
            "reg": [
                {"name": "cordic_rx_phase[18:0]", "bits": 19},
                {"bits": 13},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 1 }, "options": {"hspace": 400, "bits": 32, "lanes": 1}
        }


MAIN_GAIN_TX
^^^^^^^^^^^^

`Address: 0xf000d000 + 0x24 = 0xf000d024`

    TX gain

    .. wavedrom::
        :caption: MAIN_GAIN_TX

        {
            "reg": [
                {"name": "gain_tx[11:0]", "bits": 12},
                {"bits": 20},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 1 }, "options": {"hspace": 400, "bits": 32, "lanes": 1}
        }


MAIN_GAIN_RX
^^^^^^^^^^^^

`Address: 0xf000d000 + 0x28 = 0xf000d028`

    RX gain

    .. wavedrom::
        :caption: MAIN_GAIN_RX

        {
            "reg": [
                {"name": "gain_rx[11:0]", "bits": 12},
                {"bits": 20},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 1 }, "options": {"hspace": 400, "bits": 32, "lanes": 1}
        }


MAIN_PHYSICS_RUN
^^^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0x2c = 0xf000d02c`

    Start/stop resonator physics

    .. wavedrom::
        :caption: MAIN_PHYSICS_RUN

        {
            "reg": [
                {"name": "physics_run", "bits": 1},
                {"bits": 31},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


MAIN_PHYSICS_BUSY
^^^^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0x30 = 0xf000d030`

    Physics engine busy flag

    .. wavedrom::
        :caption: MAIN_PHYSICS_BUSY

        {
            "reg": [
                {"name": "physics_busy", "bits": 1},
                {"bits": 31},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


MAIN_HS_DBG_ADDR
^^^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0x34 = 0xf000d034`

    HS debug RAM address

    .. wavedrom::
        :caption: MAIN_HS_DBG_ADDR

        {
            "reg": [
                {"name": "hs_dbg_addr[15:0]", "bits": 16},
                {"bits": 16},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 1 }, "options": {"hspace": 400, "bits": 32, "lanes": 1}
        }


MAIN_HS_DBG_WDATA
^^^^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0x38 = 0xf000d038`

    HS debug RAM write data

    .. wavedrom::
        :caption: MAIN_HS_DBG_WDATA

        {
            "reg": [
                {"name": "hs_dbg_wdata[31:0]", "bits": 32}
            ], "config": {"hspace": 400, "bits": 32, "lanes": 1 }, "options": {"hspace": 400, "bits": 32, "lanes": 1}
        }


MAIN_HS_DBG_RDATA
^^^^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0x3c = 0xf000d03c`

    HS debug RAM read data

    .. wavedrom::
        :caption: MAIN_HS_DBG_RDATA

        {
            "reg": [
                {"name": "hs_dbg_rdata[31:0]", "bits": 32}
            ], "config": {"hspace": 400, "bits": 32, "lanes": 1 }, "options": {"hspace": 400, "bits": 32, "lanes": 1}
        }


MAIN_LS_DBG_ADDR
^^^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0x40 = 0xf000d040`

    LS debug RAM address

    .. wavedrom::
        :caption: MAIN_LS_DBG_ADDR

        {
            "reg": [
                {"name": "ls_dbg_addr[15:0]", "bits": 16},
                {"bits": 16},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 1 }, "options": {"hspace": 400, "bits": 32, "lanes": 1}
        }


MAIN_LS_DBG_WDATA
^^^^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0x44 = 0xf000d044`

    LS debug RAM write data

    .. wavedrom::
        :caption: MAIN_LS_DBG_WDATA

        {
            "reg": [
                {"name": "ls_dbg_wdata[31:0]", "bits": 32}
            ], "config": {"hspace": 400, "bits": 32, "lanes": 1 }, "options": {"hspace": 400, "bits": 32, "lanes": 1}
        }


MAIN_LS_DBG_RDATA
^^^^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0x48 = 0xf000d048`

    LS debug RAM read data

    .. wavedrom::
        :caption: MAIN_LS_DBG_RDATA

        {
            "reg": [
                {"name": "ls_dbg_rdata[31:0]", "bits": 32}
            ], "config": {"hspace": 400, "bits": 32, "lanes": 1 }, "options": {"hspace": 400, "bits": 32, "lanes": 1}
        }


MAIN_SD_CMD
^^^^^^^^^^^

`Address: 0xf000d000 + 0x4c = 0xf000d04c`

    SD command register

    .. wavedrom::
        :caption: MAIN_SD_CMD

        {
            "reg": [
                {"name": "sd_cmd[7:0]", "bits": 8},
                {"bits": 24},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 1 }, "options": {"hspace": 400, "bits": 32, "lanes": 1}
        }


MAIN_SD_STATUS
^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0x50 = 0xf000d050`

    SD status flags

    .. wavedrom::
        :caption: MAIN_SD_STATUS

        {
            "reg": [
                {"name": "sd_status[7:0]", "bits": 8},
                {"bits": 24},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 1 }, "options": {"hspace": 400, "bits": 32, "lanes": 1}
        }


MAIN_PHASE_INC
^^^^^^^^^^^^^^

`Address: 0xf000d000 + 0x54 = 0xf000d054`

    CORDIC_DAC phase increment

    .. wavedrom::
        :caption: MAIN_PHASE_INC

        {
            "reg": [
                {"name": "phase_inc[18:0]", "bits": 19},
                {"bits": 13},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 1 }, "options": {"hspace": 400, "bits": 32, "lanes": 1}
        }


