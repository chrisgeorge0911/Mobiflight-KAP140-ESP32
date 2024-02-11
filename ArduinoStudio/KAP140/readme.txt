#1 RightBlock
413007
4 = value length
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
#8P


PFT 1      	2	10100
PFT 2      	2	10200
display test 	2	88888
baro flashing 	2	13007

baro press & delay 	0	19500


