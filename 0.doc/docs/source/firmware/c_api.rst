<!--
SPDX-FileCopyrightText: 2026 Ahmed Imamović
SPDX-FileCopyrightText: 2026 Tarik Hamedović
SPDX-License-Identifier: CC-BY-SA-4.0
-->

C API Reference
===============

This section exposes the firmware C API using Doxygen + Breathe.

The API is defined in the headers under:

``2.soc/2.sw/uberclock/inc/uberclock``

Core API
--------

.. doxygenfile:: uberclock.h
   :project: uberclock

Hardware Interface
------------------

.. doxygenfile:: uberclock_hw.h
   :project: uberclock

Channels
--------

.. doxygenfile:: uberclock_channels.h
   :project: uberclock

FIFO
----

.. doxygenfile:: uberclock_fifo.h
   :project: uberclock

FFT
---

.. doxygenfile:: uberclock_fft.h
   :project: uberclock

Tracking
--------

.. doxygenfile:: uberclock_track.h
   :project: uberclock
