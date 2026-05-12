# 3 Tone tracking 

In this file the tracking of the three tones/modes of our oscillator is presented. Each tone can drift in frequnecy as time passes, or temperature and other parameters change. 
The challange is to lock onto these modes and always track how and where they move. To discover and roughly find the resonant frequency of single modes a `track3` [algortihm](https://chili-chips-ba.github.io/uberClock/examples/tracking_algorithm.html) is used.
It positions the CPU genarated tones and the CORDIC mixer frequencies near the resonant frequnecy so that `trackQ` [algortihm](https://chili-chips-ba.github.io/uberClock/examples/tracking_algorithm.html) can take over from that starting point.  

## C300 Mode

From our lab data this tone is expected to be found in frequency range: 10.003840 to 10.004000 MHz. 
Using the serial console inteface of the algorithm: `track3 <ch> <start_hz> [step_hz] [max_steps] [N] [center_hz] [delta_hz]`
We try and find this tone in few iterations. 

Firts iteration:
```
uberClock> track3 1 10002840 20 10 2048 1000 20
track3: ch=1 start=10002840 Hz step=20 Hz max_steps=10 N=2048 center=1000 Hz delta=20 Hz Fs=10000 Hz sig3={980,1000,1020} Hz
track3 step=0 phase_down_1=10002840 Hz inc=10327372 bins={201,205,209} pwr={3535,554,323}
track3 step=1 phase_down_1=10002860 Hz inc=10327393 bins={201,205,209} pwr={188,114,66}
track3 step=2 phase_down_1=10002880 Hz inc=10327414 bins={201,205,209} pwr={51,33,42}
track3 step=3 phase_down_1=10002900 Hz inc=10327434 bins={201,205,209} pwr={18,16,25}
track3 step=4 phase_down_1=10002920 Hz inc=10327455 bins={201,205,209} pwr={15,25,25}
track3 step=5 phase_down_1=10002940 Hz inc=10327476 bins={201,205,209} pwr={20,16,13}
track3 step=6 phase_down_1=10002960 Hz inc=10327496 bins={201,205,209} pwr={16,32,17}
track3 lock: phase_down_1=10002960 Hz inc=10327496 center=1000 left=980 right=1020
```
Then the second and final iteration:
```
uberClock> track3 1 10002940 1 20 2048 1000 10
track3: ch=1 start=10002940 Hz step=1 Hz max_steps=20 N=2048 center=1000 Hz delta=10 Hz Fs=10000 Hz sig3={990,1000,1010} Hz
track3 step=0 phase_down_1=10002940 Hz inc=10327476 bins={203,205,207} pwr={60067,136499,18232}
track3 step=1 phase_down_1=10002941 Hz inc=10327477 bins={203,205,207} pwr={75089,110155,16630}
track3 step=2 phase_down_1=10002942 Hz inc=10327478 bins={203,205,207} pwr={67740,57706,12101}
track3 step=3 phase_down_1=10002943 Hz inc=10327479 bins={203,205,207} pwr={74213,33618,10973}
track3 step=4 phase_down_1=10002944 Hz inc=10327480 bins={203,205,207} pwr={94839,25360,12059}
track3 step=5 phase_down_1=10002945 Hz inc=10327481 bins={203,205,207} pwr={129631,16819,4922}
track3 step=6 phase_down_1=10002946 Hz inc=10327482 bins={203,205,207} pwr={4352,25171,68193}
track3 step=7 phase_down_1=10002947 Hz inc=10327483 bins={203,205,207} pwr={6651,34312,72923}
track3 step=8 phase_down_1=10002948 Hz inc=10327484 bins={203,205,207} pwr={9542,69438,91629}
track3 step=9 phase_down_1=10002949 Hz inc=10327485 bins={203,205,207} pwr={8819,71914,90698}
track3 step=10 phase_down_1=10002950 Hz inc=10327486 bins={203,205,207} pwr={8676,69903,84677}
track3 step=11 phase_down_1=10002951 Hz inc=10327487 bins={203,205,207} pwr={10158,62017,76351}
track3 step=12 phase_down_1=10002952 Hz inc=10327488 bins={203,205,207} pwr={15947,54087,63095}
track3 step=13 phase_down_1=10002953 Hz inc=10327489 bins={203,205,207} pwr={24303,54736,49692}
track3 lock: phase_down_1=10002953 Hz inc=10327489 center=1000 left=990 right=1010
```



## A100 Mode

From our lab data this tone is expected to be found in frequency range: 6.261641 to 6.271641 MHz.

Iteration for this mode:

```
uberClock> track3 2 6261000 1000 10 2048 1000 100
track3: ch=2 start=6261000 Hz step=1000 Hz max_steps=10 N=2048 center=1000 Hz delta=100 Hz Fs=10000 Hz sig3={900,1000,1100} Hz
track3 step=0 phase_down_2=6261000 Hz inc=6464132 bins={184,205,225} pwr={0,108,0}
track3 step=1 phase_down_2=6262000 Hz inc=6465164 bins={184,205,225} pwr={0,74,0}
track3 step=2 phase_down_2=6263000 Hz inc=6466197 bins={184,205,225} pwr={0,106,0}
track3 step=3 phase_down_2=6264000 Hz inc=6467229 bins={184,205,225} pwr={0,93,0}
track3 step=4 phase_down_2=6265000 Hz inc=6468262 bins={184,205,225} pwr={0,99,0}
track3 step=5 phase_down_2=6266000 Hz inc=6469294 bins={184,205,225} pwr={0,105,0}
track3 step=6 phase_down_2=6267000 Hz inc=6470326 bins={184,205,225} pwr={0,95,0}
track3 step=7 phase_down_2=6268000 Hz inc=6471359 bins={184,205,225} pwr={77,95,97}
track3 step=8 phase_down_2=6269000 Hz inc=6472391 bins={184,205,225} pwr={96,69,121}
track3 step=9 phase_down_2=6270000 Hz inc=6473424 bins={184,205,225} pwr={105,130,67}
track3 lock: phase_down_2=6270000 Hz inc=6473424 center=1000 left=900 right=1100


```

## C100 Mode
From our lab data this tone is expected to be found in frequency range: 3.388438 to 3.388594MHz.

This is mode is found as:

```
uberClock> track3 3 3387400 10 10 2048 1000 20
track3: ch=3 start=3387400 Hz step=10 Hz max_steps=10 N=2048 center=1000 Hz delta=20 Hz Fs=10000 Hz sig3={980,1000,1020} Hz
track3 step=0 phase_down_3=3387400 Hz inc=3497301 bins={201,205,209} pwr={14621,15243,11202}
track3 step=1 phase_down_3=3387410 Hz inc=3497311 bins={201,205,209} pwr={13495,13321,9311}
track3 step=2 phase_down_3=3387420 Hz inc=3497321 bins={201,205,209} pwr={14570,10784,7988}
track3 step=3 phase_down_3=3387430 Hz inc=3497331 bins={201,205,209} pwr={12620,9838,7987}
track3 step=4 phase_down_3=3387440 Hz inc=3497342 bins={201,205,209} pwr={10305,8390,8350}
track3 step=5 phase_down_3=3387450 Hz inc=3497352 bins={201,205,209} pwr={10714,9075,7482}
track3 step=6 phase_down_3=3387460 Hz inc=3497362 bins={201,205,209} pwr={8867,9387,6490}
track3 lock: phase_down_3=3387460 Hz inc=3497362 center=1000 left=980 right=1020
```


# Tracking

Leaning on the found tones and the CLI interface: `trackq_start <f1> <f2> <f3> [N] [center_hz] [delta_ch1_hz] [delta_ch2_hz] [delta_ch3_hz]`,
we start the tracking and print out the state every 5 seconds. All frequnecies are rough estimations of the current mixer frequnecy value + the 1kHz offset that is chosen as the point of the vertex.

```
uberClock> trackq_start 10002953 6270000 3387460 2048 1000 10 100 20
trackq_start: ch1=10002953 Hz ch2=6270000 Hz ch3=3387459 Hz N=2048 center=1000 Hz delta={10,100,20} Hz sig3={{990,1000,1010},{900,1000,1100},{980,1000,1020}} Hz interval=2 s
trackq hf vertex: ch1=10003952.591Hz ch2=6270999.743Hz ch3=3388454.227Hz
trackq hf vertex: ch1=10003952.591Hz ch2=6270989.468Hz ch3=3388449.384Hz
trackq hf vertex: ch1=10003952.591Hz ch2=6270977.848Hz ch3=3388445.509Hz
trackq hf vertex: ch1=10003951.622Hz ch2=6270966.016Hz ch3=3388444.541Hz
trackq hf vertex: ch1=10003950.059Hz ch2=6270960.719Hz ch3=3388443.572Hz
trackq hf vertex: ch1=10003949.441Hz ch2=6270955.188Hz ch3=3388442.604Hz
trackq hf vertex: ch1=10003949.540Hz ch2=6270953.199Hz ch3=3388441.635Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270951.865Hz ch3=3388441.635Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270945.502Hz ch3=3388437.761Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270945.502Hz ch3=3388436.792Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270945.502Hz ch3=3388436.792Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270941.062Hz ch3=3388435.824Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270935.811Hz ch3=3388434.855Hz


...after some time...

trackq hf vertex: ch1=10003949.685Hz ch2=6270890.293Hz ch3=3388414.515Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270885.451Hz ch3=3388414.515Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388414.515Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270889.236Hz ch3=3388414.515Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388414.515Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388414.515Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270883.513Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270880.839Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270877.096Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270871.984Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270871.891Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270870.922Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270869.953Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270869.953Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270868.985Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270868.985Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270868.985Hz ch3=3388413.546Hz
trackq hf vertex: ch1=10003949.685Hz ch2=6270868.985Hz ch3=3388412.578Hz
```


 Using the lowspeed debug signal capture we can look at the three tones in three channels, shown in the pictures below. We observe that the algorithm will try and stay near the interpolated vertex with the central 1kHz tone.  

 
<img width="1295" height="555" alt="image" src="https://github.com/user-attachments/assets/82087cb3-5908-431e-bf4a-6906a9e5142a" />
<img width="1295" height="555" alt="image" src="https://github.com/user-attachments/assets/40db3eaf-0d2a-4aa4-aaa0-b3759c2dc776" />
<img width="1295" height="555" alt="image" src="https://github.com/user-attachments/assets/994bdcf5-0955-47ed-9a82-b9ecc0bc589a" />


All three modes (9 tones) can be seen coming out of the XTAL into the FPGA board with high-speed debug capture.

<img width="1139" height="1023" alt="image" src="https://github.com/user-attachments/assets/3cdd1d4f-3b93-4424-8c77-231ab8801f41" />


 
