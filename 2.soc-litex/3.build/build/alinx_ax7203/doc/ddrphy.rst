DDRPHY
======

Register Listing for DDRPHY
---------------------------

+----------------------------------------------------------------+------------------------------------------------+
| Register                                                       | Address                                        |
+================================================================+================================================+
| :ref:`DDRPHY_RST <DDRPHY_RST>`                                 | :ref:`0xf000b800 <DDRPHY_RST>`                 |
+----------------------------------------------------------------+------------------------------------------------+
| :ref:`DDRPHY_DLY_SEL <DDRPHY_DLY_SEL>`                         | :ref:`0xf000b804 <DDRPHY_DLY_SEL>`             |
+----------------------------------------------------------------+------------------------------------------------+
| :ref:`DDRPHY_HALF_SYS8X_TAPS <DDRPHY_HALF_SYS8X_TAPS>`         | :ref:`0xf000b808 <DDRPHY_HALF_SYS8X_TAPS>`     |
+----------------------------------------------------------------+------------------------------------------------+
| :ref:`DDRPHY_WLEVEL_EN <DDRPHY_WLEVEL_EN>`                     | :ref:`0xf000b80c <DDRPHY_WLEVEL_EN>`           |
+----------------------------------------------------------------+------------------------------------------------+
| :ref:`DDRPHY_WLEVEL_STROBE <DDRPHY_WLEVEL_STROBE>`             | :ref:`0xf000b810 <DDRPHY_WLEVEL_STROBE>`       |
+----------------------------------------------------------------+------------------------------------------------+
| :ref:`DDRPHY_RDLY_DQ_RST <DDRPHY_RDLY_DQ_RST>`                 | :ref:`0xf000b814 <DDRPHY_RDLY_DQ_RST>`         |
+----------------------------------------------------------------+------------------------------------------------+
| :ref:`DDRPHY_RDLY_DQ_INC <DDRPHY_RDLY_DQ_INC>`                 | :ref:`0xf000b818 <DDRPHY_RDLY_DQ_INC>`         |
+----------------------------------------------------------------+------------------------------------------------+
| :ref:`DDRPHY_RDLY_DQ_BITSLIP_RST <DDRPHY_RDLY_DQ_BITSLIP_RST>` | :ref:`0xf000b81c <DDRPHY_RDLY_DQ_BITSLIP_RST>` |
+----------------------------------------------------------------+------------------------------------------------+
| :ref:`DDRPHY_RDLY_DQ_BITSLIP <DDRPHY_RDLY_DQ_BITSLIP>`         | :ref:`0xf000b820 <DDRPHY_RDLY_DQ_BITSLIP>`     |
+----------------------------------------------------------------+------------------------------------------------+
| :ref:`DDRPHY_WDLY_DQ_BITSLIP_RST <DDRPHY_WDLY_DQ_BITSLIP_RST>` | :ref:`0xf000b824 <DDRPHY_WDLY_DQ_BITSLIP_RST>` |
+----------------------------------------------------------------+------------------------------------------------+
| :ref:`DDRPHY_WDLY_DQ_BITSLIP <DDRPHY_WDLY_DQ_BITSLIP>`         | :ref:`0xf000b828 <DDRPHY_WDLY_DQ_BITSLIP>`     |
+----------------------------------------------------------------+------------------------------------------------+
| :ref:`DDRPHY_RDPHASE <DDRPHY_RDPHASE>`                         | :ref:`0xf000b82c <DDRPHY_RDPHASE>`             |
+----------------------------------------------------------------+------------------------------------------------+
| :ref:`DDRPHY_WRPHASE <DDRPHY_WRPHASE>`                         | :ref:`0xf000b830 <DDRPHY_WRPHASE>`             |
+----------------------------------------------------------------+------------------------------------------------+

DDRPHY_RST
^^^^^^^^^^

