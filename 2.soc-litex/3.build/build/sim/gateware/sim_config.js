[
    {
        "module": "clocker",
        "interface": [
            "sys_clk"
        ],
        "args": {
            "freq_hz": 1000000,
            "phase_deg": 0
        },
        "tickfirst": true
    },
    {
        "module": "serial2console",
        "interface": [
            "serial",
            "sys_clk"
        ]
    },
    {
        "timebase": 500000
    }
]