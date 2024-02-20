# KAP140 Autopilot

This work builds on the Efis & FCU (Mobiflight-A320-Efis-Fcu-Display-with-ESP32) by @gagagu 

This repo consists of the .ino file to upload to an ESP32 board, the circuit schematic to build the hardware, and the mobiflight configuration files.

Disclaimer:  This is what I've used to get this device up and running on my system against MSFS2020 using Mobiflight. I cannot guarantee it will work on your system.

## How it works



## LCD Text configuration
Mobiflight by default does not currently support OLED devices*. However, it does support i2c LCD devices. An LCD device configuration is used to send all the data from Mobiflight to the Mega 2560 Pro. Connected to the 2560 Pro is an ESP32 board which emulates an LCD display, and receives the I2C data.

The ESP32 board, with the uploaded KAP140.ino, parses this data, and drives the 3 OLED displays via a I2C multiplexer.




```
#1 RightBlock
|413007 4 = value length
1 = padding if required
3007 = baro or alt

#2  BaroMode
0 = hg
1 = mm


#3  Barodisplay
0 = not displayed
1 = displayed


#4 Ap modes 5 bits  ACHIJ
A HDGLOCK
C NAV1LOCK
H  ROL
I REV
J APR

#5 hold modes 2 bits  BK
B VERTHOLD
K ALT


#6  preflight  3 values  OMN
O State - on/off
M  show everything (preflight)
N  preflight complete


#7 ARM states DEFG
D ALTARM
E NAVARM
F APRARM
G REVARM


#8 baro initialise state P
P Baro Initialised

#0%
#1L!!!!!
#2?
#3@
#4ACHIJ
#5BK
#6OMN
#7DEFG
#8PQ
```

PFT 1      	2	10100
PFT 2      	2	10200
display test 	2	88888
baro flashing 	2	13007

baro press & delay 	0	19500

## KAP140 firmware

The KAP140 firmware also does some processing of the data to fill in the gaps for missing functionality and data from the aircraft (in my case, the WBSim Cessna 152).



target alt +- 200 = no alert

below alt + climbing (+vs) = solid alert
below alt + descending (-vs) = flashing alert
above alt + climbing (+vs) = flashing alert
above alt + descending (-vs) = solid alert

more than 1000 +- target = no alert

no alert = 0
solid alert = 1
flashing alert = 2

Alert ncalc expression
A = Indicated alt
T = Target AP alt
V = vertical speed
if((A>T-200)&&(A<T+200),0,if(((A<T)&&(A>T-1000)&&(V>0))||((A>T)&&(A<T+1000)&&(V<0)),1,if(((A<T)&&(A>T-1000)&&(V<0))||((A>T)&&(A<T+1000)&&(V>0)),2,0)))

if((A>T-200)&&(A<T+200),0,
    if(((A<T)&&(A>T-1000)&&(V>0))||((A>T)&&(A<T+1000)&&(V<0)),1,if(((A<T)&&(A>T-1000)&&(V<0))||((A>T)&&(A<T+1000)&&(V>0)),2,0)))


if(B=1&&S=-1,1,if(B=1&&S=1,1,if(B=0&&S=-1,0,if(B=0&&S=0,if(#-!<4,0,1),if(B=0&&S=1,1,-1)))))

if(B=1&&S=1,1,if(B=0&&S=1,1,if(B=0&&S=0,0,1)))