`Address: 0xf000b800 + 0x0 = 0xf000b800`


    .. wavedrom::
        :caption: DDRPHY_RST

        {
            "reg": [
                {"name": "rst", "bits": 1},
                {"bits": 31},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


DDRPHY_DLY_SEL
^^^^^^^^^^^^^^

`Address: 0xf000b800 + 0x4 = 0xf000b804`


    .. wavedrom::
        :caption: DDRPHY_DLY_SEL

        {
            "reg": [
                {"name": "dly_sel[3:0]", "bits": 4},
                {"bits": 28},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


DDRPHY_HALF_SYS8X_TAPS
^^^^^^^^^^^^^^^^^^^^^^

`Address: 0xf000b800 + 0x8 = 0xf000b808`


    .. wavedrom::
        :caption: DDRPHY_HALF_SYS8X_TAPS

        {
            "reg": [
                {"name": "half_sys8x_taps[4:0]", "attr": 'reset: 16', "bits": 5},
                {"bits": 27},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


DDRPHY_WLEVEL_EN
^^^^^^^^^^^^^^^^

`Address: 0xf000b800 + 0xc = 0xf000b80c`


    .. wavedrom::
        :caption: DDRPHY_WLEVEL_EN

        {
            "reg": [
                {"name": "wlevel_en", "bits": 1},
                {"bits": 31},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


DDRPHY_WLEVEL_STROBE
^^^^^^^^^^^^^^^^^^^^

`Address: 0xf000b800 + 0x10 = 0xf000b810`


    .. wavedrom::
        :caption: DDRPHY_WLEVEL_STROBE

        {
            "reg": [
                {"name": "wlevel_strobe", "bits": 1},
                {"bits": 31},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


DDRPHY_RDLY_DQ_RST
^^^^^^^^^^^^^^^^^^

`Address: 0xf000b800 + 0x14 = 0xf000b814`


    .. wavedrom::
        :caption: DDRPHY_RDLY_DQ_RST

        {
            "reg": [
                {"name": "rdly_dq_rst", "bits": 1},
                {"bits": 31},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


DDRPHY_RDLY_DQ_INC
^^^^^^^^^^^^^^^^^^

`Address: 0xf000b800 + 0x18 = 0xf000b818`


    .. wavedrom::
        :caption: DDRPHY_RDLY_DQ_INC

        {
            "reg": [
                {"name": "rdly_dq_inc", "bits": 1},
                {"bits": 31},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


DDRPHY_RDLY_DQ_BITSLIP_RST
^^^^^^^^^^^^^^^^^^^^^^^^^^

`Address: 0xf000b800 + 0x1c = 0xf000b81c`


    .. wavedrom::
        :caption: DDRPHY_RDLY_DQ_BITSLIP_RST

        {
            "reg": [
                {"name": "rdly_dq_bitslip_rst", "bits": 1},
                {"bits": 31},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


DDRPHY_RDLY_DQ_BITSLIP
^^^^^^^^^^^^^^^^^^^^^^

`Address: 0xf000b800 + 0x20 = 0xf000b820`


    .. wavedrom::
        :caption: DDRPHY_RDLY_DQ_BITSLIP

        {
            "reg": [
                {"name": "rdly_dq_bitslip", "bits": 1},
                {"bits": 31},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


DDRPHY_WDLY_DQ_BITSLIP_RST
^^^^^^^^^^^^^^^^^^^^^^^^^^

`Address: 0xf000b800 + 0x24 = 0xf000b824`


    .. wavedrom::
        :caption: DDRPHY_WDLY_DQ_BITSLIP_RST

        {
            "reg": [
                {"name": "wdly_dq_bitslip_rst", "bits": 1},
                {"bits": 31},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


DDRPHY_WDLY_DQ_BITSLIP
^^^^^^^^^^^^^^^^^^^^^^

`Address: 0xf000b800 + 0x28 = 0xf000b828`


    .. wavedrom::
        :caption: DDRPHY_WDLY_DQ_BITSLIP

        {
            "reg": [
                {"name": "wdly_dq_bitslip", "bits": 1},
                {"bits": 31},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


DDRPHY_RDPHASE
^^^^^^^^^^^^^^

`Address: 0xf000b800 + 0x2c = 0xf000b82c`


    .. wavedrom::
        :caption: DDRPHY_RDPHASE

        {
            "reg": [
                {"name": "rdphase[1:0]", "attr": 'reset: 2', "bits": 2},
                {"bits": 30},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


DDRPHY_WRPHASE
^^^^^^^^^^^^^^

`Address: 0xf000b800 + 0x30 = 0xf000b830`


    .. wavedrom::
        :caption: DDRPHY_WRPHASE

        {
            "reg": [
                {"name": "wrphase[1:0]", "attr": 'reset: 3', "bits": 2},
                {"bits": 30},
            ], "config": {"hspace": 400, "bits": 32, "lanes": 4 }, "options": {"hspace": 400, "bits": 32, "lanes": 4}
        }


